/*
 *                            COPYRIGHT
 *
 *  librnd, modular 2D CAD framework
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
 */

/* Temporary API for pre-4.0.0 multi-sheet support in sch-rnd */

#include "conf_multi_temp.h"

typedef struct rnd_conf_state_s {
	htsi_t conf_interns;
	int conf_files_inited;
	int rnd_conf_in_production;
	lht_doc_t *rnd_conf_main_root[RND_CFR_max_alloc];
	long rnd_conf_main_root_replace_cnt[RND_CFR_max_alloc];
	int rnd_conf_main_root_lock[RND_CFR_max_alloc];
	int rnd_conf_lht_dirty[RND_CFR_max_alloc];
	lht_doc_t *rnd_conf_plug_root[RND_CFR_max_alloc];
	htsp_t *rnd_conf_fields;
	vmst_t merge_subtree;
	int rnd_conf_rev;
} rnd_conf_state_t;

