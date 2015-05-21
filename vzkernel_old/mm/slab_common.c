/*
 * Slab allocator functions that are independent of the allocator strategy
 *
 * (C) 2012 Christoph Lameter <cl@linux.com>
 */
#include <linux/slab.h>

#include <linux/mm.h>
#include <linux/poison.h>
#include <linux/interrupt.h>
#include <linux/memory.h>
#include <linux/compiler.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <asm/page.h>
#include <linux/memcontrol.h>
#include <trace/events/kmem.h>

#include "slab.h"

enum slab_state slab_state;
LIST_HEAD(slab_caches);
DEFINE_MUTEX(slab_mutex);
struct kmem_cache *kmem_cache;

/*
 * Determine the size of a slab object
 */
unsigned int kmem_cache_size(struct kmem_cache *s)
{
	return s->object_size;
}
EXPORT_SYMBOL(kmem_cache_size);

#ifdef CONFIG_DEBUG_VM
static int kmem_cache_sanity_check(const char *name, size_t size)
{
	struct kmem_cache *s = NULL;

	if (!name || in_interrupt() || size < sizeof(void *) ||
		size > KMALLOC_MAX_SIZE) {
		pr_err("kmem_cache_create(%s) integrity check failed\n", name);
		return -EINVAL;
	}

	list_for_each_entry(s, &slab_caches, list) {
		char tmp;
		int res;

		/*
		 * This happens when the module gets unloaded and doesn't
		 * destroy its slab cache and no-one else reuses the vmalloc
		 * area of the module.  Print a warning.
		 */
		res = probe_kernel_address(s->name, tmp);
		if (res) {
			pr_err("Slab cache with size %d has lost its name\n",
			       s->object_size);
			continue;
		}

		if (!strcmp(s->name, name)) {
			pr_err("%s (%s): Cache name already exists.\n",
			       __func__, name);
			dump_stack();
			s = NULL;
			return -EINVAL;
		}
	}

	WARN_ON(strchr(name, ' '));	/* It confuses parsers */
	return 0;
}
#else
static inline int kmem_cache_sanity_check(const char *name, size_t size)
{
	return 0;
}
#endif

#ifdef CONFIG_MEMCG_KMEM
void slab_init_memcg_params(struct kmem_cache *s)
{
	s->memcg_params.is_root_cache = true;
	INIT_LIST_HEAD(&s->memcg_params.list);
	RCU_INIT_POINTER(s->memcg_params.memcg_caches, NULL);
}

static int init_memcg_params(struct kmem_cache *s,
		struct mem_cgroup *memcg, struct kmem_cache *root_cache)
{
	struct memcg_cache_array *arr;

	if (memcg) {
		s->memcg_params.is_root_cache = false;
		s->memcg_params.memcg = memcg;
		s->memcg_params.root_cache = root_cache;
		return 0;
	}

	slab_init_memcg_params(s);

	if (!memcg_nr_cache_ids)
		return 0;

	arr = kzalloc(sizeof(struct memcg_cache_array) +
		      memcg_nr_cache_ids * sizeof(void *),
		      GFP_KERNEL);
	if (!arr)
		return -ENOMEM;

	RCU_INIT_POINTER(s->memcg_params.memcg_caches, arr);
	return 0;
}

static void destroy_memcg_params(struct kmem_cache *s)
{
	if (is_root_cache(s))
		kfree(rcu_access_pointer(s->memcg_params.memcg_caches));
}

static int update_memcg_params(struct kmem_cache *s, int new_array_size)
{
	struct memcg_cache_array *old, *new;

	if (!is_root_cache(s))
		return 0;

	new = kzalloc(sizeof(struct memcg_cache_array) +
		      new_array_size * sizeof(void *), GFP_KERNEL);
	if (!new)
		return -ENOMEM;

	old = rcu_dereference_protected(s->memcg_params.memcg_caches,
					lockdep_is_held(&slab_mutex));
	if (old)
		memcpy(new->entries, old->entries,
		       memcg_nr_cache_ids * sizeof(void *));

	rcu_assign_pointer(s->memcg_params.memcg_caches, new);
	if (old)
		kfree_rcu(old, rcu);
	return 0;
}

