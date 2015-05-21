/*
 * include/linux/vzlist.h
 *
 * Copyright (c) 2015 Parallels IP Holdings GmbH
 *
 */

#ifndef _LINUX_VZLIST_H
#define _LINUX_VZLIST_H

#include <linux/types.h>
#include <linux/ioctl.h>

#define __user

#ifndef __ENVID_T_DEFINED__
#define __ENVID_T_DEFINED__
typedef unsigned int envid_t;
#endif

struct vzlist_veidctl {
	unsigned int	num;
	envid_t	*id;
};

struct vzlist_vepidctl {
	envid_t		veid;
	unsigned int	num;
	pid_t *pid;
};

struct vzlist_veipctl {
	envid_t		veid;
	unsigned int	num;
	void *ip;
};

#define VZLISTTYPE		'x'
#define VZCTL_GET_VEIDS		_IOR(VZLISTTYPE, 1, struct vzlist_veidctl)
#define VZCTL_GET_VEPIDS	_IOR(VZLISTTYPE, 2, struct vzlist_vepidctl)
#define VZCTL_GET_VEIPS		_IOR(VZLISTTYPE, 3, struct vzlist_veipctl)
#define VZCTL_GET_VEIP6S	_IOR(VZLISTTYPE, 4, struct vzlist_veipctl)

#endif /* _LINUX_VZLIST_H */
