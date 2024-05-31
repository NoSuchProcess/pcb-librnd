RND_INLINE void NO_DEBUG(const char *fmt, ...) { }

#define wr_trace NO_DEBUG
#define if_trace NO_DEBUG


typedef struct pb2_6_ray_s {
	pb2_ctx_t *ctx;
	pb2_face_t *newf;

	gdl_list_t faces_hit; /* list all faces ever hit */
} pb2_6_ray_t;

/* remember a face that got hit by the ray */
RND_INLINE void pb2_6_ray_face_hit(pb2_6_ray_t *rctx, pb2_face_t *f, pb2_seg_t *seg)
{
	TODO("remove seg from the arg list, not needed");

	if ((f == NULL) || (f == rctx->newf))
		return;

	wr_trace(" H S%ld F%ld\n", PB2_UID_GET(seg), PB2_UID_GET(f));

	f->step6.hit++;
	if (f->step6.link.parent == NULL)
		gdl_append(&rctx->faces_hit, f, step6.link);
}

static void pb2_6_ray_seg_hit(void *udata, pb2_seg_t *seg)
{
	pb2_6_ray_t *rctx = udata;
	pb2_curve_t *curve = pb2_seg_parent_curve(seg);

	if ((curve->face[0] == rctx->newf) || (curve->face[1] == rctx->newf)) {
		if (curve->face_1_implicit)
			return; /* ignore curves that are between our target face and the next face */
	}

	pb2_6_ray_face_hit(rctx, curve->face[0], seg);
	if (!curve->face_1_implicit)
		pb2_6_ray_face_hit(rctx, curve->face[1], seg);
}

/* there are no intersections, so disjoint face that's fully within another
   face (e.g. a simple poly hole) can be detected: all curves of the face has
   one face the same as newf and the other face the same as the outer face,
   plus the outer face is larger than the inner face */
RND_INLINE pb2_face_t *pb2_wrapping_face_implicit(pb2_ctx_t *ctx, pb2_face_t *newf, int *is_implicit, int keep_all)
{
	pb2_face_t *bestf = NULL, *other;
	long n;

	if (is_implicit != NULL)
		*is_implicit = 0;

	for(n = 0; n < newf->num_curves; n++) {
		pb2_cgout_t *o = newf->outs[n];
		pb2_curve_t *c = o->curve;

		if (c->face[1] == NULL)
			return NULL;

		if (c->face[0] == newf) {
			other = c->face[1];
		}
		else if (c->face[1] == newf) {
			other = c->face[0];
		}
		else {
			assert(!"one of the curve's faces should be this face as this face has it as a child curve");
			abort();
		}

		if (other == &ctx->root)
			return NULL; /* face has an outside curve */

		if (other->area <= newf->area)
			continue; /* A wrapping face is always larger; we may still touch a smaller face in self-intersecting cases so go on searching */

		/* bestf is our candidate for the outer face */
		if (bestf == NULL) {
			bestf = other;
			if (keep_all) {
				/* test case: bottom triangle hole in si_class2a; inner positive
				   island would be found next and the bestf != other check below
				   woudl return NULL before setting is_implicit */
				break;
			}
		}
		else if (bestf != other)
			return NULL;
	}

	/* Implicit means there is any curve which newf and bestf share and that
	   curve was marked implicit hole */
	if (is_implicit != NULL) {
		for(n = 0; n < newf->num_curves; n++) {
			pb2_cgout_t *o = newf->outs[n];
			pb2_curve_t *c = o->curve;
			if (((c->face[0] == newf) && (c->face[1] == bestf)) || ((c->face[1] == newf) && (c->face[0] == bestf))) {
				*is_implicit = c->face_1_implicit;
			}
		}
	}

	return bestf;
}