int memcg_update_all_caches(int num_memcgs)
{
	struct kmem_cache *s;
	int ret = 0;

	mutex_lock(&slab_mutex);
	list_for_each_entry(s, &slab_caches, list) {
		ret = update_memcg_params(s, num_memcgs);
		/*
		 * Instead of freeing the memory, we'll just leave the caches
		 * up to this point in an updated state.
		 */
		if (ret)
			break;
	}
	mutex_unlock(&slab_mutex);
	return ret;
}
#else
static inline int init_memcg_params(struct kmem_cache *s,
		struct mem_cgroup *memcg, struct kmem_cache *root_cache)
{
	return 0;
}

static inline void destroy_memcg_params(struct kmem_cache *s)
{
}
#endif /* CONFIG_MEMCG_KMEM */

/*
 * Figure out what the alignment of the objects will be given a set of
 * flags, a user specified alignment and the size of the objects.
 */
unsigned long calculate_alignment(unsigned long flags,
		unsigned long align, unsigned long size)
{
	/*
	 * If the user wants hardware cache aligned objects then follow that
	 * suggestion if the object is sufficiently large.
	 *
	 * The hardware cache alignment cannot override the specified
	 * alignment though. If that is greater then use it.
	 */
	if (flags & SLAB_HWCACHE_ALIGN) {
		unsigned long ralign = cache_line_size();
		while (size <= ralign / 2)
			ralign /= 2;
		align = max(align, ralign);
	}

	if (align < ARCH_SLAB_MINALIGN)
		align = ARCH_SLAB_MINALIGN;

	return ALIGN(align, sizeof(void *));
}

static struct kmem_cache *
do_kmem_cache_create(char *name, size_t object_size, size_t size, size_t align,
		     unsigned long flags, void (*ctor)(void *),
		     struct mem_cgroup *memcg, struct kmem_cache *root_cache)
{
	struct kmem_cache *s;
	int err;

	err = -ENOMEM;
	s = kmem_cache_zalloc(kmem_cache, GFP_KERNEL);
	if (!s)
		goto out;

	s->name = name;
	s->object_size = object_size;
	s->size = size;
	s->align = align;
	s->ctor = ctor;

	err = init_memcg_params(s, memcg, root_cache);
	if (err)
		goto out_free_cache;

	err = __kmem_cache_create(s, flags);
	if (err)
		goto out_free_cache;

	s->refcount = 1;
	list_add(&s->list, &slab_caches);
out:
	if (err)
		return ERR_PTR(err);
	return s;

out_free_cache:
	destroy_memcg_params(s);
	kfree(s);
	goto out;
}

/*
 * kmem_cache_create - Create a cache.
 * @name: A string which is used in /proc/slabinfo to identify this cache.
 * @size: The size of objects to be created in this cache.
 * @align: The required alignment for the objects.
 * @flags: SLAB flags
 * @ctor: A constructor for the objects.
 *
 * Returns a ptr to the cache on success, NULL on failure.
 * Cannot be called within a interrupt, but can be interrupted.
 * The @ctor is run when new pages are allocated by the cache.
 *
 * The flags are
 *
 * %SLAB_POISON - Poison the slab with a known test pattern (a5a5a5a5)
 * to catch references to uninitialised memory.
 *
 * %SLAB_RED_ZONE - Insert `Red' zones around the allocated memory to check
 * for buffer overruns.
 *
 * %SLAB_HWCACHE_ALIGN - Align the objects in this cache to a hardware
 * cacheline.  This can be beneficial if you're counting cycles as closely
 * as davem.
 */
