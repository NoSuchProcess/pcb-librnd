int pb2_face_polarity_at_verbose = 0;

typedef struct {
	long cnt;
	char poly;
	rnd_vector_t pt;
	rnd_vector_t dir;           /* relative direction of VP from RP */
	pb2_slope_t dir_slope;

	/* optional callback on segment the ray is crossing */
	void (*seg_hit)(void *udata, pb2_seg_t *seg);
	void *udata;
} pb2_3_face_polarity_t;

RND_INLINE void pb2_3_seg_hit(pb2_3_face_polarity_t *pctx, pb2_seg_t *seg)
{
	TODO("stub: if we need to consider stubs, do this only if  the 'pctx->poly' counter of the seg is odd");
	pctx->cnt++;

	if (pctx->seg_hit != NULL)
		pctx->seg_hit(pctx->udata, seg);
}

RND_INLINE void pb2_3_fp_at_endp(pb2_3_face_polarity_t *pctx, rnd_vector_t p, rnd_vector_t other_end, pb2_seg_t *seg)
{
	if (pb2_face_polarity_at_verbose) rnd_trace("  endpoint hit at %ld;%ld\n", p[0], p[1]);

	TODO("arc: think over other_end: for arcs it should probably be the tangent from p, the actual endpoint is probably not used below");

	/* miss in x */
	if (p[0] < pctx->pt[0]) {
		if (pb2_face_polarity_at_verbose) rnd_trace("   ignore: miss in x: %ld < %ld\n", p[0] < pctx->pt[0]);
		return;
	}

	/* miss in y: our ray is always slightly below or above */
	if ((pctx->dir[1] > 0) && (other_end[1] <= p[1]) ) {
		if (pb2_face_polarity_at_verbose) rnd_trace("   ignore: miss in y: ray is down, seg going up\n");
		return;
	}
	if ((pctx->dir[1] < 0) && (other_end[1] >= p[1]) ) {
		if (pb2_face_polarity_at_verbose) rnd_trace("   ignore: miss in y: ray is up, seg going down\n");
		return;
	}

	/* if we are starting on a shared endpoint: verify angles; if dir is bending
	   back to the left faster, the ray is starting to the left of    */
	if ((p[0] == pctx->pt[0]) && (p[1] == pctx->pt[1])) {
		pb2_slope_t slp_seg = pb2_slope(p, other_end);
		if (pb2_face_polarity_at_verbose) rnd_trace("   slope: dir=%f seg=%f\n", pctx->dir_slope, slp_seg);

		/* special case testing on vertical vetors where slope would be infinite */
		if (pctx->dir[0] == 0) {
			/* the ray startpoint's direction is vertical; the segment must be right of it (ray.x > 0) to get hit */
			if (slp_seg.dx > 0) {
				pb2_3_seg_hit(pctx, seg);
				if (pb2_face_polarity_at_verbose) rnd_trace("   slope dir v1 is left of seg -> cnt=%ld\n", pctx->cnt);
			}
			else {
				if (pb2_face_polarity_at_verbose) rnd_trace("   slope dir v1 is right of seg -> ignore\n");
			}
			return;
		}
		if (slp_seg.dx == 0) {
			/* the segment is vertical; ray startpoint must be left of it (ray.x < 0) to hit */
			if ((long)pctx->dir[0] < 0) {
				pb2_3_seg_hit(pctx, seg);
				if (pb2_face_polarity_at_verbose) rnd_trace("   slope dir v2 is left of seg -> cnt=%ld\n", pctx->cnt);
			}
			else {
				if (pb2_face_polarity_at_verbose) rnd_trace("   slope dir v2 is right of seg -> ignore\n");
			}
			return;
		}

		if (pctx->dir[1] < 0) { /* dir pointing up */
			if ((slp_seg.s < 0) || PB2_SLOPE_LT(pctx->dir_slope, slp_seg)) {
				pb2_3_seg_hit(pctx, seg);
				if (pb2_face_polarity_at_verbose) rnd_trace("   slope dir up  is left of seg -> cnt=%ld\n", pctx->cnt);
					return;
			}
		}
		else if (pctx->dir[1] > 0) { /* dir pointing down */
			if ((slp_seg.s > 0) || PB2_SLOPE_LT(slp_seg, pctx->dir_slope)) {
				pb2_3_seg_hit(pctx, seg);
				if (pb2_face_polarity_at_verbose) rnd_trace("   slope dir dn  is left of seg -> cnt=%ld\n", pctx->cnt);
				return;
			}
		}

		if (pb2_face_polarity_at_verbose) rnd_trace("   slope: dir is right of seg -> ignore\n");
		return;
	}

	/* the ray is coming from more than 1 unit away from left, slightly
	   into the seg in y direction: that's a hit */
	pb2_3_seg_hit(pctx, seg);
	if (pb2_face_polarity_at_verbose) rnd_trace("    cnt=%ld\n", pctx->cnt);
}

