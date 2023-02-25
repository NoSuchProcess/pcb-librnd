/*
       polygon clipping functions. harry eaton implemented the algorithm
       described in "A Closed Set of Algorithms for Performing Set
       Operations on Polygonal Regions in the Plane" which the original
       code did not do. I also modified it for integer coordinates
       and faster computation. The license for this modified copy was
       switched to the GPL per term (3) of the original LGPL license.
       Copyright (C) 2006 harry eaton

       English translation of the original paper:
       https://web.archive.org/web/20160418014630/http://www.complex-a5.ru/polyboolean/downloads/polybool_eng.pdf

   based on:
       poly_Boolean: a polygon clip library
       Copyright (C) 1997  Alexey Nikitin, Michael Leonov
       (also the authors of the paper describing the actual algorithm)
       leonov@propro.iis.nsk.su

   in turn based on:
       nclip: a polygon clip library
       Copyright (C) 1993  Klamer Schutte
 
       This program is free software; you can redistribute it and/or
       modify it under the terms of the GNU General Public
       License as published by the Free Software Foundation; either
       version 2 of the License, or (at your option) any later version.
 
       This program is distributed in the hope that it will be useful,
       but WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
       General Public License for more details.
 
       You should have received a copy of the GNU General Public
       License along with this program; if not, write to the Free
       Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 
      polygon1.c
      (C) 1997 Alexey Nikitin, Michael Leonov
      (C) 1993 Klamer Schutte

      all cases where original (Klamer Schutte) code is present
      are marked
*/

#include	<assert.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<setjmp.h>
#include	<string.h>

#include <librnd/rnd_config.h>
#include <librnd/core/math_helper.h>
#include <librnd/core/heap.h>
#include <librnd/core/compat_cc.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/box.h>


#include "polyconf.h"

#include "polyarea.h"
#include "rtree.h"
#include "rtree2_compat.h"
#include "pa.h"

#include "pa_math.c"
#include "pa_vect.c"
#include "pa_debug.c"
#include "pa_pline.c"
#include "pa_polyarea.c"
#include "pa_cvc.c"

/***************************************************************/
/* routines for processing intersections */

/*
node_label
 (C) 2006 harry eaton
*/
static unsigned int node_label(rnd_vnode_t * pn)
{
	rnd_cvc_list_t *first_l, *l;
	char this_poly;
	int region = UNKNWN;

	assert(pn);
	assert(pn->cvc_next);
	this_poly = pn->cvc_next->poly;
	/* search counter-clockwise in the cross vertex connectivity (CVC) list
	 *
	 * check for shared edges (that could be prev or next in the list since the angles are equal)
	 * and check if this edge (pn -> pn->next) is found between the other poly's entry and exit
	 */
	if (pn->cvc_next->angle == pn->cvc_next->prev->angle)
		l = pn->cvc_next->prev;
	else
		l = pn->cvc_next->next;

	first_l = l;
	while ((l->poly == this_poly) && (l != first_l->prev))
		l = l->next;
	assert(l->poly != this_poly);

	assert(l && l->angle >= 0 && l->angle <= 4.0);
	if (l->poly != this_poly) {
		if (l->side == 'P') {
			if (l->parent->prev->point[0] == pn->next->point[0] && l->parent->prev->point[1] == pn->next->point[1]) {
				region = SHARED2;
				pn->shared = l->parent->prev;
			}
			else
				region = INSIDE;
		}
		else {
			if (l->angle == pn->cvc_next->angle) {
				assert(l->parent->next->point[0] == pn->next->point[0] && l->parent->next->point[1] == pn->next->point[1]);
				region = SHARED;
				pn->shared = l->parent;
			}
			else
				region = OUTSIDE;
		}
	}
	if (region == UNKNWN) {
		for (l = l->next; l != pn->cvc_next; l = l->next) {
			if (l->poly != this_poly) {
				if (l->side == 'P')
					region = INSIDE;
				else
					region = OUTSIDE;
				break;
			}
		}
	}
	assert(region != UNKNWN);
	assert(NODE_LABEL(pn) == UNKNWN || NODE_LABEL(pn) == region);
	LABEL_NODE(pn, region);
	if (region == SHARED || region == SHARED2)
		return UNKNWN;
	return region;
}																/* node_label */


/* some structures for handling segment intersections using the rtrees */

typedef struct seg {
	rnd_box_t box;
	rnd_vnode_t *v;
	rnd_pline_t *p;
	int intersected;
} seg;

