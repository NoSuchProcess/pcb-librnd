RND_INLINE void pb2_8_cleanup(pb2_ctx_t *ctx)
{
	pb2_curve_t *c, *cn;
	pb2_face_t *f, *fn;
	pb2_cgnode_t *n, *nn;
	pb2_seg_t *s, *sn;

	rnd_rtree_uninit(&ctx->seg_tree);

	/* free faces */
	for(f = gdl_first(&ctx->faces); f != NULL; f = fn) {
		fn = gdl_next(&ctx->faces, f);
		free(f);
	}

	/* free curves */
	for(c = gdl_first(&ctx->curves); c != NULL; c = cn) {
		cn = gdl_next(&ctx->curves, c);
		free(c);
	}

	/* free nodes */
	for(n = gdl_first(&ctx->cgnodes); n != NULL; n = nn) {
		nn = gdl_next(&ctx->cgnodes, n);
		free(n);
	}

	/* free segs */
	for(s = ctx->all_segs; s != NULL; s = sn) {
		sn = s->next_all;
		free(s);
	}

	/* Note: outs are not free'd, they are allocated as part of nodes */

	vtp0_uninit(&ctx->outtmp);
}