static rnd_rtree_dir_t pb2_3_fp_cb(void *udata, void *obj, const rnd_rtree_box_t *box)
{
	pb2_3_face_polarity_t *pctx = udata;
	pb2_seg_t *seg = obj;
	rnd_vector_t p1, p2;

	if (seg->discarded)
		return 0;

	/* even number of overlapping edges (of the target poly) crossed is no-op;
	   this happens with overlapping edges, e.g. on stubs. An odd number
	   of overlaps (most typically 1 edge) matters */
	if ((pctx->poly == 'A') && ((seg->cntA % 2)  == 0))
		return 0;
	if ((pctx->poly == 'B') && ((seg->cntB % 2) == 0))
		return 0;

	/* (pctx->poly == 'F') means: operate on faces, not input polygons; any
	   non-discarded seg counts as 1 as we have already remvoed stubs*/

	p1[0] = seg->start[0]; p1[1] = seg->start[1];
	p2[0] = seg->end[0];   p2[1] = seg->end[1];

	if (pb2_face_polarity_at_verbose) rnd_trace(" seg found: %ld;%ld .. %ld;%ld cntA=%ld cntB=%ld uid=%ld\n", p1[0], p1[1], p2[0], p2[1], seg->cntA, seg->cntB, PB2_UID_GET(seg));

	/* ignore horizontal segs - it is enough to hiot its non-horizontal neighbors */
	if (p1[1] == p2[1]) {
		if (pb2_face_polarity_at_verbose) rnd_trace("  horizontal (ignored)\n");
		return 0;
	}

	/* hit a seg endpoint */
	if (p1[1] == pctx->pt[1]) {
		if (pb2_face_polarity_at_verbose) rnd_trace("  (end p1)\n");
		pb2_3_fp_at_endp(pctx, p1, p2, seg);
		return 0;
	}
	if (p2[1] == pctx->pt[1]) {
		if (pb2_face_polarity_at_verbose) rnd_trace("  (end p2)\n");
		pb2_3_fp_at_endp(pctx, p2, p1, seg);
		return 0;
	}

	/* probably crossing somewhere in the middle - verify */
	if ((p1[1] > pctx->pt[1]) && (p2[1] > pctx->pt[1]))
		return 0; /* miss in y */

	if ((p1[1] < pctx->pt[1]) && (p2[1] < pctx->pt[1]))
		return 0; /* miss in y */

	if ((p1[0] < pctx->pt[0]) && (p2[0] < pctx->pt[0]))
		return 0; /* miss in x */

	{
		rnd_vector_t v1, v2;
		double cross;
		Vsub2(v1, p2, p1);
		Vsub2(v2, pctx->pt, p1);
		TODO("bingun: use big coords instead (maybe a generic cross-product sign code?)");
		cross = (double)v1[0] * (double)v2[1] - (double)v2[0] * (double)v1[1];

		if ((p1[1] > p2[1]) && (cross > 0)) {
			if (pb2_face_polarity_at_verbose) rnd_trace("  midpoint miss 1 (not crossing)\n");
			return 0;
		}
		if ((p1[1] < p2[1]) && (cross < 0)) {
			if (pb2_face_polarity_at_verbose) rnd_trace("  midpoint miss 2 (not crossing)\n");
			return 0;
		}
	}


	/* real intersection in the middle */
	pb2_3_seg_hit(pctx, seg);
	if (pb2_face_polarity_at_verbose) rnd_trace("  midpoint hit\n cnt=%ld\n", pctx->cnt);

	return 0;
}


