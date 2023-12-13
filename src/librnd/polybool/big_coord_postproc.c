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
	int pp_overlap;
} pa_pp_isc_t;

/* Report if there's an intersection between the edge starting from cl->v
   and the edge described by the segment in b */
static rnd_r_dir_t pa_pp_isc_cb(const rnd_box_t *b, void *cl)
{
	pa_pp_isc_t *ctx = (pa_pp_isc_t *)cl;
	pa_seg_t *s = (pa_seg_t *)b;
	pa_big_vector_t isc1, isc2;
	int num_isc, refuse_2isc = 0;
	rnd_vnode_t *pp_other;

	TODO("arc: need to figure self isc and overlapping arc-arc cases for T shaped self isc here");
#if 0
	rnd_trace(" ppisc: ctx = %p:%ld;%ld - %p:%ld;%ld\n", ctx->v, ctx->v->point[0], ctx->v->point[1], ctx->v->next, ctx->v->next->point[0], ctx->v->next->point[1]);
	rnd_trace("        seg = %p:%ld;%ld - %p:%ld;%ld - %p:%ld;%ld\n", s->v->prev, s->v->prev->point[0], s->v->prev->point[1], s->v, s->v->point[0], s->v->point[1], s->v->next, s->v->next->point[0], s->v->next->point[1]);
#endif

	/* detect point-point overlap of rounded coords; this can be the basis of
	   an X-topology self-intersection, see test case fixedo */
	if ((s->v->next != ctx->v) && (s->v->next->point[0] == ctx->v->point[0]) && (s->v->next->point[1] == ctx->v->point[1])) {
		rnd_trace("   pp-overlap at %ld %ld\n", ctx->v->point[0], ctx->v->point[1]);
		ctx->pp_overlap = 1;
		pp_other = s->v->next;
	}
	else if ((s->v != ctx->v) && (s->v->point[0] == ctx->v->point[0]) && (s->v->point[1] == ctx->v->point[1])) {
		rnd_trace("   pp-overlap at %ld %ld\n", ctx->v->point[0], ctx->v->point[1]);
		ctx->pp_overlap = 1;
		pp_other = s->v;
	}

	if (ctx->pp_overlap) { /* allocate preliminary cvc at the overlap so the cvc crossing list can be built later */
		pa_big_vector_t pt;

		pa_big_load(pt.x, ctx->v->point[0]);
		pa_big_load(pt.y, ctx->v->point[1]);

		if (ctx->v->cvclst_prev == NULL) {
			ctx->v->cvclst_prev = pa_prealloc_conn_desc(pt);
			ctx->v->cvclst_prev->PP_OTHER = pp_other;
rnd_trace("  pp_other set: %p -> %p\n", ctx->v, pp_other);
		}
		if (ctx->v->cvclst_next == NULL) {
			ctx->v->cvclst_next = pa_prealloc_conn_desc(pt);
			ctx->v->cvclst_prev->PP_OTHER = pp_other;
rnd_trace("  pp_other sEt: %p -> %p\n", ctx->v, pp_other);
		}
	}

	/* T shaped self intersection: one endpoint of ctx->v is the same as the
	   neighbor's (s) endpoint, the other endpoint of ctx->v is on s.
	   Test case: fixedf */
	if ((s->v->next == ctx->v) || (s->v == ctx->v)) {
		if ((s->v == ctx->v) && (s->v->next == ctx->v->next)) {
			/* if endpoints fully match by pointer that means we've found the very same line - skip this check, it's not a self intersection */
			rnd_trace("  skip same edge isc 1\n");
			refuse_2isc = 1;
			goto skip1;
		}

		if (pa_big_is_node_on_line(ctx->v->next, s->v, s->v->next)) {
			rnd_trace("   offend T 1: %ld;%ld - %ld;%ld ctx=%ld;%ld - %ld;%ld\n", s->v->point[0], s->v->point[1], s->v->next->point[0], s->v->next->point[1], ctx->v->point[0], ctx->v->point[1], ctx->v->next->point[0], ctx->v->next->point[1]);
			rnd_trace("               %p - %p       %p - %p\n", s->v, s->v->next, ctx->v, ctx->v->next);
			return rnd_RTREE_DIR_FOUND_STOP;
		}
		return RND_R_DIR_NOT_FOUND;
	}

	skip1:;
	num_isc = pa_isc_edge_edge(s->v, s->v->next, ctx->v, ctx->v->next, &isc1, &isc2);
/*rnd_trace(" num_isc=%d refuse_2isc=%d\n", num_isc, refuse_2isc);*/
	if (refuse_2isc && (num_isc == 2))
		return RND_R_DIR_NOT_FOUND;

/*
rnd_trace("  ? isc? %ld;%ld %ld;%ld   with  %ld;%ld %ld;%ld -> %d\n",
	s->v->point[0], s->v->point[1], s->v->next->point[0], s->v->next->point[1],
	ctx->v->point[0], ctx->v->point[1], ctx->v->next->point[0], ctx->v->next->point[1],
	num_isc);
*/

	/* ignore perfect endpoint matches - self-touching is okay, self-intersection
	   is the problem; typical example is bowtie: it's split up into two triangles
	   with the same vertex in the center, but that's not self-intersection.
	   Note: num_isc == 2 means overlapping lines, that needs to be handled. */
	if (num_isc == 1) {
		pa_big_vector_t s1, s2, c1, c2;
		pa_big_load_cvc(&s1, s->v);
		pa_big_load_cvc(&s2, s->v->next);
		pa_big_load_cvc(&c1, ctx->v);
		pa_big_load_cvc(&c2, ctx->v->next);
/*rnd_trace("    IGNORE: %d %d %d %d\n", pa_big_vect_equ(s1, isc1), pa_big_vect_equ(s2, isc1), pa_big_vect_equ(c1, isc1), pa_big_vect_equ(c2, isc1));*/
		if (pa_big_vect_equ(s1, isc1) || pa_big_vect_equ(s2, isc1) || pa_big_vect_equ(c1, isc1) || pa_big_vect_equ(c2, isc1))
			num_isc--;
	}

	if (num_isc > 0) {
		rnd_trace("   offend0: %ld;%ld - %ld;%ld numisc=%d\n", s->v->point[0], s->v->point[1], s->v->next->point[0], s->v->next->point[1], num_isc);
		return rnd_RTREE_DIR_FOUND_STOP;
	}

	return RND_R_DIR_NOT_FOUND;
}

