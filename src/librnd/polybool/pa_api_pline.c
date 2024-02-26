/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2017, 2018, 2023 Tibor 'Igor2' Palinkas
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
 *
 *  This is a full rewrite of pcb-rnd's (and PCB's) polygon lib originally
 *  written by Harry Eaton in 2006, in turn building on "poly_Boolean: a
 *  polygon clip library" by Alexey Nikitin, Michael Leonov from 1997 and
 *  "nclip: a polygon clip library" Klamer Schutte from 1993.
 *
 *  English translation of the original paper the lib is largely based on:
 *  https://web.archive.org/web/20160418014630/http://www.complex-a5.ru/polyboolean/downloads/polybool_eng.pdf
 *
 */


/* High level pline API functions (called mostly or only from outside) */

void rnd_poly_contour_clear(rnd_pline_t *pl)
{
	rnd_vnode_t *n;

	assert(pl != NULL);

	while((n = pl->head->next) != pl->head) {
		rnd_poly_vertex_exclude(NULL, n);
		free(n);
	}

	free(pl->head); pl->head = NULL;

	pa_pline_init(pl);
}

/***/


typedef struct {
	rnd_vector_t l1, l2;
	rnd_coord_t cx, cy;
} pline_isect_line_t;

static rnd_r_dir_t pline_isect_line_cb(const rnd_box_t * b, void *cl)
{
	pline_isect_line_t *ctx = (pline_isect_line_t *)cl;
	pa_seg_t *s = (pa_seg_t *)b;
	rnd_vector_t S1, S2;

	if (rnd_vect_inters2(s->v->point, s->v->next->point, ctx->l1, ctx->l2, S1, S2)) {
		ctx->cx = S1[0];
		ctx->cy = S1[1];
		return RND_R_DIR_CANCEL; /* found */
	}

	return RND_R_DIR_NOT_FOUND;
}

rnd_bool rnd_pline_isect_line(rnd_pline_t *pl, rnd_coord_t lx1, rnd_coord_t ly1, rnd_coord_t lx2, rnd_coord_t ly2, rnd_coord_t *cx, rnd_coord_t *cy)
{
	pline_isect_line_t ctx;
	rnd_box_t lbx;
	ctx.l1[0] = lx1; ctx.l1[1] = ly1;
	ctx.l2[0] = lx2; ctx.l2[1] = ly2;
	lbx.X1 = MIN(lx1, lx2);
	lbx.Y1 = MIN(ly1, ly2);
	lbx.X2 = MAX(lx1, lx2);
	lbx.Y2 = MAX(ly1, ly2);

	if (pl->tree == NULL)
		pl->tree = rnd_poly_make_edge_tree(pl);

	if (rnd_r_search(pl->tree, &lbx, NULL, pline_isect_line_cb, &ctx, NULL) == RND_R_DIR_CANCEL) {
		if (cx != NULL) *cx = ctx.cx;
		if (cy != NULL) *cy = ctx.cy;
		return rnd_true;
	}
	return rnd_false;
}

typedef struct pa_cin_ctx_s {
	int f;
	int point_on_edge_is_in;/* configuration */
#ifdef PA_BIGCOORD_ISC
	pa_big_vector_t p_big;
	unsigned p_is_big:1;    /* p_big is available from input (need to do accurate calculations) */
	unsigned p_has_big:1;   /* p_big is filled in (from input or converted from p) */
#endif
	unsigned compatibility:1; /* with the original poly lib */

	rnd_vector_t p;
} pa_cin_ctx_t;

#ifdef PA_BIGCOORD_ISC
	/* this version is accurate but expensive */
#	define PA_CIN_CROSS_BIG \
	int cross_sgn = pa_big_vnode_vnode_cross_sgn(sv, sv_next, p->p_big)
#endif

/* this can easily overflow with 64 bit coords */
#define PA_CIN_CROSS_SMALL \
	rnd_vector_t v1, v2; \
	rnd_long64_t cross; \
	Vsub2(v1, s->v->next->point, s->v->point); \
	Vsub2(v2, p->p, s->v->point); \
	cross = (rnd_long64_t)v1[0] * (rnd_long64_t)v2[1] - (rnd_long64_t)v2[0] * (rnd_long64_t)v1[1];

