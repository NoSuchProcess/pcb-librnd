/*
 *                            COPYRIGHT
 *
 *  librnd, 2d CAD framework
 *  Copyright (C) 2022 Tibor 'Igor2' Palinkas
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
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */


/* Generate an unique list of object files names, rename objects as needed and
   call ar(1) with this list. This is needed so that 'ar x' produces distinct
   object files without overwriting any of them. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/default/ht.h"

static char count[256];
#define MAX_COUNT (count+255)

/* cmd is accumulated in a single growing buffer */
static char *cmd;
static int used, alloced;
static void append(const char *s)
{
	int len = strlen(s);
	if (used+len+1 > alloced) {
		alloced = used+len+1024;
		cmd = realloc(cmd, alloced);
	}
	memcpy(cmd + used, s, len+1);
	used += len;
}

int main(int argc, char *argv[])
{
	int n, res = -1, ren_used = 0;
	ht_t *ht;
	char **ren; /* renames: [n+0]=old_name, [n+1]=new_name */

	if (argc < 5) {
		fprintf(stderr, "Error: not enough arguments.\nUsage: aru ar rvu out.a a.o b.o ... z.o\n");
		exit(1);
	}

	append(argv[1]); /* ar */
	append(" ");
	append(argv[2]); /* rvu */
	append(" ");
	append(argv[3]); /* foo.a */

	ren = malloc(sizeof(char *) * argc * 2);

	ht = ht_alloc(0);
	for(n = 4; n < argc; n++) {
		char *fn = argv[n], *cn, *s, *basename = argv[n];

		for(s = basename; *s != '\0'; s++)
			if (*s == '/')
				basename = s+1;

		append(" ");

		/* check for duplicates */
		cn = ht_get(ht, basename);
		if (cn != NULL) {
			char *newfn, *end;
			int len = strlen(fn);

			cn++;
			if (cn > MAX_COUNT) {
				fprintf(stderr, "Too many redundancies on %s\n", fn);
				goto error;
			}

			newfn = malloc(len+32);
			memcpy(newfn, fn, len);

			/* calculate new file name by replacing trailing .o */
			end = newfn + len - 1;
			if (*end == 'o') end--;
			if (*end != '.') end++;
			sprintf(end, "__%x.o", (int)(cn - count));

			/* rename and remember the rename */
			ren[ren_used++] = fn;
			ren[ren_used++] = newfn;
			rename(fn, newfn);
			fn = newfn;
		}
		else
			cn = count;

		append(fn);
		ht_set(ht, basename, cn);
	}

	ht_free(ht);
	res = system(cmd);

	error:;
	/* rename objects back */
	for(n = 0; n < ren_used; n+=2)
		rename(ren[n+1], ren[n]);

	return res;
}