/* Return 1 if there's any intersection between pl and any island
   of pa (including self intersection within the pa). Sets *pp_overlap_out
   to 1 if there's a point-point overlap is detected */
RND_INLINE int big_bool_ppl_isc(rnd_polyarea_t *pa, rnd_pline_t *pl, rnd_vnode_t *v, int *pp_overlap_out)
{
	pa_pp_isc_t tmp;
	rnd_polyarea_t *paother;

	paother = pa;
	do {
		rnd_pline_t *plother;

		for(plother = paother->contours; plother != NULL; plother = plother->next) {
			rnd_box_t box;
			int res;


			box.X1 = pa_min(v->point[0], v->next->point[0])-1;
			box.Y1 = pa_min(v->point[1], v->next->point[1])-1;
			box.X2 = pa_max(v->point[0], v->next->point[0])+1;
			box.Y2 = pa_max(v->point[1], v->next->point[1])+1;

			if ((plother->xmax < box.X1) || (plother->ymax < box.Y1)) continue;
			if ((plother->xmin > box.X2) || (plother->ymin > box.Y2)) continue;

rnd_trace(" checking: %ld;%ld - %ld;%ld\n", v->point[0], v->point[1], v->next->point[0], v->next->point[1]);

			tmp.v = v;
			tmp.pp_overlap = 0;
			res = rnd_r_search(plother->tree, &box, NULL, pa_pp_isc_cb, &tmp, NULL);
			rnd_trace("  res=%d %d (intersected: %d pp-overlap: %d)\n", res, rnd_RTREE_DIR_FOUND, (res & rnd_RTREE_DIR_FOUND), tmp.pp_overlap);
			if (tmp.pp_overlap)
				*pp_overlap_out = 1;
			if (res & rnd_RTREE_DIR_FOUND)
				return 1;
		}

	} while((paother = paother->f) != pa);

	return 0;
}


static int seg_too_short(rnd_vnode_t *a, rnd_vnode_t *b)
{
	/* See triangle flip in test case fixedx; worst case it can happen
	   ends up in two corners diagonally arranged with both dx and dy
	   being +-1. The square distance in that case equals to 2. */
	return rnd_vect_dist2(a->point, b->point) <= 2.0;
}