static rnd_r_dir_t pa_cin_crossing_small(pa_cin_ctx_t *p, pa_seg_t *s)
{
	/* special case: point-on-point is a point_on_edge_is_in case; test case: gixedl */
	if (!p->compatibility) {
		if ((p->p[0] == s->v->point[0]) && (p->p[1] == s->v->point[1])) {
			p->f = p->point_on_edge_is_in;
			return RND_R_DIR_CANCEL;
		}
		if ((p->p[0] == s->v->next->point[0]) && (p->p[1] == s->v->next->point[1])) {
			p->f = p->point_on_edge_is_in;
			return RND_R_DIR_CANCEL;
		}
	}

	/* special case: horizontal line - point is either on it or not */
	if (!p->compatibility && (s->v->point[1] == s->v->next->point[1])) {

		if (p->p[1] != s->v->point[1])
			return RND_R_DIR_FOUND_CONTINUE; /* not the same y, no intersection */

		if ((p->p[0] > s->v->point[0]) && (p->p[0] > s->v->next->point[0]))
			return RND_R_DIR_FOUND_CONTINUE; /* point is right of the line, no isc */

		/* check if we are on the line */
		if (s->v->point[0] <= s->v->next->point[0]) {
			if ((p->p[0] >= s->v->point[0]) && (p->p[0] <= s->v->next->point[0])) {
				p->f = p->point_on_edge_is_in;
				return RND_R_DIR_CANCEL;
			}
		}
		else {
			if ((p->p[0] >= s->v->next->point[0]) && (p->p[0] <= s->v->point[0])) {
				p->f = p->point_on_edge_is_in;
				return RND_R_DIR_CANCEL;
			}
		}

		/* point is left of the horizontal line - there are too many cases,
		   just ignore the horizontal line and let the connecting non-horizontal
		   lines catch the ray */
		return RND_R_DIR_FOUND_CONTINUE;
	}

	/* the horizontal cutting line is between vectors s->v and s->v->next, but
	   these two can be in any order; because poly contour is CCW, this means if
	   the edge is going up, we went from inside to outside, else we went
	   from outside to inside */
	if (s->v->point[1] <= p->p[1]) {
		if (s->v->next->point[1] > p->p[1]) { /* this also happens to blocks horizontal poly edges because they are only == */
			PA_CIN_CROSS_SMALL;
			if (cross == 0) { /* special case: if the point is on any edge, the point is in the poly */
				p->f = p->point_on_edge_is_in;
				return RND_R_DIR_CANCEL;
			}
			if (cross > 0)
				p->f++;
		}
	}
	else { /* since the other side was <=, when we get here we also blocked horizontal lines of the negative direction */
		if (s->v->next->point[1] <= p->p[1]) {
			PA_CIN_CROSS_SMALL;
			if (cross == 0) { /* special case: if the point is on any edge, the point is in the poly */
				p->f = p->point_on_edge_is_in;
				return RND_R_DIR_CANCEL;
			}
			if (cross < 0)
				p->f--;
		}
	}

	return RND_R_DIR_FOUND_CONTINUE;
}

#ifdef PA_BIGCOORD_ISC

#define bigcmp(a, b) pa_big_coord_cmp(a, b)

