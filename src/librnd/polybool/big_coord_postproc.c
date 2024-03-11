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

#include "pa_config.h"

#include "rtree.h"
#include "rtree2_compat.h"

#include <librnd/core/error.h>


#if DEBUG_RISK
#	undef DEBUG_RISK
#	define DEBUG_RISK rnd_trace
#else
RND_INLINE PA_RISK_DUMMY(const char *fmt, ...) {}
#	undef DEBUG_RISK
#	define DEBUG_RISK PA_RISK_DUMMY
#endif


/* There's a line between vnodes l1 and l2, with an isc point in the middle;
   sp-s-sn is another line touching isc in s; return 1 if s polyline is
   actually going through (crossing) l, return 0 if it's bouncing back.
   Similar to pa_cvc_crossing_at_node(), but this happened after rounding
   so l1 doesn't have any node at isc and thus doesn't have a CVC to work from. */
static int pa_pp_crossing_in_touchpoint(pa_big_vector_t isc, rnd_vnode_t *l1, rnd_vnode_t *l2, rnd_vnode_t *sp, rnd_vnode_t *s, rnd_vnode_t *sn)
{
#if 1
	pa_big_angle_t al, asp, asn;
	rnd_coord_t qsp, qsn, side_sp, side_sn;

	DEBUG_RISK("   Bounce back? l: %ld;%ld .. %ld;%ld; s:  %ld;%ld .. %ld;%ld .. %ld;%ld\n",
		l1->point[0], l1->point[1], l2->point[0], l2->point[1],
		sp->point[0], sp->point[1], s->point[0], s->point[1], sn->point[0], sn->point[1]);

	pa_big_calc_angle_nn(&al, l1, l2);
	pa_big_calc_angle_nn(&asp, s, sp);
	pa_big_calc_angle_nn(&asn, s, sn);

	pa_angle_sub(asp, asp, al);
	pa_angle_sub(asn, asn, al);

	qsp = asp[4]; if (qsp < 0) qsp += 4;
	qsn = asn[4]; if (qsn < 0) qsn += 4;

	side_sp = qsp < 2;
	side_sn = qsn < 2;

	DEBUG_RISK("    angles: qsp:%d qsn:%d side_sp:%d side_sn:%d\n", qsp, qsn, side_sp, side_sn);
	if (side_sp != side_sn)
		return 1;
#endif

	return 0;
}


int pa_isc_edge_edge(rnd_vnode_t *v1a, rnd_vnode_t *v1b, rnd_vnode_t *v2a, rnd_vnode_t *v2b, pa_big_vector_t *isc1, pa_big_vector_t *isc2);

typedef struct pa_pp_isc_s {
	rnd_vnode_t *v;         /* input: first node of the edge we are checking */
	int pp_overlap;
	int from_selfisc;
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
		DEBUG_RISK("   pp-overlap at %ld %ld\n", ctx->v->point[0], ctx->v->point[1]);
		ctx->pp_overlap = 1;
		pp_other = s->v->next;
	}
	else if ((s->v != ctx->v) && (s->v->point[0] == ctx->v->point[0]) && (s->v->point[1] == ctx->v->point[1])) {
		DEBUG_RISK("   pp-overlap at %ld %ld\n", ctx->v->point[0], ctx->v->point[1]);
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
		}
		if (ctx->v->cvclst_next == NULL) {
			ctx->v->cvclst_next = pa_prealloc_conn_desc(pt);
			ctx->v->cvclst_prev->PP_OTHER = pp_other;
		}
	}

	/* T shaped self intersection: one endpoint of ctx->v is the same as the
	   neighbor's (s) endpoint, the other endpoint of ctx->v is on s.
	   Test case: fixedf */
	if ((s->v->next == ctx->v) || (s->v == ctx->v)) {
		if ((s->v == ctx->v) && (s->v->next == ctx->v->next)) {
			/* if endpoints fully match by pointer that means we've found the very same line - skip this check, it's not a self intersection */
			DEBUG_RISK("  skip same edge isc 1\n");
			refuse_2isc = 1;
			goto skip1;
		}

		if (pa_big_is_node_on_line(ctx->v->next, s->v, s->v->next)) {
			DEBUG_RISK("   offend T 1: %ld;%ld - %ld;%ld ctx=%ld;%ld - %ld;%ld\n", s->v->point[0], s->v->point[1], s->v->next->point[0], s->v->next->point[1], ctx->v->point[0], ctx->v->point[1], ctx->v->next->point[0], ctx->v->next->point[1]);
			DEBUG_RISK("               %p - %p       %p - %p\n", s->v, s->v->next, ctx->v, ctx->v->next);
			return rnd_RTREE_DIR_FOUND_STOP;
		}
		return RND_R_DIR_NOT_FOUND;
	}

	skip1:;
	num_isc = pa_isc_edge_edge(s->v, s->v->next, ctx->v, ctx->v->next, &isc1, &isc2);
