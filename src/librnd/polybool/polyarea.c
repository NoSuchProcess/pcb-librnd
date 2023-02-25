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
#include <librnd/poly/rtree.h>
#include <librnd/core/math_helper.h>
#include <librnd/core/heap.h>
#include <librnd/core/compat_cc.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/poly/polyarea.h>
#include <librnd/core/box.h>
#include <librnd/poly/rtree2_compat.h>

#include "polyconf.h"

#include "pa_math.c"
#include "pa_vect.c"

/* note that a vertex v's Flags.status represents the edge defined by
 * v to v->next (i.e. the edge is forward of v)
 */
#define ISECTED 3
#define UNKNWN  0
#define INSIDE  1
#define OUTSIDE 2
#define SHARED 3
#define SHARED2 4

#define TOUCHES 99

#define NODE_LABEL(n)  ((n)->Flags.status)
#define LABEL_NODE(n,l) ((n)->Flags.status = (l))

#define error(code)  longjmp(*(e), code)

#define MemGet(ptr, type) \
  if (RND_UNLIKELY(((ptr) = (type *)malloc(sizeof(type))) == NULL))	\
    error(rnd_err_no_memory);

#undef DEBUG_LABEL
#undef DEBUG_ALL_LABELS
#undef DEBUG_JUMP
#undef DEBUG_GATHER
#undef DEBUG_ANGLE
#undef DEBUG

#ifndef NDEBUG
#include <stdarg.h>
RND_INLINE void DEBUGP(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	rnd_vfprintf(stderr, fmt, ap);
	va_end(ap);
}
#else
RND_INLINE void DEBUGP(const char *fmt, ...) { }
#endif

#ifdef DEBUG
static char *theState(rnd_vnode_t * v);

static void pline_dump(rnd_vnode_t * v)
{
	rnd_vnode_t *s, *n;

	s = v;
	do {
		n = v->next;
		rnd_fprintf(stderr, "Line [%#mS %#mS %#mS %#mS 10 10 \"%s\"]\n",
								v->point[0], v->point[1], n->point[0], n->point[1], theState(v));
	}
	while ((v = v->next) != s);
}

static void poly_dump(rnd_polyarea_t * p)
{
	rnd_polyarea_t *f = p;
	rnd_pline_t *pl;

	do {
		pl = p->contours;
		do {
			pline_dump(pl->head);
			fprintf(stderr, "NEXT rnd_pline_t\n");
		}
		while ((pl = pl->next) != NULL);
		fprintf(stderr, "NEXT POLY\n");
	}
	while ((p = p->f) != f);
}
#endif

/***************************************************************/
/* routines for processing intersections */

/*
node_add
 (C) 1993 Klamer Schutte
 (C) 1997 Alexey Nikitin, Michael Leonov
 (C) 2006 harry eaton

 returns a bit field in new_point that indicates where the
 point was.
 1 means a new node was created and inserted
 4 means the intersection was not on the dest point
*/
rnd_vnode_t *rnd_poly_node_add_single(rnd_vnode_t *dest, rnd_vector_t po)
{
	rnd_vnode_t *p;

	if (vect_equal(po, dest->point))
		return dest;
	if (vect_equal(po, dest->next->point))
		return dest->next;
	p = rnd_poly_node_create(po);
	if (p == NULL)
		return NULL;
	p->cvc_prev = p->cvc_next = NULL;
	p->Flags.status = UNKNWN;
	return p;
}																/* node_add */


/*
new_descriptor
  (C) 2006 harry eaton
*/
static rnd_cvc_list_t *new_descriptor(rnd_vnode_t * a, char poly, char side)
{
	rnd_cvc_list_t *l = (rnd_cvc_list_t *) malloc(sizeof(rnd_cvc_list_t));
	rnd_vector_t v;
	register double ang, dx, dy;

	if (!l)
		return NULL;
	l->head = NULL;
	l->parent = a;
	l->poly = poly;
	l->side = side;
	l->next = l->prev = l;
	if (side == 'P')							/* previous */
		vect_sub(v, a->prev->point, a->point);
	else													/* next */
		vect_sub(v, a->next->point, a->point);
	/* Uses slope/(slope+1) in quadrant 1 as a proxy for the angle.
	 * It still has the same monotonic sort result
	 * and is far less expensive to compute than the real angle.
	 */
	if (vect_equal(v, rnd_vect_zero)) {
		if (side == 'P') {
			if (a->prev->cvc_prev == (rnd_cvc_list_t *) - 1)
				a->prev->cvc_prev = a->prev->cvc_next = NULL;
			rnd_poly_vertex_exclude(NULL, a->prev);
			vect_sub(v, a->prev->point, a->point);
		}
		else {
			if (a->next->cvc_prev == (rnd_cvc_list_t *) - 1)
				a->next->cvc_prev = a->next->cvc_next = NULL;
			rnd_poly_vertex_exclude(NULL, a->next);
			vect_sub(v, a->next->point, a->point);
		}
	}
	assert(!vect_equal(v, rnd_vect_zero));
	dx = fabs((double) v[0]);
	dy = fabs((double) v[1]);
	ang = dy / (dy + dx);
	/* now move to the actual quadrant */
	if (v[0] < 0 && v[1] >= 0)
		ang = 2.0 - ang;						/* 2nd quadrant */
	else if (v[0] < 0 && v[1] < 0)
		ang += 2.0;									/* 3rd quadrant */
	else if (v[0] >= 0 && v[1] < 0)
		ang = 4.0 - ang;						/* 4th quadrant */
	l->angle = ang;
	assert(ang >= 0.0 && ang <= 4.0);
#ifdef DEBUG_ANGLE
	DEBUGP("node on %c at %#mD assigned angle %g on side %c\n", poly, a->point[0], a->point[1], ang, side);
#endif
	return l;
}

