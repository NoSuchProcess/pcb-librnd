RND_INLINE pb2_face_t *pb2_wrapping_face(pb2_ctx_t *ctx, pb2_face_t *newf, int *is_implicit);

/* corner case: current face may be fully within another face without any
   connection. Test case: fixed00, gixed0 */
RND_INLINE void pb2_4_curve_other_face(pb2_ctx_t *ctx, pb2_curve_t *c)
{
	if (c->face[1] == NULL) {
		c->face[1] = pb2_wrapping_face(ctx, c->face[0], NULL);
		c->face_1_implicit = 1;
	}
}

RND_INLINE void pb2_4_destroy_curve_face(pb2_ctx_t *ctx, pb2_face_t *f)
{
	long n;

	if (f->destroy)
		return;

	f->destroy = 1;
	for(n = 0; n < f->num_curves; n++) {
		pb2_curve_t *c = f->outs[n]->curve;

		f->outs[n]->corner_mark = 0;

		/* decouple this face from the curve */
		if ((c->face[1] != NULL) && c->face_1_implicit)
			c->face[1] = NULL;

		if (c->face[1] == f)
			c->face[1] = NULL;

		if (c->face[0] == f) {
			c->face[0] = NULL;
			if (c->face[1] != NULL) {
				c->face[0] = c->face[1];
				c->face[1] = NULL;
			}
		}
		c->face_1_implicit = 0;
	}
}

RND_INLINE void pb2_4_prune_curve(pb2_ctx_t *ctx, pb2_curve_t *c, int destroy_faces)
{
	c->pruned = 1;
	if (destroy_faces) {
	}
}

/* Returns 1 if any curve is marked */
RND_INLINE int pb2_4_prune_curves(pb2_ctx_t *ctx)
{
	pb2_curve_t *c;
	int changed = 0;

	for(c = gdl_first(&ctx->curves); c != NULL; c = gdl_next(&ctx->curves, c)) {

		if (c->face[0] == NULL) {
			/* happens on a stub that has both ends connected but no face on either
			   side, e.g. the center pillar of the bone example; test case: fixedm */
			assert(c->face[1] == NULL); /* faces are filled in sequentially from 0 */
			c->pruned = 1;
			/* shouldn't destroy faces for a stub */
		}

		if (c->pruned)
			continue; /* step 1 may have marked the curve pruned if it's a dangling stub */

		pb2_4_curve_other_face(ctx, c);

		if (c->face[1] != NULL) {
			/* two adjacent faces */
			if (c->face[0]->out == c->face[1]->out) {
				c->pruned = c->face_destroy = 1;
				changed = 1;
			}
		}
		else {
			/* one adjacent face */

			if (c->face[0]->out == 0) {
				c->pruned = c->face_destroy = 1;
				changed = 1;
			}
		}
	}

	if (changed) {
		for(c = gdl_first(&ctx->curves); c != NULL; c = gdl_next(&ctx->curves, c)) {

			/* mark related faces destroyed if the curve got pruned now and was not a stub */
			if (c->face_destroy) {
				pb2_face_t *f0 = c->face[0], *f1 = c->face[1]; /* need to cache because pb2_4_destroy_curve_face() changes c->face[] */

				if (f0 != NULL) pb2_4_destroy_curve_face(ctx, f0);
				if (f1 != NULL) pb2_4_destroy_curve_face(ctx, f1);
				c->face_destroy = 0;
			}
		}

		/* remove implicit attachment if wrapping face has been destroyed */
		for(c = gdl_first(&ctx->curves); c != NULL; c = gdl_next(&ctx->curves, c)) {
			if (c->face_1_implicit) {
				if (c->face[1]->destroy) {
					c->face_1_implicit = 0;
					c->face[1] = NULL;
				}
			}
		}
	}

	return changed;
}
