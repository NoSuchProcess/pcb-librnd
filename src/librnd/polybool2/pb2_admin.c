/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2024)
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
 */

RND_INLINE int pb2_seg_nonzero(pb2_seg_t *seg)
{
	if (seg->start[1] == seg->end[1])
		return 0;
	if (seg->start[1] > seg->end[1])
		return +1;
	return -1;
}

RND_INLINE void pb2_1_seg_bbox(pb2_seg_t *seg)
{
	switch(seg->shape_type) {
		case RND_VNODE_LINE:
			seg->bbox.x1 = pa_min(seg->start[0], seg->end[0]);   seg->bbox.y1 = pa_min(seg->start[1], seg->end[1]);
			seg->bbox.x2 = pa_max(seg->start[0], seg->end[0])+1; seg->bbox.y2 = pa_max(seg->start[1], seg->end[1])+1;
			break;
		case RND_VNODE_ARC:
			pb2_arc_bbox(seg);
			break;
	}
}

/* does not set shape, does not calculate bbox, does not register in rtree or ctx  */
RND_INLINE pb2_seg_t *pb2_seg_new_(pb2_ctx_t *ctx, const rnd_vector_t p1, const rnd_vector_t p2)
{
	pb2_seg_t *seg = calloc(sizeof(pb2_seg_t), 1);
	PB2_UID_SET(seg);
	memcpy(seg->start, p1, sizeof(rnd_vector_t));
	memcpy(seg->end, p2, sizeof(rnd_vector_t));
	assert((p1[0] != p2[0]) || (p1[1] != p2[1])); /* 0 length seg is bad */
	/* caller needs to fill in shape and call pb2_seg_new_bbox_and_reg_() */
	return seg;
}

RND_INLINE void pb2_seg_new_bbox_and_reg_(pb2_ctx_t *ctx, pb2_seg_t *seg)
{
	pb2_1_seg_bbox(seg);
	rnd_rtree_insert(&ctx->seg_tree, seg, &seg->bbox);
	seg->next_all = ctx->all_segs;
	ctx->all_segs = seg;
}


RND_INLINE pb2_seg_t *pb2_seg_new_common(pb2_ctx_t *ctx, pb2_seg_t *seg, char poly)
{
	switch(poly) {
		case 'B':
			seg->non0B = pb2_seg_nonzero(seg);
			break;
		case 'A':
		default:
			seg->non0A = pb2_seg_nonzero(seg);
			break;
	}
	return seg;
}

RND_INLINE pb2_seg_t *pb2_seg_new_line(pb2_ctx_t *ctx, const rnd_vector_t p1, const rnd_vector_t p2, char poly)
{
	pb2_seg_t *seg = pb2_seg_new_(ctx, p1, p2);
	pb2_seg_new_bbox_and_reg_(ctx, seg);
	return pb2_seg_new_common(ctx, seg, poly);
}