/*
insert_descriptor
  (C) 2006 harry eaton

   argument a is a cross-vertex node.
   argument poly is the polygon it comes from ('A' or 'B')
   argument side is the side this descriptor goes on ('P' for previous
   'N' for next.
   argument start is the head of the list of cvclists
*/
static rnd_cvc_list_t *insert_descriptor(rnd_vnode_t * a, char poly, char side, rnd_cvc_list_t * start)
{
	rnd_cvc_list_t *l, *newone, *big, *small;

	if (!(newone = new_descriptor(a, poly, side)))
		return NULL;
	/* search for the rnd_cvc_list_t for this point */
	if (!start) {
		start = newone;							/* return is also new, so we know where start is */
		start->head = newone;				/* circular list */
		return newone;
	}
	else {
		l = start;
		do {
			assert(l->head);
			if (l->parent->point[0] == a->point[0]
					&& l->parent->point[1] == a->point[1]) {	/* this rnd_cvc_list_t is at our point */
				start = l;
				newone->head = l->head;
				break;
			}
			if (l->head->parent->point[0] == start->parent->point[0]
					&& l->head->parent->point[1] == start->parent->point[1]) {
				/* this seems to be a new point */
				/* link this cvclist to the list of all cvclists */
				for (; l->head != newone; l = l->next)
					l->head = newone;
				newone->head = start;
				return newone;
			}
			l = l->head;
		}
		while (1);
	}
	assert(start);
	l = big = small = start;
	do {
		if (l->next->angle < l->angle) {	/* find start/end of list */
			small = l->next;
			big = l;
		}
		else if (newone->angle >= l->angle && newone->angle <= l->next->angle) {
			/* insert new cvc if it lies between existing points */
			newone->prev = l;
			newone->next = l->next;
			l->next = l->next->prev = newone;
			return newone;
		}
	}
	while ((l = l->next) != start);
	/* didn't find it between points, it must go on an end */
	if (big->angle <= newone->angle) {
		newone->prev = big;
		newone->next = big->next;
		big->next = big->next->prev = newone;
		return newone;
	}
	assert(small->angle >= newone->angle);
	newone->next = small;
	newone->prev = small->prev;
	small->prev = small->prev->next = newone;
	return newone;
}

/*
node_add_point
 (C) 1993 Klamer Schutte
 (C) 1997 Alexey Nikitin, Michael Leonov

 return 1 if new node in b, 2 if new node in a and 3 if new node in both
*/

static rnd_vnode_t *node_add_single_point(rnd_vnode_t * a, rnd_vector_t p)
{
	rnd_vnode_t *next_a, *new_node;

	next_a = a->next;

	new_node = rnd_poly_node_add_single(a, p);
	assert(new_node != NULL);

	new_node->cvc_prev = new_node->cvc_next = (rnd_cvc_list_t *) - 1;

	if (new_node == a || new_node == next_a)
		return NULL;

	return new_node;
}																/* node_add_point */

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

/*
 add_descriptors
 (C) 2006 harry eaton
*/
static rnd_cvc_list_t *add_descriptors(rnd_pline_t * pl, char poly, rnd_cvc_list_t * list)
{
	rnd_vnode_t *node = pl->head;

	do {
		if (node->cvc_prev) {
			assert(node->cvc_prev == (rnd_cvc_list_t *) - 1 && node->cvc_next == (rnd_cvc_list_t *) - 1);
			list = node->cvc_prev = insert_descriptor(node, poly, 'P', list);
			if (!node->cvc_prev)
				return NULL;
			list = node->cvc_next = insert_descriptor(node, poly, 'N', list);
			if (!node->cvc_next)
				return NULL;
		}
	}
	while ((node = node->next) != pl->head);
	return list;
}

static inline void cntrbox_adjust(rnd_pline_t * c, const rnd_vector_t p)
{
	c->xmin = min(c->xmin, p[0]);
	c->xmax = max(c->xmax, p[0] + 1);
	c->ymin = min(c->ymin, p[1]);
	c->ymax = max(c->ymax, p[1] + 1);
}

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

static inline int cntrbox_inside(rnd_pline_t * c1, rnd_pline_t * c2)
{
	assert(c1 != NULL && c2 != NULL);
	return ((c1->xmin >= c2->xmin) && (c1->ymin >= c2->ymin) && (c1->xmax <= c2->xmax) && (c1->ymax <= c2->ymax));
}

/*****************************************************************/
/* Routines for making labels */

static rnd_r_dir_t count_contours_i_am_inside(const rnd_box_t * b, void *cl)
{
	rnd_pline_t *me = (rnd_pline_t *) cl;
	rnd_pline_t *check = (rnd_pline_t *) b;

	if (rnd_poly_contour_in_contour(check, me))
		return RND_R_DIR_FOUND_CONTINUE;
	return RND_R_DIR_NOT_FOUND;
}

/* cntr_in_M_rnd_polyarea_t
returns poly is inside outfst ? rnd_true : rnd_false */
static int cntr_in_M_rnd_polyarea_t(rnd_pline_t * poly, rnd_polyarea_t * outfst, rnd_bool test)
{
	rnd_polyarea_t *outer = outfst;
	rnd_heap_t *heap;

	assert(poly != NULL);
	assert(outer != NULL);

	heap = rnd_heap_create();
	do {
		if (cntrbox_inside(poly, outer->contours))
			rnd_heap_insert(heap, outer->contours->area, (void *) outer);
	}
	/* if checking touching, use only the first polygon */
	while (!test && (outer = outer->f) != outfst);
	/* we need only check the smallest poly container
	 * but we must loop in case the box container is not
	 * the poly container */
	do {
		int cnt;

		if (rnd_heap_is_empty(heap))
			break;
		outer = (rnd_polyarea_t *) rnd_heap_remove_smallest(heap);

		rnd_r_search(outer->contour_tree, (rnd_box_t *) poly, NULL, count_contours_i_am_inside, poly, &cnt);
		switch (cnt) {
		case 0:										/* Didn't find anything in this piece, Keep looking */
			break;
		case 1:										/* Found we are inside this piece, and not any of its holes */
			rnd_heap_destroy(&heap);
			return rnd_true;
		case 2:										/* Found inside a hole in the smallest polygon so far. No need to check the other polygons */
			rnd_heap_destroy(&heap);
			return rnd_false;
		default:
			printf("Something strange here\n");
			break;
		}
	}
	while (1);
	rnd_heap_destroy(&heap);
	return rnd_false;
}																/* cntr_in_M_rnd_polyarea_t */

