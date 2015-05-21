#ifndef _LINUX_COMPAT_H
#define _LINUX_COMPAT_H
/*
 * These are the type definitions for the architecture specific
 * syscall compatibility layer.
 */

#ifdef CONFIG_COMPAT

#include <linux/stat.h>
#include <linux/param.h>	/* for HZ */
#include <linux/sem.h>
#include <linux/socket.h>
#include <linux/if.h>
#include <linux/fs.h>
#include <linux/aio_abi.h>	/* for aio_context_t */


#ifndef COMPAT_USE_64BIT_TIME
#define COMPAT_USE_64BIT_TIME 0
#endif

#ifndef __SC_DELOUSE
#define __SC_DELOUSE(t,v) ((t)(unsigned long)(v))
#endif

#define COMPAT_SYSCALL_DEFINE1(name, ...) \
        COMPAT_SYSCALL_DEFINEx(1, _##name, __VA_ARGS__)
#define COMPAT_SYSCALL_DEFINE2(name, ...) \
	COMPAT_SYSCALL_DEFINEx(2, _##name, __VA_ARGS__)
#define COMPAT_SYSCALL_DEFINE3(name, ...) \
	COMPAT_SYSCALL_DEFINEx(3, _##name, __VA_ARGS__)
#define COMPAT_SYSCALL_DEFINE4(name, ...) \
	COMPAT_SYSCALL_DEFINEx(4, _##name, __VA_ARGS__)
#define COMPAT_SYSCALL_DEFINE5(name, ...) \
	COMPAT_SYSCALL_DEFINEx(5, _##name, __VA_ARGS__)
#define COMPAT_SYSCALL_DEFINE6(name, ...) \
	COMPAT_SYSCALL_DEFINEx(6, _##name, __VA_ARGS__)

#define COMPAT_SYSCALL_DEFINEx(x, name, ...)				\
	asmlinkage long compat_sys##name(__MAP(x,__SC_DECL,__VA_ARGS__));\
	static __inline__ long C_SYSC##name(__MAP(x,__SC_DECL,__VA_ARGS__));\
	asmlinkage long compat_SyS##name(__MAP(x,__SC_LONG,__VA_ARGS__))\
	{								\
		return C_SYSC##name(__MAP(x,__SC_DELOUSE,__VA_ARGS__));	\
	}								\
	SYSCALL_ALIAS(compat_sys##name, compat_SyS##name);		\
	static __inline__ long C_SYSC##name(__MAP(x,__SC_DECL,__VA_ARGS__))

#ifndef compat_user_stack_pointer
#define compat_user_stack_pointer() current_user_stack_pointer()
#endif
#ifndef compat_sigaltstack	/* we'll need that for MIPS */
typedef struct compat_sigaltstack {
	compat_uptr_t			ss_sp;
	int				ss_flags;
	compat_size_t			ss_size;
} compat_stack_t;
#endif

#define compat_jiffies_to_clock_t(x)	\
		(((unsigned long)(x) * COMPAT_USER_HZ) / HZ)

typedef __compat_uid32_t	compat_uid_t;
typedef __compat_gid32_t	compat_gid_t;

struct compat_sel_arg_struct;
struct rusage;

struct compat_itimerspec {
	struct compat_timespec it_interval;
	struct compat_timespec it_value;
};

struct compat_utimbuf {
	compat_time_t		actime;
	compat_time_t		modtime;
};

struct compat_itimerval {
	struct compat_timeval	it_interval;
	struct compat_timeval	it_value;
};

struct compat_tms {
	compat_clock_t		tms_utime;
	compat_clock_t		tms_stime;
	compat_clock_t		tms_cutime;
	compat_clock_t		tms_cstime;
};

struct compat_timex {
	compat_uint_t modes;
	compat_long_t offset;
	compat_long_t freq;
	compat_long_t maxerror;
	compat_long_t esterror;
	compat_int_t status;
	compat_long_t constant;
	compat_long_t precision;
	compat_long_t tolerance;
	struct compat_timeval time;
	compat_long_t tick;
	compat_long_t ppsfreq;
	compat_long_t jitter;
	compat_int_t shift;
	compat_long_t stabil;
	compat_long_t jitcnt;
	compat_long_t calcnt;
	compat_long_t errcnt;
	compat_long_t stbcnt;
	compat_int_t tai;

	compat_int_t:32; compat_int_t:32; compat_int_t:32; compat_int_t:32;
	compat_int_t:32; compat_int_t:32; compat_int_t:32; compat_int_t:32;
	compat_int_t:32; compat_int_t:32; compat_int_t:32;
};

#define _COMPAT_NSIG_WORDS	(_COMPAT_NSIG / _COMPAT_NSIG_BPW)

typedef struct {
	compat_sigset_word	sig[_COMPAT_NSIG_WORDS];
} compat_sigset_t;

struct compat_sigaction {
#ifndef __ARCH_HAS_IRIX_SIGACTION
	compat_uptr_t			sa_handler;
	compat_ulong_t			sa_flags;
#else
	compat_uint_t			sa_flags;
	compat_uptr_t			sa_handler;
#endif
#ifdef __ARCH_HAS_SA_RESTORER
	compat_uptr_t			sa_restorer;
#endif
	compat_sigset_t			sa_mask __attribute__((packed));
};


struct compat_iovec {
	compat_uptr_t	iov_base;
	compat_size_t	iov_len;
};

struct compat_rlimit {
	compat_ulong_t	rlim_cur;
	compat_ulong_t	rlim_max;
};

struct compat_rusage {
	struct compat_timeval ru_utime;
	struct compat_timeval ru_stime;
	compat_long_t	ru_maxrss;
	compat_long_t	ru_ixrss;
	compat_long_t	ru_idrss;
	compat_long_t	ru_isrss;
	compat_long_t	ru_minflt;
	compat_long_t	ru_majflt;
	compat_long_t	ru_nswap;
	compat_long_t	ru_inblock;
	compat_long_t	ru_oublock;
	compat_long_t	ru_msgsnd;
	compat_long_t	ru_msgrcv;
	compat_long_t	ru_nsignals;
	compat_long_t	ru_nvcsw;
	compat_long_t	ru_nivcsw;
};


struct compat_siginfo;


struct compat_dirent {
	u32		d_ino;
	compat_off_t	d_off;
	u16		d_reclen;
	char		d_name[256];
};

struct compat_ustat {
	compat_daddr_t		f_tfree;
	compat_ino_t		f_tinode;
	char			f_fname[6];
	char			f_fpack[6];
};

#define COMPAT_SIGEV_PAD_SIZE	((SIGEV_MAX_SIZE/sizeof(int)) - 3)

typedef struct compat_sigevent {
	compat_sigval_t sigev_value;
	compat_int_t sigev_signo;
	compat_int_t sigev_notify;
	union {
		compat_int_t _pad[COMPAT_SIGEV_PAD_SIZE];
		compat_int_t _tid;

		struct {
			compat_uptr_t _function;
			compat_uptr_t _attribute;
		} _sigev_thread;
	} _sigev_un;
} compat_sigevent_t;

struct compat_ifmap {
	compat_ulong_t mem_start;
	compat_ulong_t mem_end;
	unsigned short base_addr;
	unsigned char irq;
	unsigned char dma;
	unsigned char port;
};

struct compat_if_settings {
	unsigned int type;	/* Type of physical device or protocol */
	unsigned int size;	/* Size of the data allocated by the caller */
	compat_uptr_t ifs_ifsu;	/* union of pointers */
};

struct compat_ifreq {
	union {
		char	ifrn_name[IFNAMSIZ];    /* if name, e.g. "en0" */
	} ifr_ifrn;
	union {
		struct	sockaddr ifru_addr;
		struct	sockaddr ifru_dstaddr;
		struct	sockaddr ifru_broadaddr;
		struct	sockaddr ifru_netmask;
		struct	sockaddr ifru_hwaddr;
		short	ifru_flags;
		compat_int_t	ifru_ivalue;
		compat_int_t	ifru_mtu;
		struct	compat_ifmap ifru_map;
		char	ifru_slave[IFNAMSIZ];   /* Just fits the size */
		char	ifru_newname[IFNAMSIZ];
		compat_caddr_t	ifru_data;
		struct	compat_if_settings ifru_settings;
	} ifr_ifru;
};

struct compat_ifconf {
	compat_int_t	ifc_len;                /* size of buffer */
	compat_caddr_t  ifcbuf;
};

struct compat_robust_list {
	compat_uptr_t			next;
};

struct compat_robust_list_head {
	struct compat_robust_list	list;
	compat_long_t			futex_offset;
	compat_uptr_t			list_op_pending;
};

#ifdef CONFIG_COMPAT_OLD_SIGACTION
struct compat_old_sigaction {
	compat_uptr_t			sa_handler;
	compat_old_sigset_t		sa_mask;
	compat_ulong_t			sa_flags;
	compat_uptr_t			sa_restorer;
};
#endif

struct compat_statfs;
struct compat_statfs64;
struct compat_old_linux_dirent;
struct compat_linux_dirent;
struct linux_dirent64;
struct compat_msghdr;
struct compat_mmsghdr;
struct compat_sysinfo;
struct compat_sysctl_args;
struct compat_kexec_segment;
struct compat_mq_attr;
struct compat_msgbuf;


asmlinkage long compat_sys_sendfile(int out_fd, int in_fd,
				    compat_off_t *offset, compat_size_t count);
asmlinkage long compat_sys_sendfile64(int out_fd, int in_fd,
				    compat_loff_t *offset, compat_size_t count);
asmlinkage long compat_sys_sigaltstack(const compat_stack_t *uss_ptr,
				       compat_stack_t *uoss_ptr);

int compat_restore_altstack(const compat_stack_t *uss);
int __compat_save_altstack(compat_stack_t *, unsigned long);
#define compat_save_altstack_ex(uss, sp) do { \
	compat_stack_t *__uss = uss; \
	struct task_struct *t = current; \
	put_user_ex(ptr_to_compat((void *)t->sas_ss_sp), &__uss->ss_sp); \
	put_user_ex(sas_ss_flags(sp), &__uss->ss_flags); \
	put_user_ex(t->sas_ss_size, &__uss->ss_size); \
} while (0);

asmlinkage long compat_sys_sched_rr_get_interval(compat_pid_t pid,
						 struct compat_timespec *interval);

asmlinkage long compat_sys_fanotify_mark(int, unsigned int, __u32, __u32,
					    int, const char *);

#ifdef CONFIG_QUOTA_COMPAT

#define QC_QUOTAON  0x0100	/* enable quotas */
#define QC_QUOTAOFF 0x0200	/* disable quotas */
/* GETQUOTA, SETQUOTA and SETUSE which were at 0x0300-0x0500 has now other parameteres */
#define QC_SYNC     0x0600	/* sync disk copy of a filesystems quotas */
#define QC_SETQLIM  0x0700	/* set limits */
/* GETSTATS at 0x0800 is now longer... */
#define QC_GETINFO  0x0900	/* get info about quotas - graces, flags... */
#define QC_SETINFO  0x0A00	/* set info about quotas */
#define QC_SETGRACE 0x0B00	/* set inode and block grace */
#define QC_SETFLAGS 0x0C00	/* set flags for quota */
#define QC_GETQUOTA 0x0D00	/* get limits and usage */
#define QC_SETQUOTA 0x0E00	/* set limits and usage */
#define QC_SETUSE   0x0F00	/* set usage */
/* 0x1000 used by old RSQUASH */
#define QC_GETSTATS 0x1100	/* get collected stats */

struct compat_dqblk {
	unsigned int dqb_ihardlimit;
	unsigned int dqb_isoftlimit;
	unsigned int dqb_curinodes;
	unsigned int dqb_bhardlimit;
	unsigned int dqb_bsoftlimit;
	qsize_t dqb_curspace;
	__kernel_time_t dqb_btime;
	__kernel_time_t dqb_itime;
};

#ifdef CONFIG_COMPAT

struct compat_compat_dqblk {
	compat_uint_t	dqb_ihardlimit;
	compat_uint_t	dqb_isoftlimit;
	compat_uint_t	dqb_curinodes;
	compat_uint_t	dqb_bhardlimit;
	compat_uint_t	dqb_bsoftlimit;
	compat_u64	dqb_curspace;
	compat_time_t	dqb_btime;
	compat_time_t	dqb_itime;
};

#endif

struct compat_dqinfo {
	unsigned int dqi_bgrace;
	unsigned int dqi_igrace;
	unsigned int dqi_flags;
	unsigned int dqi_blocks;
	unsigned int dqi_free_blk;
	unsigned int dqi_free_entry;
};

struct compat_dqstats {
	__u32 lookups;
	__u32 drops;
	__u32 reads;
	__u32 writes;
	__u32 cache_hits;
	__u32 allocated_dquots;
	__u32 free_dquots;
	__u32 syncs;
	__u32 version;
};
#endif /* CONFIG_QUOTA_COMPAT */

#else

#define is_compat_task() (0)

#endif /* CONFIG_COMPAT */
#endif /* _LINUX_COMPAT_H */
