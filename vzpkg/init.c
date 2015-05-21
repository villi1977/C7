/* $Id: init.c,v 1.5 2006/05/11 14:23:39 kir Exp $
 * Init -- very simple init stub
 * Shamelessly borrowed from old vzpkgtools.
 *
 * Copyright (C) 2004, 2005, SWsoft. Licensed under GNU GPL v.2.
 *
 * Compile statically on an appropriate architecture
 * using dietlibc:
 * diet -Os gcc -static -s -o myinit.ARCH init.c
 */

/* define this to make our init mount proc fs at startup */
#define MOUNT_PROC 1

#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#ifdef MOUNT_PROC
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
/* Taken from /usr/src/linux/include/fs.h */
#define MS_POSIXACL	(1<<16) /* VFS does not apply the umask */
#define MS_ACTIVE	(1<<30)
#define MS_NOUSER	(1<<31)
#endif

/* Set a signal handler */
static void setsig(struct sigaction *sa, int sig, 
		   void (*fun)(int), int flags)
{
	sa->sa_handler = fun;
	sa->sa_flags = flags;
	sigemptyset(&sa->sa_mask);
	sigaction(sig, sa, NULL);
}

/*
 * SIGCHLD: one of our children has died.
 */
void chld_handler()
{
	int st;

	/* R.I.P. all children */
	while((waitpid(-1, &st, WNOHANG)) > 0)
		;
}


/*
 * The main loop
 */ 
int main(int argc, char * argv[])
{
	struct sigaction sa;
	int i;

	if (geteuid() != 0) {
		fprintf(stderr, "%s: must be superuser\n", argv[0]);
		exit(1);
	}

	if (getpid() != 1) {
		fprintf(stderr, "%s: must be a process with PID=1\n", argv[0]);
		exit(1);
	}

#ifdef MOUNT_PROC
	mkdir("/proc", 0555);
	mount("proc", "/proc", "proc",
		MS_POSIXACL|MS_ACTIVE|MS_NOUSER|0xec0000, 0);
#endif

	/* Ignore all signals */
	for(i = 1; i <= NSIG; i++)
		setsig(&sa, i, SIG_IGN, SA_RESTART);

	setsig(&sa, SIGCHLD, chld_handler, SA_RESTART);

	close(0);
	close(1);
	close(2);
  	setsid();


	for(;;)
#if defined (__dietlibc__) && defined (__ia64__)
/* As of dietlibc-0.29, pause() is not implemented for ia64,
 * so use sleep() instead. --kir
 */
		sleep(3600);
#else
		pause();
#endif
}