struct kmem_cache *
kmem_cache_create(const char *name, size_t size, size_t align,
		  unsigned long flags, void (*ctor)(void *))
{
	struct kmem_cache *s;
	char *cache_name;
	int err;

	get_online_cpus();
	memcg_get_cache_ids();
	mutex_lock(&slab_mutex);

	err = kmem_cache_sanity_check(name, size);
	if (err)
		goto out_unlock;

	/*
	 * Some allocators will constraint the set of valid flags to a subset
	 * of all flags. We expect them to define CACHE_CREATE_MASK in this
	 * case, and we'll just provide them with a sanitized version of the
	 * passed flags.
	 */
	flags &= CACHE_CREATE_MASK;

	s = __kmem_cache_alias(name, size, align, flags, ctor);
	if (s)
		goto out_unlock;

	cache_name = kstrdup(name, GFP_KERNEL);
	if (!cache_name) {
		err = -ENOMEM;
		goto out_unlock;
	}

	s = do_kmem_cache_create(cache_name, size, size,
				 calculate_alignment(flags, align, size),
				 flags, ctor, NULL, NULL);
	if (IS_ERR(s)) {
		err = PTR_ERR(s);
		kfree(cache_name);
	}

out_unlock:
	mutex_unlock(&slab_mutex);
	memcg_put_cache_ids();
	put_online_cpus();

	if (err) {
		if (flags & SLAB_PANIC)
			panic("kmem_cache_create: Failed to create slab '%s'. Error %d\n",
				name, err);
		else {
			printk(KERN_WARNING "kmem_cache_create(%s) failed with error %d",
				name, err);
			dump_stack();
		}
		return NULL;
	}
	return s;
}
EXPORT_SYMBOL(kmem_cache_create);

static int do_kmem_cache_shutdown(struct kmem_cache *s,
		struct list_head *release, bool *need_rcu_barrier)
{
	if (__kmem_cache_shutdown(s) != 0) {
		printk(KERN_ERR "kmem_cache_destroy %s: "
		       "Slab cache still has objects\n", s->name);
		dump_stack();
		return -EBUSY;
	}

	if (s->flags & SLAB_DESTROY_BY_RCU)
		*need_rcu_barrier = true;

#ifdef CONFIG_MEMCG_KMEM
	if (!is_root_cache(s))
		list_del(&s->memcg_params.list);
#endif
	list_move(&s->list, release);
	return 0;
}

static void do_kmem_cache_release(struct list_head *release,
				  bool need_rcu_barrier)
{
	struct kmem_cache *s, *s2;

	if (need_rcu_barrier)
		rcu_barrier();

	list_for_each_entry_safe(s, s2, release, list) {
#ifdef SLAB_SUPPORTS_SYSFS
		sysfs_slab_remove(s);
#else
		slab_kmem_cache_release(s);
#endif
	}
}

#ifdef CONFIG_MEMCG_KMEM
/*
 * memcg_create_kmem_cache - Create a cache for a memory cgroup.
 * @memcg: The memory cgroup the new cache is for.
 * @root_cache: The parent of the new cache.
 *
 * This function attempts to create a kmem cache that will serve allocation
 * requests going from @memcg to @root_cache. The new cache inherits properties
 * from its parent.
 */
void memcg_create_kmem_cache(struct mem_cgroup *memcg,
			     struct kmem_cache *root_cache)
{
	static char memcg_name_buf[NAME_MAX + 1]; /* protected by slab_mutex */
	struct cgroup_subsys_state *css = mem_cgroup_css(memcg);
	struct memcg_cache_array *arr;
	struct kmem_cache *s = NULL;
	char *cache_name;
	int idx;

	get_online_cpus();
	mutex_lock(&slab_mutex);

	/*
	 * The memory cgroup could have been deactivated while the cache
	 * creation work was pending.
	 */
	if (!memcg_kmem_is_active(memcg))
		goto out_unlock;

	idx = memcg_cache_id(memcg);
	arr = rcu_dereference_protected(root_cache->memcg_params.memcg_caches,
					lockdep_is_held(&slab_mutex));

	/*
	 * Since per-memcg caches are created asynchronously on first
	 * allocation (see memcg_kmem_get_cache()), several threads can try to
	 * create the same cache, but only one of them may succeed.
	 */
	if (arr->entries[idx])
		goto out_unlock;

	rcu_read_lock();
	strlcpy(memcg_name_buf, cgroup_name(css->cgroup), NAME_MAX + 1);
	rcu_read_unlock();
	cache_name = kasprintf(GFP_KERNEL, "%s(%d:%s)", root_cache->name,
			       css_id(css), memcg_name_buf);
	if (!cache_name)
		goto out_unlock;

