/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#ifndef RND_LIB_WGET
#define RND_LIB_WGET

typedef struct {
	const char **header;
	const char *post_file;

	/* Spare: see doc/developer/spare.txt */
	void (*spare_f1)(void), (*spare_f2)(void);
	long spare_l1, spare_l2, spare_l3, spare_l4;
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
	double spare_d1, spare_d2, spare_d3, spare_d4;
	rnd_coord_t spare_c1, spare_c2, spare_c3, spare_c4;
} rnd_wget_opts_t;

/* download the document at url to a local file ofn; if update is non-zero,
   try to continue the download or update the file */
int rnd_wget_disk(const char *url, const char *ofn, int update, const rnd_wget_opts_t *opts);


/* download and stream the document at url; if update is non-zero,
   try to continue the download or update the file */
FILE *rnd_wget_popen(const char *url, int update, const rnd_wget_opts_t *opts);

#endif