RND_INLINE int pb2_3_ray_cast(pb2_ctx_t *ctx, rnd_vector_t pt, char poly, rnd_vector_t direction, void (*seg_hit_cb)(void *udata, pb2_seg_t *s), void *udata)
{
	pb2_3_face_polarity_t pctx;
	rnd_rtree_box_t sb;
	static const rnd_vector_t pzero = {0};

	if (pb2_face_polarity_at_verbose) rnd_trace("Polarity test from %ld;%ld dir %ld;%ld in poly %c\n", pt[0], pt[1], (long)direction[0], (long)direction[1], poly);

	assert(direction[0] <= 0);
	assert(direction[1] != 0);

	pctx.cnt = 0;
	pctx.poly = poly;
	pctx.dir[0] = direction[0]; pctx.dir[1] = direction[1];
	pctx.pt[0] = pt[0]; pctx.pt[1] = pt[1];
	pctx.dir_slope = pb2_slope(pzero, direction);
	pctx.seg_hit = seg_hit_cb;
	pctx.udata = udata;


	sb.x1 = pt[0]; sb.y1 = pt[1];
	sb.x2 = RND_COORD_MAX; sb.y2 = pt[1]+1;
	rnd_rtree_search_obj(&ctx->seg_tree, &sb, pb2_3_fp_cb, &pctx);


	if (pb2_face_polarity_at_verbose) rnd_trace(" cnt final=%ld (odd is in)\n\n", pctx.cnt);

	return (pctx.cnt % 2); /* odd means inside which should return 1 */
}

/* Returns the polarity of a face in an input polygon at a vritual point
   that's left of pt but is inifintely close to pt. Assumes pt is a
   rightmost point of the face */
RND_INLINE int pb2_3_face_polarity_at(pb2_ctx_t *ctx, rnd_vector_t pt, char poly, rnd_vector_t direction)
{
	return pb2_3_ray_cast(ctx, pt, poly, direction, NULL, NULL);
}


RND_INLINE void pb2_8_cleanup(pb2_ctx_t *ctx);

/* Published for testing */
int pb2_face_polarity_at(pb2_ctx_t *ctx, rnd_vector_t pt, rnd_vector_t direction)
{
	int res;

	pb2_1_to_topo(ctx);

	if (pb2_face_polarity_at_verbose)
		pb2_draw(ctx, "polarity.svg", PB2_DRAW_INPUT_POLY | PB2_DUMP_SEGS | PB2_DUMP_CURVES | PB2_DUMP_CURVE_GRAPH);

	res = pb2_3_face_polarity_at(ctx, pt, 'A', direction);

	pb2_8_cleanup(ctx);
	return res;
}

