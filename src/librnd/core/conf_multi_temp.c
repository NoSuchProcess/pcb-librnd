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

static gdl_list_t designs;


typedef struct rnd_conf_plug_state_s {
	const char *cookie;    /* compared as pointer */
	void *globvar;         /* pointer to the global variable the plugin is using */
	long size;             /* sizeof() the globvar struct */
	gdl_elem_t link;       /* either in global plug_states or in rnd_conf_state_t's plug_states */
	char saved[1];         /* (overallocated) a copy of the last content of globvar when design is not active (switched away); not used in global plug_states */
} rnd_conf_plug_state_t;

static gdl_list_t plug_states;

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

	gdl_list_t plug_states; /* extra saves */
};

static void rnd_conf_state_copy_nat(htsp_t *dst, const htsp_t *src)
{
	htsp_entry_t *e;
	for(e = htsp_first(src); e != NULL; e = htsp_next(src, e)) {
		rnd_conf_native_t *nat_src = e->value, *nat_dst;
		nat_dst = rnd_conf_alloc_field_(nat_src->val.any, nat_src->array_size,
			nat_src->type, nat_src->hash_path, nat_src->description, nat_src->flags);
		htsp_set(dst, (char *)nat_dst->hash_path, nat_dst);
	}
}

rnd_conf_state_t *rnd_conf_state_alloc(void)
{
	rnd_conf_state_t *res = calloc(sizeof(rnd_conf_state_t), 1);

	res->rnd_conf_fields = htsp_alloc(strhash, strkeyeq);
	rnd_conf_state_copy_nat(res->rnd_conf_fields, rnd_conf_fields);
	return res;
}

void rnd_conf_state_free(rnd_conf_state_t *cs)
{
	TODO("free all fields");
	free(cs);
}


#define SAVE(field, reset) \
	memcpy(&dst->field, &field, sizeof(field)); \
	if (reset) memset(&field, 0, sizeof(field));

void rnd_conf_state_save(rnd_conf_state_t *dst)
{
	assert(!dst->valid);

	SAVE(conf_interns, 1);
	SAVE(conf_files_inited, 1);
	SAVE(rnd_conf_in_production, 1);
	SAVE(rnd_conf_main_root, 1);
	SAVE(rnd_conf_main_root_replace_cnt, 1);
	SAVE(rnd_conf_main_root_lock, 1);
	SAVE(rnd_conf_lht_dirty, 1);
	SAVE(rnd_conf_plug_root, 1);
	SAVE(rnd_conf_fields, 0);
	SAVE(merge_subtree, 1);
	SAVE(rnd_conf_rev, 1);

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

void rnd_conf_state_init_from(rnd_conf_state_t *src)
{
	rnd_conf_role_t r;

	conf_interns = src->conf_interns;
	conf_files_inited = src->conf_files_inited;
	rnd_conf_in_production = src->rnd_conf_in_production;

	for(r = 0; r < RND_CFR_max_real; r++) {
		if ((r == RND_CFR_DESIGN) || (r == RND_CFR_PROJECT))
			continue;
		rnd_conf_main_root[r] = src->rnd_conf_main_root[r];
		rnd_conf_main_root_replace_cnt[r] = src->rnd_conf_main_root_replace_cnt[r];
		rnd_conf_main_root_lock[r] = src->rnd_conf_main_root_lock[r];
		rnd_conf_lht_dirty[r] = src->rnd_conf_lht_dirty[r];
		rnd_conf_plug_root[r] = src->rnd_conf_plug_root[r];
	}
}



void rnd_conf_state_plug_reg(void *globvar, long size, const char *cookie)
{
	rnd_conf_plug_state_t *pst = calloc(sizeof(rnd_conf_plug_state_t), 1);
	pst->cookie = cookie;
	pst->globvar = globvar;
	pst->size = size;

	gdl_append(&plug_states, pst, link);

	TODO("gdl_append() in all existing designs");
}

static void rnd_conf_state_plug_unreg_all_cookie_(gdl_list_t *lst, const char *cookie)
{
	rnd_conf_plug_state_t *n, *next;
	for(n = gdl_first(lst); n != NULL; n = next) {
		next = n->link.next;
		if (n->cookie == cookie) {
			gdl_remove(lst, n, link);
			free(n);
		}
	}
}

void rnd_conf_state_plug_unreg_all_cookie(const char *cookie)
{
	rnd_conf_state_plug_unreg_all_cookie_(&plug_states, cookie);
	TODO("rnd_conf_state_plug_unreg_all_cookie_() in all existing designs");
}

void rnd_conf_state_new_design(rnd_design_t *dsg)
{
	rnd_conf_plug_state_t *n, *nnew;

	memset(&dsg->link, 0, sizeof(dsg->link)); /* can't trust caller to zero this */
	gdl_append(&designs, dsg, link);

	for(n = gdl_first(&plug_states); n != NULL; n = n->link.next) {
		nnew = calloc(sizeof(rnd_conf_plug_state_t) + n->size, 1);
		nnew->cookie = n->cookie;
		nnew->globvar = n->globvar;
		nnew->size = n->size;
		gdl_append(&dsg->saved_rnd_conf->plug_states, nnew, link);
	}
}

void rnd_conf_state_del_design(rnd_design_t *dsg)
{
	gdl_remove(&designs, dsg, link);

	rnd_conf_plug_state_t *n, *next;
	for(n = gdl_first(&dsg->saved_rnd_conf->plug_states); n != NULL; n = next) {
		next = n->link.next;
		free(n);
	}
	memset(&dsg->saved_rnd_conf->plug_states, 0, sizeof(dsg->saved_rnd_conf->plug_states));
}