#ifdef DEBUG

static char *theState(rnd_vnode_t * v)
{
	static char u[] = "UNKNOWN";
	static char i[] = "INSIDE";
	static char o[] = "OUTSIDE";
	static char s[] = "SHARED";
	static char s2[] = "SHARED2";

	switch (NODE_LABEL(v)) {
	case INSIDE:
		return i;
	case OUTSIDE:
		return o;
	case SHARED:
		return s;
	case SHARED2:
		return s2;
	default:
		return u;
	}
}

#ifdef DEBUG_ALL_LABELS
static void print_labels(rnd_pline_t * a)
{
	rnd_vnode_t *c = a->head;

	do {
		DEBUGP("%#mD->%#mD labeled %s\n", c->point[0], c->point[1], c->next->point[0], c->next->point[1], theState(c));
	}
	while ((c = c->next) != a->head);
}
#endif
#endif

/*
label_contour
 (C) 2006 harry eaton
 (C) 1993 Klamer Schutte
 (C) 1997 Alexey Nikitin, Michael Leonov
*/

static rnd_bool label_contour(rnd_pline_t * a)
{
	rnd_vnode_t *cur = a->head;
	rnd_vnode_t *first_labelled = NULL;
	int label = UNKNWN;

	do {
		if (cur->cvc_next) {				/* examine cross vertex */
			label = node_label(cur);
			if (first_labelled == NULL)
				first_labelled = cur;
			continue;
		}

		if (first_labelled == NULL)
			continue;

		/* This labels nodes which aren't cross-connected */
		assert(label == INSIDE || label == OUTSIDE);
		LABEL_NODE(cur, label);
	}
	while ((cur = cur->next) != first_labelled);
#ifdef DEBUG_ALL_LABELS
	print_labels(a);
	DEBUGP("\n\n");
#endif
	return rnd_false;
}																/* label_contour */

static rnd_bool cntr_label_rnd_polyarea_t(rnd_pline_t * poly, rnd_polyarea_t * ppl, rnd_bool test)
{
	assert(ppl != NULL && ppl->contours != NULL);
	if (poly->Flags.status == ISECTED) {
		label_contour(poly);				/* should never get here when rnd_bool is rnd_true */
	}
	else if (cntr_in_M_rnd_polyarea_t(poly, ppl, test)) {
		if (test)
			return rnd_true;
		poly->Flags.status = INSIDE;
	}
	else {
		if (test)
			return rnd_false;
		poly->Flags.status = OUTSIDE;
	}
	return rnd_false;
}																/* cntr_label_rnd_polyarea_t */

static rnd_bool M_rnd_polyarea_t_label_separated(rnd_pline_t * afst, rnd_polyarea_t * b, rnd_bool touch)
{
	rnd_pline_t *curc = afst;

	for (curc = afst; curc != NULL; curc = curc->next) {
		if (cntr_label_rnd_polyarea_t(curc, b, touch) && touch)
			return rnd_true;
	}
	return rnd_false;
}

static rnd_bool M_rnd_polyarea_t_label(rnd_polyarea_t * afst, rnd_polyarea_t * b, rnd_bool touch)
{
	rnd_polyarea_t *a = afst;
	rnd_pline_t *curc;

	assert(a != NULL);
	do {
		for (curc = a->contours; curc != NULL; curc = curc->next)
			if (cntr_label_rnd_polyarea_t(curc, b, touch)) {
				if (touch)
					return rnd_true;
			}
	}
	while (!touch && (a = a->f) != afst);
	return rnd_false;
}

/****************************************************************/

/* routines for temporary storing resulting contours */
static void InsCntr(jmp_buf * e, rnd_pline_t * c, rnd_polyarea_t ** dst)
{
	rnd_polyarea_t *newp;

	if (*dst == NULL) {
		MemGet(*dst, rnd_polyarea_t);
		(*dst)->f = (*dst)->b = *dst;
		newp = *dst;
	}
	else {
		MemGet(newp, rnd_polyarea_t);
		newp->f = *dst;
		newp->b = (*dst)->b;
		newp->f->b = newp->b->f = newp;
	}
	newp->contours = c;
	newp->contour_tree = rnd_r_create_tree();
	rnd_r_insert_entry(newp->contour_tree, (rnd_box_t *) c);
	c->next = NULL;
}																/* InsCntr */

static void
PutContour(jmp_buf * e, rnd_pline_t * cntr, rnd_polyarea_t ** contours, rnd_pline_t ** holes,
					 rnd_polyarea_t * owner, rnd_polyarea_t * parent, rnd_pline_t * parent_contour)
{
	assert(cntr != NULL);
	assert(cntr->Count > 2);
	cntr->next = NULL;

	if (cntr->Flags.orient == RND_PLF_DIR) {
		if (owner != NULL)
			rnd_r_delete_entry(owner->contour_tree, (rnd_box_t *) cntr);
		InsCntr(e, cntr, contours);
	}
	/* put hole into temporary list */
	else {
		/* if we know this belongs inside the parent, put it there now */
		if (parent_contour) {
			cntr->next = parent_contour->next;
			parent_contour->next = cntr;
			if (owner != parent) {
				if (owner != NULL)
					rnd_r_delete_entry(owner->contour_tree, (rnd_box_t *) cntr);
				rnd_r_insert_entry(parent->contour_tree, (rnd_box_t *) cntr);
			}
		}
		else {
			cntr->next = *holes;
			*holes = cntr;						/* let cntr be 1st hole in list */
			/* We don't insert the holes into an r-tree,
			 * they just form a linked list */
			if (owner != NULL)
				rnd_r_delete_entry(owner->contour_tree, (rnd_box_t *) cntr);
		}
	}
}																/* PutContour */

static inline void remove_contour(rnd_polyarea_t * piece, rnd_pline_t * prev_contour, rnd_pline_t * contour, int remove_rtree_entry)
{
	if (piece->contours == contour)
		piece->contours = contour->next;
	else if (prev_contour != NULL) {
		assert(prev_contour->next == contour);
		prev_contour->next = contour->next;
	}

	contour->next = NULL;

	if (remove_rtree_entry)
		rnd_r_delete_entry(piece->contour_tree, (rnd_box_t *) contour);
}

