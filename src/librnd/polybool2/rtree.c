 /*
  *                            COPYRIGHT
  *
  *  pcb-rnd, interactive printed circuit board design
  *  Copyright (C) 2017,2020 Tibor 'Igor2' Palinkas
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

#include <librnd/rnd_config.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <librnd/core/unit.h>
#include "pa_config.h"
#include "rtree.h"

#include <genrtree/genrtree_impl.h>
#include <genrtree/genrtree_search.h>
#include <genrtree/genrtree_delete.h>
#include <genrtree/genrtree_debug.h>
#include "rtree2_compat.h"

/* Temporary compatibility layer for the transition */
rnd_rtree_t *rnd_r_create_tree(void)
{
	rnd_rtree_t *root = malloc(sizeof(rnd_rtree_t));
	rnd_rtree_init(root);
	return root;
}

void rnd_r_destroy_tree(rnd_rtree_t **tree)
{
	rnd_rtree_uninit(*tree);
	free(*tree);
	*tree = NULL;
}

#if PA_VERIFY_RTREE_ADD
static int in_tree(rnd_rtree_t *rtree, const rnd_box_t *which)
{
	rnd_rtree_it_t it;
	rnd_box_t *b;

	for(b = rnd_rtree_first(&it, rtree, which); b != NULL; b = rnd_rtree_next(&it))
		if (b == which)
			return 1;

	return 0;
}
#endif

void rnd_r_insert_entry(rnd_rtree_t *rtree, const rnd_box_t *which)
{
	assert(which != NULL);
#if PA_VERIFY_RTREE_ADD
	assert(!in_tree(rtree, which));
#endif
	rnd_rtree_insert(rtree, (void *)which, (rnd_rtree_box_t *)which); /* assumes first field is the bounding box */
}

void rnd_r_insert_array(rnd_rtree_t *rtree, const rnd_box_t *boxlist[], rnd_cardinal_t len)
{
	rnd_cardinal_t n;

	if (len == 0)
		return;

	assert(boxlist != 0);

	for(n = 0; n < len; n++)
		rnd_r_insert_entry(rtree, boxlist[n]);
}

rnd_bool rnd_r_delete_entry(rnd_rtree_t *rtree, const rnd_box_t *which)
{
	assert(which != NULL);
	return rnd_rtree_delete(rtree, (void *)which, (rnd_rtree_box_t *)which) == 0; /* assumes first field is the bounding box */
}

rnd_bool rnd_r_delete_entry_free_data(rnd_rtree_t *rtree, rnd_box_t *box, void (*free_data)(void *d))
{
	void *obj = box; /* assumes first field is the bounding box */
	assert(obj != NULL);
	if (rnd_rtree_delete(rtree, obj, (rnd_rtree_box_t *)box) != 0)
		return rnd_false;
	free_data(obj);
	return rnd_true;
}

typedef struct {
	rnd_r_dir_t (*region_in_search)(const rnd_box_t *region, void *closure);
	rnd_r_dir_t (*rectangle_in_region)(const rnd_box_t *box, void *closure);
	void *clo;
} r_cb_t;

static rnd_rtree_dir_t r_cb_node(void *ctx_, void *obj, const rnd_rtree_box_t *box)
{
	r_cb_t *ctx = (r_cb_t *)ctx_;
	return ctx->region_in_search((const rnd_box_t *)box, ctx->clo);
}

static rnd_rtree_dir_t r_cb_obj(void *ctx_, void *obj, const rnd_rtree_box_t *box)
{
	r_cb_t *ctx = (r_cb_t *)ctx_;
	return ctx->rectangle_in_region((const rnd_box_t *)obj, ctx->clo);
}


rnd_r_dir_t rnd_r_search(rnd_rtree_t *rtree, const rnd_box_t *query,
	rnd_r_dir_t (*region_in_search)(const rnd_box_t *region, void *closure),
	rnd_r_dir_t (*rectangle_in_region)(const rnd_box_t *box, void *closure),
	void *closure, int *num_found)
{
	rnd_r_dir_t res;
	rnd_rtree_cardinal_t out_cnt;
	r_cb_t ctx;
	ctx.region_in_search = region_in_search;
	ctx.rectangle_in_region = rectangle_in_region;
	ctx.clo = closure;

	res = rnd_rtree_search_any(rtree, (const rnd_rtree_box_t *)query,
		(ctx.region_in_search != NULL) ? r_cb_node : NULL,
		(ctx.rectangle_in_region != NULL) ? r_cb_obj : NULL,
		&ctx, &out_cnt);

	if (num_found != NULL)
		*num_found = out_cnt;

	return res;
}

void rnd_r_free_tree_data(rnd_rtree_t *rtree, void (*free)(void *ptr))
{
	rnd_rtree_it_t it;
	void *o;

	for(o = rnd_rtree_all_first(&it, rtree); o != NULL; o = rnd_rtree_all_next(&it))
		free(o);
}