/* same as above, but with big coords */
RND_INLINE rnd_r_dir_t pa_cin_crossing_big(pa_cin_ctx_t *p, pa_seg_t *s)
{
	pa_big_vector_t sv, sv_next;

	rnd_pa_big_load_cvc(&sv, s->v);
	rnd_pa_big_load_cvc(&sv_next, s->v->next);

	/* cache big coords of the point */
	if (!p->p_has_big) {
		pa_big_load(p->p_big.x, p->p[0]);
		pa_big_load(p->p_big.y, p->p[1]);
		p->p_has_big = 1;
	}

	/* special case: point-on-point is a point_on_edge_is_in case; test case: gixedl */
	if (!p->compatibility) {
		if ((bigcmp(p->p_big.x, sv.x) == 0) && (bigcmp(p->p_big.y, sv.y) == 0)) {
			p->f = p->point_on_edge_is_in;
			return RND_R_DIR_CANCEL;
		}
		if ((bigcmp(p->p_big.x, sv_next.x) == 0) && (bigcmp(p->p_big.y, sv_next.y) == 0)) {
			p->f = p->point_on_edge_is_in;
			return RND_R_DIR_CANCEL;
		}
	}

	/* special case: horizontal line - point is either on it or not */
	if (!p->compatibility && (bigcmp(sv.y, sv_next.y) == 0)) {

		if (bigcmp(p->p_big.y, sv_next.y) != 0)
			return RND_R_DIR_FOUND_CONTINUE; /* not the same y, no intersection */

		if ((bigcmp(p->p_big.x, sv.x) > 0) && (bigcmp(p->p_big.x, sv_next.x) > 0))
			return RND_R_DIR_FOUND_CONTINUE; /* point is right of the line, no isc */

		/* check if we are on the line */
		if (bigcmp(sv.x, sv_next.x) <= 0) {
			if ((bigcmp(p->p_big.x, sv.x) >= 0) && (bigcmp(p->p_big.x, sv_next.x) <= 0)) {
				p->f = p->point_on_edge_is_in;
				return RND_R_DIR_CANCEL;
			}
		}
		else {
			if ((bigcmp(p->p_big.x, sv_next.x) >= 0) && (bigcmp(p->p_big.x, sv.x) <= 0)) {
				p->f = p->point_on_edge_is_in;
				return RND_R_DIR_CANCEL;
			}
		}

		/* point is left of the horizontal line - there are too many cases,
		   just ignore the horizontal line and let the connecting non-horizontal
		   lines catch the ray */
		return RND_R_DIR_FOUND_CONTINUE;
	}


	if (s->v->point[1] <= p->p[1]) {
		if (s->v->next->point[1] > p->p[1]) {
			PA_CIN_CROSS_BIG;
			if (cross_sgn == 0) {
				p->f = p->point_on_edge_is_in;
				return RND_R_DIR_CANCEL;
			}
			if (cross_sgn > 0)
				p->f++;
		}
	}
	else {
		if (s->v->next->point[1] <= p->p[1]) {
			PA_CIN_CROSS_BIG;
			if (cross_sgn == 0) {
				p->f = p->point_on_edge_is_in;
				return RND_R_DIR_CANCEL;
			}
			if (cross_sgn < 0)
				p->f--;
		}
	}

	return RND_R_DIR_FOUND_CONTINUE;
}

#undef bigcmp
#endif

static rnd_r_dir_t pa_cin_crossing(const rnd_box_t *b, void *cl)
{
	pa_seg_t *s = (pa_seg_t *)b;   /* polygon edge, line segment */
	pa_cin_ctx_t *p = cl;          /* horizontal cutting line */

#ifdef PA_BIGCOORD_ISC
	if (p->p_is_big || (s->v->cvclst_prev != NULL) || (s->v->next->cvclst_prev != NULL)) {
/*
		rnd_trace("&&&&& line=%ld;%ld - %ld;%ld  pt=%ld;%ld\n",
			s->v->point[0], s->v->point[1], s->v->next->point[0], s->v->next->point[1],
			p->p[0], p->p[1]);
*/
		return pa_cin_crossing_big(p, s);
	}
#endif

	return pa_cin_crossing_small(p, s);
}

int pa_pline_is_point_inside(const rnd_pline_t *pl, rnd_vector_t pt)
{
	pa_cin_ctx_t ctx;
	rnd_box_t ray;

	/* quick exit if point has not even in th bbox of pl*/
	if (!pa_is_point_in_pline_box(pl, pt))
		return rnd_false;

	/* run a horizontal ray from the point to x->infinity and count (in ctx.f)
	   how it crosses poly edges with different winding */
	ctx.f = 0;
	ctx.point_on_edge_is_in = 1; /* for compatibility */
	ctx.p[0] = ray.X1 = pt[0];
	ctx.p[1] = ray.Y1 = pt[1];
	ctx.p_is_big = ctx.p_has_big = 0;
	ctx.compatibility = 1;
	ray.X2 = RND_COORD_MAX;
	ray.Y2 = pt[1] + 1;

	rnd_r_search(pl->tree, &ray, NULL, pa_cin_crossing, &ctx, NULL);

	return ctx.f;
}

int pa_pline_is_point_on_seg(pa_seg_t *s, rnd_vector_t pt)
{
	pa_cin_ctx_t ctx;
#define ON_EDGE_COOKIE 1234

	/* run a horizontal ray from the point to x->infinity and count (in ctx.f)
	   how it crosses poly edges with different winding */
	ctx.f = 0;
	ctx.point_on_edge_is_in = ON_EDGE_COOKIE;
	ctx.p[0] = pt[0];
	ctx.p[1] = pt[1];
	ctx.p_is_big = ctx.p_has_big = 0;
	ctx.compatibility = 0;

#ifdef PA_BIGCOORD_ISC
	pa_cin_crossing_big(&ctx, s);
#else
	pa_cin_crossing_small(&ctx, s)
#endif

	return ctx.f == ON_EDGE_COOKIE;
}


