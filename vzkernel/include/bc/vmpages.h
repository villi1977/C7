/*
 *  include/bc/vmpages.h
 *
 *  Copyright (C) 2005  SWsoft
 *  All rights reserved.
 *  
 *  Licensing governed by "linux/COPYING.SWsoft" file.
 *
 */

#ifndef __UB_PAGES_H_
#define __UB_PAGES_H_

#include <linux/linkage.h>
#include <linux/sched.h>	/* for get_exec_ub() */
#include <linux/mm.h>
#include <bc/beancounter.h>
#include <bc/decl.h>

/*
 * Check whether vma has private or copy-on-write mapping.
 */
#define VM_UB_PRIVATE(__flags, __file)					\
		( ((__flags) & VM_WRITE) ?				\
			(__file) == NULL || !((__flags) & VM_SHARED) :	\
			0						\
		)

UB_DECLARE_FUNC(int, ub_memory_charge(struct mm_struct *mm,
			unsigned long size,
			unsigned vm_flags,
			struct file *vm_file,
			int strict))
UB_DECLARE_VOID_FUNC(ub_memory_uncharge(struct mm_struct *mm,
			unsigned long size,
			unsigned vm_flags,
			struct file *vm_file))

struct shmem_inode_info;

UB_DECLARE_FUNC(int, ub_locked_charge(struct mm_struct *mm,
			unsigned long size))
UB_DECLARE_VOID_FUNC(ub_locked_uncharge(struct mm_struct *mm,
			unsigned long size))
UB_DECLARE_FUNC(int, ub_lockedshm_charge(struct shmem_inode_info *shi,
			unsigned long size))
UB_DECLARE_VOID_FUNC(ub_lockedshm_uncharge(struct shmem_inode_info *shi,
			unsigned long size))

extern void __ub_update_oomguarpages(struct user_beancounter *ub);

static inline int ub_swap_full(struct user_beancounter *ub)
{
	return (ub->ub_parms[UB_SWAPPAGES].held * 2 >
			ub->ub_parms[UB_SWAPPAGES].limit);
}

void __show_ub_mem(struct user_beancounter *ub);
void show_ub_mem(struct user_beancounter *ub);

#endif /* __UB_PAGES_H_ */
