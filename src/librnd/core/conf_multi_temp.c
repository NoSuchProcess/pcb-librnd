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

struct rnd_conf_state_s {
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

	int valid;
};

rnd_conf_state_t *rnd_conf_state_alloc(void)
{
	return calloc(sizeof(rnd_conf_state_t), 1);
}

void rnd_conf_state_free(rnd_conf_state_t *cs)
{
	TODO("free all fields");
	free(cs);
}


#define SAVE(field) \
	memcpy(&dst->field, &field, sizeof(field)); \
	memset(&field, 0, sizeof(field));

void rnd_conf_state_save(rnd_conf_state_t *dst)
{
	assert(!dst->valid);

	SAVE(conf_interns);
	SAVE(conf_files_inited);
	SAVE(rnd_conf_in_production);
	SAVE(rnd_conf_main_root);
	SAVE(rnd_conf_main_root_replace_cnt);
	SAVE(rnd_conf_main_root_lock);
	SAVE(rnd_conf_lht_dirty);
	SAVE(rnd_conf_plug_root);
	SAVE(rnd_conf_fields);
	SAVE(merge_subtree);
	SAVE(rnd_conf_rev);

	dst->valid = 1;
}

#undef SAVE

#define LOAD(field) \
	memcpy(&field, &src->field, sizeof(field)); \
	memset(&src->field, 0, sizeof(field));

void rnd_conf_state_load(rnd_conf_state_t *src)
{
	assert(src->valid);

	LOAD(conf_interns);
	LOAD(conf_files_inited);
	LOAD(rnd_conf_in_production);
	LOAD(rnd_conf_main_root);
	LOAD(rnd_conf_main_root_replace_cnt);
	LOAD(rnd_conf_main_root_lock);
	LOAD(rnd_conf_lht_dirty);
	LOAD(rnd_conf_plug_root);
	LOAD(rnd_conf_fields);
	LOAD(merge_subtree);
	LOAD(rnd_conf_rev);

	src->valid = 0;
}

#undef LOAD
