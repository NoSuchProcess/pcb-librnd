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

/* conf system multi-design support (conf struct global var swapping) */

#include "conf_multi.h"
#include "project.h"

gdl_list_t rnd_designs;


typedef struct rnd_conf_plug_state_s {
	const char *cookie;    /* compared as pointer */
	void *globvar;         /* pointer to the global variable the plugin is using */
	long size;             /* sizeof() the globvar struct */
	gdl_elem_t link;       /* either in global plug_states or in rnd_conf_state_t's plug_states */
	char saved[1];         /* (overallocated) a copy of the last content of globvar when design is not active (switched away); not used in global plug_states */
} rnd_conf_plug_state_t;

static gdl_list_t plug_states;

/* When 1, do not keep a per design copy, all designs should use the central */
static const char rnd_conf_main_root_is_shared[RND_CFR_max_alloc] = {
	1, /* RND_CFR_INTERNAL */
	1, /* RND_CFR_SYSTEM */
	1, /* RND_CFR_DEFAULTDSG */
	1, /* RND_CFR_USER */
	1, /* RND_CFR_ENV */
	0, /* RND_CFR_PROJECT */
	0, /* RND_CFR_DESIGN */
	1, /* RND_CFR_CLI */
	1, /* RND_CFR_max_real */
	0, /* RND_CFR_file */
	0  /* RND_CFR_binary */
};

static long rnd_conf_lht_last_edits[RND_CFR_max_alloc]; /* shared docs: remember which edit revision got merged last */

struct rnd_conf_state_s {
	htsi_t conf_interns;
	int conf_files_inited;
	int rnd_conf_in_production;

	/* from these not all are duplicated, see rnd_conf_main_root_is_shared[] */
	lht_doc_t *rnd_conf_main_root[RND_CFR_max_alloc];
	long rnd_conf_main_root_replace_cnt[RND_CFR_max_alloc];
	int rnd_conf_main_root_lock[RND_CFR_max_alloc];
	int rnd_conf_lht_dirty[RND_CFR_max_alloc];

	long rnd_conf_lht_last_edits[RND_CFR_max_alloc];

/*	lht_doc_t *rnd_conf_plug_root[RND_CFR_max_alloc];  - always shared for all roles as we are not editing or reloading these */ 
	htsp_t *rnd_conf_fields;
	vmst_t merge_subtree;
	int rnd_conf_rev;
	rnd_conf_t rnd_conf;

	int valid;

	gdl_list_t plug_states; /* extra saves */
};

static rnd_conf_native_t *rnd_conf_dup_field_into(htsp_t *dst, const rnd_conf_native_t *nat_src)
{
	rnd_conf_native_t *nat_dst;
	nat_dst = rnd_conf_alloc_field_(nat_src->val.any, nat_src->array_size,
		nat_src->type, nat_src->hash_path, nat_src->description, nat_src->flags, 0);
	nat_dst->shared = nat_src->shared;
	htsp_set(dst, (char *)nat_dst->hash_path, nat_dst);
	return nat_dst;
}

static void rnd_conf_state_copy_nats(htsp_t *dst, const htsp_t *src)
{
	htsp_entry_t *e;
	for(e = htsp_first(src); e != NULL; e = htsp_next(src, e))
		rnd_conf_dup_field_into(dst, (rnd_conf_native_t *)e->value);
}

