/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
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
 */

#include <librnd/core/conf.h>
#include <liblihata/dom.h>

typedef struct rnd_anyload_s rnd_anyload_t;

struct rnd_anyload_s {
	int (*load_subtree)(const rnd_anyload_t *al, rnd_hidlib_t *hl, lht_node_t *root);
	int (*load_file)(const rnd_anyload_t *al, rnd_hidlib_t *hl, const char *filename, const char *type, lht_node_t *nd);
	const char *cookie;
};


int rnd_anyload_reg(const char *root_regex, const rnd_anyload_t *al);
void rnd_anyload_unreg_by_cookie(const char *cookie);

/* Load a file or pack: path may be a lihata file (either anything we can load
   or a pcb-rnd-anyload-v*) or a directory that has an anyload.lht in it.
   Return 0 on success. */
int rnd_anyload(rnd_hidlib_t *hidlib, const char *path);

/* if non-zero: merge and update the conf after loading anyloads */
extern int rnd_anyload_conf_needs_update;