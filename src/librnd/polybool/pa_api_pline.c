/*
       Copyright (C) 2006 harry eaton

   based on:
       poly_Boolean: a polygon clip library
       Copyright (C) 1997  Alexey Nikitin, Michael Leonov
       (also the authors of the paper describing the actual algorithm)
       leonov@propro.iis.nsk.su

   in turn based on:
       nclip: a polygon clip library
       Copyright (C) 1993  Klamer Schutte
 
       This program is free software; you can redistribute it and/or
       modify it under the terms of the GNU General Public
       License as published by the Free Software Foundation; either
       version 2 of the License, or (at your option) any later version.
 
       This program is distributed in the hope that it will be useful,
       but WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
       General Public License for more details.
 
       You should have received a copy of the GNU General Public
       License along with this program; if not, write to the Free
       Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 
      polygon1.c
      (C) 1997 Alexey Nikitin, Michael Leonov
      (C) 1993 Klamer Schutte

      all cases where original (Klamer Schutte) code is present
      are marked
*/

void rnd_poly_contour_clear(rnd_pline_t * c)
{
	rnd_vnode_t *cur;

	assert(c != NULL);
	while ((cur = c->head->next) != c->head) {
		rnd_poly_vertex_exclude(NULL, cur);
		free(cur);
	}
	free(c->head);
	c->head = NULL;
	pa_pline_init(c);
}

void rnd_poly_contour_del(rnd_pline_t ** c)
{
	rnd_vnode_t *cur, *prev;

	if (*c == NULL)
		return;
	for (cur = (*c)->head->prev; cur != (*c)->head; cur = prev) {
		prev = cur->prev;
		if (cur->cvclst_next != NULL) {
			free(cur->cvclst_next);
			free(cur->cvclst_prev);
		}
		free(cur);
	}
	if ((*c)->head->cvclst_next != NULL) {
		free((*c)->head->cvclst_next);
		free((*c)->head->cvclst_prev);
	}
	/* FIXME -- strict aliasing violation.  */
	if ((*c)->tree) {
		rnd_rtree_t *r = (*c)->tree;
		rnd_r_free_tree_data(r, free);
		rnd_r_destroy_tree(&r);
	}
	free((*c)->head);
	free(*c), *c = NULL;
}

void rnd_poly_contour_pre(rnd_pline_t * C, rnd_bool optimize)
{
	double area = 0;
	rnd_vnode_t *p, *c;
	rnd_vector_t p1, p2;

	assert(C != NULL);

	if (optimize) {
		for (c = (p = C->head)->next; c != C->head; c = (p = c)->next) {
			/* if the previous node is on the same line with this one, we should remove it */
			Vsub2(p1, c->point, p->point);
			Vsub2(p2, c->next->point, c->point);
			/* If the product below is zero then
			 * the points on either side of c 
			 * are on the same line!
			 * So, remove the point c
			 */

			if (rnd_vect_det2(p1, p2) == 0) {
				rnd_poly_vertex_exclude(C, c);
				free(c);
				c = p;
			}
		}
	}
	C->Count = 0;
	C->xmin = C->xmax = C->head->point[0];
	C->ymin = C->ymax = C->head->point[1];

	p = (c = C->head)->prev;
	if (c != p) {
		do {
			/* calculate area for orientation */
			area += (double) (p->point[0] - c->point[0]) * (p->point[1] + c->point[1]);
			pa_pline_box_bump(C, c->point);
			C->Count++;
		}
		while ((c = (p = c)->next) != C->head);
	}
	C->area = RND_ABS(area);
	if (C->Count > 2)
		C->flg.orient = ((area < 0) ? RND_PLF_INV : RND_PLF_DIR);
	C->tree = (rnd_rtree_t *) rnd_poly_make_edge_tree(C);
}																/* poly_PreContour */

void rnd_poly_contours_free(rnd_pline_t ** pline)
{
	rnd_pline_t *pl;

	while ((pl = *pline) != NULL) {
		*pline = pl->next;
		rnd_poly_contour_del(&pl);
	}
}

rnd_bool rnd_poly_contour_copy(rnd_pline_t **dst, const rnd_pline_t *src)
{
	rnd_vnode_t *cur, *newnode;

	assert(src != NULL);
	*dst = NULL;
	*dst = pa_pline_new(src->head->point);
	if (*dst == NULL)
		return rnd_false;

	(*dst)->Count = src->Count;
	(*dst)->flg.orient = src->flg.orient;
	(*dst)->xmin = src->xmin, (*dst)->xmax = src->xmax;
	(*dst)->ymin = src->ymin, (*dst)->ymax = src->ymax;
	(*dst)->area = src->area;

	for (cur = src->head->next; cur != src->head; cur = cur->next) {
		if ((newnode = rnd_poly_node_create(cur->point)) == NULL)
			return rnd_false;
		/* newnode->flg = cur->flg; */
		rnd_poly_vertex_include((*dst)->head->prev, newnode);
	}
	(*dst)->tree = (rnd_rtree_t *) rnd_poly_make_edge_tree(*dst);
	return rnd_true;
}

