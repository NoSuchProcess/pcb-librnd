RND_INLINE void pb2_2_prelim_face_area_bbox_(rnd_vector_t *p, rnd_vector_t *c, rnd_rtree_box_t *bbox, double *area)
{
	*area += ((double)(*p)[0] - (double)(*c)[0]) * ((double)(*p)[1] + (double)(*c)[1]);

	if ((*c)[0]     < bbox->x1) bbox->x1 = (*c)[0];
	if (((*c)[0]+1) > bbox->x2) bbox->x2 = (*c)[0]+1;
	if ((*c)[1]     < bbox->y1) bbox->y1 = (*c)[1];
	if (((*c)[1]+1) > bbox->y2) bbox->y2 = (*c)[1]+1;
}

RND_INLINE double pb2_2_prelim_face_area_bbox(vtp0_t *outs, rnd_rtree_box_t *bbox, long *count_out)
{
	long n, count = 0;
	rnd_vector_t *p = NULL, *c, *first = NULL;
	double area = 0;

	if (outs->used == 0)
		return -1;

	bbox->x1 = bbox->y1 = +RND_COORD_MAX;
	bbox->x2 = bbox->y2 = -RND_COORD_MAX;

	for(n = 0; n < outs->used; n++) {
		pb2_cgout_t *o = outs->array[n];
		pb2_seg_t *s, *start;

		start = o->reverse ? gdl_last(&o->curve->segs) : gdl_first(&o->curve->segs);
		for(s = start; s != NULL; s = o->reverse ? gdl_prev(&o->curve->segs, s) : gdl_next(&o->curve->segs, s)) {
			c = o->reverse ? &s->end : &s->start;

			if (p != NULL) /* we don't have prev (p) on first iteration (n==0 and s==start) */
				pb2_2_prelim_face_area_bbox_(p, c, bbox, &area);

			count++;
			p = c;
			if (first == NULL)
				first = c;
		}
	}

	/* we didn't have prev (p) on first iteration - but now we have the
	   last point in p==c and still remember the first */
	pb2_2_prelim_face_area_bbox_(p, first, bbox, &area);

	*count_out = count;
	return area;
}

/* Return the next outgoing edge (by angle) after o, within o's node; ignore
   outgoing edges with pruned curves */
RND_INLINE pb2_cgout_t *pb2_2_out_next_in_cgnode(pb2_cgout_t *o)
{
	int idx;
	
	for(idx = o->nd_idx - 1; idx != o->nd_idx; idx--) {
		pb2_cgout_t *res;

		if (idx < 0)
			idx = o->node->num_edges - 1;
		
		res = &o->node->edges[idx];
		if (!res->curve->pruned)
			return res;
	}

	return NULL;
}

/* return the other end of o's curve */
RND_INLINE pb2_cgout_t *pb2_2_out_traverse_curve(pb2_cgout_t *o)
{
	if (o == o->curve->out_start)
		return o->curve->out_end;
	return o->curve->out_start;
}

/* greedy collection of a face from an unmarked corner */
RND_INLINE pb2_face_t *pb2_2_map_faces(pb2_ctx_t *ctx, pb2_cgout_t *start)
{
	pb2_cgout_t *o, *on;
	pb2_face_t *face;
	long n, count;
	double area;
	rnd_rtree_box_t bbox;

	/* collect cgouts in ctx->outtmp, mark all corners affected */
	ctx->outtmp.used = 0;
	o = start;
	do {
		o->corner_mark = 1;
		on = pb2_2_out_traverse_curve(o);
		vtp0_append(&ctx->outtmp, o);
		assert(on != NULL); /* curves must end in nodes, even if the node has only one curve connected (stub) */

		o = pb2_2_out_next_in_cgnode(on);
		if (on == o)
			return NULL; /* corner case: endpoint of a stub - discard the stub */

	} while(o != start);

	area = pb2_2_prelim_face_area_bbox(&ctx->outtmp, &bbox, &count);

	if (area < 0)
		return NULL; /* ignore the outer face */

	/* Curves of a face (closed loop) collected in ctx->curvetmp, allocate
	   the face for it */
	face = calloc(sizeof(pb2_face_t) + sizeof(pb2_cgout_t *) * (ctx->outtmp.used-1), 1);
	PB2_UID_SET(face);
	gdl_append(&ctx->faces, face, link);

	memcpy(&face->bbox, &bbox, sizeof(bbox));
	face->area = area;
	face->num_curves = ctx->outtmp.used;
	memcpy(face->outs, ctx->outtmp.array, sizeof(pb2_cgout_t *) * ctx->outtmp.used);

	/* set each curve's face */
	for(n = 0; n < ctx->outtmp.used; n++) {
		pb2_cgout_t *o = ctx->outtmp.array[n];
		pb2_curve_t *c = o->curve;
		if (c->face[0] == NULL) c->face[0] = face;
		else if (c->face[1] == NULL) c->face[1] = face;
		else if (c->face_1_implicit) {
			/* happens on step5 selective rebuild; test case: simple01 */
			c->face[1] = face;
			c->face_1_implicit = 0;
		}
		else {
			assert(!"pb2_2_map_faces(): curve with 3 faces");
			abort();
		}
	}

	return face;
}

/* Map faces between curves */
RND_INLINE void pb2_2_face_map(pb2_ctx_t *ctx)
{
	pb2_curve_t *c;

	/* try mapping from both ends of each curve if the given corner is not yet
	   part of any face */
	for(c = gdl_first(&ctx->curves); c != NULL; c = gdl_next(&ctx->curves, c)) {
		if (c->pruned)
			continue;
		if (!c->out_start->corner_mark)
			pb2_2_map_faces(ctx, c->out_start);
		if (!c->out_end->corner_mark)
			pb2_2_map_faces(ctx, c->out_end);
	}

	/* do not free outtmp: allocation is going to be reused in step 6 */
}

