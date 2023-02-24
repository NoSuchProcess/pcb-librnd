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
	struct seg *s = (struct seg *)b;
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
	struct seg *s = (struct seg *)b;
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
	struct seg *s = (struct seg *)b;
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
