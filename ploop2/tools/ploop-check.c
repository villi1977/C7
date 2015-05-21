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
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/stat.h>
#include <getopt.h>
#include <linux/types.h>
#include <string.h>
#include <stdlib.h>

#include "ploop.h"
#include "common.h"

static void usage(void)
{
	fprintf(stderr, "Usage: ploop check [-u UUID] DiskDescriptor.xml\n"
"	UUID := check all deltas up to top image with this UUID\n"
"       ploop check [options] DELTA\n"
"	DELTA := path to image file\n"
"	-f, --force          - force check even if dirty flag is clear\n"
"	-F, --hard-force     - -f and try to fix even fatal errors (dangerous)\n"
"	-c, --check          - check for duplicated blocks and holes\n"
"	-r, --ro             - do not modify DELTA (read-only access)\n"
"	-s, --silent         - be silent, report only errors\n"
"	-d, --drop-inuse     - drop image \"in use\" flag\n"
"	-R, --raw            - DELTA is a raw ploop image\n"
"	-b, --blocksize SIZE - cluster block size in sectors (for raw images)\n"
"	-S, --repair-sparse  - repair sparse image\n"
	);
}

int plooptool_check(int argc, char ** argv)
{
	int i, idx;
	const int def_flags = CHECK_TALKATIVE;
	int flags = def_flags;
	unsigned int blocksize = 0;
	char *endptr;
	const char *uuid = NULL;
	static struct option options[] = {
		{"force", no_argument, NULL, 'f'},
		{"hard-force", no_argument, NULL, 'F'},
		{"check", no_argument, NULL, 'c'},
		{"drop-inuse", no_argument, NULL, 'd'},
		{"ro", no_argument, NULL, 'r'},
		{"silent", no_argument, NULL, 's'},
		{"raw", no_argument, NULL, 'R'},
		{"blocksize", required_argument, NULL, 'b'},
		{"repair-sparse", no_argument, NULL, 'S'},
		{"uuid", required_argument, NULL, 'u'},
		{ NULL, 0, NULL, 0 }
	};

	while ((i = getopt_long(argc, argv, "fFcrsdRb:Su:", options, &idx)) != EOF) {
		switch (i) {
		case 'f':
			/* try to repair non-fatal conditions */
			flags |= CHECK_FORCE;
			break;
		case 'F':
			/* try to repair even fatal conditions */
			flags |= (CHECK_FORCE | CHECK_HARDFORCE);
			break;
		case 'c':
			/* build bitmap and check for duplicate blocks */
			flags |= CHECK_DETAILED;
			break;
		case 'd':
			flags |= CHECK_DROPINUSE;
			break;
		case 'r':
			flags |= CHECK_READONLY;
			break;
		case 'R':
			flags |= CHECK_RAW;
			break;
		case 'b':
			blocksize = strtoul(optarg, &endptr, 0);
			if (*endptr != '\0') {
				usage();
				return SYSEXIT_PARAM;
			}
			break;
		case 's':
			flags &= ~CHECK_TALKATIVE;
			break;
		case 'S':
			flags |= CHECK_REPAIR_SPARSE;
			break;
		case 'u':
			uuid = parse_uuid(optarg);
			if (!uuid)
				return SYSEXIT_PARAM;
			break;
		default:
			usage();
			return SYSEXIT_PARAM;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1) {
		usage();
		return SYSEXIT_PARAM;
	}

	if (is_xml_fname(argv[0])) {
		struct ploop_disk_images_data *di;
		int ret;

		if (flags != def_flags || blocksize)
			fprintf(stderr, "WARNING: options are ignored "
					"for DiskDescriptor.xml form\n");

		ret = ploop_open_dd(&di, argv[0]);
		if (ret)
			return ret;

		ret = check_dd(di, uuid);

		ploop_close_dd(di);

		return ret;
	}

	/* non-ddxml form */
	if (uuid) {
		fprintf(stderr, "Option -u is only applicable to "
				"DiskDescriptor.xml syntax\n");
		return SYSEXIT_PARAM;
	}

	return ploop_check(argv[0], flags, &blocksize);
}
