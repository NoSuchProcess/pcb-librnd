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
TODO("Maybe use Bentley-Ottman here instead of building trees? But how to handle arcs?")

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
				pa_error(pa_err_no_memory);
		}
	}
	return conn_list;
}

/* Corner case: if a sloped line of polygon B crosses a triangular double-slope
   of polygon A very close to the corner of A, it may yield a short stub of
   A. Example case: gixedk at point 'o' at 331.9999;471:

               329;454
                 |B
                 |
              A  |
    319;475  ----o------x 332;471
                /|    A
              /  |B
            /A   |
      331;477    333.03;476.83

   In collect we are coming from 331;477 and should leave toward 319;475.
   This doesnt happen if the stub between o and x exists because then pa_label
   will think the upper B segment is outside.

   When a stub detected, mark both cvcs to be ignored and whenever the code
   needs to iterate around the cvcs of a point, skip over ignored cvcs.
*/

static void pa_conn_list_remove_stubs(pa_conn_desc_t **head)
{
	pa_conn_desc_t *n = *head, *n2, *next;

	if ((n == NULL) || (n == n->next))
		return;

	do {
		next = n->next;

		/* a stub has two edges of the same poly in the same direction... */
		if ((n->poly == n->next->poly) && pa_angle_equ(n, n->next)) {
			rnd_vnode_t *remote_curr, *remote_next;

			n2 = n->next;

			remote_curr = n->side == 'N' ? n->parent->next : n->parent->prev;
			remote_next = n2->side == 'N' ? n2->parent->next : n2->parent->prev;

			/* ...reaching the same remote node */
			if (remote_curr == remote_next)
				n->ignore = n2->ignore = 1; /* ignore this stub */
		}
	} while((n = next) != *head);
}

/* Compute intersections between all islands of pa_a and pa_b. If all_iscs is
   true, collect and label all intersections, else stop after the first
   (usefule if only the fact of the intersection is interesting) */
static void pa_polyarea_intersect(jmp_buf *e, rnd_polyarea_t *pa_a, rnd_polyarea_t *pa_b, int all_iscs)
{
	rnd_polyarea_t *a = pa_a, *b = pa_b;
	pa_conn_desc_t *conn_list = NULL;

#ifdef DEBUG_PAISC_DUMP
	pa_debug_dump(stderr, "pa_polyarea_intersect PRE pa_a", pa_a, 0);
	pa_debug_dump(stderr, "pa_polyarea_intersect PRE pa_b", pa_b, 0);
#endif


	if ((pa_a == NULL) || (pa_b == NULL))
		pa_error(pa_err_bad_parm);

	/* calculate all intersections between pa_a and pa_b, iterating over the
	   islands if pa_b; also add all of pa_b's intersections in conn_list */
	do {
		do {
			if (pa_polyarea_box_overlap(a, b))
				pa_intersect(e, a, b, all_iscs);
		} while (all_iscs && ((a = a->f) != pa_a));

		conn_list = pa_polyarea_list_intersected(e, conn_list, b, 'B');
		pa_debug_print_cvc(conn_list);
	} while (all_iscs && (b = b->f) != pa_b);

	/* add all intersections of all of pa_a's islands to the conn list */
	do {
		conn_list = pa_polyarea_list_intersected(e, conn_list, a, 'A');
		pa_conn_list_remove_stubs(&conn_list);
		pa_debug_print_cvc(conn_list);
	} while (all_iscs && ((a = a->f) != pa_a));

#ifdef DEBUG_PAISC_DUMP
	pa_debug_dump(stderr, "pa_polyarea_intersect POST pa_a", pa_a, 0);
	pa_debug_dump(stderr, "pa_polyarea_intersect POST pa_b", pa_b, 0);
#endif


	/* Note: items of conn_list are stored plines of pa_a and pa_b */
}

/* Returns 1 if nd is participating in a true crossing; a self-touching
   of >< topology is not considered a true crossing, but an X topology is. */
int pa_cvc_crossing_at_node(rnd_vnode_t *nd)
{
	pa_conn_desc_t *c, *cp, *cn;

	c = nd->cvclst_prev;
	if (c == NULL)
		return 0;
/*	rnd_trace("  me:   %ld;%ld {%ld;%ld} %ld;%ld (%p) %c\n",
		c->parent->prev->point[0], c->parent->prev->point[1],
		c->parent->point[0], c->parent->point[1],
		c->parent->next->point[0], c->parent->next->point[1],
		c->parent, c->side
		);*/

	cn = nd->cvclst_prev->next;
/*	rnd_trace("  next: %ld;%ld {%ld;%ld} %ld;%ld (%p) %c\n",
		cn->parent->prev->point[0], cn->parent->prev->point[1],
		cn->parent->point[0], cn->parent->point[1],
		cn->parent->next->point[0], cn->parent->next->point[1],
		cn->parent, cn->side
		);*/

	cp = nd->cvclst_prev->prev;
/*	rnd_trace("  prev: %ld;%ld {%ld;%ld} %ld;%ld (%p) %c\n",
		cp->parent->prev->point[0], cp->parent->prev->point[1],
		cp->parent->point[0], cp->parent->point[1],
		cp->parent->next->point[0], cp->parent->next->point[1],
		cp->parent, cp->side
		);*/

	if ((cn->parent != nd) && (cp->parent != nd)) {
		if (cn->parent != cp->parent) {
			/* Special case: test case gixedo2 around 152;626:
			          144;616
			          +,
			            \
			   me        '\
			   92;650      '-----+ 155;627
			   +-----------+-----+ 154;628
			              /'\
			            /'   '\
			          +'       '+
			      141;644      155;632
			
			   The check is coming from 92;65 and figures 155;627 and 155;632->*
			   are the next and prev incoming nodes into this CVC. This looks like
			   a crossing because they are on different * point than 'me'. However
			   they are not on the same different path, so that path is not crossing
			   'me'. */
			return 0;
		}
		
		/* if either neighbour is our own node, that means we could proceed that
		   way from this crossing, which means it's a >< topology. If both
		   neighbours are some other node's cvc, we are in a X crossing, going
		   either left or right woudl jump us to another portion of the polyline */
		return 1;
	}

	return 0;
}

/* Overlapping nodes na and nb are on the same integer coordinates. Figure
   if there's any overlapping lines coming out of them. Related test case:
   fixedy. */
int pa_cvc_line_line_overlap(rnd_vnode_t *na, rnd_vnode_t *nb)
{
	double aa1, aa2, ab1, ab2;
	rnd_vector_t v;

/*	rnd_trace("   ll overlap:\n");*/

	Vsub2(v, na->next->point, na->point);
	aa1 = pa_vect_to_angle_small(v);
/*	rnd_trace("    %ld;%ld ->%f\n", (long)v[0], (long)v[1], aa1);*/

	Vsub2(v, na->prev->point, na->point);
	aa2 = pa_vect_to_angle_small(v);
/*	rnd_trace("    %ld;%ld ->%f\n", (long)v[0], (long)v[1], aa2);*/

	Vsub2(v, nb->next->point, nb->point);
	ab1 = pa_vect_to_angle_small(v);
/*	rnd_trace("    %ld;%ld ->%f\n", (long)v[0], (long)v[1], ab1);*/

	Vsub2(v, nb->prev->point, nb->point);
	ab2 = pa_vect_to_angle_small(v);
/*	rnd_trace("    %ld;%ld ->%f\n", (long)v[0], (long)v[1], ab2);*/

	return (aa1 == ab1) || (aa1 == ab2) || (aa2 == ab1) || (aa2 == ab2);
}