struct polyarea_info {
	rnd_box_t BoundingBox;
	rnd_polyarea_t *pa;
};

static rnd_r_dir_t heap_it(const rnd_box_t * b, void *cl)
{
	rnd_heap_t *heap = (rnd_heap_t *) cl;
	struct polyarea_info *pa_info = (struct polyarea_info *) b;
	rnd_pline_t *p = pa_info->pa->contours;
	if (p->Count == 0)
		return RND_R_DIR_NOT_FOUND;										/* how did this happen? */
	rnd_heap_insert(heap, p->area, pa_info);
	return RND_R_DIR_FOUND_CONTINUE;
}

struct find_inside_info {
	jmp_buf jb;
	rnd_pline_t *want_inside;
	rnd_pline_t *result;
};

static rnd_r_dir_t find_inside(const rnd_box_t * b, void *cl)
{
	struct find_inside_info *info = (struct find_inside_info *) cl;
	rnd_pline_t *check = (rnd_pline_t *) b;
	/* Do test on check to see if it inside info->want_inside */
	/* If it is: */
	if (check->Flags.orient == RND_PLF_DIR) {
		return RND_R_DIR_NOT_FOUND;
	}
	if (rnd_poly_contour_in_contour(info->want_inside, check)) {
		info->result = check;
		longjmp(info->jb, 1);
	}
	return RND_R_DIR_NOT_FOUND;
}

void rnd_poly_insert_holes(jmp_buf * e, rnd_polyarea_t * dest, rnd_pline_t ** src)
{
	rnd_polyarea_t *curc;
	rnd_pline_t *curh, *container;
	rnd_heap_t *heap;
	rnd_rtree_t *tree;
	int i;
	int num_polyareas = 0;
	struct polyarea_info *all_pa_info, *pa_info;

	if (*src == NULL)
		return;											/* empty hole list */
	if (dest == NULL)
		error(rnd_err_bad_parm);				/* empty contour list */

	/* Count dest polyareas */
	curc = dest;
	do {
		num_polyareas++;
	}
	while ((curc = curc->f) != dest);

	/* make a polyarea info table */
	/* make an rtree of polyarea info table */
	all_pa_info = (struct polyarea_info *) malloc(sizeof(struct polyarea_info) * num_polyareas);
	tree = rnd_r_create_tree();
	i = 0;
	curc = dest;
	do {
		all_pa_info[i].BoundingBox.X1 = curc->contours->xmin;
		all_pa_info[i].BoundingBox.Y1 = curc->contours->ymin;
		all_pa_info[i].BoundingBox.X2 = curc->contours->xmax;
		all_pa_info[i].BoundingBox.Y2 = curc->contours->ymax;
		all_pa_info[i].pa = curc;
		rnd_r_insert_entry(tree, (const rnd_box_t *) &all_pa_info[i]);
		i++;
	}
	while ((curc = curc->f) != dest);

	/* loop through the holes and put them where they belong */
	while ((curh = *src) != NULL) {
		*src = curh->next;

		container = NULL;
		/* build a heap of all of the polys that the hole is inside its bounding box */
		heap = rnd_heap_create();
		rnd_r_search(tree, (rnd_box_t *) curh, NULL, heap_it, heap, NULL);
		if (rnd_heap_is_empty(heap)) {
#ifndef NDEBUG
#ifdef DEBUG
			poly_dump(dest);
#endif
#endif
			rnd_poly_contour_del(&curh);
			error(rnd_err_bad_parm);
		}
		/* Now search the heap for the container. If there was only one item
		 * in the heap, assume it is the container without the expense of
		 * proving it.
		 */
		pa_info = (struct polyarea_info *) rnd_heap_remove_smallest(heap);
		if (rnd_heap_is_empty(heap)) {	/* only one possibility it must be the right one */
			assert(rnd_poly_contour_in_contour(pa_info->pa->contours, curh));
			container = pa_info->pa->contours;
		}
		else {
			do {
				if (rnd_poly_contour_in_contour(pa_info->pa->contours, curh)) {
					container = pa_info->pa->contours;
					break;
				}
				if (rnd_heap_is_empty(heap))
					break;
				pa_info = (struct polyarea_info *) rnd_heap_remove_smallest(heap);
			}
			while (1);
		}
		rnd_heap_destroy(&heap);
		if (container == NULL) {
			/* bad input polygons were given */
#ifndef NDEBUG
#ifdef DEBUG
			poly_dump(dest);
#endif
#endif
			curh->next = NULL;
			rnd_poly_contour_del(&curh);
			error(rnd_err_bad_parm);
		}
		else {
			/* Need to check if this new hole means we need to kick out any old ones for reprocessing */
			while (1) {
				struct find_inside_info info;
				rnd_pline_t *prev;

				info.want_inside = curh;

				/* Set jump return */
				if (setjmp(info.jb)) {
					/* Returned here! */
				}
				else {
					info.result = NULL;
					/* Rtree search, calling back a routine to longjmp back with data about any hole inside the added one */
					/*   Be sure not to bother jumping back to report the main contour! */
					rnd_r_search(pa_info->pa->contour_tree, (rnd_box_t *) curh, NULL, find_inside, &info, NULL);

					/* Nothing found? */
					break;
				}

				/* We need to find the contour before it, so we can update its next pointer */
				prev = container;
				while (prev->next != info.result) {
					prev = prev->next;
				}

				/* Remove hole from the contour */
				remove_contour(pa_info->pa, prev, info.result, rnd_true);

				/* Add hole as the next on the list to be processed in this very function */
				info.result->next = *src;
				*src = info.result;
			}
			/* End check for kicked out holes */

			/* link at front of hole list */
			curh->next = container->next;
			container->next = curh;
			rnd_r_insert_entry(pa_info->pa->contour_tree, (rnd_box_t *) curh);

		}
	}
	rnd_r_destroy_tree(&tree);
	free(all_pa_info);
}																/* rnd_poly_insert_holes */