	s = do_kmem_cache_create(cache_name, root_cache->object_size,
				 root_cache->size, root_cache->align,
				 root_cache->flags, root_cache->ctor,
				 memcg, root_cache);
	/*
	 * If we could not create a memcg cache, do not complain, because
	 * that's not critical at all as we can always proceed with the root
	 * cache.
	 */
	if (IS_ERR(s)) {
		kfree(cache_name);
		goto out_unlock;
	}

	list_add(&s->memcg_params.list, &root_cache->memcg_params.list);

	/*
	 * Since readers won't lock (see cache_from_memcg_idx()), we need a
	 * barrier here to ensure nobody will see the kmem_cache partially
	 * initialized.
	 */
	smp_wmb();
	arr->entries[idx] = s;

out_unlock:
	mutex_unlock(&slab_mutex);
	put_online_cpus();
}

void memcg_deactivate_kmem_caches(struct mem_cgroup *memcg)
{
	int idx;
	struct memcg_cache_array *arr;
	struct kmem_cache *s, *c;

	idx = memcg_cache_id(memcg);

	get_online_cpus();
	mutex_lock(&slab_mutex);
	list_for_each_entry(s, &slab_caches, list) {
		if (!is_root_cache(s))
			continue;

		arr = rcu_dereference_protected(s->memcg_params.memcg_caches,
						lockdep_is_held(&slab_mutex));
		c = arr->entries[idx];
		if (!c)
			continue;

		__kmem_cache_shrink(c, true);
		arr->entries[idx] = NULL;
	}
	mutex_unlock(&slab_mutex);
	put_online_cpus();
}

void memcg_destroy_kmem_caches(struct mem_cgroup *memcg)
{
	LIST_HEAD(release);
	bool need_rcu_barrier = false;
	struct kmem_cache *s, *s2;

	get_online_cpus();
	mutex_lock(&slab_mutex);
	list_for_each_entry_safe(s, s2, &slab_caches, list) {
		if (is_root_cache(s) || s->memcg_params.memcg != memcg)
			continue;
		/*
		 * The cgroup is about to be freed and therefore has no charges
		 * left. Hence, all its caches must be empty by now.
		 */
		BUG_ON(do_kmem_cache_shutdown(s, &release, &need_rcu_barrier));
	}
	mutex_unlock(&slab_mutex);
	put_online_cpus();

	do_kmem_cache_release(&release, need_rcu_barrier);
}
#endif /* CONFIG_MEMCG_KMEM */

void slab_kmem_cache_release(struct kmem_cache *s)
{
	destroy_memcg_params(s);
	kfree(s->name);
	kmem_cache_free(kmem_cache, s);
}

void kmem_cache_destroy(struct kmem_cache *s)
{
	struct kmem_cache *c, *c2;
	LIST_HEAD(release);
	bool need_rcu_barrier = false;
	bool busy = false;

	BUG_ON(!is_root_cache(s));

	get_online_cpus();
	mutex_lock(&slab_mutex);

	s->refcount--;
	if (s->refcount)
		goto out_unlock;

	for_each_memcg_cache_safe(c, c2, s) {
		if (do_kmem_cache_shutdown(c, &release, &need_rcu_barrier))
			busy = true;
	}

	if (!busy)
		do_kmem_cache_shutdown(s, &release, &need_rcu_barrier);

out_unlock:
	mutex_unlock(&slab_mutex);
	put_online_cpus();

	do_kmem_cache_release(&release, need_rcu_barrier);
}
EXPORT_SYMBOL(kmem_cache_destroy);

/**
 * kmem_cache_shrink - Shrink a cache.
 * @cachep: The cache to shrink.
 *
 * Releases as many slabs as possible for a cache.
 * To help debugging, a zero exit status indicates all slabs were released.
 */
int kmem_cache_shrink(struct kmem_cache *cachep)
{
	int ret;

	get_online_cpus();
	mutex_lock(&slab_mutex);
	ret = __kmem_cache_shrink(cachep, false);
	mutex_unlock(&slab_mutex);
	put_online_cpus();
	return ret;
}
EXPORT_SYMBOL(kmem_cache_shrink);

