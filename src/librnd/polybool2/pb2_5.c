RND_INLINE void pb2_5_cleanup(pb2_ctx_t *ctx)
{
	pb2_face_t *f, *next;

	/* cheap free of all destroyed faces */
	for(f = gdl_first(&ctx->faces); f != NULL; f = next) {
		next = gdl_next(&ctx->faces, f);
		if (f->destroy) {
			gdl_remove(&ctx->faces, f, link);
			free(f);
		}
	}
}

RND_INLINE void pb2_5_face_remap(pb2_ctx_t *ctx)
{
	pb2_curve_t *c;

	pb2_5_cleanup(ctx);
	pb2_2_face_map(ctx);
	pb2_3_face_polarity(ctx);


	for(c = gdl_first(&ctx->curves); c != NULL; c = gdl_next(&ctx->curves, c))
		if (!c->pruned)
			pb2_4_curve_other_face(ctx, c);
}