/* Returns whether pt is within the bbox of pl */
rnd_bool pa_is_vnode_in_pline_box(const rnd_pline_t *pl, const rnd_vnode_t *nd)
{

	if (pa_is_point_in_pline_box(pl, nd->point))
		return 1;

	if (nd->cvclst_prev != NULL) {
		rnd_vector_t pt;

		/* round down */
		pt[0] = nd->point[0]-1;
		pt[1] = nd->point[1]-1;

		if (pa_is_point_in_pline_box(pl, pt))
			return 1;
	}

	return 0;
}

int pa_pline_is_vnode_inside(const rnd_pline_t *pl, const rnd_vnode_t *nd, int point_on_edge_is_in)
{
	pa_cin_ctx_t ctx;
	rnd_box_t ray;

	/* quick exit if point has not even in th bbox of pl*/
	if (!pa_is_vnode_in_pline_box(pl, nd))
		return rnd_false;

	/* run a horizontal ray from the point to x->infinity and count (in ctx.f)
	   how it crosses poly edges with different winding */
	ctx.f = 0;
	ctx.point_on_edge_is_in = point_on_edge_is_in;
	ctx.p[0] = ray.X1 = nd->point[0];
	ctx.p[1] = ray.Y1 = nd->point[1];
	ctx.compatibility = 0;
	ray.X2 = RND_COORD_MAX;
	ray.Y2 = nd->point[1] + 1;
	ctx.p_has_big = 0;

	if (nd->cvclst_prev != NULL) {
		ctx.p_big = nd->cvclst_prev->isc;
		ctx.p_is_big = 1;
		ray.Y1--;
		ray.X1--;
	}
	else
		ctx.p_is_big = 0;

	rnd_r_search(pl->tree, &ray, NULL, pa_cin_crossing, &ctx, NULL);

	return ctx.f;
}


/* Algorithm from http://www.exaflop.org/docs/cgafaq/cga2.html

   "Given a simple polygon, find some point inside it. Here is a method based
   on the proof that there exists an internal diagonal, in [O'Rourke, 13-14].
   The idea is that the midpoint of a diagonal is interior to the polygon.

   1.  Identify a convex vertex v; let its adjacent vertices be a and b.
   2.  For each other vertex n do:
   2a. If n is inside avb, compute distance to v (orthogonal to ab).
   2b. Save point n if distance is a new min.
   3.  If no point is inside, return midpoint of ab, or centroid of avb.
   4.  Else if some point inside, nv is internal: return its midpoint."

   [O'Rourke]: Computational Geometry in C (2nd Ed.)
               Joseph O'Rourke, Cambridge University Press 1998,
               ISBN 0-521-64010-5 Pbk, ISBN 0-521-64976-5 Hbk */
RND_INLINE void pa_pline_interior_pt(rnd_pline_t *poly, rnd_vector_t v)
{
	rnd_vnode_t *pt1, *pt2, *pt3, *n, *min_n = NULL;
	double dist, min_dist = 0.0, dir = (poly->flg.orient == RND_PLF_DIR) ? 1.0 : -1.0;

	/* Step 1: find a convex node on the polygon; result is pt1, pt2 and pt3
	   loaded with adjacent nodes; pt1 is "a", pt3 is "b" and pt2 is "v". */
	pt1 = poly->head;
	do {
		double dot_product;
		pt2 = pt1->next; pt3 = pt2->next;
		dot_product = dot_orthogonal_to_direction(pt1->point, pt2->point, pt3->point, pt2->point);

		if (dot_product * dir > 0.0)
			break;
	} while((pt1 = pt1->next) != poly->head);

	/* Step 2: loop over remaining vertices (from pt3 to pt2) */
	n = pt3;
	do {
		if (!point_in_triangle(pt1->point, pt2->point, pt3->point, n->point))
			continue; /* 2.a. current node "n" is outside pt1 pt2 pt3 triangle */

		/* 2.b. "n" is inside; compute distance to pt2 (v) orthogonal
		   to pt1 - pt3) and record minimum */
		dist = dot_orthogonal_to_direction(n->point, pt2->point, pt1->point, pt3->point);
		if ((min_n == NULL) || (dist < min_dist)) {
			min_dist = dist;
			min_n = n;
		}
	} while((n = n->next) != pt2);

	/* Were any "n" found inside pt1 pt2 pt3? */
	if (min_n == NULL) {
		/* Step 3. not found: return midpoint of pt1 pt3 */
		v[0] = (pt1->point[0] + pt3->point[0]) / 2;
		v[1] = (pt1->point[1] + pt3->point[1]) / 2;
	}
	else {
		/* Step 4. found: return mid point of min_q, pt2 */
		v[0] = (pt2->point[0] + min_n->point[0]) / 2;
		v[1] = (pt2->point[1] + min_n->point[1]) / 2;
	}
}

