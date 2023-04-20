/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2023 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2023)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.*
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "rtree.h"
#include "rtree2_compat.h"

int pa_isc_edge_edge(rnd_vnode_t *v1a, rnd_vnode_t *v1b, rnd_vnode_t *v2a, rnd_vnode_t *v2b, pa_big_vector_t *isc1, pa_big_vector_t *isc2);

typedef struct pa_pp_isc_s {
	rnd_vnode_t *v;         /* input: first node of the edge we are checking */
} pa_pp_isc_t;

static rnd_r_dir_t pa_pp_isc_cb(const rnd_box_t *b, void *cl)
{
	pa_pp_isc_t *ctx = (pa_pp_isc_t *)cl;
	pa_seg_t *s = (pa_seg_t *)b;
	pa_big_vector_t isc1, isc2;
	int num_isc;

	num_isc = pa_isc_edge_edge(s->v, s->v->next, ctx->v, ctx->v->next, &isc1, &isc2);
	if (num_isc > 0)
		return rnd_RTREE_DIR_FOUND_STOP;

	return RND_R_DIR_NOT_FOUND;
}

RND_INLINE int big_bool_ppl_isc(rnd_polyarea_t *pa, rnd_pline_t *pl, rnd_vnode_t *v, int is_hole)
{
	pa_pp_isc_t tmp;
	rnd_polyarea_t *paother;

	for(paother = pa->f; paother != pa; paother = paother->f) {
		rnd_pline_t *plother = paother->contours;

		if (plother != NULL) {
			rnd_box_t box;
			box.X1 = pa_min(v->point[0], v->next->point[0])-1;
			box.Y1 = pa_min(v->point[1], v->next->point[1])-1;
			box.X2 = pa_max(v->point[0], v->next->point[0])+1;
			box.Y2 = pa_max(v->point[1], v->next->point[1])+1;

			tmp.v = v;
			if (rnd_r_search(plother->tree, &box, NULL, pa_pp_isc_cb, &tmp, NULL) & rnd_RTREE_DIR_FOUND)
				return 1;
		}

TODO("if is_hole, need to check other holes as well");

	}
	return 0;
}

RND_INLINE int big_bool_ppl_(rnd_polyarea_t *pa, rnd_pline_t *pl, int is_hole)
{
	rnd_vnode_t *v = pl->head;

	do {
		if (v->flg.rounded) {
			v->flg.rounded = 0;
			if (big_bool_ppl_isc(pa, pl, v->prev, is_hole) || big_bool_ppl_isc(pa, pl, v, is_hole)) {
				TODO("merge pline here");
				rnd_trace("*** big_bool_ppl_() merge!\n");
				return 1;
			}
		}
	} while((v = v->next) != pl->head);

	return 0;
}

/* Does an iteration of postprocessing; returns whether pa changed (0 or 1) */
RND_INLINE int big_bool_ppa_(rnd_polyarea_t *pa)
{
	rnd_polyarea_t *pn = pa;
	do {
		rnd_pline_t *pl = pn->contours;
		if (pl != NULL) {
			if (big_bool_ppl_(pn, pl, 0))
				return 1;

			for(pl = pa->contours->next; pl != NULL; pl = pl->next)
				if (big_bool_ppl_(pn, pl, 0))
					return 1;
		}
	} while ((pn = pn->f) != pa);
	return 0;
}

void pa_big_bool_postproc(rnd_polyarea_t *pa)
{
	/* keep iterating as long as there are things to be fixed */
	while(big_bool_ppa_(pa)) ;
}