int slab_is_available(void)
{
	return slab_state >= UP;
}

#ifndef CONFIG_SLOB
/* Create a cache during boot when no slab services are available yet */
void __init create_boot_cache(struct kmem_cache *s, const char *name, size_t size,
		unsigned long flags)
{
	int err;

	s->name = name;
	s->size = s->object_size = size;
	s->align = calculate_alignment(flags, ARCH_KMALLOC_MINALIGN, size);

	slab_init_memcg_params(s);

	err = __kmem_cache_create(s, flags);

	if (err)
		panic("Creation of kmalloc slab %s size=%zu failed. Reason %d\n",
					name, size, err);

	s->refcount = -1;	/* Exempt from merging for now */
}

struct kmem_cache *__init create_kmalloc_cache(const char *name, size_t size,
				unsigned long flags)
{
	struct kmem_cache *s = kmem_cache_zalloc(kmem_cache, GFP_NOWAIT);

	if (!s)
		panic("Out of memory when creating slab %s\n", name);

	create_boot_cache(s, name, size, flags);
	list_add(&s->list, &slab_caches);
	s->refcount = 1;
	return s;
}

struct kmem_cache *kmalloc_caches[KMALLOC_SHIFT_HIGH + 1];
EXPORT_SYMBOL(kmalloc_caches);

#ifdef CONFIG_ZONE_DMA
struct kmem_cache *kmalloc_dma_caches[KMALLOC_SHIFT_HIGH + 1];
EXPORT_SYMBOL(kmalloc_dma_caches);
#endif

/*
 * Conversion table for small slabs sizes / 8 to the index in the
 * kmalloc array. This is necessary for slabs < 192 since we have non power
 * of two cache sizes there. The size of larger slabs can be determined using
 * fls.
 */
static s8 size_index[24] = {
	3,	/* 8 */
	4,	/* 16 */
	5,	/* 24 */
	5,	/* 32 */
	6,	/* 40 */
	6,	/* 48 */
	6,	/* 56 */
	6,	/* 64 */
	1,	/* 72 */
	1,	/* 80 */
	1,	/* 88 */
	1,	/* 96 */
	7,	/* 104 */
	7,	/* 112 */
	7,	/* 120 */
	7,	/* 128 */
	2,	/* 136 */
	2,	/* 144 */
	2,	/* 152 */
	2,	/* 160 */
	2,	/* 168 */
	2,	/* 176 */
	2,	/* 184 */
	2	/* 192 */
};

static inline int size_index_elem(size_t bytes)
{
	return (bytes - 1) / 8;
}

/*
 * Find the kmem_cache structure that serves a given size of
 * allocation
 */
struct kmem_cache *kmalloc_slab(size_t size, gfp_t flags)
{
	int index;

	if (size > KMALLOC_MAX_SIZE) {
		WARN_ON_ONCE(!(flags & __GFP_NOWARN));
		return NULL;
	}

	if (size <= 192) {
		if (!size)
			return ZERO_SIZE_PTR;

		index = size_index[size_index_elem(size)];
	} else
		index = fls(size - 1);

#ifdef CONFIG_ZONE_DMA
	if (unlikely((flags & GFP_DMA)))
		return kmalloc_dma_caches[index];

#endif
	return kmalloc_caches[index];
}

/*
 * Create the kmalloc array. Some of the regular kmalloc arrays
 * may already have been created because they were needed to
 * enable allocations for slab creation.
 */