/* used by pcb-rnd */
rnd_vnode_t *rnd_pline_seg2vnode(void *box)
{
	seg *seg = box;
	return seg->v;
}

typedef struct _insert_node_task insert_node_task;

struct _insert_node_task {
	insert_node_task *next;
	seg *node_seg;
	rnd_vnode_t *new_node;
};

typedef struct info {
	double m, b;
	rnd_rtree_t *tree;
	rnd_vnode_t *v;
	struct seg *s;
	jmp_buf *env, sego, *touch;
	int need_restart;
	insert_node_task *node_insert_list;
} info;

typedef struct contour_info {
	rnd_pline_t *pa;
	jmp_buf restart;
	jmp_buf *getout;
	int need_restart;
	insert_node_task *node_insert_list;
} contour_info;


/*
 * adjust_tree()
 * (C) 2006 harry eaton
 * This replaces the segment in the tree with the two new segments after
 * a vertex has been added
 */
static int adjust_tree(rnd_rtree_t * tree, struct seg *s)
{
	struct seg *q;

	q = (seg *) malloc(sizeof(struct seg));
	if (!q)
		return 1;
	q->intersected = 0;
	q->v = s->v;
	q->p = s->p;
	q->box.X1 = min(q->v->point[0], q->v->next->point[0]);
	q->box.X2 = max(q->v->point[0], q->v->next->point[0]) + 1;
	q->box.Y1 = min(q->v->point[1], q->v->next->point[1]);
	q->box.Y2 = max(q->v->point[1], q->v->next->point[1]) + 1;
	rnd_r_insert_entry(tree, (const rnd_box_t *) q);
	q = (seg *) malloc(sizeof(struct seg));
	if (!q)
		return 1;
	q->intersected = 0;
	q->v = s->v->next;
	q->p = s->p;
	q->box.X1 = min(q->v->point[0], q->v->next->point[0]);
	q->box.X2 = max(q->v->point[0], q->v->next->point[0]) + 1;
	q->box.Y1 = min(q->v->point[1], q->v->next->point[1]);
	q->box.Y2 = max(q->v->point[1], q->v->next->point[1]) + 1;
	rnd_r_insert_entry(tree, (const rnd_box_t *) q);
	rnd_r_delete_entry(tree, (const rnd_box_t *) s);
	free(s);
	return 0;
}

/*
 * seg_in_region()
 * (C) 2006, harry eaton
 * This prunes the search for boxes that don't intersect the segment.
 */
static rnd_r_dir_t seg_in_region(const rnd_box_t * b, void *cl)
{
	struct info *i = (struct info *) cl;
	double y1, y2;
	/* for zero slope the search is aligned on the axis so it is already pruned */
	if (i->m == 0.)
		return RND_R_DIR_FOUND_CONTINUE;
	y1 = i->m * b->X1 + i->b;
	y2 = i->m * b->X2 + i->b;
	if (min(y1, y2) >= b->Y2)
		return RND_R_DIR_NOT_FOUND;
	if (max(y1, y2) < b->Y1)
		return RND_R_DIR_NOT_FOUND;
	return RND_R_DIR_FOUND_CONTINUE;											/* might intersect */
}

/* Prepend a deferred node-insertion task to a list */
static insert_node_task *prepend_insert_node_task(insert_node_task * list, seg * seg, rnd_vnode_t * new_node)
{
	insert_node_task *task = (insert_node_task *) malloc(sizeof(*task));
	task->node_seg = seg;
	task->new_node = new_node;
	task->next = list;
	return task;
}

/*
 * seg_in_seg()
 * (C) 2006 harry eaton
 * This routine checks if the segment in the tree intersect the search segment.
 * If it does, the plines are marked as intersected and the point is marked for
 * the cvclist. If the point is not already a vertex, a new vertex is inserted
 * and the search for intersections starts over at the beginning.
 * That is potentially a significant time penalty, but it does solve the snap rounding
 * problem. There are efficient algorithms for finding intersections with snap
 * rounding, but I don't have time to implement them right now.
 */