rnd_conf_state_t *rnd_conf_state_alloc(void)
{
	rnd_conf_state_t *res = calloc(sizeof(rnd_conf_state_t), 1);

	res->rnd_conf_fields = htsp_alloc(strhash, strkeyeq);
	rnd_conf_state_copy_nats(res->rnd_conf_fields, rnd_conf_fields_master);
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

#define SAVE_SHARED(field, reset) \
	for(r = 0; r < RND_CFR_max_real; r++) { \
		if (!rnd_conf_main_root_is_shared[r]) { \
			memcpy(&dst->field[r], &field[r], sizeof(field[r])); \
			if (reset) memset(&field[r], 0, sizeof(field[r])); \
		} \
	}

void rnd_conf_state_save(rnd_conf_state_t *dst)
{
	rnd_conf_role_t r;
	rnd_conf_plug_state_t *n;
	assert(!dst->valid);

	SAVE(conf_interns, 1);
	SAVE(conf_files_inited, 1);
	SAVE(rnd_conf_in_production, 1);
	SAVE_SHARED(rnd_conf_main_root, 1);
	SAVE_SHARED(rnd_conf_main_root_replace_cnt, 1);
	SAVE_SHARED(rnd_conf_main_root_lock, 1);
	SAVE_SHARED(rnd_conf_lht_dirty, 1);
	SAVE(rnd_conf_lht_last_edits, 0);

/*	SAVE(rnd_conf_plug_root, 1);*/
	SAVE(rnd_conf_fields, 0);
	SAVE(merge_subtree, 1);
	SAVE(rnd_conf_rev, 1);
	SAVE(rnd_conf, 1);

	for(n = gdl_first(&dst->plug_states); n != NULL; n = n->link.next) {
		memcpy(&n->saved, n->globvar, n->size);
		memset(n->globvar, 0, n->size); /* for easier debug */
	}

	dst->valid = 1;
}

#undef SAVE

#define LOAD(field) \
	memcpy(&field, &src->field, sizeof(field)); \
	memset(&src->field, 0, sizeof(field));

#define LOAD_SHARED(field) \
	for(r = 0; r < RND_CFR_max_real; r++) { \
		if (!rnd_conf_main_root_is_shared[r]) { \
			memcpy(&field[r], &src->field[r], sizeof(field[r])); \
			memset(&src->field[r], 0, sizeof(field[r])); \
		} \
	}

void rnd_conf_state_load(rnd_conf_state_t *src)
{
	rnd_conf_role_t r;
	rnd_conf_plug_state_t *n;

	assert(src->valid);

	LOAD(conf_interns);
	LOAD(conf_files_inited);
	LOAD(rnd_conf_in_production);
	LOAD_SHARED(rnd_conf_main_root);
	LOAD_SHARED(rnd_conf_main_root_replace_cnt);
	LOAD_SHARED(rnd_conf_main_root_lock);
	LOAD_SHARED(rnd_conf_lht_dirty);
	LOAD(rnd_conf_lht_last_edits);
/*	LOAD(rnd_conf_plug_root);*/
	LOAD(rnd_conf_fields);
	LOAD(merge_subtree);
	LOAD(rnd_conf_rev);
	LOAD(rnd_conf);

	for(n = gdl_first(&src->plug_states); n != NULL; n = n->link.next) {
		memcpy(n->globvar, &n->saved, n->size);
		memset(&n->saved, 0, n->size); /* for easier debug */
	}

	src->valid = 0;
}

#undef LOAD

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
	gdl_append(&rnd_designs, dsg, link);

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
	rnd_conf_plug_state_t *n, *next;

	gdl_remove(&rnd_designs, dsg, link);

	for(n = gdl_first(&dsg->saved_rnd_conf->plug_states); n != NULL; n = next) {
		next = n->link.next;
		free(n);
	}
	memset(&dsg->saved_rnd_conf->plug_states, 0, sizeof(dsg->saved_rnd_conf->plug_states));
}


void rnd_conf_multi_merge_after_switch(rnd_design_t *dsg)
{
	rnd_conf_role_t r;
	int need_merge = 0;

/*	rnd_trace("SW: %p\n", dsg);*/
	for(r = 0; r < RND_CFR_max_real; r++) {
		if (rnd_conf_lht_last_edits[r] < rnd_conf_lht_edits[r]) {
/*			rnd_trace(" [%d] %d < %d\n", r, rnd_conf_lht_last_edits[r], rnd_conf_lht_edits[r]);*/
			need_merge = 1;
		}
		rnd_conf_lht_last_edits[r] = rnd_conf_lht_edits[r];
	}

	if (need_merge) {
/*		rnd_trace(" MERGE\n");*/
		rnd_conf_merge_all(NULL);
	}

	/*rnd_trace("after switch conf root design: %p\n", rnd_conf_main_root[RND_CFR_DESIGN]);*/
}

void rnd_conf_multi_pre_new_design(rnd_conf_state_t **ncs)
{
	rnd_conf_main_root[RND_CFR_PROJECT] = NULL;
	rnd_conf_main_root[RND_CFR_DESIGN] = NULL;
	*ncs = rnd_conf_state_alloc();
	rnd_conf_fields = (*ncs)->rnd_conf_fields;
}

void rnd_conf_multi_post_new_design(rnd_conf_state_t **ncs, rnd_design_t *dsg)
{
	if (dsg == NULL) {
		/* failed to load/create design */
		rnd_conf_state_free(*ncs);
	}
	else {
		/* new design is dsg */
		dsg->saved_rnd_conf = *ncs;
	}
	*ncs = NULL;
}

void rnd_multi_load_prj_for_dsg(rnd_design_t *dsg)
{
	rnd_project_t *prj = dsg->project;

	if (prj == NULL)
		return;

	/*rnd_trace("prj4dsg: d=%p p=%p %p glob=%p\n", dsg, prj, prj->root, rnd_conf_main_root[RND_CFR_PROJECT]);*/
	if (prj->root == NULL) {
		/* initial root with the first design the project file is loaded with */
		rnd_conf_load_as(RND_CFR_PROJECT, prj->fullpath, 0);
		prj->root = rnd_conf_main_root[RND_CFR_PROJECT];
		/*rnd_trace(" load\n");*/
	}
	else {
		/* we have already loaded the project conf, just bind it */
		rnd_conf_free(RND_CFR_PROJECT);
		rnd_conf_main_root[RND_CFR_PROJECT] = prj->root;
		rnd_conf_merge_all(NULL);
		/*rnd_trace(" bind\n");*/
	}

}