RND_INLINE pb2_face_t *pb2_wrapping_face(pb2_ctx_t *ctx, pb2_face_t *newf, int *is_implicit, int keep_all)
{
	pb2_6_ray_t rctx = {0};
	pb2_face_t *f, *bestf = NULL;
	double best_area = -1;

	/* cheapest test: implicit attachment of a fully wrapped face */
	bestf = pb2_wrapping_face_implicit(ctx, newf, is_implicit, keep_all);
	if (bestf != NULL)
		return bestf;

	/* There are no intersections: start a ray within the face (the polarity
	   point is safe for that) and remember what other faces got hit */
	rctx.ctx = ctx;
	rctx.newf = newf;
	wr_trace("WR: F%ld ray from: %ld;%ld:\n", PB2_UID_GET(newf), newf->polarity_pt[0], newf->polarity_pt[1]);
	pb2_3_ray_cast(ctx, newf->polarity_pt, 'F', newf->polarity_dir, pb2_6_ray_seg_hit, &rctx);

	/* process the list of faces hit: find the odd-hit one with the largest
	   area and reset hit counts and step6.link */
	while((f = gdl_first(&rctx.faces_hit)) != NULL) {
		gdl_remove(&rctx.faces_hit, f, step6.link);
		wr_trace(" F%ld hit=%ld area=%f (best_area=%f)\n", PB2_UID_GET(f), f->step6.hit, f->area, best_area);
		if (((f->step6.hit % 2) == 1) && ((best_area < 0) || (f->area < best_area))) {
			bestf = f;
			best_area = f->area;
		wr_trace("   best\n");
		}
		f->step6.hit = 0;
	}

	if (bestf == NULL) {
		/* there are no top level holes, remove them from the output (orphaned holes) */
		if (newf->out == 0) {
			wr_trace(" -> NULL\n");
			return NULL;
		}

		/* crete positive top islands under root */
		bestf = &ctx->root;
	}

	wr_trace(" -> %ld\n", PB2_UID_GET(bestf));

	return bestf;
}

/* Insert newf into the face-tree at ctx->root */
RND_INLINE void pb2_6_insert_face(pb2_ctx_t *ctx, pb2_face_t *newf)
{
	int is_implicit;
	pb2_face_t *bestf = pb2_wrapping_face(ctx, newf, &is_implicit, !newf->out);

	if (bestf == NULL) {
		if_trace("pb2_6_insert_face: skip F%ld: no parent\n", PB2_UID_GET(newf));
		return;
	}

	if ((newf->out == 0) && !is_implicit) {
		/* test case: fixed03 */
		if_trace("pb2_6_insert_face: skip F%ld: hole already drawn by parent\n", PB2_UID_GET(newf));
		return;
	}

	/* link in under bestf */
	if (bestf->parent == NULL) {
		/* we went from bigger to smaller; if we are to insert a smaller positive
		   into a bigger negative and the negative, that means an island within a
		   hole. If the negative is not in the output that means it's an implicit
		   hole, created by the C shaped positive outline; in this case put newf
		   under the root (because the negative is going to be omitted anyway).
		   Test case: class2bc2 */
		bestf = &ctx->root;
	}

	newf->next = bestf->children;
	bestf->children = newf;
	newf->parent = bestf;

}

static int cmp_face_area(const void *a_, const void *b_)
{
	const pb2_face_t * const *a = a_;
	const pb2_face_t * const *b = b_;
	return ((*a)->area > (*b)->area) ? -1 : +1;
}

RND_INLINE void pb2_6_polarity(pb2_ctx_t *ctx)
{
	pb2_face_t *f;
	vtp0_t *faces; /* ordered list of faces from large area to small */
	long nf;

	/* create the virtual root face */
	ctx->root.inA = ctx->root.inB = ctx->root.out = 0;
	ctx->root.children = ctx->root.next = NULL;
	ctx->root.num_curves = 0;
	PB2_UID_CLR(&ctx->root);

	/* sort faces by area */
	faces = &ctx->outtmp; /* reuse outtmp, hopefully avoids a malloc */
	vtp0_enlarge(faces, gdl_length(&ctx->faces)+1);
	faces->used = 0;
	for(f = gdl_first(&ctx->faces); f != NULL; f = gdl_next(&ctx->faces, f))
		vtp0_append(faces, f);

	qsort(faces->array, faces->used, sizeof(pb2_face_t *), cmp_face_area);

	for(nf = 0; nf < faces->used; nf++)
		pb2_6_insert_face(ctx, faces->array[nf]);

	/* do not free faces/outtmp: done in step 8 */
}
