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

#include <librnd/core/error.h>


int pa_isc_edge_edge(rnd_vnode_t *v1a, rnd_vnode_t *v1b, rnd_vnode_t *v2a, rnd_vnode_t *v2b, pa_big_vector_t *isc1, pa_big_vector_t *isc2);

typedef struct pa_pp_isc_s {
	rnd_vnode_t *v;         /* input: first node of the edge we are checking */
} pa_pp_isc_t;

/* Report if there's an intersection between the edge starting from cl->v
   and the edge described by the segment in b */
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

/* Return 1 if there's any intersection between pl and any island
   of pa (including self intersection within the pa) */
RND_INLINE int big_bool_ppl_isc(rnd_polyarea_t *pa, rnd_pline_t *pl, rnd_vnode_t *v)
{
	pa_pp_isc_t tmp;
	rnd_polyarea_t *paother;

	paother = pa;
	do {
		rnd_pline_t *plother = paother->contours;

		if (plother != NULL) {
			rnd_box_t box;
			int res;

rnd_trace(" checking: %ld;%ld - %ld;%ld\n", v->point[0], v->point[1], v->next->point[0], v->next->point[1]);

			box.X1 = pa_min(v->point[0], v->next->point[0])-1;
			box.Y1 = pa_min(v->point[1], v->next->point[1])-1;
			box.X2 = pa_max(v->point[0], v->next->point[0])+1;
			box.Y2 = pa_max(v->point[1], v->next->point[1])+1;

			tmp.v = v;
			res = rnd_r_search(plother->tree, &box, NULL, pa_pp_isc_cb, &tmp, NULL);
			rnd_trace("  res=%d %d (return: %d)\n", res, rnd_RTREE_DIR_FOUND, (res & rnd_RTREE_DIR_FOUND));
			if (res & rnd_RTREE_DIR_FOUND)
				return 1;
		}

	} while((paother = paother->f) != pa);

	return 0;
}


/* Check each vertex in pl: if it is risky, check if there's any intersection
   on the incoming or outgoing edge of that vertex. If there is, stop and
   return 1, otherwise return 0. */
RND_INLINE int big_bool_ppl_(rnd_polyarea_t *pa, rnd_pline_t *pl)
{
	rnd_vnode_t *v = pl->head;
	int res = 0;

	do {
		if (v->flg.risk) {
			v->flg.risk = 0;
rnd_trace("check risk for self-intersection:\n");
			if (!res && (big_bool_ppl_isc(pa, pl, v->prev) || big_bool_ppl_isc(pa, pl, v))) {
rnd_trace("  self-intersection occured! Shedule selfi-resolve\n");
				res = 1; /* can't return here, we need to clear all the v->flg.risk bits */
			}
		}
	} while((v = v->next) != pl->head);

	return res;
}

/* Does an iteration of postprocessing; returns whether pa changed (0 or 1) */
RND_INLINE int big_bool_ppa_(rnd_polyarea_t **pa)
{
	int res = 0;
	rnd_polyarea_t *pn = *pa;

	do {
		rnd_pline_t *pl = pn->contours;
		if (pl != NULL) {

			/* One poly island of pa intersects another poly island of pa. This
			   happens when high precision coord of non-intersecting islands are
			   rounded to output integers and the rounding error introduces
			   a new crossing somewhere. Merge the two islands. */
			for(pl = (*pa)->contours; pl != NULL; pl = pl->next)
				if (big_bool_ppl_(pn, pl))
					res = 1; /* can not return here, need to clear all the risk flags */
		}
	} while ((pn = pn->f) != *pa);
	return res;
}

void pa_big_bool_postproc(rnd_polyarea_t **pa)
{
	if (big_bool_ppa_(pa))
		rnd_polyarea_split_selfisc(pa);
}