/* Check each vertex in pl: if it is risky, check if there's any intersection
   on the incoming or outgoing edge of that vertex. If there is, stop and
   return 1, otherwise return 0. */
RND_INLINE int big_bool_ppl_(rnd_polyarea_t *pa, rnd_pline_t *pl, int already_bad)
{
	rnd_vnode_t *v = pl->head, *next;
	int res = 0, rebuild_tree = 0, pp_overlap = 0;

	do {
		if (v->flg.risk) {
			v->flg.risk = 0;
rnd_trace("check risk for self-intersection at %ld;%ld:\n", v->point[0], v->point[1]);
			if (!res && (seg_too_short(v->prev, v) || seg_too_short(v, v->next))) {
				/* Related test case: fixedx; causes triangle flip; can't happen
				   if the length of the edge of the triangle is larger than our rounding
				   limit. Since this is checked only if v is rounded, it happens
				   rarely so it is okay to schedule an expensive self-isc check
				   that will then do a real triangle-flip detection. */
				rnd_trace("  segment too short next to a rounded corner! Shedule selfi-resolve\n");
				res = 1;
			}
			else if (!res && (big_bool_ppl_isc(pa, pl, v->prev, &pp_overlap) || big_bool_ppl_isc(pa, pl, v, &pp_overlap))) {
rnd_trace("  self-intersection occured! Shedule selfi-resolve\n");
				res = 1; /* can't return here, we need to clear all the v->flg.risk bits */
			}
		}

		assert(v->cvclst_next == NULL || v->cvclst_next->prelim);

		next = v->next;

		/* remove redundant neighbor points */
		if ((v->point[0] == v->next->point[0]) && (v->point[1] == v->next->point[1])) {
			v->prev->next = next;
			next->prev = v->prev;
			if (v->cvclst_prev != NULL)
				free(v->cvclst_prev);
			if (v->cvclst_next != NULL)
				free(v->cvclst_next);
			free(v);
			pl->Count--;
			rebuild_tree = 1; /* we can't easily remove the segment, unfortunately */
		}

	} while((v = next) != pl->head);

	if (rebuild_tree) {
		rnd_r_free_tree_data(pl->tree, free);
		rnd_r_destroy_tree(&pl->tree);
		pl->tree = rnd_poly_make_edge_tree(pl);
	}


	/* Do the expensive point-point overlap X-crossing test only if
	   there was a point-point overlap and if we still think the
	   polygon is not self-intersecting. If it is already classified
	   self-intersecting, skip this test, the selfisc resolver will
	   handle all occurances without prior verification here */
	if (pp_overlap && !already_bad && (res == 0)) {
		/* There was a point-point overlap somewhere along this pline. This
		   means either a >< kind of self-touch or a X kind of crossing in
		   that point. We need to build the cvc lists to figure. */
		rnd_vnode_t *n, *pp_other;

		rnd_trace(" pp-overlap X-crossing risk...\n");
		pa_add_conn_desc(pl, 'A', NULL);

		/* evaluate crossings */
		n = pl->head;
		do {
			if (n->cvclst_prev != NULL) {
				rnd_trace("  X-crossing check at %ld;%ld\n", n->point[0], n->point[1]);
				pp_other = n->cvclst_prev->PP_OTHER;
rnd_trace("  pp_other get: %p -> %p\n", n, pp_other);

				if (pa_cvc_crossing_at_node(n)) {
					res = 1;
					rnd_trace("   X-crossing detected!! schedule selfisc\n");
				}
				else if (pa_cvc_line_line_overlap(n, pp_other)) {
					res = 1;
					rnd_trace("   LL-overlap detected!! schedule selfisc\n");
				}
			}
		} while((res == 0) && (n = n->next) != pl->head);
	}

	if (pp_overlap) { /* remove the cvc's to keep output clean */
		rnd_vnode_t *n = pl->head;
		do {
			if (n->cvclst_prev != NULL) {
				free(n->cvclst_prev);
				free(n->cvclst_next);
				n->cvclst_prev = n->cvclst_next = NULL;
				/* no need to unlink, all cvcs are free'd here */
			}
		} while((n = n->next) != pl->head);
	}

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
			for(pl = pn->contours; pl != NULL; pl = pl->next)
				if (big_bool_ppl_(pn, pl, res))
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

