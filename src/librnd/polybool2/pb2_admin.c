RND_INLINE void pb2_1_seg_bbox(pb2_seg_t *seg)
{
	seg->bbox.x1 = pa_min(seg->start[0], seg->end[0]);   seg->bbox.y1 = pa_min(seg->start[1], seg->end[1]);
	seg->bbox.x2 = pa_max(seg->start[0], seg->end[0])+1; seg->bbox.y2 = pa_max(seg->start[1], seg->end[1])+1;
}

RND_INLINE pb2_seg_t *pb2_seg_new(pb2_ctx_t *ctx, const rnd_vector_t p1, const rnd_vector_t p2)
{
	pb2_seg_t *seg = calloc(sizeof(pb2_seg_t), 1);
	PB2_UID_SET(seg);
	memcpy(seg->start, p1, sizeof(rnd_vector_t));
	memcpy(seg->end, p2, sizeof(rnd_vector_t));
	pb2_1_seg_bbox(seg);
	rnd_rtree_insert(&ctx->seg_tree, seg, &seg->bbox);
	assert((p1[0] != p2[0]) || (p1[1] != p2[1])); /* 0 length seg is bad */
	seg->next_all = ctx->all_segs;
	ctx->all_segs = seg;
	return seg;
}

RND_INLINE pb2_seg_t *pb2_seg_new_alike(pb2_ctx_t *ctx, const rnd_vector_t p1, const rnd_vector_t p2, const pb2_seg_t *copy_from)
{
	pb2_seg_t *seg = pb2_seg_new(ctx, p1, p2);
	seg->cntA = copy_from->cntA;
	seg->cntB = copy_from->cntB;
	seg->discarded = copy_from->discarded;
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