rnd_bool pa_pline_inside_pline(rnd_pline_t *outer, rnd_pline_t *inner)
{
	rnd_vector_t ipt;

	assert((outer != NULL) && (inner != NULL));

	/* cheapest test: if inner's bbox is not in outer's bbox */
	if (!pa_pline_box_inside(inner, outer))
		return 0;

/* This optimization is disabled:
   because there is a corner case where it fails: fixedj at 496;555 which
   happens to be inner->head; it's right on a horizontal line of the outer
   poly and pa_pline_is_point_inside() returns false in compatibility
   mode. It'd be possible to change this, or try with another point but
   there are probably new corner cases introduced. */
#if 0
	/* if there is a point in inner that's not within outer... */
	if (!pa_pline_is_point_inside(outer, inner->head->point))
		return 0;
#endif

	/* pa_pline_interior_pt() tends to fail for tiny polygons because of the
	   constraint that it needs to pick an integer point: if the poly is a
	   very small triangle, the "inner" point typically falls slightly outside.
	   Do a slower but more reliable check corner by corner. Use majority vote
	   because one point may be outside while majority of the poly is inside.
	   Related test case: gixedo, gixedq */
	if (inner->area < 3) {
		rnd_vnode_t *n = inner->head;
		int p_in = 0, p_out = 0;
		do {
			int r = pa_pline_is_point_inside(outer, n->point);
			if (r)
				p_in++;
			else
				p_out++;
		} while((n = n->next) != inner->head);
		return (p_in >= p_out);
	}

	/* ...but that may still be only a shared point while the rest of inner
	   is simply all outside. Since they are not intersecting if a random
	   internal point of inner is inside outer, the whole inner is inside outer */
	pa_pline_interior_pt(inner, ipt);
	return pa_pline_is_point_inside(outer, ipt);
}

/***/

typedef struct {
	rnd_coord_t cx, cy, r;
	double r2;
} pline_isect_circ_t;

static rnd_r_dir_t pline_isect_circ_cb(const rnd_box_t * b, void *cl)
{
	pline_isect_circ_t *ctx = (pline_isect_circ_t *)cl;
	pa_seg_t *s = (pa_seg_t *)b;
	rnd_vector_t S1, S2;
	rnd_vector_t ray1, ray2;
	double ox, oy, dx, dy, l;

	/* Cheap: if either line endpoint is within the circle, we sure have an intersection */
	if ((RND_SQUARE(s->v->point[0] - ctx->cx) + RND_SQUARE(s->v->point[1] - ctx->cy)) <= ctx->r2)
		return RND_R_DIR_CANCEL; /* found */
	if ((RND_SQUARE(s->v->next->point[0] - ctx->cx) + RND_SQUARE(s->v->next->point[1] - ctx->cy)) <= ctx->r2)
		return RND_R_DIR_CANCEL; /* found */

	dx = s->v->point[0] - s->v->next->point[0];
	dy = s->v->point[1] - s->v->next->point[1];
	l = sqrt(RND_SQUARE(dx) + RND_SQUARE(dy));
	ox = -dy / l * (double)ctx->r;
	oy = dx / l * (double)ctx->r;

	ray1[0] = ctx->cx - ox; ray1[1] = ctx->cy - oy;
	ray2[0] = ctx->cx + ox; ray2[1] = ctx->cy + oy;

	if (rnd_vect_inters2(s->v->point, s->v->next->point, ray1, ray2, S1, S2))
		return RND_R_DIR_CANCEL; /* found */

	return RND_R_DIR_NOT_FOUND;
}

