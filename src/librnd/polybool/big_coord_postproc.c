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

/* Return 1 if there's any intersection between pl and any _other_ island
   of pa (assumes no self intersection within an island) */
RND_INLINE int big_bool_ppl_isc(rnd_polyarea_t *pa, rnd_pline_t *pl, rnd_vnode_t *v, int is_hole)
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

TODO("if is_hole, need to check other holes as well");

	} while((paother = paother->f) != pa);

	return 0;
}

void pa_remove_contour(rnd_polyarea_t *pa, rnd_pline_t *prev_contour, rnd_pline_t *contour, int remove_from_rtree);

/* pa intersects another island (a sibling of pa). Merge them and return the new,
   merged polyarea that contains (at least) one less island. */
RND_INLINE rnd_polyarea_t *big_bool_ppl_merge(rnd_polyarea_t *pa, rnd_pline_t *pl, int pl_is_hole)
{
	rnd_polyarea_t *pb, *res = NULL;

	if (pa->f == pa) {
		/* self intersection */
		TODO("this needs to be hanled differently, see pa_self_isc");
		rnd_trace("    can't handle\n");
		return NULL;
	}

	/* pb is the "rest of the polygon", pa is removed from it to be a separate island */
	pb = pa->f;
	pa->b->f = pa->f;
	pa->f->b = pa->b;
	pa->f = pa->b = pa;

	/* unite pa with the rest (pb) */
/*	rnd_trace("merge\n");*/
	if (rnd_polyarea_boolean_free(pa, pb, &res, RND_PBO_UNITE) == 0) {
/*		rnd_trace(" Yes!\n");*/
		return res;
	}

	return NULL;
}

/* Check each vertex in pl: if it is rounded, check if there's any intersection
   on the incoming or outgoing edge of that vertex. If there is, the island
   of this polyline needs to be merged and the resulting polyarea that
   replaces pa is returned. Returns NULL if no merge done. */
RND_INLINE rnd_polyarea_t *big_bool_ppl_(rnd_polyarea_t *pa, rnd_pline_t *pl, int is_hole)
{
	rnd_vnode_t *v = pl->head;

	do {
		if (v->flg.risk) {
			v->flg.risk = 0;
rnd_trace("check risk:\n");
			if (big_bool_ppl_isc(pa, pl, v->prev, is_hole) || big_bool_ppl_isc(pa, pl, v, is_hole)) {
rnd_trace("  oops\n");
				return big_bool_ppl_merge(pa, pl, is_hole);
			}
		}
	} while((v = v->next) != pl->head);

	return NULL;
}

/* Does an iteration of postprocessing; returns whether pa changed (0 or 1) */
RND_INLINE int big_bool_ppa_(rnd_polyarea_t **pa)
{
	rnd_polyarea_t *pn = *pa;
	do {
		rnd_pline_t *pl = pn->contours;
		if (pl != NULL) {

			/* One poly island of pa intersects another poly island of pa. This
			   happens when high precision coord of non-intersecting islands are
			   rounded to output integers and the rounding error introduces
			   a new crossing somewhere. Merge the two islands. */
			rnd_polyarea_t *res = big_bool_ppl_(pn, pl, 0);
			if (res != NULL) {
				*pa = res;
				return 1;
			}

			TODO("Think over if this can happen to cutouts as well");
			for(pl = (*pa)->contours->next; pl != NULL; pl = pl->next) {
				rnd_polyarea_t *res = big_bool_ppl_(pn, pl, 1);
				if (res != NULL) {
					*pa = res;
					return 1;
				}
			}
		}
	} while ((pn = pn->f) != *pa);
	return 0;
}

void pa_big_bool_postproc(rnd_polyarea_t **pa)
{
	/* keep iterating as long as there are things to be fixed */
	while(big_bool_ppa_(pa)) ;
}