/****************************************************************/
/* routines for collecting result */

typedef enum {
	FORW, BACKW
} DIRECTION;

/* Start Rule */
typedef int (*S_Rule) (rnd_vnode_t *, DIRECTION *);

/* Jump Rule  */
typedef int (*J_Rule) (char, rnd_vnode_t *, DIRECTION *);

static int UniteS_Rule(rnd_vnode_t * cur, DIRECTION * initdir)
{
	*initdir = FORW;
	return (NODE_LABEL(cur) == OUTSIDE) || (NODE_LABEL(cur) == SHARED);
}

static int IsectS_Rule(rnd_vnode_t * cur, DIRECTION * initdir)
{
	*initdir = FORW;
	return (NODE_LABEL(cur) == INSIDE) || (NODE_LABEL(cur) == SHARED);
}

static int SubS_Rule(rnd_vnode_t * cur, DIRECTION * initdir)
{
	*initdir = FORW;
	return (NODE_LABEL(cur) == OUTSIDE) || (NODE_LABEL(cur) == SHARED2);
}

static int XorS_Rule(rnd_vnode_t * cur, DIRECTION * initdir)
{
	if (cur->Flags.status == INSIDE) {
		*initdir = BACKW;
		return rnd_true;
	}
	if (cur->Flags.status == OUTSIDE) {
		*initdir = FORW;
		return rnd_true;
	}
	return rnd_false;
}

static int IsectJ_Rule(char p, rnd_vnode_t * v, DIRECTION * cdir)
{
	assert(*cdir == FORW);
	return (v->Flags.status == INSIDE || v->Flags.status == SHARED);
}

static int UniteJ_Rule(char p, rnd_vnode_t * v, DIRECTION * cdir)
{
	assert(*cdir == FORW);
	return (v->Flags.status == OUTSIDE || v->Flags.status == SHARED);
}

static int XorJ_Rule(char p, rnd_vnode_t * v, DIRECTION * cdir)
{
	if (v->Flags.status == INSIDE) {
		*cdir = BACKW;
		return rnd_true;
	}
	if (v->Flags.status == OUTSIDE) {
		*cdir = FORW;
		return rnd_true;
	}
	return rnd_false;
}

static int SubJ_Rule(char p, rnd_vnode_t * v, DIRECTION * cdir)
{
	if (p == 'B' && v->Flags.status == INSIDE) {
		*cdir = BACKW;
		return rnd_true;
	}
	if (p == 'A' && v->Flags.status == OUTSIDE) {
		*cdir = FORW;
		return rnd_true;
	}
	if (v->Flags.status == SHARED2) {
		if (p == 'A')
			*cdir = FORW;
		else
			*cdir = BACKW;
		return rnd_true;
	}
	return rnd_false;
}

/* return the edge that comes next.
 * if the direction is BACKW, then we return the next vertex
 * so that prev vertex has the flags for the edge
 *
 * returns rnd_true if an edge is found, rnd_false otherwise
 */
static int jump(rnd_vnode_t ** cur, DIRECTION * cdir, J_Rule rule)
{
	rnd_cvc_list_t *d, *start;
	rnd_vnode_t *e;
	DIRECTION newone;

	if (!(*cur)->cvc_prev) {			/* not a cross-vertex */
		if (*cdir == FORW ? (*cur)->Flags.mark : (*cur)->prev->Flags.mark)
			return rnd_false;
		return rnd_true;
	}
#ifdef DEBUG_JUMP
	DEBUGP("jump entering node at %$mD\n", (*cur)->point[0], (*cur)->point[1]);
#endif
	if (*cdir == FORW)
		d = (*cur)->cvc_prev->prev;
	else
		d = (*cur)->cvc_next->prev;
	start = d;
	do {
		e = d->parent;
		if (d->side == 'P')
			e = e->prev;
		newone = *cdir;
		if (!e->Flags.mark && rule(d->poly, e, &newone)) {
			if ((d->side == 'N' && newone == FORW) || (d->side == 'P' && newone == BACKW)) {
#ifdef DEBUG_JUMP
				if (newone == FORW)
					DEBUGP("jump leaving node at %#mD\n", e->next->point[0], e->next->point[1]);
				else
					DEBUGP("jump leaving node at %#mD\n", e->point[0], e->point[1]);
#endif
				*cur = d->parent;
				*cdir = newone;
				return rnd_true;
			}
		}
	}
	while ((d = d->prev) != start);
	return rnd_false;
}

static int Gather(rnd_vnode_t * start, rnd_pline_t ** result, J_Rule v_rule, DIRECTION initdir)
{
	rnd_vnode_t *cur = start, *newn;
	DIRECTION dir = initdir;
#ifdef DEBUG_GATHER
	DEBUGP("gather direction = %d\n", dir);
#endif
	assert(*result == NULL);
	do {
		/* see where to go next */
		if (!jump(&cur, &dir, v_rule))
			break;
		/* add edge to polygon */
		if (!*result) {
			*result = rnd_poly_contour_new(cur->point);
			if (*result == NULL)
				return rnd_err_no_memory;
		}
		else {
			if ((newn = rnd_poly_node_create(cur->point)) == NULL)
				return rnd_err_no_memory;
			rnd_poly_vertex_include((*result)->head->prev, newn);
		}
#ifdef DEBUG_GATHER
		DEBUGP("gather vertex at %#mD\n", cur->point[0], cur->point[1]);
#endif
		/* Now mark the edge as included.  */
		newn = (dir == FORW ? cur : cur->prev);
		newn->Flags.mark = 1;
		/* for SHARED edge mark both */
		if (newn->shared)
			newn->shared->Flags.mark = 1;

		/* Advance to the next edge.  */
		cur = (dir == FORW ? cur->next : cur->prev);
	}
	while (1);
	return rnd_err_ok;
}																/* Gather */