void __init create_kmalloc_caches(unsigned long flags)
{
	int i;

	/*
	 * Patch up the size_index table if we have strange large alignment
	 * requirements for the kmalloc array. This is only the case for
	 * MIPS it seems. The standard arches will not generate any code here.
	 *
	 * Largest permitted alignment is 256 bytes due to the way we
	 * handle the index determination for the smaller caches.
	 *
	 * Make sure that nothing crazy happens if someone starts tinkering
	 * around with ARCH_KMALLOC_MINALIGN
	 */
	BUILD_BUG_ON(KMALLOC_MIN_SIZE > 256 ||
		(KMALLOC_MIN_SIZE & (KMALLOC_MIN_SIZE - 1)));

	for (i = 8; i < KMALLOC_MIN_SIZE; i += 8) {
		int elem = size_index_elem(i);

		if (elem >= ARRAY_SIZE(size_index))
			break;
		size_index[elem] = KMALLOC_SHIFT_LOW;
	}

	if (KMALLOC_MIN_SIZE >= 64) {
		/*
		 * The 96 byte size cache is not used if the alignment
		 * is 64 byte.
		 */
		for (i = 64 + 8; i <= 96; i += 8)
			size_index[size_index_elem(i)] = 7;

	}

	if (KMALLOC_MIN_SIZE >= 128) {
		/*
		 * The 192 byte sized cache is not used if the alignment
		 * is 128 byte. Redirect kmalloc to use the 256 byte cache
		 * instead.
		 */
		for (i = 128 + 8; i <= 192; i += 8)
			size_index[size_index_elem(i)] = 8;
	}
	for (i = KMALLOC_SHIFT_LOW; i <= KMALLOC_SHIFT_HIGH; i++) {
		if (!kmalloc_caches[i]) {
			kmalloc_caches[i] = create_kmalloc_cache(NULL,
							1 << i, flags);
		}

		/*
		 * Caches that are not of the two-to-the-power-of size.
		 * These have to be created immediately after the
		 * earlier power of two caches
		 */
		if (KMALLOC_MIN_SIZE <= 32 && !kmalloc_caches[1] && i == 6)
			kmalloc_caches[1] = create_kmalloc_cache(NULL, 96, flags);

		if (KMALLOC_MIN_SIZE <= 64 && !kmalloc_caches[2] && i == 7)
			kmalloc_caches[2] = create_kmalloc_cache(NULL, 192, flags);
	}

	/* Kmalloc array is now usable */
	slab_state = UP;

	for (i = 0; i <= KMALLOC_SHIFT_HIGH; i++) {
		struct kmem_cache *s = kmalloc_caches[i];
		char *n;

		if (s) {
			n = kasprintf(GFP_NOWAIT, "kmalloc-%d", kmalloc_size(i));

			BUG_ON(!n);
			s->name = n;
		}
	}

#ifdef CONFIG_ZONE_DMA
	for (i = 0; i <= KMALLOC_SHIFT_HIGH; i++) {
		struct kmem_cache *s = kmalloc_caches[i];

		if (s) {
			int size = kmalloc_size(i);
			char *n = kasprintf(GFP_NOWAIT,
				 "dma-kmalloc-%d", size);

			BUG_ON(!n);
			kmalloc_dma_caches[i] = create_kmalloc_cache(n,
				size, SLAB_CACHE_DMA | flags);
		}
	}
#endif
}
#endif /* !CONFIG_SLOB */

void *kmalloc_order(size_t size, gfp_t flags, unsigned int order)
{
	void *ret;
	struct page *page;

	flags |= __GFP_COMP;
	page = alloc_kmem_pages(flags, order);
	ret = page ? page_address(page) : NULL;
	kmemleak_alloc(ret, size, 1, flags);
	return ret;
}
EXPORT_SYMBOL(kmalloc_order);

#ifdef CONFIG_TRACING
void *kmalloc_order_trace(size_t size, gfp_t flags, unsigned int order)
{
	void *ret = kmalloc_order(size, flags, order);
	trace_kmalloc(_RET_IP_, ret, size, PAGE_SIZE << order, flags);
	return ret;
}
EXPORT_SYMBOL(kmalloc_order_trace);
#endif

#ifdef CONFIG_SLABINFO
static void print_slabinfo_header(struct seq_file *m)
{
	/*
	 * Output format version, so at least we can change it
	 * without _too_ many complaints.
	 */
#ifdef CONFIG_DEBUG_SLAB
	seq_puts(m, "slabinfo - version: 2.1 (statistics)\n");
#else
	seq_puts(m, "slabinfo - version: 2.1\n");
#endif
	seq_puts(m, "# name            <active_objs> <num_objs> <objsize> "
		 "<objperslab> <pagesperslab>");
	seq_puts(m, " : tunables <limit> <batchcount> <sharedfactor>");
	seq_puts(m, " : slabdata <active_slabs> <num_slabs> <sharedavail>");
#ifdef CONFIG_DEBUG_SLAB
	seq_puts(m, " : globalstat <listallocs> <maxobjs> <grown> <reaped> "
		 "<error> <maxfreeable> <nodeallocs> <remotefrees> <alienoverflow>");
	seq_puts(m, " : cpustat <allochit> <allocmiss> <freehit> <freemiss>");
#endif
	seq_putc(m, '\n');
}