static rnd_r_dir_t seg_in_seg(const rnd_box_t * b, void *cl)
{
	struct info *i = (struct info *) cl;
	struct seg *s = (struct seg *) b;
	rnd_vector_t s1, s2;
	int cnt;
	rnd_vnode_t *new_node;

	/* When new nodes are added at the end of a pass due to an intersection
	 * the segments may be altered. If either segment we're looking at has
	 * already been intersected this pass, skip it until the next pass.
	 */
	if (s->intersected || i->s->intersected)
		return RND_R_DIR_NOT_FOUND;

	cnt = rnd_vect_inters2(s->v->point, s->v->next->point, i->v->point, i->v->next->point, s1, s2);
	if (!cnt)
		return RND_R_DIR_NOT_FOUND;
	if (i->touch)									/* if checking touches one find and we're done */
		longjmp(*i->touch, TOUCHES);
	i->s->p->Flags.status = ISECTED;
	s->p->Flags.status = ISECTED;
	for (; cnt; cnt--) {
		rnd_bool done_insert_on_i = rnd_false;
		new_node = node_add_single_point(i->v, cnt > 1 ? s2 : s1);
		if (new_node != NULL) {
#ifdef DEBUG_INTERSECT
			DEBUGP("new intersection on segment \"i\" at %#mD\n", cnt > 1 ? s2[0] : s1[0], cnt > 1 ? s2[1] : s1[1]);
#endif
			i->node_insert_list = prepend_insert_node_task(i->node_insert_list, i->s, new_node);
			i->s->intersected = 1;
			done_insert_on_i = rnd_true;
		}
		new_node = node_add_single_point(s->v, cnt > 1 ? s2 : s1);
		if (new_node != NULL) {
#ifdef DEBUG_INTERSECT
			DEBUGP("new intersection on segment \"s\" at %#mD\n", cnt > 1 ? s2[0] : s1[0], cnt > 1 ? s2[1] : s1[1]);
#endif
			i->node_insert_list = prepend_insert_node_task(i->node_insert_list, s, new_node);
			s->intersected = 1;
			return RND_R_DIR_NOT_FOUND;									/* Keep looking for intersections with segment "i" */
		}
		/* Skip any remaining r_search hits against segment i, as any further
		 * intersections will be rejected until the next pass anyway.
		 */
		if (done_insert_on_i)
			longjmp(*i->env, 1);
	}
	return RND_R_DIR_NOT_FOUND;
}

void *rnd_poly_make_edge_tree(rnd_pline_t *pb)
{
	struct seg *s;
	rnd_vnode_t *bv;
	rnd_rtree_t *ans = rnd_r_create_tree();
	bv = pb->head;
	do {
		s = (seg *) malloc(sizeof(struct seg));
		s->intersected = 0;
		if (bv->point[0] < bv->next->point[0]) {
			s->box.X1 = bv->point[0];
			s->box.X2 = bv->next->point[0] + 1;
		}
		else {
			s->box.X2 = bv->point[0] + 1;
			s->box.X1 = bv->next->point[0];
		}
		if (bv->point[1] < bv->next->point[1]) {
			s->box.Y1 = bv->point[1];
			s->box.Y2 = bv->next->point[1] + 1;
		}
		else {
			s->box.Y2 = bv->point[1] + 1;
			s->box.Y1 = bv->next->point[1];
		}
		s->v = bv;
		s->p = pb;
		rnd_r_insert_entry(ans, (const rnd_box_t *) s);
	}
	while ((bv = bv->next) != pb->head);
	return (void *) ans;
}

static rnd_r_dir_t get_seg(const rnd_box_t * b, void *cl)
{
	struct info *i = (struct info *) cl;
	struct seg *s = (struct seg *) b;
	if (i->v == s->v) {
		i->s = s;
		return RND_R_DIR_CANCEL; /* found */
	}
	return RND_R_DIR_NOT_FOUND;
}

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
	rnd_cvc_list_t *the_list = NULL;

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
			if (curcB->Flags.status == ISECTED) {
				the_list = add_descriptors(curcB, 'B', the_list);
				if (RND_UNLIKELY(the_list == NULL))
					error(rnd_err_no_memory);
			}
	}
	while (add && (b = b->f) != bfst);
	do {
		for (curcA = a->contours; curcA != NULL; curcA = curcA->next)
			if (curcA->Flags.status == ISECTED) {
				the_list = add_descriptors(curcA, 'A', the_list);
				if (RND_UNLIKELY(the_list == NULL))
					error(rnd_err_no_memory);
			}
	}
	while (add && (a = a->f) != afst);
}																/* M_rnd_polyarea_t_intersect */

#include "pa_lab_contour.c"
#include "pa_collect_tmp.c"
#include "pa_collect.c"

#include "pa_api_bool.c"
#include "pa_api_vertex.c"
#include "pa_api_pline.c"
#include "pa_api_polyarea.c"
#include "pa_api_check.c"
#include "pa_api_misc.c"
