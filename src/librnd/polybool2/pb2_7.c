RND_INLINE rnd_pline_t *pb2_7_mkpline(pb2_ctx_t *ctx, pb2_face_t *f, int positive)
{
	pb2_face_it_t it;
	rnd_pline_t *pl;
	rnd_vector_t pt;
	long count;

	if (pb2_face_it_first(&it, f, NULL, pt, !positive) == NULL)
		return NULL;

	pl = pa_pline_new(pt);
	count = 1;

	while(pb2_face_it_next(&it, NULL, pt) != NULL) {
		TODO("arc: this assumes lines only; for arcs use the seg return of _next()");
		if (rnd_poly_vertex_include(pl->head->prev, rnd_poly_node_create(pt)) == 0)
			count++;
	}

	/* Cheaper version of pa_pline_update(pl, 1), reusing fields of face: */
	pl->xmin = f->bbox.x1; pl->ymin = f->bbox.y1;
	pl->xmax = f->bbox.x2; pl->ymax = f->bbox.y2;
	pl->area = f->area;
	pl->Count = count;
	pl->flg.orient = positive ? RND_PLF_DIR : RND_PLF_INV;
	if (!ctx->inhibit_edge_tree)
		pl->tree = rnd_poly_make_edge_tree(pl);

	return pl;
}

RND_INLINE void pb2_7_mkpline_and_insert(pb2_ctx_t *ctx, rnd_polyarea_t **res, rnd_polyarea_t **parent_pa, pb2_face_t *f, int positive)
{
	rnd_pline_t *pl = pb2_7_mkpline(ctx, f, positive);

	assert(pl != NULL);
	if (pl == NULL)
		return;

	if (positive) {
		rnd_polyarea_t *pa = pa_polyarea_alloc();
		pa->contours = pl;
		*parent_pa = pa;
		rnd_polyarea_m_include(res, pa);
	}
	else {
		/* insert hole in parent positive */
		if (!pa_polyarea_insert_pline(*parent_pa, pl))
			pa_pline_free(&pl);
	}
}


RND_INLINE int pb2_7_face_is_sorrunded_by_holes(pb2_face_t *f)
{
	long n;

	for(n = 0; n < f->num_curves; n++) {
		pb2_cgout_t *o = f->outs[n];
		pb2_curve_t *c = o->curve;
		pb2_face_t *other = NULL;

		if (c->face[0] == f)
			other = c->face[1];
		else if (c->face[1] == f)
			other = c->face[0];

		if ((other != NULL) && (other->out))
			return 0;

	}
	return 1;
}


RND_INLINE void pb2_7_output_subtree(pb2_ctx_t *ctx, rnd_polyarea_t **res, pb2_face_t *parent, rnd_polyarea_t *parent_pa)
{
	pb2_face_t *f;

	for(f = parent->children; f != NULL; f = f->next) {
		if (f->out && parent->out && pb2_7_face_is_sorrunded_by_holes(f)) {
			/* corner case: in self-intersection resolvemenent of test case si_class2a
			   a central positive area is formed that's not part of any larger hole but
			   is sorrunded by smaller holes. It appears as a positive island but
			   does not need to be exported. */
			/* if we wanted to output it as a new island: rnd_polyarea_t *parent_pa = NULL; pb2_7_output_subtree(ctx, res, f, new_parent_pa); */
			if_trace("omitting F%ld: positive-in-positive, sorrunded by islands\n", PB2_UID_GET(f));
		}
		else {
			assert(f->out != parent->out);
			pb2_7_mkpline_and_insert(ctx, res, &parent_pa, f, f->out);
			pb2_7_output_subtree(ctx, res, f, parent_pa);
		}
	}
}


RND_INLINE void pb2_7_output(pb2_ctx_t *ctx, rnd_polyarea_t **res)
{
	rnd_polyarea_t *parent_pa = NULL;

	*res = NULL;
	pb2_7_output_subtree(ctx, res, &ctx->root, parent_pa);
}