rnd_bool rnd_pline_isect_circ(rnd_pline_t *pl, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r)
{
	pline_isect_circ_t ctx;
	rnd_box_t cbx;
	ctx.cx = cx; ctx.cy = cy;
	ctx.r = r; ctx.r2 = (double)r * (double)r;
	cbx.X1 = cx - r; cbx.Y1 = cy - r;
	cbx.X2 = cx + r; cbx.Y2 = cy + r;

	if (pl->tree == NULL)
		pl->tree = rnd_poly_make_edge_tree(pl);

	return rnd_r_search(pl->tree, &cbx, NULL, pline_isect_circ_cb, &ctx, NULL) == RND_R_DIR_CANCEL;
}

/***/

/* If the circle does not intersect the polygon (the caller needs to check this)
   return whether the circle is fully within the polygon or not.
   Shoots a ray to the right from center+radius, then one to the left from
   center-radius; if both ray cross odd number of pline segments, we are in
   (or intersecting). */
typedef struct {
	int cnt;
	rnd_coord_t cx, cy;
	int dx;
} pline_embrace_t;

static rnd_r_dir_t pline_embraces_circ_cb(const rnd_box_t * b, void *cl)
{
	pline_embrace_t *e = cl;
	pa_seg_t *s = (pa_seg_t *)b;
	double dx;
	rnd_coord_t lx;
	rnd_coord_t x1 = s->v->point[0];
	rnd_coord_t y1 = s->v->point[1];
	rnd_coord_t x2 = s->v->next->point[0];
	rnd_coord_t y2 = s->v->next->point[1];

	/* ray is below or above the line - no chance */
	if ((e->cy < y1) && (e->cy < y2))
		return RND_R_DIR_NOT_FOUND;
	if ((e->cy >= y1) && (e->cy >= y2))
		return RND_R_DIR_NOT_FOUND;

	/* calculate line's X for e->cy */
	dx = (double)(x2 - x1) / (double)(y2 - y1);
	lx = rnd_round((double)x1 + (double)(e->cy - y1) * dx);
	if (e->dx < 0) { /* going left */
		if (lx < e->cx)
			e->cnt++;
	}
	else { /* going roght */
		if (lx > e->cx)
			e->cnt++;
	}

	return RND_R_DIR_NOT_FOUND;
}

rnd_bool rnd_pline_embraces_circ(rnd_pline_t *pl, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r)
{
	rnd_box_t bx;
	pline_embrace_t e;

	bx.Y1 = cy; bx.Y2 = cy+1;
	e.cy = cy;
	if (pl->tree == NULL)
		pl->tree = rnd_poly_make_edge_tree(pl);

	/* ray to the right */
	bx.X1 = e.cx = cx + r;
	bx.X2 = RND_COORD_MAX;
	e.dx = +1;
	e.cnt = 0;
	rnd_r_search(pl->tree, &bx, NULL, pline_embraces_circ_cb, &e, NULL);
	if ((e.cnt % 2) == 0)
		return rnd_false;

	/* ray to the left */
	bx.X1 = -RND_COORD_MAX;
	bx.X2 = e.cx = cx - r;
	e.dx = -1;
	e.cnt = 0;
	rnd_r_search(pl->tree, &bx, NULL, pline_embraces_circ_cb, &e, NULL);
	if ((e.cnt % 2) == 0)
		return rnd_false;

	return rnd_true;
}

/***/

rnd_bool rnd_pline_overlaps_circ(rnd_pline_t *pl, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r)
{
	rnd_box_t cbx, pbx;
	cbx.X1 = cx - r; cbx.Y1 = cy - r;
	cbx.X2 = cx + r; cbx.Y2 = cy + r;
	pbx.X1 = pl->xmin; pbx.Y1 = pl->ymin;
	pbx.X2 = pl->xmax; pbx.Y2 = pl->ymax;

	/* if there's no overlap in bounding boxes, don't do any expensive calc */
	if (!(rnd_box_intersect(&cbx, &pbx)))
		return rnd_false;

	if (pl->tree == NULL)
		pl->tree = rnd_poly_make_edge_tree(pl);

	if (rnd_pline_isect_circ(pl, cx, cy, r))
		return rnd_true;

	return rnd_pline_embraces_circ(pl, cx, cy, r);
}
