/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2023 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2023)
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
 *  This is a full rewrite of pcb-rnd's (and PCB's) polygon lib originally
 *  written by Harry Eaton in 2006, in turn building on "poly_Boolean: a
 *  polygon clip library" by Alexey Nikitin, Michael Leonov from 1997 and
 *  "nclip: a polygon clip library" Klamer Schutte from 1993.
 *
 *  English translation of the original paper the lib is largely based on:
 *  https://web.archive.org/web/20160418014630/http://www.complex-a5.ru/polyboolean/downloads/polybool_eng.pdf
 *
 */


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

		/* If this actually hits anything, we are long-jumped back to the beginning */
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

/* Find first new intersections between a and b and create new nodes there and
   mark everything intersected; returns non-zero if it should be re-run with
   the same args because there are potentially more intersections to map */
RND_INLINE int pa_intersect_impl(jmp_buf *jb, rnd_polyarea_t *b, rnd_polyarea_t *a, int all_iscs)
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

		if (!all_iscs) {
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

/* Find all intersections between a and b and create new nodes there and mark
   them intersected */
static void pa_intersect(jmp_buf *jb, rnd_polyarea_t *b, rnd_polyarea_t *a, int all_iscs)
{
	/* restart the search as many times as the implementation asks for the restart */
	while(pa_intersect_impl(jb, b, a, all_iscs)) ;
}

/* Collect all intersections from pa onto conn_list */
static pa_conn_desc_t *pa_polyarea_list_intersected(jmp_buf *e, pa_conn_desc_t *conn_list, rnd_polyarea_t *pa, char poly_label)
{
	rnd_pline_t *n;

	for (n = pa->contours; n != NULL; n = n->next) {
		if (n->flg.llabel == PA_PLL_ISECTED) {
			conn_list = pa_add_conn_desc(n, poly_label, conn_list);
			if (RND_UNLIKELY(conn_list == NULL))
				error(rnd_err_no_memory);
		}
	}
	return conn_list;
}

/* Compute intersections between all islands of pa_a and pa_b. If all_iscs is
   true, collect and label all intersections, else stop after the first
   (usefule if only the fact of the intersection is interesting) */
static void pa_polyarea_intersect(jmp_buf *e, rnd_polyarea_t *pa_a, rnd_polyarea_t *pa_b, int all_iscs)
{
	rnd_polyarea_t *a, *b;
	pa_conn_desc_t *conn_list = NULL;

	if ((pa_a == NULL) || (pa_b == NULL))
		error(rnd_err_bad_parm);

	/* calculate all intersections between pa_a and pa_b, iterating over the
	   islands if pa_b; also add all of pa_b's intersections in conn_list */
	b = pa_b;
	do {
		do {
			if (pa_polyarea_box_overlap(a, b))
				pa_intersect(e, a, b, all_iscs);
		} while (all_iscs && ((a = a->f) != pa_a));

		conn_list = pa_polyarea_list_intersected(e, conn_list, b, 'B');
	} while (all_iscs && (b = b->f) != pa_b);

	/* add all intersections of all of pa_a's islands to the conn list */
	a = pa_a;
	do {
		conn_list = pa_polyarea_list_intersected(e, conn_list, a, 'A');
	} while (all_iscs && ((a = a->f) != pa_a));

	/* Note: items of conn_list are stored plines of pa_a and pa_b */
}