RND_INLINE void pb2_seg_arc_update_cache(pb2_ctx_t *ctx, pb2_seg_t *seg)
{
	double sa, ea, r1, r2, ravg, cx, cy;

	sa = atan2(seg->start[1] - seg->shape.arc.center[1], seg->start[0] - seg->shape.arc.center[0]);
	ea = atan2(seg->end[1] - seg->shape.arc.center[1], seg->end[0] - seg->shape.arc.center[0]);
	seg->shape.arc.start = sa;
	if (seg->shape.arc.adir) {
		/* Positive delta; CW in svg; CCW in gengeo and C */
		if (ea < sa)
			ea += 2 * G2D_PI;
		seg->shape.arc.delta = ea - sa;
	}
	else {
		/* Negative delta; CCW in svg; CW in gengeo and C */
		if (ea > sa)
			ea -= 2 * G2D_PI;
		seg->shape.arc.delta = ea - sa;
	}

	r1 = rnd_vect_dist2(seg->start, seg->shape.arc.center);
	if (r1 != 0)
		r1 = sqrt(r1);

	if (!Vequ2(seg->start, seg->end)) {
		r2 = rnd_vect_dist2(seg->end, seg->shape.arc.center);
		if (r2 != 0)
			r2 = sqrt(r2);
	}

	/* endpoints may not be on the arc with the original center due to rounding
	   errors on intersections that created the new integer endpoints;
	   calculate a new accurate center */
	if (fabs(r1-r2) > 0.001) {
		double dx, dy, dl, hdl, nx, ny, mx, my, cx, cy, h;

		ravg = (r1+r2)/2.0;

		dx = seg->end[0] - seg->start[0]; dy = seg->end[1] - seg->start[1];
		dl = sqrt(dx*dx + dy*dy); hdl = dl/2.0;
		dx /= dl; dy /= dl;
		mx = (double)(seg->end[0] + seg->start[0])/2.0; my = (double)(seg->end[1] + seg->start[1])/2.0;

		nx = -dy;
		ny = dx;
		h = sqrt(ravg*ravg - hdl*hdl); /* distance from midpoint to center */

		seg->shape.arc.r = ravg;
		seg->shape.arc.cx = mx + nx*h;
		seg->shape.arc.cy = my + ny*h;

#if 0
		pa_trace(" Uarc #", Plong(PB2_UID_GET(seg)), " ", Pvect(seg->start), " -> ", Pvect(seg->end), 0);
		pa_trace(" r ", Pdouble(r1), " ", Pdouble(r2), " ", Pdouble(ravg), 0);
		pa_trace(" mid: ", Pdouble(mx), " ", Pdouble(my), " n: ", Pdouble(nx), " ", Pdouble(ny), " h: ", Pdouble(h), 0);
		pa_trace(" cent: ", Pdouble(seg->shape.arc.cx), " ", Pdouble(seg->shape.arc.cy), "\n",  0);
#endif
	}
	else {
		/* original integer center is accurate enough */
		seg->shape.arc.r = r1;
		seg->shape.arc.cx = seg->shape.arc.center[0];
		seg->shape.arc.cy = seg->shape.arc.center[1];

#if 0
		pa_trace(" uarc #", Plong(PB2_UID_GET(seg)), " ", Pvect(seg->start), " -> ", Pvect(seg->end), 0);
		pa_trace(" r ", Pdouble(r1), 0);
		pa_trace(" cent: ", Pdouble(seg->shape.arc.cx), " ", Pdouble(seg->shape.arc.cy), "\n",  0);
#endif
	}
}

RND_INLINE void pb2_seg_update_cache(pb2_ctx_t *ctx, pb2_seg_t *seg)
{
	switch(seg->shape_type) {
		case RND_VNODE_LINE: break;
		case RND_VNODE_ARC: pb2_seg_arc_update_cache(ctx, seg); break;
	}
}


RND_INLINE pb2_seg_t *pb2_seg_new_arc(pb2_ctx_t *ctx, const rnd_vector_t p1, const rnd_vector_t p2, const rnd_vector_t center, int adir, char poly)
{
	pb2_seg_t *seg = pb2_seg_new_(ctx, p1, p2);
	seg->shape_type = RND_VNODE_ARC;
	seg->shape.arc.center[0] = center[0];
	seg->shape.arc.center[1] = center[1];
	seg->shape.arc.adir = adir;
	pb2_seg_arc_update_cache(ctx, seg);
	pb2_seg_new_bbox_and_reg_(ctx, seg);
	return pb2_seg_new_common(ctx, seg, poly);
}

RND_INLINE pb2_seg_t *pb2_new_seg_from(pb2_ctx_t *ctx, const pb2_seg_t *src, char poly_id)
{
	pb2_seg_t *seg = pb2_seg_new_(ctx, src->start, src->end);
	seg->shape_type = src->shape_type;
	memcpy(&seg->shape, &src->shape, sizeof(src->shape));
	pb2_seg_new_common(ctx, seg, poly_id);
	pb2_seg_new_bbox_and_reg_(ctx, seg);
	return seg;
}