/*
 * rnd_pline_isect_line()
 * (C) 2017, 2018 Tibor 'Igor2' Palinkas
*/

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
		pl->tree = (rnd_rtree_t *) rnd_poly_make_edge_tree(pl);

	if (rnd_r_search(pl->tree, &lbx, NULL, pline_isect_line_cb, &ctx, NULL) == RND_R_DIR_CANCEL) {
		if (cx != NULL) *cx = ctx.cx;
		if (cy != NULL) *cy = ctx.cy;
		return rnd_true;
	}
	return rnd_false;
}

static rnd_r_dir_t flip_cb(const rnd_box_t * b, void *cl)
{
	pa_seg_t *s = (pa_seg_t *) b;
	s->v = s->v->prev;
	return RND_R_DIR_FOUND_CONTINUE;
}

void rnd_poly_contour_inv(rnd_pline_t * c)
{
	rnd_vnode_t *cur, *next;
	int r;

	assert(c != NULL);
	cur = c->head;
	do {
		next = cur->next;
		cur->next = cur->prev;
		cur->prev = next;
		/* fix the segment tree */
	}
	while ((cur = next) != c->head);
	c->flg.orient ^= 1;
	if (c->tree) {
		rnd_r_search(c->tree, NULL, NULL, flip_cb, NULL, &r);
		assert(r == c->Count);
	}
}

typedef struct pip {
	int f;
	rnd_vector_t p;
	jmp_buf env;
} pip;


static rnd_r_dir_t crossing(const rnd_box_t * b, void *cl)
{
	pa_seg_t *s = (pa_seg_t *) b;   /* polygon edge, line segment */
	struct pip *p = (struct pip *) cl;  /* horizontal cutting line */

	/* the horizontal cutting line is between vectors s->v and s->v->next, but
	   these two can be in any order; because poly contour is CCW, this means if
	   the edge is going up, we went from inside to outside, else we went
	   from outside to inside */
	if (s->v->point[1] <= p->p[1]) {
		if (s->v->next->point[1] > p->p[1]) { /* this also happens to blocks horizontal poly edges because they are only == */
			rnd_vector_t v1, v2;
			rnd_long64_t cross;
			Vsub2(v1, s->v->next->point, s->v->point);
			Vsub2(v2, p->p, s->v->point);
			cross = (rnd_long64_t) v1[0] * v2[1] - (rnd_long64_t) v2[0] * v1[1];
			if (cross == 0) { /* special case: if the point is on any edge, the point is in the poly */
				p->f = 1;
				longjmp(p->env, 1);
			}
			if (cross > 0)
				p->f += 1;
		}
	}
	else { /* since the other side was <=, when we get here we also blocked horizontal lines of the negative direction */
		if (s->v->next->point[1] <= p->p[1]) {
			rnd_vector_t v1, v2;
			rnd_long64_t cross;
			Vsub2(v1, s->v->next->point, s->v->point);
			Vsub2(v2, p->p, s->v->point);
			cross = (rnd_long64_t) v1[0] * v2[1] - (rnd_long64_t) v2[0] * v1[1];
			if (cross == 0) { /* special case: if the point is on any edge, the point is in the poly */
				p->f = 1;
				longjmp(p->env, 1);
			}
			if (cross < 0)
				p->f -= 1;
		}
	}

	return RND_R_DIR_FOUND_CONTINUE;
}

int rnd_poly_contour_inside(const rnd_pline_t *c, rnd_vector_t p)
{
	struct pip info;
	rnd_box_t ray;

	if (!pa_is_point_in_pline_box(c, p))
		return rnd_false;

	/* run a horizontal ray from the point to x->infinity and count (in info.f)
	   how it crosses poly edges with different winding */
	info.f = 0;
	info.p[0] = ray.X1 = p[0];
	info.p[1] = ray.Y1 = p[1];
	ray.X2 = RND_COORD_MAX;
	ray.Y2 = p[1] + 1;
	if (setjmp(info.env) == 0)
		rnd_r_search(c->tree, &ray, NULL, crossing, &info, NULL);
	return info.f;
}

/***/

