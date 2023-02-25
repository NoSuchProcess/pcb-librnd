/* Map intersections between contours, using pa_segment; the actual low level
   intersection code is in pa_segment, this is the high level part. */

/* Use rtree to find A-B polyline intersections. Whenever a new vertex is
   added, the search for intersections is re-started because the rounding
   could alter the topology otherwise.
   This vould use a faster algorithm for snap rounding intersection finding.
   The best algorithm is probably found in:

   "Improved output-sensitive snap rounding," John Hershberger, Proceedings
   of the 22nd annual symposium on Computational geometry, 2006, pp 357-366.
   http://doi.acm.org/10.1145/1137856.1137909

   Algorithms described by de Berg, or Goodrich or Halperin, or Hobby would
   probably work as well. */
TODO("Maybe use Bentley-Ottman here instead of building trees?");

typedef struct pa_contour_info_s {
	rnd_pline_t *pa;
	jmp_buf restart;
	jmp_buf *getout;
	int need_restart;
	pa_insert_node_task_t *node_insert_list;
} pa_contour_info_t;

static rnd_r_dir_t pa_contour_bounds_touch_cb(const rnd_box_t *b, void *ctx_)
{
	pa_contour_info_t *ctr_ctx = (pa_contour_info_t *)ctx_;
	rnd_pline_t *pla = ctr_ctx->pa, *plb = (rnd_pline_t *)b;
	rnd_pline_t *rtree_over, *looping_over;
	rnd_vnode_t *loop_pt;
	pa_seg_seg_t segctx;
	jmp_buf restart;

	/* Have seg_in_seg_cb return to our desired location if it touches */
	segctx.env = &restart;
	segctx.touch = ctr_ctx->getout;
	segctx.need_restart = 0;
	segctx.node_insert_list = ctr_ctx->node_insert_list;

	/* Pick which contour has the fewer points, and do the loop
	   over that. The r_tree makes hit-testing against a contour
	   faster, so we want to do that on the bigger contour. */
	if (pla->Count < plb->Count) {
		rtree_over = plb;
		looping_over = pla;
	}
	else {
		rtree_over = pla;
		looping_over = plb;
	}

	/* Loop over the nodes in the smaller contour */
	loop_pt = looping_over->head;
	do {
		pa_find_seg_for_pt(looping_over->tree, loop_pt, &segctx); /* fills in segctx */

		/* If we're going to have another pass anyway, skip this */
		if (segctx.s->intersected && (segctx.node_insert_list != NULL))
			continue;

		if (setjmp(restart))
			continue;

		/* NB: If this actually hits anything, we are teleported back to the beginning */
		segctx.tree = rtree_over->tree;
		if (segctx.tree != NULL) {
			int seen;
			rnd_r_search(segctx.tree, &segctx.s->box, seg_in_region_cb, seg_in_seg_cb, &segctx, &seen);
			if (RND_UNLIKELY(seen)) {
				assert(0); /* failed to allocate memory */
			}
		}
	}
	while((loop_pt = loop_pt->next) != looping_over->head);

	ctr_ctx->node_insert_list = segctx.node_insert_list;
	if (segctx.need_restart)
		ctr_ctx->need_restart = 1;
	return RND_R_DIR_NOT_FOUND;
}


static int pa_intersect_impl(jmp_buf *jb, rnd_polyarea_t *b, rnd_polyarea_t *a, int add)
{
	pa_contour_info_t ctr_ctx;
	int need_restart = 0;

	ctr_ctx.need_restart = 0;
	ctr_ctx.node_insert_list = NULL;

	/* Search the r-tree of the object with most contours
	   We loop over the contours of "a". Swap if necessary. */
	if (a->contour_tree->size > b->contour_tree->size)
		SWAP(rnd_polyarea_t *, a, b);

	/* loop over the contours of polyarea "a" */
	for(ctr_ctx.pa = a->contours; ctr_ctx.pa != NULL; ctr_ctx.pa = ctr_ctx.pa->next) {
		rnd_box_t box;
		jmp_buf out;

		ctr_ctx.getout = NULL;

		if (!add) {
			int res = setjmp(out);
			if (res != 0) /* The intersection test short-circuited back here, we need to clean up, then longjmp to jb */
				longjmp(*jb, res);
			ctr_ctx.getout = &out;
		}

		box.X1 = ctr_ctx.pa->xmin;       box.Y1 = ctr_ctx.pa->ymin;
		box.X2 = ctr_ctx.pa->xmax + 1;   box.Y2 = ctr_ctx.pa->ymax + 1;

		rnd_r_search(b->contour_tree, &box, NULL, pa_contour_bounds_touch_cb, &ctr_ctx, NULL);
		if (ctr_ctx.need_restart)
			need_restart = 1;
	}

	/* Process any deferred node insertions */
	if (pa_exec_node_tasks(ctr_ctx.node_insert_list) > 0)
		need_restart = 1; /* Any new nodes could intersect */

	return need_restart;
}

static int pa_intersect(jmp_buf * jb, rnd_polyarea_t * b, rnd_polyarea_t * a, int add)
{
	int call_count = 1;
	while (pa_intersect_impl(jb, b, a, add))
		call_count++;
	return 0;
}

static void M_rnd_polyarea_t_intersect(jmp_buf * e, rnd_polyarea_t * afst, rnd_polyarea_t * bfst, int add)
{
	rnd_polyarea_t *a = afst, *b = bfst;
	rnd_pline_t *curcA, *curcB;
	pa_conn_desc_t *the_list = NULL;

	if (a == NULL || b == NULL)
		error(rnd_err_bad_parm);
	do {
		do {
			if (a->contours->xmax >= b->contours->xmin &&
					a->contours->ymax >= b->contours->ymin &&
					a->contours->xmin <= b->contours->xmax && a->contours->ymin <= b->contours->ymax) {
				if (RND_UNLIKELY(pa_intersect(e, a, b, add)))
					error(rnd_err_no_memory);
			}
		}
		while (add && (a = a->f) != afst);
		for (curcB = b->contours; curcB != NULL; curcB = curcB->next)
			if (curcB->flg.llabel == PA_PLL_ISECTED) {
				the_list = add_descriptors(curcB, 'B', the_list);
				if (RND_UNLIKELY(the_list == NULL))
					error(rnd_err_no_memory);
			}
	}
	while (add && (b = b->f) != bfst);
	do {
		for (curcA = a->contours; curcA != NULL; curcA = curcA->next)
			if (curcA->flg.llabel == PA_PLL_ISECTED) {
				the_list = add_descriptors(curcA, 'A', the_list);
				if (RND_UNLIKELY(the_list == NULL))
					error(rnd_err_no_memory);
			}
	}
	while (add && (a = a->f) != afst);
}																/* M_rnd_polyarea_t_intersect */