/* Find the lowest rightmost point of f and put it in f->polarity_pt and f->polarity_dir */
RND_INLINE void pb2_3_face_find_polarity_pt(pb2_face_t *f)
{
	pb2_face_it_t it;
	rnd_vector_t first, last, best, curr, dir_end, best_left, best_right;
	int save_right, best_has_left, best_has_right;

	pb2_face_it_first(&it, f, NULL, first, 0);
	last[0] = first[0];
	last[1] = first[1];

	/* first point is the best (so far) */
	best[0] = first[0];
	best[1] = first[1];
	save_right = 1;
	best_has_left = 0;
	best_has_right = 0;

	while(pb2_face_it_next(&it, NULL, curr) != NULL) {
		if (save_right) {
			best_right[0] = curr[0];
			best_right[1] = curr[1];
			best_has_right = 1;
			save_right = 0;
		}
		if (curr[0] >= best[0]) {
			best[0] = curr[0];
			best[1] = curr[1];
			best_left[0] = last[0];
			best_left[1] = last[1];
			best_has_left = 1;
			best_has_right = 0;
			save_right = 1;
		}
		last[0] = curr[0];
		last[1] = curr[1];
	}

	if (!best_has_left) {
		/* best was the first point - left side would be the last point seen */
		best_left[0] = last[0];
		best_left[1] = last[1];
	}

	if (!best_has_right) {
		/* best was the last point - right side is the first point */
		best_right[0] = first[0];
		best_right[1] = first[1];
	}

	if (pb2_face_polarity_at_verbose) rnd_trace("pb2_3_face_find_polarity_pt: %ld;%ld -> {%ld;%ld} -> %ld;%ld\n", best_left[0], best_left[1], best[0], best[1], best_right[0], best_right[1]);

	f->polarity_pt[0] = best[0];
	f->polarity_pt[1] = best[1];

	/* This would be dir_end/2 - best to have an average for the mid point
	   but that would fail if left and right were too close, e.g. corners
	   of an 1*1 sqaure, so rather just multiply everything by 2 - the scale
	   of the direction vector doesn't matter */
	TODO("bignum: this will require wider integers");
	dir_end[0] = (best_left[0] + best_right[0]);
	dir_end[1] = (best_left[1] + best_right[1]);
	f->polarity_dir[0] = (dir_end[0] - best[0]*2);
	f->polarity_dir[1] = (dir_end[1] - best[1]*2);

	/* Corner case: all three points (best_left, best, best_right) are on the
	   same vertical line; make sure the direction vector is pointing left.
	   Test case: fixedy3, upper left part */
	if ((best[0] == best_left[0]) && (best[0] == best_right[0]))
		f->polarity_dir[0] = -1;

	/* horizontal direction vector is invalid, the ray has to be shifted a tiny
	   bit up or down */
	if (f->polarity_dir[1] == 0) {
		f->polarity_dir[0] = -1000000;
		f->polarity_dir[1] = 1;
	}

	if (pb2_face_polarity_at_verbose) rnd_trace("  dir_end=%ld;%ld dir=%ld;%ld\n", dir_end[0], dir_end[1], (long)f->polarity_dir[0], (long)f->polarity_dir[1]);
}

/* Map faces between curves */
RND_INLINE void pb2_3_face_polarity(pb2_ctx_t *ctx)
{
	pb2_face_t *f;

	for(f = gdl_first(&ctx->faces); f != NULL; f = gdl_next(&ctx->faces, f)) {

		/* skip if already computed (in step 5) */
		if (f->pol_valid)
			continue;

		pb2_3_face_find_polarity_pt(f);

		f->inA = pb2_3_face_polarity_at(ctx, f->polarity_pt, 'A', f->polarity_dir);
		if (ctx->has_B)
			f->inB = pb2_3_face_polarity_at(ctx, f->polarity_pt, 'B', f->polarity_dir);
		else
			f->inB = 0;

		f->pol_valid = 1;

		switch(ctx->op) {
			case RND_PBO_UNITE: f->out = f->inA || f->inB; break;
			case RND_PBO_SUB:   f->out = f->inA && !f->inB; break;
			case RND_PBO_ISECT: f->out = f->inA && f->inB; break;
			case RND_PBO_XOR:   f->out = f->inA ^ f->inB; break;
			case RND_PBO_CANON: f->out = f->inA; break;
			default: assert(!"invalid operation"); abort();
		}
	}
}