void *slab_start(struct seq_file *m, loff_t *pos)
{
	mutex_lock(&slab_mutex);
	return seq_list_start(&slab_caches, *pos);
}

void *slab_next(struct seq_file *m, void *p, loff_t *pos)
{
	return seq_list_next(p, &slab_caches, pos);
}

void slab_stop(struct seq_file *m, void *p)
{
	mutex_unlock(&slab_mutex);
}

static void
memcg_accumulate_slabinfo(struct kmem_cache *s, struct slabinfo *info)
{
	struct kmem_cache *c;
	struct slabinfo sinfo;

	if (!is_root_cache(s))
		return;

	for_each_memcg_cache(c, s) {
		memset(&sinfo, 0, sizeof(sinfo));
		get_slabinfo(c, &sinfo);

		info->active_slabs += sinfo.active_slabs;
		info->num_slabs += sinfo.num_slabs;
		info->shared_avail += sinfo.shared_avail;
		info->active_objs += sinfo.active_objs;
		info->num_objs += sinfo.num_objs;
	}
}

static void cache_show(struct kmem_cache *s, struct seq_file *m)
{
	struct slabinfo sinfo;

	memset(&sinfo, 0, sizeof(sinfo));
	get_slabinfo(s, &sinfo);

	memcg_accumulate_slabinfo(s, &sinfo);

	seq_printf(m, "%-17s %6lu %6lu %6u %4u %4d",
		   cache_name(s), sinfo.active_objs, sinfo.num_objs, s->size,
		   sinfo.objects_per_slab, (1 << sinfo.cache_order));

	seq_printf(m, " : tunables %4u %4u %4u",
		   sinfo.limit, sinfo.batchcount, sinfo.shared);
	seq_printf(m, " : slabdata %6lu %6lu %6lu",
		   sinfo.active_slabs, sinfo.num_slabs, sinfo.shared_avail);
	slabinfo_show_stats(m, s);
	seq_putc(m, '\n');
}

static int slab_show(struct seq_file *m, void *p)
{
	struct kmem_cache *s = list_entry(p, struct kmem_cache, list);

	if (p == slab_caches.next)
		print_slabinfo_header(m);
	if (is_root_cache(s))
		cache_show(s, m);
	return 0;
}

#ifdef CONFIG_MEMCG_KMEM
int memcg_slab_show(struct mem_cgroup *memcg, struct seq_file *m, void *p)
{
	struct kmem_cache *s = list_entry(p, struct kmem_cache, list);

	if (p == slab_caches.next)
		print_slabinfo_header(m);
	if (!is_root_cache(s) && s->memcg_params.memcg == memcg)
		cache_show(s, m);
	return 0;
}
#endif

/*
 * slabinfo_op - iterator that generates /proc/slabinfo
 *
 * Output layout:
 * cache-name
 * num-active-objs
 * total-objs
 * object size
 * num-active-slabs
 * total-slabs
 * num-pages-per-slab
 * + further values on SMP and with statistics enabled
 */
static const struct seq_operations slabinfo_op = {
	.start = slab_start,
	.next = slab_next,
	.stop = slab_stop,
	.show = slab_show,
};

static int slabinfo_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &slabinfo_op);
}

static const struct file_operations proc_slabinfo_operations = {
	.open		= slabinfo_open,
	.read		= seq_read,
	.write          = slabinfo_write,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static int __init slab_proc_init(void)
{
	proc_create("slabinfo", S_IRUSR, NULL, &proc_slabinfo_operations);
	return 0;
}
module_init(slab_proc_init);
#endif /* CONFIG_SLABINFO */