/*DEBUG_RISK(" num_isc=%d refuse_2isc=%d\n", num_isc, refuse_2isc);*/
	if (refuse_2isc && (num_isc == 2))
		return RND_R_DIR_NOT_FOUND;

/*
DEBUG_RISK("  ? isc? %ld;%ld %ld;%ld   with  %ld;%ld %ld;%ld -> %d\n",
	s->v->point[0], s->v->point[1], s->v->next->point[0], s->v->next->point[1],
	ctx->v->point[0], ctx->v->point[1], ctx->v->next->point[0], ctx->v->next->point[1],
	num_isc);
*/

	/* ignore perfect endpoint matches - self-touching is okay, self-intersection
	   is the problem; typical example is bowtie: it's split up into two triangles
	   with the same vertex in the center, but that's not self-intersection.
	   Note: num_isc == 2 means overlapping lines, that needs to be handled. */
	if (num_isc == 1) {
		int on_s, on_c, on_s2, on_c2;
		pa_big_vector_t s1, s2, c1, c2;
		pa_big_load_cvc(&s1, s->v);
		pa_big_load_cvc(&s2, s->v->next);
		pa_big_load_cvc(&c1, ctx->v);
		pa_big_load_cvc(&c2, ctx->v->next);
/*DEBUG_RISK("    IGNORE: %d %d %d %d\n", pa_big_vect_equ(s1, isc1), pa_big_vect_equ(s2, isc1), pa_big_vect_equ(c1, isc1), pa_big_vect_equ(c2, isc1));*/

		/* Special case: there may be an edge that goes through a node without
		   having a node there with CVC. This can be detected as: there is only
		   one isc point and it's the endpoint of s or c, but not both. */
		on_s = pa_big_vect_equ(s1, isc1) || (on_s2 = pa_big_vect_equ(s2, isc1));
		on_c = pa_big_vect_equ(c1, isc1) || (on_c2 = pa_big_vect_equ(c2, isc1));
		if (!ctx->from_selfisc && on_s && (on_s != on_c)) {
			/* In that case, there are two possibilities: the 2-segment line
			   that's having its middle point on the other line is either
			   "bouncing back" or crossing the line.
			   Test case for crossing: gixed5 around 265;586, where the line
			   is 256;587..265;585 */
			if (on_s) {
				/* Line is ctx, 2-seg line is s */
				if (on_s2) {
					if (pa_pp_crossing_in_touchpoint(isc1, ctx->v, ctx->v->next,  s->v, s->v->next, s->v->next->next))
						return rnd_RTREE_DIR_FOUND_STOP;
				}
				else {
					if (pa_pp_crossing_in_touchpoint(isc1, ctx->v, ctx->v->next,  s->v->prev, s->v, s->v->next))
						return rnd_RTREE_DIR_FOUND_STOP;
				}
			}
			else if (on_c) {
				/* Line is s, 2-seg line is ctx */
				if (on_c2) {
					if (pa_pp_crossing_in_touchpoint(isc1, s->v, s->v->next,  ctx->v, ctx->v->next, ctx->v->next->next))
						return rnd_RTREE_DIR_FOUND_STOP;
				}
				else {
					if (pa_pp_crossing_in_touchpoint(isc1, s->v, s->v->next,  ctx->v->prev, ctx->v, ctx->v->next))
						return rnd_RTREE_DIR_FOUND_STOP;
				}
			}
		}
		if (on_s || on_c)
			num_isc--;
	}

	if (num_isc > 0) {
		DEBUG_RISK("   offend0: %ld;%ld - %ld;%ld numisc=%d\n", s->v->point[0], s->v->point[1], s->v->next->point[0], s->v->next->point[1], num_isc);
		return rnd_RTREE_DIR_FOUND_STOP;
	}

	return RND_R_DIR_NOT_FOUND;
}