/* Algorithm from http://www.exaflop.org/docs/cgafaq/cga2.html
 *
 * "Given a simple polygon, find some point inside it. Here is a method based
 * on the proof that there exists an internal diagonal, in [O'Rourke, 13-14].
 * The idea is that the midpoint of a diagonal is interior to the polygon.
 *
 * 1.  Identify a convex vertex v; let its adjacent vertices be a and b.
 * 2.  For each other vertex q do:
 * 2a. If q is inside avb, compute distance to v (orthogonal to ab).
 * 2b. Save point q if distance is a new min.
 * 3.  If no point is inside, return midpoint of ab, or centroid of avb.
 * 4.  Else if some point inside, qv is internal: return its midpoint."
 *
 * [O'Rourke]: Computational Geometry in C (2nd Ed.)
 *             Joseph O'Rourke, Cambridge University Press 1998,
 *             ISBN 0-521-64010-5 Pbk, ISBN 0-521-64976-5 Hbk
 */
static void poly_ComputeInteriorPoint(rnd_pline_t * poly, rnd_vector_t v)
{
	rnd_vnode_t *pt1, *pt2, *pt3, *q;
	rnd_vnode_t *min_q = NULL;
	double dist;
	double min_dist = 0.0;
	double dir = (poly->flg.orient == RND_PLF_DIR) ? 1. : -1;

	/* Find a convex node on the polygon */
	pt1 = poly->head;
	do {
		double dot_product;

		pt2 = pt1->next;
		pt3 = pt2->next;

		dot_product = dot_orthogonal_to_direction(pt1->point, pt2->point, pt3->point, pt2->point);

		if (dot_product * dir > 0.)
			break;
	}
	while ((pt1 = pt1->next) != poly->head);

	/* Loop over remaining vertices */
	q = pt3;
	do {
		/* Is current vertex "q" outside pt1 pt2 pt3 triangle? */
		if (!point_in_triangle(pt1->point, pt2->point, pt3->point, q->point))
			continue;

		/* NO: compute distance to pt2 (v) orthogonal to pt1 - pt3) */
		/*     Record minimum */
		dist = dot_orthogonal_to_direction(q->point, pt2->point, pt1->point, pt3->point);
		if (min_q == NULL || dist < min_dist) {
			min_dist = dist;
			min_q = q;
		}
	}
	while ((q = q->next) != pt2);

	/* Were any "q" found inside pt1 pt2 pt3? */
	if (min_q == NULL) {
		/* NOT FOUND: Return midpoint of pt1 pt3 */
		v[0] = (pt1->point[0] + pt3->point[0]) / 2;
		v[1] = (pt1->point[1] + pt3->point[1]) / 2;
	}
	else {
		/* FOUND: Return mid point of min_q, pt2 */
		v[0] = (pt2->point[0] + min_q->point[0]) / 2;
		v[1] = (pt2->point[1] + min_q->point[1]) / 2;
	}
}


/* NB: This function assumes the caller _knows_ the contours do not
 *     intersect. If the contours intersect, the result is undefined.
 *     It will return the correct result if the two contours share
 *     common points between their contours. (Identical contours
 *     are treated as being inside each other).
 */
int rnd_poly_contour_in_contour(rnd_pline_t * poly, rnd_pline_t * inner)
{
	rnd_vector_t point;
	assert(poly != NULL);
	assert(inner != NULL);
	if (pa_pline_box_inside(inner, poly)) {
		/* We need to prove the "inner" contour is not outside
		 * "poly" contour. If it is outside, we can return.
		 */
		if (!rnd_poly_contour_inside(poly, inner->head->point))
			return 0;

		poly_ComputeInteriorPoint(inner, point);
		return rnd_poly_contour_inside(poly, point);
	}
	return 0;
}



/*
 * rnd_pline_isect_circle()
 * (C) 2017 Tibor 'Igor2' Palinkas
*/

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
		pl->tree = (rnd_rtree_t *) rnd_poly_make_edge_tree(pl);

	return rnd_r_search(pl->tree, &cbx, NULL, pline_isect_circ_cb, &ctx, NULL) == RND_R_DIR_CANCEL;
}


/*
 * rnd_pline_embraces_circle()
 * If the circle does not intersect the polygon (the caller needs to check this)
 * return whether the circle is fully within the polygon or not.
 * Shoots a ray to the right from center+radius, then one to the left from
 * center-radius; if both ray cross odd number of pline segments, we are in
 * (or intersecting).
 * (C) 2017 Tibor 'Igor2' Palinkas
*/
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
		pl->tree = (rnd_rtree_t *) rnd_poly_make_edge_tree(pl);

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

/*
 * rnd_pline_isect_circle()
 * (C) 2017 Tibor 'Igor2' Palinkas
*/
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
		pl->tree = (rnd_rtree_t *) rnd_poly_make_edge_tree(pl);

	if (rnd_pline_isect_circ(pl, cx, cy, r))
		return rnd_true;

	return rnd_pline_embraces_circ(pl, cx, cy, r);
}