static void Collect1(jmp_buf * e, rnd_vnode_t * cur, DIRECTION dir, rnd_polyarea_t ** contours, rnd_pline_t ** holes, J_Rule j_rule)
{
	rnd_pline_t *p = NULL;							/* start making contour */
	int errc = rnd_err_ok;
	if ((errc = Gather(dir == FORW ? cur : cur->next, &p, j_rule, dir)) != rnd_err_ok) {
		if (p != NULL)
			rnd_poly_contour_del(&p);
		error(errc);
	}
	if (!p)
		return;
	rnd_poly_contour_pre(p, rnd_true);
	if (p->Count > 2) {
#ifdef DEBUG_GATHER
		DEBUGP("adding contour with %d vertices and direction %c\n", p->Count, p->Flags.orient ? 'F' : 'B');
#endif
		PutContour(e, p, contours, holes, NULL, NULL, NULL);
	}
	else {
		/* some sort of computation error ? */
#ifdef DEBUG_GATHER
		DEBUGP("Bad contour! Not enough points!!\n");
#endif
		rnd_poly_contour_del(&p);
	}
}

static void Collect(jmp_buf * e, rnd_pline_t * a, rnd_polyarea_t ** contours, rnd_pline_t ** holes, S_Rule s_rule, J_Rule j_rule)
{
	rnd_vnode_t *cur, *other;
	DIRECTION dir;

	cur = a->head;
	do {
		if (s_rule(cur, &dir) && cur->Flags.mark == 0)
			Collect1(e, cur, dir, contours, holes, j_rule);
		other = cur;
		if ((other->cvc_prev && jump(&other, &dir, j_rule)))
			Collect1(e, other, dir, contours, holes, j_rule);
	}
	while ((cur = cur->next) != a->head);
}																/* Collect */


static int
cntr_Collect(jmp_buf * e, rnd_pline_t ** A, rnd_polyarea_t ** contours, rnd_pline_t ** holes,
						 int action, rnd_polyarea_t * owner, rnd_polyarea_t * parent, rnd_pline_t * parent_contour)
{
	rnd_pline_t *tmprev;

	if ((*A)->Flags.status == ISECTED) {
		switch (action) {
		case RND_PBO_UNITE:
			Collect(e, *A, contours, holes, UniteS_Rule, UniteJ_Rule);
			break;
		case RND_PBO_ISECT:
			Collect(e, *A, contours, holes, IsectS_Rule, IsectJ_Rule);
			break;
		case RND_PBO_XOR:
			Collect(e, *A, contours, holes, XorS_Rule, XorJ_Rule);
			break;
		case RND_PBO_SUB:
			Collect(e, *A, contours, holes, SubS_Rule, SubJ_Rule);
			break;
		};
	}
	else {
		switch (action) {
		case RND_PBO_ISECT:
			if ((*A)->Flags.status == INSIDE) {
				tmprev = *A;
				/* disappear this contour (rtree entry removed in PutContour) */
				*A = tmprev->next;
				tmprev->next = NULL;
				PutContour(e, tmprev, contours, holes, owner, NULL, NULL);
				return rnd_true;
			}
			break;
		case RND_PBO_XOR:
			if ((*A)->Flags.status == INSIDE) {
				tmprev = *A;
				/* disappear this contour (rtree entry removed in PutContour) */
				*A = tmprev->next;
				tmprev->next = NULL;
				rnd_poly_contour_inv(tmprev);
				PutContour(e, tmprev, contours, holes, owner, NULL, NULL);
				return rnd_true;
			}
			/* break; *//* Fall through (I think this is correct! pcjc2) */
		case RND_PBO_UNITE:
		case RND_PBO_SUB:
			if ((*A)->Flags.status == OUTSIDE) {
				tmprev = *A;
				/* disappear this contour (rtree entry removed in PutContour) */
				*A = tmprev->next;
				tmprev->next = NULL;
				PutContour(e, tmprev, contours, holes, owner, parent, parent_contour);
				return rnd_true;
			}
			break;
		}
	}
	return rnd_false;
}																/* cntr_Collect */

static void M_B_AREA_Collect(jmp_buf * e, rnd_polyarea_t * bfst, rnd_polyarea_t ** contours, rnd_pline_t ** holes, int action)
{
	rnd_polyarea_t *b = bfst;
	rnd_pline_t **cur, **next, *tmp;

	assert(b != NULL);
	do {
		for (cur = &b->contours; *cur != NULL; cur = next) {
			next = &((*cur)->next);
			if ((*cur)->Flags.status == ISECTED)
				continue;

			if ((*cur)->Flags.status == INSIDE)
				switch (action) {
				case RND_PBO_XOR:
				case RND_PBO_SUB:
					rnd_poly_contour_inv(*cur);
				case RND_PBO_ISECT:
					tmp = *cur;
					*cur = tmp->next;
					next = cur;
					tmp->next = NULL;
					tmp->Flags.status = UNKNWN;
					PutContour(e, tmp, contours, holes, b, NULL, NULL);
					break;
				case RND_PBO_UNITE:
					break;								/* nothing to do - already included */
				}
			else if ((*cur)->Flags.status == OUTSIDE)
				switch (action) {
				case RND_PBO_XOR:
				case RND_PBO_UNITE:
					/* include */
					tmp = *cur;
					*cur = tmp->next;
					next = cur;
					tmp->next = NULL;
					tmp->Flags.status = UNKNWN;
					PutContour(e, tmp, contours, holes, b, NULL, NULL);
					break;
				case RND_PBO_ISECT:
				case RND_PBO_SUB:
					break;								/* do nothing, not included */
				}
		}
	}
	while ((b = b->f) != bfst);
}


static inline int contour_is_first(rnd_polyarea_t * a, rnd_pline_t * cur)
{
	return (a->contours == cur);
}


static inline int contour_is_last(rnd_pline_t * cur)
{
	return (cur->next == NULL);
}


static inline void remove_polyarea(rnd_polyarea_t ** list, rnd_polyarea_t * piece)
{
	/* If this item was the start of the list, advance that pointer */
	if (*list == piece)
		*list = (*list)->f;

	/* But reset it to NULL if it wraps around and hits us again */
	if (*list == piece)
		*list = NULL;

	piece->b->f = piece->f;
	piece->f->b = piece->b;
	piece->f = piece->b = piece;
}