/* Return 1 if there's any intersection between pl and any island
   of pa (including self intersection within the pa). Sets *pp_overlap_out
   to 1 if there's a point-point overlap is detected */
RND_INLINE int big_bool_ppl_isc(rnd_polyarea_t *pa, rnd_pline_t *pl, rnd_vnode_t *v, int *pp_overlap_out, int from_selfisc)
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

DEBUG_RISK(" checking: %ld;%ld - %ld;%ld\n", v->point[0], v->point[1], v->next->point[0], v->next->point[1]);

			tmp.v = v;
			tmp.pp_overlap = 0;
			tmp.from_selfisc = from_selfisc;
			res = rnd_r_search(plother->tree, &box, NULL, pa_pp_isc_cb, &tmp, NULL);
			DEBUG_RISK("  res=%d %d (intersected: %d pp-overlap: %d)\n", res, rnd_RTREE_DIR_FOUND, (res & rnd_RTREE_DIR_FOUND), tmp.pp_overlap);
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

RND_INLINE int seg_is_stub(rnd_vnode_t *edge1, rnd_vnode_t *edge2, rnd_vnode_t *pt)
{
	int res;

	if ((pt->point[0] == edge1->point[0]) && (pt->point[1] == edge1->point[1]))
		return 0;
	if ((pt->point[0] == edge2->point[0]) && (pt->point[1] == edge2->point[1]))
		return 0;

	/* Corner case: test case gixedn: the overlap is created by a drift on
	   the north end of the bottom-left vertical edge. This drags the middle
	   of the edge onto the point 762;846. This is not detected because 762;846
	   is not marked risky. Solution: check if the next point (pt) after this
	   segment (edge1..edge2) falls on the edge; if it does, we do have a new
	   stub */
	res = pa_big_is_node_on_line(pt, edge1, edge2);

	if (res) { /* need to create a node at the intersection for cvc in selfisc */
		rnd_vnode_t *new_node = rnd_poly_node_create(pt->point);
		rnd_poly_vertex_include_force(edge1, new_node);
	}

/*	DEBUG_RISK("STUB CHECK for %ld;%ld: %d\n", pt->point[0], pt->point[1], res);*/
	
	return res;
}

/* Check each vertex in pl: if it is risky, check if there's any intersection
   on the incoming or outgoing edge of that vertex. If there is, stop and
   return 1, otherwise return 0. */