RND_INLINE pb2_seg_t *pb2_seg_new_alike(pb2_ctx_t *ctx, const rnd_vector_t p1, const rnd_vector_t p2, const pb2_seg_t *copy_from)
{
	pb2_seg_t *seg = pb2_seg_new_(ctx, p1, p2);
	seg->shape_type = copy_from->shape_type;
	memcpy(&seg->shape, &copy_from->shape, sizeof(copy_from->shape));
	seg->cntA = copy_from->cntA;
	seg->cntB = copy_from->cntB;
	seg->non0A = copy_from->non0A;
	seg->non0B = copy_from->non0B;
	seg->discarded = copy_from->discarded;
	seg->shape_type = copy_from->shape_type;
	if (seg->shape_type > 0)
		memcpy(&seg->shape, &copy_from->shape, sizeof(seg->shape));
	pb2_seg_update_cache(ctx, seg);
	pb2_seg_new_bbox_and_reg_(ctx, seg);
	return seg;
}

/* Create a new curve and insert sa in it */
RND_INLINE pb2_curve_t *pb2_curve_new_for_seg(pb2_ctx_t *ctx, pb2_seg_t *s)
{
	pb2_curve_t *c;

	c = calloc(sizeof(pb2_curve_t), 1);
	PB2_UID_SET(c);
	gdl_append(&c->segs, s, link);

	gdl_append(&ctx->curves, c, link);

	return c;
}

RND_INLINE void pb2_curve_free(pb2_ctx_t *ctx, pb2_curve_t *c)
{
	assert(gdl_first(&c->segs) == NULL);
	gdl_remove(&ctx->curves, c, link);
	free(c);
}

/*** face util ***/
typedef struct pb2_face_it_s {
	pb2_face_t *face;
	pb2_seg_t *start;
	long out_idx;
	unsigned cfg_reverse:1;

	/* next item to return */
	pb2_curve_t *curve;
	pb2_seg_t *seg;
	unsigned reverse:1;
	unsigned finished:1;
} pb2_face_it_t;

RND_INLINE void pb2_face_it_jump_curve(pb2_face_it_t *it)
{
	pb2_cgout_t *o;

	if (it->cfg_reverse) {
		it->out_idx--;
		if (it->out_idx < 0) {
			it->finished = 1;
			it->seg = NULL;
			return;
		}
	}
	else {
		it->out_idx++;
		if (it->out_idx >= it->face->num_curves) {
			it->finished = 1;
			it->seg = NULL;
			return;
		}
	}
	o = it->face->outs[it->out_idx];
	it->curve = o->curve;
	it->reverse = o->reverse ^ it->cfg_reverse;
	it->seg = it->reverse ? gdl_last(&o->curve->segs) : gdl_first(&o->curve->segs);
}

RND_INLINE pb2_seg_t *pb2_face_it_next(pb2_face_it_t *it, int *reverse_out, rnd_vector_t cursor_out)
{
	pb2_seg_t *res;

	/* prepare the output */
	res = it->seg;
	if (reverse_out != NULL)
		*reverse_out = it->reverse;

	if (it->seg != NULL) {
		if (cursor_out != NULL)
			memcpy(cursor_out, (it->reverse ? it->seg->end : it->seg->start), sizeof(rnd_vector_t));

		/* jump to the next */
		it->seg = it->reverse ? gdl_prev(&it->curve->segs, it->seg) : gdl_next(&it->curve->segs, it->seg);
		if (it->seg == NULL)
			pb2_face_it_jump_curve(it);
	}

	return res;
}

RND_INLINE pb2_seg_t *pb2_face_it_first(pb2_face_it_t *it, pb2_face_t *face, int *reverse_out, rnd_vector_t cursor_out, int cfg_reverse)
{
	it->finished = 0;
	it->face = face;
	it->out_idx = cfg_reverse ? it->face->num_curves : -1;
	it->cfg_reverse = cfg_reverse;
	pb2_face_it_jump_curve(it);
	return pb2_face_it_next(it, reverse_out, cursor_out);
}