static void M_rnd_polyarea_separate_isected(jmp_buf * e, rnd_polyarea_t ** pieces, rnd_pline_t ** holes, rnd_pline_t ** isected)
{
	rnd_polyarea_t *a = *pieces;
	rnd_polyarea_t *anext;
	rnd_pline_t *curc, *next, *prev;
	int finished;

	if (a == NULL)
		return;

	/* TODO: STASH ENOUGH INFORMATION EARLIER ON, SO WE CAN REMOVE THE INTERSECTED
	   CONTOURS WITHOUT HAVING TO WALK THE FULL DATA-STRUCTURE LOOKING FOR THEM. */

	do {
		int hole_contour = 0;
		int is_outline = 1;

		anext = a->f;
		finished = (anext == *pieces);

		prev = NULL;
		for (curc = a->contours; curc != NULL; curc = next, is_outline = 0) {
			int is_first = contour_is_first(a, curc);
			int is_last = contour_is_last(curc);
			int isect_contour = (curc->Flags.status == ISECTED);

			next = curc->next;

			if (isect_contour || hole_contour) {

				/* Reset the intersection flags, since we keep these pieces */
				if (curc->Flags.status != ISECTED)
					curc->Flags.status = UNKNWN;

				remove_contour(a, prev, curc, !(is_first && is_last));

				if (isect_contour) {
					/* Link into the list of intersected contours */
					curc->next = *isected;
					*isected = curc;
				}
				else if (hole_contour) {
					/* Link into the list of holes */
					curc->next = *holes;
					*holes = curc;
				}
				else {
					assert(0);
				}

				if (is_first && is_last) {
					remove_polyarea(pieces, a);
					rnd_polyarea_free(&a);				/* NB: Sets a to NULL */
				}

			}
			else {
				/* Note the item we just didn't delete as the next
				   candidate for having its "next" pointer adjusted.
				   Saves walking the contour list when we delete one. */
				prev = curc;
			}

			/* If we move or delete an outer contour, we need to move any holes
			   we wish to keep within that contour to the holes list. */
			if (is_outline && isect_contour)
				hole_contour = 1;

		}

		/* If we deleted all the pieces of the polyarea, *pieces is NULL */
	}
	while ((a = anext), *pieces != NULL && !finished);
}


struct find_inside_m_pa_info {
	jmp_buf jb;
	rnd_polyarea_t *want_inside;
	rnd_pline_t *result;
};

static rnd_r_dir_t find_inside_m_pa(const rnd_box_t * b, void *cl)
{
	struct find_inside_m_pa_info *info = (struct find_inside_m_pa_info *) cl;
	rnd_pline_t *check = (rnd_pline_t *) b;
	/* Don't report for the main contour */
	if (check->Flags.orient == RND_PLF_DIR)
		return RND_R_DIR_NOT_FOUND;
	/* Don't look at contours marked as being intersected */
	if (check->Flags.status == ISECTED)
		return RND_R_DIR_NOT_FOUND;
	if (cntr_in_M_rnd_polyarea_t(check, info->want_inside, rnd_false)) {
		info->result = check;
		longjmp(info->jb, 1);
	}
	return RND_R_DIR_NOT_FOUND;
}


static void M_rnd_polyarea_t_update_primary(jmp_buf * e, rnd_polyarea_t ** pieces, rnd_pline_t ** holes, int action, rnd_polyarea_t * bpa)
{
	rnd_polyarea_t *a = *pieces;
	rnd_polyarea_t *b;
	rnd_polyarea_t *anext;
	rnd_pline_t *curc, *next, *prev;
	rnd_box_t box;
	/* int inv_inside = 0; */
	int del_inside = 0;
	int del_outside = 0;
	int finished;

	if (a == NULL)
		return;

	switch (action) {
	case RND_PBO_ISECT:
		del_outside = 1;
		break;
	case RND_PBO_UNITE:
	case RND_PBO_SUB:
		del_inside = 1;
		break;
	case RND_PBO_XOR:								/* NOT IMPLEMENTED OR USED */
		/* inv_inside = 1; */
		assert(0);
		break;
	}

	box = *((rnd_box_t *) bpa->contours);
	b = bpa;
	while ((b = b->f) != bpa) {
		rnd_box_t *b_box = (rnd_box_t *) b->contours;
		RND_MAKE_MIN(box.X1, b_box->X1);
		RND_MAKE_MIN(box.Y1, b_box->Y1);
		RND_MAKE_MAX(box.X2, b_box->X2);
		RND_MAKE_MAX(box.Y2, b_box->Y2);
	}

	if (del_inside) {

		do {
			anext = a->f;
			finished = (anext == *pieces);

			/* Test the outer contour first, as we may need to remove all children */

			/* We've not yet split intersected contours out, just ignore them */
			if (a->contours->Flags.status != ISECTED &&
					/* Pre-filter on bounding box */
					((a->contours->xmin >= box.X1) && (a->contours->ymin >= box.Y1)
					 && (a->contours->xmax <= box.X2)
					 && (a->contours->ymax <= box.Y2)) &&
					/* Then test properly */
					cntr_in_M_rnd_polyarea_t(a->contours, bpa, rnd_false)) {

				/* Delete this contour, all children -> holes queue */

				/* Delete the outer contour */
				curc = a->contours;
				remove_contour(a, NULL, curc, rnd_false);	/* Rtree deleted in poly_Free below */
				/* a->contours now points to the remaining holes */
				rnd_poly_contour_del(&curc);

				if (a->contours != NULL) {
					/* Find the end of the list of holes */
					curc = a->contours;
					while (curc->next != NULL)
						curc = curc->next;

					/* Take the holes and prepend to the holes queue */
					curc->next = *holes;
					*holes = a->contours;
					a->contours = NULL;
				}

				remove_polyarea(pieces, a);
				rnd_polyarea_free(&a);					/* NB: Sets a to NULL */

				continue;
			}

			/* Loop whilst we find INSIDE contours to delete */
			while (1) {
				struct find_inside_m_pa_info info;
				rnd_pline_t *prev;

				info.want_inside = bpa;

				/* Set jump return */
				if (setjmp(info.jb)) {
					/* Returned here! */
				}
				else {
					info.result = NULL;
					/* r-tree search, calling back a routine to longjmp back with
					 * data about any hole inside the B polygon.
					 * NB: Does not jump back to report the main contour!
					 */
					rnd_r_search(a->contour_tree, &box, NULL, find_inside_m_pa, &info, NULL);

					/* Nothing found? */
					break;
				}

				/* We need to find the contour before it, so we can update its next pointer */
				prev = a->contours;
				while (prev->next != info.result) {
					prev = prev->next;
				}

				/* Remove hole from the contour */
				remove_contour(a, prev, info.result, rnd_true);
				rnd_poly_contour_del(&info.result);
			}
			/* End check for deleted holes */

			/* If we deleted all the pieces of the polyarea, *pieces is NULL */
		}
		while ((a = anext), *pieces != NULL && !finished);

		return;
	}
	else {
		/* This path isn't optimised for speed */
	}

	do {
		int hole_contour = 0;
		int is_outline = 1;

		anext = a->f;
		finished = (anext == *pieces);

		prev = NULL;
		for (curc = a->contours; curc != NULL; curc = next, is_outline = 0) {
			int is_first = contour_is_first(a, curc);
			int is_last = contour_is_last(curc);
			int del_contour = 0;

			next = curc->next;

			if (del_outside)
				del_contour = curc->Flags.status != ISECTED && !cntr_in_M_rnd_polyarea_t(curc, bpa, rnd_false);

			/* Skip intersected contours */
			if (curc->Flags.status == ISECTED) {
				prev = curc;
				continue;
			}

			/* Reset the intersection flags, since we keep these pieces */
			curc->Flags.status = UNKNWN;

			if (del_contour || hole_contour) {

				remove_contour(a, prev, curc, !(is_first && is_last));

				if (del_contour) {
					/* Delete the contour */
					rnd_poly_contour_del(&curc);	/* NB: Sets curc to NULL */
				}
				else if (hole_contour) {
					/* Link into the list of holes */
					curc->next = *holes;
					*holes = curc;
				}
				else {
					assert(0);
				}

				if (is_first && is_last) {
					remove_polyarea(pieces, a);
					rnd_polyarea_free(&a);				/* NB: Sets a to NULL */
				}

			}
			else {
				/* Note the item we just didn't delete as the next
				   candidate for having its "next" pointer adjusted.
				   Saves walking the contour list when we delete one. */
				prev = curc;
			}

			/* If we move or delete an outer contour, we need to move any holes
			   we wish to keep within that contour to the holes list. */
			if (is_outline && del_contour)
				hole_contour = 1;

		}

		/* If we deleted all the pieces of the polyarea, *pieces is NULL */
	}
	while ((a = anext), *pieces != NULL && !finished);
}

