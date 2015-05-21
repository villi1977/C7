/*
 *  Copyright (C) 2008-2013, Parallels, Inc. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>

#include "ploop.h"

#define FMT "/sys/block/%s/pstat"

static void usage(void)
{
	fprintf(stderr, "Usage: ploop stat [-c | -l] -d DEVICE\n");
}

static int open_sysfs_file(char * devid, char *name, int flags)
{
	char buf[PATH_MAX];

	snprintf(buf, sizeof(buf)-1, FMT "/%s", devid, name);

	return open(buf, flags);
}

static DIR * open_sysfs_dir(char * devid)
{
	char buf[sizeof(FMT) + strlen(devid)];
	snprintf(buf, sizeof(buf)-1, FMT, devid);

	return opendir(buf);
}

int plooptool_stat(int argc, char **argv)
{
	int i;
	int clear = 0;
	int load = 0;
	DIR *dp;
	struct dirent *de;
	char * device = NULL;
	int ret = 0;

	while ((i = getopt(argc, argv, "cld:")) != EOF) {
		switch (i) {
		case 'c':
			clear = 1;
			break;
		case 'l':
			load = 1;
			break;
		case 'd':
			device = optarg;
			break;
		default:
			usage();
			return SYSEXIT_PARAM;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc || !device) {
		usage();
		return SYSEXIT_PARAM;
	}

	if (memcmp(device, "/dev/", 5) == 0)
		device += 5;

	dp = open_sysfs_dir(device);
	if (dp == NULL) {
		perror("sysfs opendir");
		return SYSEXIT_SYSFS;
	}


	if (load) {
		char buf[128];

		while (fgets(buf, sizeof(buf)-1, stdin)) {
			char name[128];
			unsigned int val;
			int fd;

			buf[sizeof(buf)-1] = 0;
			if (sscanf(buf, "%s%u", name, &val) != 2)
				continue;


			fd = open_sysfs_file(device, name, O_WRONLY);
			if (fd < 0) {
				fprintf(stderr, "Variable \"%s\" is missing\n", name);
				continue;
			}
			snprintf(buf, sizeof(buf)-1, "%u\n", val);
			if (write(fd, buf, strlen(buf)) <= 0)
				perror("write");
			close(fd);
		}
		goto out;
	}


	while ((de = readdir(dp)) != NULL) {
		int fd = -1;
		int n;
		char buf[128];

		if (de->d_name[0] == '.')
			continue;

		fd = open_sysfs_file(device, de->d_name, clear ? O_WRONLY : O_RDONLY);
		if (fd < 0) {
			perror("openat");
			ret = SYSEXIT_SYSFS;
			goto out;
		}
		if (clear) {
			if (write(fd, "0\n", 2) <= 0)
				perror("write");
		} else {
			n = read(fd, buf, sizeof(buf)-1);
			if (n < 0) {
				perror("read");
				close(fd);
				ret = SYSEXIT_SYSFS;
				goto out;
			}
			buf[n] = 0;
			printf("%-20s\t%s", de->d_name, buf);
		}
		close(fd);
	}

out:
	closedir(dp);

	return ret;
}
