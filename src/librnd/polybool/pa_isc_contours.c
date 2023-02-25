/* Map intersections between contours, using pa_segment; the actual low level
   intersection code is in pa_segment, this is the high level part. */

/*
 * intersect() (and helpers)
 * (C) 2006, harry eaton
 * This uses an rtree to find A-B intersections. Whenever a new vertex is
 * added, the search for intersections is re-started because the rounding
 * could alter the topology otherwise. 
 * This should use a faster algorithm for snap rounding intersection finding.
 * The best algorithm is probably found in:
 *
 * "Improved output-sensitive snap rounding," John Hershberger, Proceedings
 * of the 22nd annual symposium on Computational geometry, 2006, pp 357-366.
 * http://doi.acm.org/10.1145/1137856.1137909
 *
 * Algorithms described by de Berg, or Goodrich or Halperin, or Hobby would
 * probably work as well.
 *
 */

static rnd_r_dir_t contour_bounds_touch(const rnd_box_t * b, void *cl)
{
	contour_info *c_info = (contour_info *) cl;
	rnd_pline_t *pa = c_info->pa;
	rnd_pline_t *pb = (rnd_pline_t *) b;
	rnd_pline_t *rtree_over;
	rnd_pline_t *looping_over;
	rnd_vnode_t *av;										/* node iterators */
	struct info info;
	rnd_box_t box;
	jmp_buf restart;

	/* Have seg_in_seg return to our desired location if it touches */
	info.env = &restart;
	info.touch = c_info->getout;
	info.need_restart = 0;
	info.node_insert_list = c_info->node_insert_list;

	/* Pick which contour has the fewer points, and do the loop
	 * over that. The r_tree makes hit-testing against a contour
	 * faster, so we want to do that on the bigger contour.
	 */
	if (pa->Count < pb->Count) {
		rtree_over = pb;
		looping_over = pa;
	}
	else {
		rtree_over = pa;
		looping_over = pb;
	}

	av = looping_over->head;
	do {													/* Loop over the nodes in the smaller contour */
		rnd_r_dir_t rres;
		/* check this edge for any insertions */
		double dx;
		info.v = av;
		/* compute the slant for region trimming */
		dx = av->next->point[0] - av->point[0];
		if (dx == 0)
			info.m = 0;
		else {
			info.m = (av->next->point[1] - av->point[1]) / dx;
			info.b = av->point[1] - info.m * av->point[0];
		}
		box.X2 = (box.X1 = av->point[0]) + 1;
		box.Y2 = (box.Y1 = av->point[1]) + 1;

		/* fill in the segment in info corresponding to this node */
		rres = rnd_r_search(looping_over->tree, &box, NULL, get_seg, &info, NULL);
		assert(rres == RND_R_DIR_CANCEL);

		/* If we're going to have another pass anyway, skip this */
		if (info.s->intersected && info.node_insert_list != NULL)
			continue;

		if (setjmp(restart))
			continue;

		/* NB: If this actually hits anything, we are teleported back to the beginning */
		info.tree = rtree_over->tree;
		if (info.tree) {
			int seen;
			rnd_r_search(info.tree, &info.s->box, seg_in_region, seg_in_seg, &info, &seen);
			if (RND_UNLIKELY(seen))
				assert(0);							/* XXX: Memory allocation failure */
		}
	}
	while ((av = av->next) != looping_over->head);

	c_info->node_insert_list = info.node_insert_list;
	if (info.need_restart)
		c_info->need_restart = 1;
	return RND_R_DIR_NOT_FOUND;
}

static int intersect_impl(jmp_buf * jb, rnd_polyarea_t * b, rnd_polyarea_t * a, int add)
{
	rnd_polyarea_t *t;
	rnd_pline_t *pa;
	contour_info c_info;
	int need_restart = 0;
	insert_node_task *task;
	c_info.need_restart = 0;
	c_info.node_insert_list = NULL;

	/* Search the r-tree of the object with most contours
	 * We loop over the contours of "a". Swap if necessary.
	 */
	if (a->contour_tree->size > b->contour_tree->size) {
		t = b;
		b = a;
		a = t;
	}

	for (pa = a->contours; pa; pa = pa->next) {	/* Loop over the contours of rnd_polyarea_t "a" */
		rnd_box_t sb;
		jmp_buf out;
		int retval;

		c_info.getout = NULL;
		c_info.pa = pa;

		if (!add) {
			retval = setjmp(out);
			if (retval) {
				/* The intersection test short-circuited back here,
				 * we need to clean up, then longjmp to jb */
				longjmp(*jb, retval);
			}
			c_info.getout = &out;
		}

		sb.X1 = pa->xmin;
		sb.Y1 = pa->ymin;
		sb.X2 = pa->xmax + 1;
		sb.Y2 = pa->ymax + 1;

		rnd_r_search(b->contour_tree, &sb, NULL, contour_bounds_touch, &c_info, NULL);
		if (c_info.need_restart)
			need_restart = 1;
	}

	/* Process any deferred node insertions */
	task = c_info.node_insert_list;
	while (task != NULL) {
		insert_node_task *next = task->next;

		/* Do insertion */
		task->new_node->prev = task->node_seg->v;
		task->new_node->next = task->node_seg->v->next;
		task->node_seg->v->next->prev = task->new_node;
		task->node_seg->v->next = task->new_node;
		task->node_seg->p->Count++;

		cntrbox_adjust(task->node_seg->p, task->new_node->point);
		if (adjust_tree(task->node_seg->p->tree, task->node_seg))
			assert(0);								/* XXX: Memory allocation failure */

		need_restart = 1;						/* Any new nodes could intersect */

		free(task);
		task = next;
	}

	return need_restart;
}

static int intersect(jmp_buf * jb, rnd_polyarea_t * b, rnd_polyarea_t * a, int add)
{
	int call_count = 1;
	while (intersect_impl(jb, b, a, add))
		call_count++;
	return 0;
}

static void M_rnd_polyarea_t_intersect(jmp_buf * e, rnd_polyarea_t * afst, rnd_polyarea_t * bfst, int add)
{
	rnd_polyarea_t *a = afst, *b = bfst;
	rnd_pline_t *curcA, *curcB;
	pa_conn_list_t *the_list = NULL;

	if (a == NULL || b == NULL)
		error(rnd_err_bad_parm);
	do {
		do {
			if (a->contours->xmax >= b->contours->xmin &&
					a->contours->ymax >= b->contours->ymin &&
					a->contours->xmin <= b->contours->xmax && a->contours->ymin <= b->contours->ymax) {
				if (RND_UNLIKELY(intersect(e, a, b, add)))
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