RND_INLINE int big_bool_ppl_(rnd_polyarea_t *pa, rnd_pline_t *pl, int already_bad, int from_selfisc)
{
	rnd_vnode_t *v = pl->head, *next;
	int res = 0, rebuild_tree = 0, pp_overlap = 0;

	if (pl->flg.risky) {
		DEBUG_RISK("pline marked risky earlier - schedule selfisc\n");
		res = 1;
		pl->flg.risky = 0;
	}

	do {
		if (v->flg.risk) {
			v->flg.risk = 0;
DEBUG_RISK("check risk for self-intersection at %ld;%ld:\n", v->point[0], v->point[1]);
			if (!res && !from_selfisc && (seg_too_short(v->prev, v) || seg_too_short(v, v->next))) {
				/* Related test case: fixedx; causes triangle flip; can't happen
				   if the length of the edge of the triangle is larger than our rounding
				   limit. Since this is checked only if v is rounded, it happens
				   rarely so it is okay to schedule an expensive self-isc check
				   that will then do a real triangle-flip detection.
				   Skip this test if we are called back from the selfisc code through
				   bool algebra: could send us into infinite loop, see test case fixedz */
				DEBUG_RISK("  segment too short next to a rounded corner! Shedule selfi-resolve\n");
				res = 1;
			}
			else if (!res && (seg_is_stub(v->prev, v, v->prev->prev) || seg_is_stub(v, v->next, v->next->next))) {
				/* see description in seg_is_stub */
				DEBUG_RISK("  segment deflected into a stub! Shedule selfi-resolve\n");
				res = 1;
			}
			else if (!res && (big_bool_ppl_isc(pa, pl, v->prev, &pp_overlap, from_selfisc) || big_bool_ppl_isc(pa, pl, v, &pp_overlap, from_selfisc))) {
DEBUG_RISK("  self-intersection occured! Shedule selfi-resolve\n");
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

		DEBUG_RISK(" pp-overlap X-crossing risk...\n");
		pa_add_conn_desc(pl, 'A', NULL);

		/* evaluate crossings */
		n = pl->head;
		do {
			if (n->cvclst_prev != NULL) {
				DEBUG_RISK("  X-crossing check at %ld;%ld\n", n->point[0], n->point[1]);
				pp_other = n->cvclst_prev->PP_OTHER;

				if (pa_cvc_crossing_at_node(n)) {
					res = 1;
					DEBUG_RISK("   X-crossing detected!! schedule selfisc\n");
				}
				else if (pa_cvc_line_line_overlap(n, pp_other)) {
					res = 1;
					DEBUG_RISK("   LL-overlap detected!! schedule selfisc\n");
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

/* Figures if an edge from a given node overlaps with another edge */
typedef struct {
	rnd_vnode_t *nd;
	int res;
} olap_edges_t;

static rnd_r_dir_t olap_edges_cb(const rnd_box_t *b, void *cl)
{
	pa_seg_t *s = (pa_seg_t *)b;
	olap_edges_t *ctx = cl;
	int on_edge;
	static int cnt;

	if (ctx->nd == s->v)
		return RND_R_DIR_NOT_FOUND;
	if (ctx->nd == s->v->next)
		return RND_R_DIR_NOT_FOUND;

	TODO("Arc: happens only on line-line or arc-arc; for arc-arc: radius and center match are required for an overlap");

	cnt++;

	on_edge = pa_pline_is_point_on_seg(s, ctx->nd->point);
	if (on_edge) {
		if (pa_pline_is_point_on_seg(s, ctx->nd->next->point)) {
			ctx->res = 1;
			return RND_R_DIR_CANCEL;
		}

		if (pa_pline_is_point_on_seg(s, ctx->nd->prev->point)) {
			ctx->res = 1;
			return RND_R_DIR_CANCEL;
		}
	}
	return RND_R_DIR_NOT_FOUND;
}


/* return 1 if pl has self-overlapping edge segments */
RND_INLINE int big_bool_self_olap(rnd_polyarea_t *pn, rnd_pline_t *pl)
{
	olap_edges_t ctx;

	ctx.res = 0;

	ctx.nd = pl->head;
	do {
		rnd_box_t ptbx;

		ptbx.X1 = ctx.nd->point[0];   ptbx.Y1 = ctx.nd->point[1];
		ptbx.X2 = ctx.nd->point[0]+1; ptbx.Y2 = ctx.nd->point[1]+1;

		rnd_r_search(pl->tree, &ptbx, NULL, olap_edges_cb, &ctx, NULL);
		if (ctx.res)
			return 1;
	} while((ctx.nd = ctx.nd->next) != pl->head);


	return 0;
}

/* Does an iteration of postprocessing; returns whether pa changed (0 or 1) */
RND_INLINE int big_bool_ppa_(rnd_polyarea_t **pa, int from_selfisc, int papa_touch_risk)
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
				if (big_bool_ppl_(pn, pl, res, from_selfisc))
					res = 1; /* can not return here, need to clear all the risk flags */
		}
	} while ((pn = pn->f) != *pa);

 /* fixedy8: when papa_touch_risk is set, there erre multiple, potentially
    overlapping islands on either input polygon. They could not intersect,
    that would have made them invalid, but there could have been overlapping
    line segments between two different islands. Now that they are merged
    into a single contour, the contour can have overlapping edge segments.
    Search for them; if found, flag this for self isc resolving */
	if ((res == 0) && papa_touch_risk) {
		pn = *pa;
		do {
			rnd_pline_t *pl = pn->contours;
			res = big_bool_self_olap(pn, pl);
		} while (!res && ((pn = pn->f) != *pa));
	}

	return res;
}

void pa_big_bool_postproc(rnd_polyarea_t **pa, int from_selfisc, int papa_touch_risk)
{
	if (big_bool_ppa_(pa, from_selfisc, papa_touch_risk))
		rnd_polyarea_split_selfisc(pa);
}