static void
M_rnd_polyarea_t_Collect_separated(jmp_buf * e, rnd_pline_t * afst, rnd_polyarea_t ** contours, rnd_pline_t ** holes, int action, rnd_bool maybe)
{
	rnd_pline_t **cur, **next;

	for (cur = &afst; *cur != NULL; cur = next) {
		next = &((*cur)->next);
		/* if we disappear a contour, don't advance twice */
		if (cntr_Collect(e, cur, contours, holes, action, NULL, NULL, NULL))
			next = cur;
	}
}

static void M_rnd_polyarea_t_Collect(jmp_buf * e, rnd_polyarea_t * afst, rnd_polyarea_t ** contours, rnd_pline_t ** holes, int action, rnd_bool maybe)
{
	rnd_polyarea_t *a = afst;
	rnd_polyarea_t *parent = NULL;			/* Quiet compiler warning */
	rnd_pline_t **cur, **next, *parent_contour;

	assert(a != NULL);
	while ((a = a->f) != afst);
	/* now the non-intersect parts are collected in temp/holes */
	do {
		if (maybe && a->contours->Flags.status != ISECTED)
			parent_contour = a->contours;
		else
			parent_contour = NULL;

		/* Take care of the first contour - so we know if we
		 * can shortcut reparenting some of its children
		 */
		cur = &a->contours;
		if (*cur != NULL) {
			next = &((*cur)->next);
			/* if we disappear a contour, don't advance twice */
			if (cntr_Collect(e, cur, contours, holes, action, a, NULL, NULL)) {
				parent = (*contours)->b;	/* InsCntr inserts behind the head */
				next = cur;
			}
			else
				parent = a;
			cur = next;
		}
		for (; *cur != NULL; cur = next) {
			next = &((*cur)->next);
			/* if we disappear a contour, don't advance twice */
			if (cntr_Collect(e, cur, contours, holes, action, a, parent, parent_contour))
				next = cur;
		}
	}
	while ((a = a->f) != afst);
}


#include "pa_api_bool.c"
#include "pa_api_vertex.c"
#include "pa_api_pline.c"
#include "pa_api_polyarea.c"
#include "pa_api_check.c"



void rnd_polyarea_get_tree_seg(void *obj, rnd_coord_t *x1, rnd_coord_t *y1, rnd_coord_t *x2, rnd_coord_t *y2)
{
	struct seg *s = obj;
	*x1 = s->v->point[0];
	*x2 = s->v->next->point[0];
	*y1 = s->v->point[1];
	*y2 = s->v->next->point[1];
}

/* how about expanding polygons so that edges can be arcs rather than
 * lines. Consider using the third coordinate to store the radius of the
 * arc. The arc would pass through the vertex points. Positive radius
 * would indicate the arc bows left (center on right of P1-P2 path)
 * negative radius would put the center on the other side. 0 radius
 * would mean the edge is a line instead of arc
 * The intersections of the two circles centered at the vertex points
 * would determine the two possible arc centers. If P2.x > P1.x then
 * the center with smaller Y is selected for positive r. If P2.y > P1.y
 * then the center with greater X is selected for positive r.
 *
 * the vec_inters2() routine would then need to handle line-line
 * line-arc and arc-arc intersections.
 *
 * perhaps reverse tracing the arc would require look-ahead to check
 * for arcs
 */
