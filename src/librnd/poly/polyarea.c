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

#define ROUND(a) (long)((a) > 0 ? ((a) + 0.5) : ((a) - 0.5))

#define EPSILON (1E-8)
#define IsZero(a, b) (fabs((a) - (b)) < EPSILON)

#undef min
#undef max
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

/*********************************************************************/
/*              L o n g   V e c t o r   S t u f f                    */
/*********************************************************************/

#define Vcopy(a,b) {(a)[0]=(b)[0];(a)[1]=(b)[1];}
int vect_equal(rnd_vector_t v1, rnd_vector_t v2);
void vect_init(rnd_vector_t v, double x, double y);
void vect_sub(rnd_vector_t res, rnd_vector_t v2, rnd_vector_t v3);

void vect_min(rnd_vector_t res, rnd_vector_t v2, rnd_vector_t v3);
void vect_max(rnd_vector_t res, rnd_vector_t v2, rnd_vector_t v3);

double rnd_vect_dist2(rnd_vector_t v1, rnd_vector_t v2);
double rnd_vect_det2(rnd_vector_t v1, rnd_vector_t v2);
double rnd_vect_len2(rnd_vector_t v1);

int rnd_vect_inters2(rnd_vector_t A, rnd_vector_t B, rnd_vector_t C, rnd_vector_t D, rnd_vector_t S1, rnd_vector_t S2);

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

/* ///////////////////////////////////////////////////////////////////////////// * /
/ *  2-Dimensional stuff
/ * ///////////////////////////////////////////////////////////////////////////// */

#define Vsub2(r,a,b)	{(r)[0] = (a)[0] - (b)[0]; (r)[1] = (a)[1] - (b)[1];}
#define Vadd2(r,a,b)	{(r)[0] = (a)[0] + (b)[0]; (r)[1] = (a)[1] + (b)[1];}
#define Vsca2(r,a,s)	{(r)[0] = (a)[0] * (s); (r)[1] = (a)[1] * (s);}
#define Vcpy2(r,a)		{(r)[0] = (a)[0]; (r)[1] = (a)[1];}
#define Vequ2(a,b)		((a)[0] == (b)[0] && (a)[1] == (b)[1])
#define Vadds(r,a,b,s)	{(r)[0] = ((a)[0] + (b)[0]) * (s); (r)[1] = ((a)[1] + (b)[1]) * (s);}
#define Vswp2(a,b) { long t; \
	t = (a)[0], (a)[0] = (b)[0], (b)[0] = t; \
	t = (a)[1], (a)[1] = (b)[1], (b)[1] = t; \
}

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

#define ISECT_BAD_PARAM (-1)
#define ISECT_NO_MEMORY (-2)


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

RND_INLINE void cntrbox_adjust(rnd_pline_t * c, const rnd_vector_t p)
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

RND_INLINE int cntrbox_inside(rnd_pline_t * c1, rnd_pline_t * c2)
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

RND_INLINE void remove_contour(rnd_polyarea_t * piece, rnd_pline_t * prev_contour, rnd_pline_t * contour, int remove_rtree_entry)
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


RND_INLINE int contour_is_first(rnd_polyarea_t * a, rnd_pline_t * cur)
{
	return (a->contours == cur);
}


RND_INLINE int contour_is_last(rnd_pline_t * cur)
{
	return (cur->next == NULL);
}


RND_INLINE void remove_polyarea(rnd_polyarea_t ** list, rnd_polyarea_t * piece)
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

/* determine if two polygons touch or overlap */
rnd_bool rnd_polyarea_touching(rnd_polyarea_t * a, rnd_polyarea_t * b)
{
	jmp_buf e;
	int code;

	if ((code = setjmp(e)) == 0) {
#ifdef DEBUG
		if (!rnd_poly_valid(a))
			return -1;
		if (!rnd_poly_valid(b))
			return -1;
#endif
		M_rnd_polyarea_t_intersect(&e, a, b, rnd_false);

		if (M_rnd_polyarea_t_label(a, b, rnd_true))
			return rnd_true;
		if (M_rnd_polyarea_t_label(b, a, rnd_true))
			return rnd_true;
	}
	else if (code == TOUCHES)
		return rnd_true;
	return rnd_false;
}

/* the main clipping routines */
int rnd_polyarea_boolean(const rnd_polyarea_t * a_org, const rnd_polyarea_t * b_org, rnd_polyarea_t ** res, int action)
{
	rnd_polyarea_t *a = NULL, *b = NULL;

	if (!rnd_polyarea_m_copy0(&a, a_org) || !rnd_polyarea_m_copy0(&b, b_org))
		return rnd_err_no_memory;

	return rnd_polyarea_boolean_free(a, b, res, action);
}																/* poly_Boolean */

/* just like poly_Boolean but frees the input polys */
int rnd_polyarea_boolean_free(rnd_polyarea_t * ai, rnd_polyarea_t * bi, rnd_polyarea_t ** res, int action)
{
	rnd_polyarea_t *a = ai, *b = bi;
	rnd_pline_t *a_isected = NULL;
	rnd_pline_t *p, *holes = NULL;
	jmp_buf e;
	int code;

	*res = NULL;

	if (!a) {
		switch (action) {
		case RND_PBO_XOR:
		case RND_PBO_UNITE:
			*res = bi;
			return rnd_err_ok;
		case RND_PBO_SUB:
		case RND_PBO_ISECT:
			if (b != NULL)
				rnd_polyarea_free(&b);
			return rnd_err_ok;
		}
	}
	if (!b) {
		switch (action) {
		case RND_PBO_SUB:
		case RND_PBO_XOR:
		case RND_PBO_UNITE:
			*res = ai;
			return rnd_err_ok;
		case RND_PBO_ISECT:
			if (a != NULL)
				rnd_polyarea_free(&a);
			return rnd_err_ok;
		}
	}

	if ((code = setjmp(e)) == 0) {
#ifdef DEBUG
		assert(rnd_poly_valid(a));
		assert(rnd_poly_valid(b));
#endif

		/* intersect needs to make a list of the contours in a and b which are intersected */
		M_rnd_polyarea_t_intersect(&e, a, b, rnd_true);

		/* We could speed things up a lot here if we only processed the relevant contours */
		/* NB: Relevant parts of a are labeled below */
		M_rnd_polyarea_t_label(b, a, rnd_false);

		*res = a;
		M_rnd_polyarea_t_update_primary(&e, res, &holes, action, b);
		M_rnd_polyarea_separate_isected(&e, res, &holes, &a_isected);
		M_rnd_polyarea_t_label_separated(a_isected, b, rnd_false);
		M_rnd_polyarea_t_Collect_separated(&e, a_isected, res, &holes, action, rnd_false);
		M_B_AREA_Collect(&e, b, res, &holes, action);
		rnd_polyarea_free(&b);

		/* free a_isected */
		while ((p = a_isected) != NULL) {
			a_isected = p->next;
			rnd_poly_contour_del(&p);
		}

		rnd_poly_insert_holes(&e, *res, &holes);
	}
	/* delete holes if any left */
	while ((p = holes) != NULL) {
		holes = p->next;
		rnd_poly_contour_del(&p);
	}

	if (code) {
		rnd_polyarea_free(res);
		return code;
	}
	assert(!*res || rnd_poly_valid(*res));
	return code;
}																/* poly_Boolean_free */

static void clear_marks(rnd_polyarea_t * p)
{
	rnd_polyarea_t *n = p;
	rnd_pline_t *c;
	rnd_vnode_t *v;

	do {
		for (c = n->contours; c; c = c->next) {
			v = c->head;
			do {
				v->Flags.mark = 0;
			}
			while ((v = v->next) != c->head);
		}
	}
	while ((n = n->f) != p);
}

/* compute the intersection and subtraction (divides "a" into two pieces)
 * and frees the input polys. This assumes that bi is a single simple polygon.
 */
int rnd_polyarea_and_subtract_free(rnd_polyarea_t * ai, rnd_polyarea_t * bi, rnd_polyarea_t ** aandb, rnd_polyarea_t ** aminusb)
{
	rnd_polyarea_t *a = ai, *b = bi;
	rnd_pline_t *p, *holes = NULL;
	jmp_buf e;
	int code;

	*aandb = NULL;
	*aminusb = NULL;

	if ((code = setjmp(e)) == 0) {

#ifdef DEBUG
		if (!rnd_poly_valid(a))
			return -1;
		if (!rnd_poly_valid(b))
			return -1;
#endif
		M_rnd_polyarea_t_intersect(&e, a, b, rnd_true);

		M_rnd_polyarea_t_label(a, b, rnd_false);
		M_rnd_polyarea_t_label(b, a, rnd_false);

		M_rnd_polyarea_t_Collect(&e, a, aandb, &holes, RND_PBO_ISECT, rnd_false);
		rnd_poly_insert_holes(&e, *aandb, &holes);
		assert(rnd_poly_valid(*aandb));
		/* delete holes if any left */
		while ((p = holes) != NULL) {
			holes = p->next;
			rnd_poly_contour_del(&p);
		}
		holes = NULL;
		clear_marks(a);
		clear_marks(b);
		M_rnd_polyarea_t_Collect(&e, a, aminusb, &holes, RND_PBO_SUB, rnd_false);
		rnd_poly_insert_holes(&e, *aminusb, &holes);
		rnd_polyarea_free(&a);
		rnd_polyarea_free(&b);
		assert(rnd_poly_valid(*aminusb));
	}
	/* delete holes if any left */
	while ((p = holes) != NULL) {
		holes = p->next;
		rnd_poly_contour_del(&p);
	}


	if (code) {
		rnd_polyarea_free(aandb);
		rnd_polyarea_free(aminusb);
		return code;
	}
	assert(!*aandb || rnd_poly_valid(*aandb));
	assert(!*aminusb || rnd_poly_valid(*aminusb));
	return code;
} /* poly_AndSubtract_free */

RND_INLINE int cntrbox_pointin(const rnd_pline_t *c, const rnd_vector_t p)
{
	return (p[0] >= c->xmin && p[1] >= c->ymin && p[0] <= c->xmax && p[1] <= c->ymax);
}

RND_INLINE int node_neighbours(rnd_vnode_t * a, rnd_vnode_t * b)
{
	return (a == b) || (a->next == b) || (b->next == a) || (a->next == b->next);
}

rnd_vnode_t *rnd_poly_node_create(rnd_vector_t v)
{
	rnd_vnode_t *res;
	rnd_coord_t *c;

	assert(v);
	res = (rnd_vnode_t *) calloc(1, sizeof(rnd_vnode_t));
	if (res == NULL)
		return NULL;
	/* bzero (res, sizeof (rnd_vnode_t) - sizeof(rnd_vector_t)); */
	c = res->point;
	*c++ = *v++;
	*c = *v;
	return res;
}

void rnd_poly_contour_init(rnd_pline_t * c)
{
	if (c == NULL)
		return;
	/* bzero (c, sizeof(rnd_pline_t)); */
	if (c->head == NULL)
		c->head = calloc(sizeof(rnd_vnode_t), 1);
	c->head->next = c->head->prev = c->head;
	c->xmin = c->ymin = RND_COORD_MAX;
	c->xmax = c->ymax = -RND_COORD_MAX-1;
	c->is_round = rnd_false;
	c->cx = 0;
	c->cy = 0;
	c->radius = 0;
}

rnd_pline_t *rnd_poly_contour_new(const rnd_vector_t v)
{
	rnd_pline_t *res;

	res = (rnd_pline_t *) calloc(1, sizeof(rnd_pline_t));
	if (res == NULL)
		return NULL;

	rnd_poly_contour_init(res);

	if (v != NULL) {
/*		res->head = calloc(sizeof(rnd_vnode_t), 1); - no need to alloc, countour_init() did so */
		res->head->next = res->head->prev = res->head;
		Vcopy(res->head->point, v);
		cntrbox_adjust(res, v);
	}

	return res;
}

void rnd_poly_contour_clear(rnd_pline_t * c)
{
	rnd_vnode_t *cur;

	assert(c != NULL);
	while ((cur = c->head->next) != c->head) {
		rnd_poly_vertex_exclude(NULL, cur);
		free(cur);
	}
	free(c->head);
	c->head = NULL;
	rnd_poly_contour_init(c);
}

void rnd_poly_contour_del(rnd_pline_t ** c)
{
	rnd_vnode_t *cur, *prev;

	if (*c == NULL)
		return;
	for (cur = (*c)->head->prev; cur != (*c)->head; cur = prev) {
		prev = cur->prev;
		if (cur->cvc_next != NULL) {
			free(cur->cvc_next);
			free(cur->cvc_prev);
		}
		free(cur);
	}
	if ((*c)->head->cvc_next != NULL) {
		free((*c)->head->cvc_next);
		free((*c)->head->cvc_prev);
	}
	/* FIXME -- strict aliasing violation.  */
	if ((*c)->tree) {
		rnd_rtree_t *r = (*c)->tree;
		rnd_r_free_tree_data(r, free);
		rnd_r_destroy_tree(&r);
	}
	free((*c)->head);
	free(*c), *c = NULL;
}

void rnd_poly_contour_pre(rnd_pline_t * C, rnd_bool optimize)
{
	double area = 0;
	rnd_vnode_t *p, *c;
	rnd_vector_t p1, p2;

	assert(C != NULL);

	if (optimize) {
		for (c = (p = C->head)->next; c != C->head; c = (p = c)->next) {
			/* if the previous node is on the same line with this one, we should remove it */
			Vsub2(p1, c->point, p->point);
			Vsub2(p2, c->next->point, c->point);
			/* If the product below is zero then
			 * the points on either side of c 
			 * are on the same line!
			 * So, remove the point c
			 */

			if (rnd_vect_det2(p1, p2) == 0) {
				rnd_poly_vertex_exclude(C, c);
				free(c);
				c = p;
			}
		}
	}
	C->Count = 0;
	C->xmin = C->xmax = C->head->point[0];
	C->ymin = C->ymax = C->head->point[1];

	p = (c = C->head)->prev;
	if (c != p) {
		do {
			/* calculate area for orientation */
			area += (double) (p->point[0] - c->point[0]) * (p->point[1] + c->point[1]);
			cntrbox_adjust(C, c->point);
			C->Count++;
		}
		while ((c = (p = c)->next) != C->head);
	}
	C->area = RND_ABS(area);
	if (C->Count > 2)
		C->Flags.orient = ((area < 0) ? RND_PLF_INV : RND_PLF_DIR);
	C->tree = (rnd_rtree_t *) rnd_poly_make_edge_tree(C);
}																/* poly_PreContour */

static rnd_r_dir_t flip_cb(const rnd_box_t * b, void *cl)
{
	struct seg *s = (struct seg *) b;
	s->v = s->v->prev;
	return RND_R_DIR_FOUND_CONTINUE;
}

void rnd_poly_contour_inv(rnd_pline_t * c)
{
	rnd_vnode_t *cur, *next;
	int r;

	assert(c != NULL);
	cur = c->head;
	do {
		next = cur->next;
		cur->next = cur->prev;
		cur->prev = next;
		/* fix the segment tree */
	}
	while ((cur = next) != c->head);
	c->Flags.orient ^= 1;
	if (c->tree) {
		rnd_r_search(c->tree, NULL, NULL, flip_cb, NULL, &r);
		assert(r == c->Count);
	}
}

void rnd_poly_vertex_exclude(rnd_pline_t *parent, rnd_vnode_t * node)
{
	assert(node != NULL);
	if (parent != NULL) {
		if (parent->head == node) /* if node is excluded from a pline, it can not remain the head */
			parent->head = node->next;
	}
	if (node->cvc_next) {
		free(node->cvc_next);
		free(node->cvc_prev);
	}
	node->prev->next = node->next;
	node->next->prev = node->prev;

	if (parent != NULL) {
		if (parent->head == node) /* special case: removing head which was the last node in pline  */
			parent->head = NULL;
	}
}

RND_INLINE void rnd_poly_vertex_include_force_(rnd_vnode_t *after, rnd_vnode_t *node)
{
	assert(after != NULL);
	assert(node != NULL);

	node->prev = after;
	node->next = after->next;
	after->next = after->next->prev = node;
}

void rnd_poly_vertex_include_force(rnd_vnode_t *after, rnd_vnode_t *node)
{
	rnd_poly_vertex_include_force_(after, node);
}

void rnd_poly_vertex_include(rnd_vnode_t *after, rnd_vnode_t *node)
{
	double a, b;

	rnd_poly_vertex_include_force_(after, node);

	/* remove points on same line */
	if (node->prev->prev == node)
		return;											/* we don't have 3 points in the poly yet */
	a = (node->point[1] - node->prev->prev->point[1]);
	a *= (node->prev->point[0] - node->prev->prev->point[0]);
	b = (node->point[0] - node->prev->prev->point[0]);
	b *= (node->prev->point[1] - node->prev->prev->point[1]);
	if (fabs(a - b) < EPSILON) {
		rnd_vnode_t *t = node->prev;
		t->prev->next = node;
		node->prev = t->prev;
		free(t);
	}
}

rnd_bool rnd_poly_contour_copy(rnd_pline_t **dst, const rnd_pline_t *src)
{
	rnd_vnode_t *cur, *newnode;

	assert(src != NULL);
	*dst = NULL;
	*dst = rnd_poly_contour_new(src->head->point);
	if (*dst == NULL)
		return rnd_false;

	(*dst)->Count = src->Count;
	(*dst)->Flags.orient = src->Flags.orient;
	(*dst)->xmin = src->xmin, (*dst)->xmax = src->xmax;
	(*dst)->ymin = src->ymin, (*dst)->ymax = src->ymax;
	(*dst)->area = src->area;

	for (cur = src->head->next; cur != src->head; cur = cur->next) {
		if ((newnode = rnd_poly_node_create(cur->point)) == NULL)
			return rnd_false;
		/* newnode->Flags = cur->Flags; */
		rnd_poly_vertex_include((*dst)->head->prev, newnode);
	}
	(*dst)->tree = (rnd_rtree_t *) rnd_poly_make_edge_tree(*dst);
	return rnd_true;
}

/**********************************************************************/
/* polygon routines */

rnd_bool rnd_polyarea_copy0(rnd_polyarea_t ** dst, const rnd_polyarea_t * src)
{
	*dst = NULL;
	if (src != NULL)
		*dst = (rnd_polyarea_t *) calloc(1, sizeof(rnd_polyarea_t));
	if (*dst == NULL)
		return rnd_false;
	(*dst)->contour_tree = rnd_r_create_tree();

	return rnd_polyarea_copy1(*dst, src);
}

rnd_bool rnd_polyarea_copy1(rnd_polyarea_t * dst, const rnd_polyarea_t * src)
{
	rnd_pline_t *cur, **last = &dst->contours;

	*last = NULL;
	dst->f = dst->b = dst;

	for (cur = src->contours; cur != NULL; cur = cur->next) {
		if (!rnd_poly_contour_copy(last, cur))
			return rnd_false;
		rnd_r_insert_entry(dst->contour_tree, (rnd_box_t *) * last);
		last = &(*last)->next;
	}
	return rnd_true;
}

void rnd_polyarea_m_include(rnd_polyarea_t ** list, rnd_polyarea_t * a)
{
	if (*list == NULL)
		a->f = a->b = a, *list = a;
	else {
		a->f = *list;
		a->b = (*list)->b;
		(*list)->b = (*list)->b->f = a;
	}
}

rnd_bool rnd_polyarea_m_copy0(rnd_polyarea_t ** dst, const rnd_polyarea_t * srcfst)
{
	const rnd_polyarea_t *src = srcfst;
	rnd_polyarea_t *di;

	*dst = NULL;
	if (src == NULL)
		return rnd_false;
	do {
		if ((di = rnd_polyarea_create()) == NULL || !rnd_polyarea_copy1(di, src))
			return rnd_false;
		rnd_polyarea_m_include(dst, di);
	}
	while ((src = src->f) != srcfst);
	return rnd_true;
}

rnd_bool rnd_polyarea_contour_include(rnd_polyarea_t * p, rnd_pline_t * c)
{
	rnd_pline_t *tmp;

	if ((c == NULL) || (p == NULL))
		return rnd_false;
	if (c->Flags.orient == RND_PLF_DIR) {
		if (p->contours != NULL)
			return rnd_false;
		p->contours = c;
	}
	else {
		if (p->contours == NULL)
			return rnd_false;
		/* link at front of hole list */
		tmp = p->contours->next;
		p->contours->next = c;
		c->next = tmp;
	}
	rnd_r_insert_entry(p->contour_tree, (rnd_box_t *) c);
	return rnd_true;
}

typedef struct pip {
	int f;
	rnd_vector_t p;
	jmp_buf env;
} pip;


static rnd_r_dir_t crossing(const rnd_box_t * b, void *cl)
{
	struct seg *s = (struct seg *) b;   /* polygon edge, line segment */
	struct pip *p = (struct pip *) cl;  /* horizontal cutting line */

	/* the horizontal cutting line is between vectors s->v and s->v->next, but
	   these two can be in any order; because poly contour is CCW, this means if
	   the edge is going up, we went from inside to outside, else we went
	   from outside to inside */
	if (s->v->point[1] <= p->p[1]) {
		if (s->v->next->point[1] > p->p[1]) { /* this also happens to blocks horizontal poly edges because they are only == */
			rnd_vector_t v1, v2;
			rnd_long64_t cross;
			Vsub2(v1, s->v->next->point, s->v->point);
			Vsub2(v2, p->p, s->v->point);
			cross = (rnd_long64_t) v1[0] * v2[1] - (rnd_long64_t) v2[0] * v1[1];
			if (cross == 0) { /* special case: if the point is on any edge, the point is in the poly */
				p->f = 1;
				longjmp(p->env, 1);
			}
			if (cross > 0)
				p->f += 1;
		}
	}
	else { /* since the other side was <=, when we get here we also blocked horizontal lines of the negative direction */
		if (s->v->next->point[1] <= p->p[1]) {
			rnd_vector_t v1, v2;
			rnd_long64_t cross;
			Vsub2(v1, s->v->next->point, s->v->point);
			Vsub2(v2, p->p, s->v->point);
			cross = (rnd_long64_t) v1[0] * v2[1] - (rnd_long64_t) v2[0] * v1[1];
			if (cross == 0) { /* special case: if the point is on any edge, the point is in the poly */
				p->f = 1;
				longjmp(p->env, 1);
			}
			if (cross < 0)
				p->f -= 1;
		}
	}

	return RND_R_DIR_FOUND_CONTINUE;
}

int rnd_poly_contour_inside(const rnd_pline_t *c, rnd_vector_t p)
{
	struct pip info;
	rnd_box_t ray;

	if (!cntrbox_pointin(c, p))
		return rnd_false;

	/* run a horizontal ray from the point to x->infinity and count (in info.f)
	   how it crosses poly edges with different winding */
	info.f = 0;
	info.p[0] = ray.X1 = p[0];
	info.p[1] = ray.Y1 = p[1];
	ray.X2 = RND_COORD_MAX;
	ray.Y2 = p[1] + 1;
	if (setjmp(info.env) == 0)
		rnd_r_search(c->tree, &ray, NULL, crossing, &info, NULL);
	return info.f;
}

rnd_bool rnd_polyarea_contour_inside(rnd_polyarea_t * p, rnd_vector_t v0)
{
	rnd_pline_t *cur;

	if ((p == NULL) || (v0 == NULL) || (p->contours == NULL))
		return rnd_false;
	cur = p->contours;
	if (rnd_poly_contour_inside(cur, v0)) {
		for (cur = cur->next; cur != NULL; cur = cur->next)
			if (rnd_poly_contour_inside(cur, v0))
				return rnd_false;
		return rnd_true;
	}
	return rnd_false;
}

rnd_bool poly_M_CheckInside(rnd_polyarea_t * p, rnd_vector_t v0)
{
	rnd_polyarea_t *cur;

	if ((p == NULL) || (v0 == NULL))
		return rnd_false;
	cur = p;
	do {
		if (rnd_polyarea_contour_inside(cur, v0))
			return rnd_true;
	}
	while ((cur = cur->f) != p);
	return rnd_false;
}

static double dot(rnd_vector_t A, rnd_vector_t B)
{
	return (double) A[0] * (double) B[0] + (double) A[1] * (double) B[1];
}

/* Compute whether point is inside a triangle formed by 3 other points */
/* Algorithm from http://www.blackpawn.com/texts/pointinpoly/default.html */
static int point_in_triangle(rnd_vector_t A, rnd_vector_t B, rnd_vector_t C, rnd_vector_t P)
{
	rnd_vector_t v0, v1, v2;
	double dot00, dot01, dot02, dot11, dot12;
	double invDenom;
	double u, v;

	/* Compute vectors */
	v0[0] = C[0] - A[0];
	v0[1] = C[1] - A[1];
	v1[0] = B[0] - A[0];
	v1[1] = B[1] - A[1];
	v2[0] = P[0] - A[0];
	v2[1] = P[1] - A[1];

	/* Compute dot products */
	dot00 = dot(v0, v0);
	dot01 = dot(v0, v1);
	dot02 = dot(v0, v2);
	dot11 = dot(v1, v1);
	dot12 = dot(v1, v2);

	/* Compute barycentric coordinates */
	invDenom = 1. / (dot00 * dot11 - dot01 * dot01);
	u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	/* Check if point is in triangle */
	return (u > 0.0) && (v > 0.0) && (u + v < 1.0);
}

/* wrapper to keep the original name short and original function RND_INLINE */
int rnd_point_in_triangle(rnd_vector_t A, rnd_vector_t B, rnd_vector_t C, rnd_vector_t P)
{
	return point_in_triangle(A, B, C, P);
}


/* Returns the dot product of rnd_vector_t A->B, and a vector
 * orthogonal to rnd_vector_t C->D. The result is not normalised, so will be
 * weighted by the magnitude of the C->D vector.
 */
static double dot_orthogonal_to_direction(rnd_vector_t A, rnd_vector_t B, rnd_vector_t C, rnd_vector_t D)
{
	rnd_vector_t l1, l2, l3;
	l1[0] = B[0] - A[0];
	l1[1] = B[1] - A[1];
	l2[0] = D[0] - C[0];
	l2[1] = D[1] - C[1];

	l3[0] = -l2[1];
	l3[1] = l2[0];

	return dot(l1, l3);
}

/* Algorithm from http://www.exaflop.org/docs/cgafaq/cga2.html
 *
 * "Given a simple polygon, find some point inside it. Here is a method based
 * on the proof that there exists an internal diagonal, in [O'Rourke, 13-14].
 * The idea is that the midpoint of a diagonal is interior to the polygon.
 *
 * 1.  Identify a convex vertex v; let its adjacent vertices be a and b.
 * 2.  For each other vertex q do:
 * 2a. If q is inside avb, compute distance to v (orthogonal to ab).
 * 2b. Save point q if distance is a new min.
 * 3.  If no point is inside, return midpoint of ab, or centroid of avb.
 * 4.  Else if some point inside, qv is internal: return its midpoint."
 *
 * [O'Rourke]: Computational Geometry in C (2nd Ed.)
 *             Joseph O'Rourke, Cambridge University Press 1998,
 *             ISBN 0-521-64010-5 Pbk, ISBN 0-521-64976-5 Hbk
 */
static void poly_ComputeInteriorPoint(rnd_pline_t * poly, rnd_vector_t v)
{
	rnd_vnode_t *pt1, *pt2, *pt3, *q;
	rnd_vnode_t *min_q = NULL;
	double dist;
	double min_dist = 0.0;
	double dir = (poly->Flags.orient == RND_PLF_DIR) ? 1. : -1;

	/* Find a convex node on the polygon */
	pt1 = poly->head;
	do {
		double dot_product;

		pt2 = pt1->next;
		pt3 = pt2->next;

		dot_product = dot_orthogonal_to_direction(pt1->point, pt2->point, pt3->point, pt2->point);

		if (dot_product * dir > 0.)
			break;
	}
	while ((pt1 = pt1->next) != poly->head);

	/* Loop over remaining vertices */
	q = pt3;
	do {
		/* Is current vertex "q" outside pt1 pt2 pt3 triangle? */
		if (!point_in_triangle(pt1->point, pt2->point, pt3->point, q->point))
			continue;

		/* NO: compute distance to pt2 (v) orthogonal to pt1 - pt3) */
		/*     Record minimum */
		dist = dot_orthogonal_to_direction(q->point, pt2->point, pt1->point, pt3->point);
		if (min_q == NULL || dist < min_dist) {
			min_dist = dist;
			min_q = q;
		}
	}
	while ((q = q->next) != pt2);

	/* Were any "q" found inside pt1 pt2 pt3? */
	if (min_q == NULL) {
		/* NOT FOUND: Return midpoint of pt1 pt3 */
		v[0] = (pt1->point[0] + pt3->point[0]) / 2;
		v[1] = (pt1->point[1] + pt3->point[1]) / 2;
	}
	else {
		/* FOUND: Return mid point of min_q, pt2 */
		v[0] = (pt2->point[0] + min_q->point[0]) / 2;
		v[1] = (pt2->point[1] + min_q->point[1]) / 2;
	}
}


/* NB: This function assumes the caller _knows_ the contours do not
 *     intersect. If the contours intersect, the result is undefined.
 *     It will return the correct result if the two contours share
 *     common points between their contours. (Identical contours
 *     are treated as being inside each other).
 */
int rnd_poly_contour_in_contour(rnd_pline_t * poly, rnd_pline_t * inner)
{
	rnd_vector_t point;
	assert(poly != NULL);
	assert(inner != NULL);
	if (cntrbox_inside(inner, poly)) {
		/* We need to prove the "inner" contour is not outside
		 * "poly" contour. If it is outside, we can return.
		 */
		if (!rnd_poly_contour_inside(poly, inner->head->point))
			return 0;

		poly_ComputeInteriorPoint(inner, point);
		return rnd_poly_contour_inside(poly, point);
	}
	return 0;
}

void rnd_polyarea_init(rnd_polyarea_t * p)
{
	p->f = p->b = p;
	p->contours = NULL;
	p->contour_tree = rnd_r_create_tree();
}

rnd_polyarea_t *rnd_polyarea_create(void)
{
	rnd_polyarea_t *res;

	if ((res = (rnd_polyarea_t *) malloc(sizeof(rnd_polyarea_t))) != NULL)
		rnd_polyarea_init(res);
	return res;
}

void rnd_poly_contours_free(rnd_pline_t ** pline)
{
	rnd_pline_t *pl;

	while ((pl = *pline) != NULL) {
		*pline = pl->next;
		rnd_poly_contour_del(&pl);
	}
}

void rnd_polyarea_free(rnd_polyarea_t ** p)
{
	rnd_polyarea_t *cur;

	if (*p == NULL)
		return;
	for (cur = (*p)->f; cur != *p; cur = (*p)->f) {
		rnd_poly_contours_free(&cur->contours);
		rnd_r_destroy_tree(&cur->contour_tree);
		cur->f->b = cur->b;
		cur->b->f = cur->f;
		free(cur);
	}
	rnd_poly_contours_free(&cur->contours);
	rnd_r_destroy_tree(&cur->contour_tree);
	free(*p), *p = NULL;
}

static rnd_bool inside_sector(rnd_vnode_t * pn, rnd_vector_t p2)
{
	rnd_vector_t cdir, ndir, pdir;
	int p_c, n_c, p_n;

	assert(pn != NULL);
	vect_sub(cdir, p2, pn->point);
	vect_sub(pdir, pn->point, pn->prev->point);
	vect_sub(ndir, pn->next->point, pn->point);

	p_c = rnd_vect_det2(pdir, cdir) >= 0;
	n_c = rnd_vect_det2(ndir, cdir) >= 0;
	p_n = rnd_vect_det2(pdir, ndir) >= 0;

	if ((p_n && p_c && n_c) || ((!p_n) && (p_c || n_c)))
		return rnd_true;
	else
		return rnd_false;
}																/* inside_sector */

/* returns rnd_true if bad contour */
typedef struct {
	int marks, lines;
#ifndef NDEBUG
	rnd_coord_t x[8], y[8];
	rnd_coord_t x1[8], y1[8], x2[8], y2[8];
	char msg[256];
#endif
} pa_chk_res_t;


#ifndef NDEBUG
#define PA_CHK_MARK(x_, y_) \
do { \
	if (res->marks < sizeof(res->x) / sizeof(res->x[0])) { \
		res->x[res->marks] = x_; \
		res->y[res->marks] = y_; \
		res->marks++; \
	} \
} while(0)
#define PA_CHK_LINE(x1_, y1_, x2_, y2_) \
do { \
	if (res->lines < sizeof(res->x1) / sizeof(res->x1[0])) { \
		res->x1[res->lines] = x1_; \
		res->y1[res->lines] = y1_; \
		res->x2[res->lines] = x2_; \
		res->y2[res->lines] = y2_; \
		res->lines++; \
	} \
} while(0)
#else
#define PA_CHK_MARK(x, y)
#define PA_CHK_LINE(x1, y1, x2, y2)
#endif


RND_INLINE rnd_bool PA_CHK_ERROR(pa_chk_res_t *res, const char *fmt, ...)
{
#ifndef NDEBUG
	va_list ap;
	va_start(ap, fmt);
	rnd_vsnprintf(res->msg, sizeof(res->msg), fmt, ap);
	va_end(ap);
#endif
	return rnd_true;
}

rnd_bool rnd_polyarea_contour_check_(rnd_pline_t *a, pa_chk_res_t *res)
{
	rnd_vnode_t *a1, *a2, *hit1, *hit2;
	rnd_vector_t i1, i2;
	int icnt;

#ifndef NDEBUG
	*res->msg = '\0';
#endif
	res->marks = res->lines = 0;

	assert(a != NULL);
	a1 = a->head;
	do {
		a2 = a1;
		do {
			if (!node_neighbours(a1, a2) && (icnt = rnd_vect_inters2(a1->point, a1->next->point, a2->point, a2->next->point, i1, i2)) > 0) {
				if (icnt > 1) {
					PA_CHK_MARK(a1->point[0], a1->point[1]);
					PA_CHK_MARK(a2->point[0], a2->point[1]);
					return PA_CHK_ERROR(res, "icnt > 1 (%d) at %mm;%mm or  %mm;%mm", icnt, a1->point[0], a1->point[1], a2->point[0], a2->point[1]);
				}

TODO(": ugly workaround: test where exactly the intersection happens and tune the endpoint of the line")
				if (rnd_vect_dist2(i1, a1->point) < RND_POLY_ENDP_EPSILON)
					hit1 = a1;
				else if (rnd_vect_dist2(i1, a1->next->point) < RND_POLY_ENDP_EPSILON)
					hit1 = a1->next;
				else
					hit1 = NULL;

				if (rnd_vect_dist2(i1, a2->point) < RND_POLY_ENDP_EPSILON)
					hit2 = a2;
				else if (rnd_vect_dist2(i1, a2->next->point) < RND_POLY_ENDP_EPSILON)
					hit2 = a2->next;
				else
					hit2 = NULL;

				if ((hit1 == NULL) && (hit2 == NULL)) {
					/* If the intersection didn't land on an end-point of either
					 * line, we know the lines cross and we return rnd_true.
					 */
					PA_CHK_LINE(a1->point[0], a1->point[1], a1->next->point[0], a1->next->point[1]);
					PA_CHK_LINE(a2->point[0], a2->point[1], a2->next->point[0], a2->next->point[1]);
					return PA_CHK_ERROR(res, "lines cross between %mm;%mm and %mm;%mm", a1->point[0], a1->point[1], a2->point[0], a2->point[1]);
				}
				else if (hit1 == NULL) {
					/* An end-point of the second line touched somewhere along the
					   length of the first line. Check where the second line leads. */
					if (inside_sector(hit2, a1->point) != inside_sector(hit2, a1->next->point)) {
						PA_CHK_MARK(a1->point[0], a1->point[1]);
						PA_CHK_MARK(hit2->point[0], hit2->point[1]);
						return PA_CHK_ERROR(res, "lines is inside sector (1) at %mm;%mm", a1->point[0], a1->point[1]);
					}
				}
				else if (hit2 == NULL) {
					/* An end-point of the first line touched somewhere along the
					   length of the second line. Check where the first line leads. */
					if (inside_sector(hit1, a2->point) != inside_sector(hit1, a2->next->point)) {
						PA_CHK_MARK(a2->point[0], a2->point[1]);
						PA_CHK_MARK(hit1->point[0], hit1->point[1]);
						return PA_CHK_ERROR(res, "lines is inside sector (2) at %mm;%mm", a2->point[0], a2->point[1]);
					}
				}
				else {
					/* Both lines intersect at an end-point. Check where they lead. */
					if (inside_sector(hit1, hit2->prev->point) != inside_sector(hit1, hit2->next->point)) {
						PA_CHK_MARK(hit1->point[0], hit2->point[1]);
						PA_CHK_MARK(hit2->point[0], hit2->point[1]);
						return PA_CHK_ERROR(res, "lines is inside sector (3) at %mm;%mm or %mm;%mm", hit1->point[0], hit1->point[1], hit2->point[0], hit2->point[1]);
					}
				}
			}
		}
		while ((a2 = a2->next) != a->head);
	}
	while ((a1 = a1->next) != a->head);
	return rnd_false;
}

rnd_bool rnd_polyarea_contour_check(rnd_pline_t *a)
{
	pa_chk_res_t res;
	return rnd_polyarea_contour_check_(a, &res);
}

void rnd_polyarea_bbox(rnd_polyarea_t * p, rnd_box_t * b)
{
	rnd_pline_t *n;
	/*int cnt;*/

	n = p->contours;
	b->X1 = b->X2 = n->xmin;
	b->Y1 = b->Y2 = n->ymin;

	for (/*cnt = 0*/; /*cnt < 2 */ n != NULL; n = n->next) {
		if (n->xmin < b->X1)
			b->X1 = n->xmin;
		if (n->ymin < b->Y1)
			b->Y1 = n->ymin;
		if (n->xmax > b->X2)
			b->X2 = n->xmax;
		if (n->ymax > b->Y2)
			b->Y2 = n->ymax;
/*		if (n == p->contours)
			cnt++;*/
	}
}

#ifndef NDEBUG
static void rnd_poly_valid_report(rnd_pline_t *c, rnd_vnode_t *pl, pa_chk_res_t *chk)
{
	rnd_vnode_t *v, *n;
	rnd_coord_t minx = RND_COORD_MAX, miny = RND_COORD_MAX, maxx = -RND_COORD_MAX, maxy = -RND_COORD_MAX;

#define update_minmax(min, max, val) \
	if (val < min) min = val; \
	if (val > max) max = val;
	if (chk != NULL)
		rnd_fprintf(stderr, "Details: %s\n", chk->msg);
	rnd_fprintf(stderr, "!!!animator start\n");
	v = pl;
	do {
		n = v->next;
		update_minmax(minx, maxx, v->point[0]);
		update_minmax(miny, maxy, v->point[1]);
		update_minmax(minx, maxx, n->point[0]);
		update_minmax(miny, maxy, n->point[1]);
	}
	while ((v = v->next) != pl);
	rnd_fprintf(stderr, "scale 1 -1\n");
	rnd_fprintf(stderr, "viewport %mm %mm - %mm %mm\n", minx, miny, maxx, maxy);
	rnd_fprintf(stderr, "frame\n");
	v = pl;
	do {
		n = v->next;
		rnd_fprintf(stderr, "line %#mm %#mm %#mm %#mm\n", v->point[0], v->point[1], n->point[0], n->point[1]);
	}
	while ((v = v->next) != pl);

	if ((chk != NULL) && (chk->marks > 0)) {
		int n, MR=RND_MM_TO_COORD(0.05);
		fprintf(stderr, "color #770000\n");
		for(n = 0; n < chk->marks; n++) {
			rnd_fprintf(stderr, "line %#mm %#mm %#mm %#mm\n", chk->x[n]-MR, chk->y[n]-MR, chk->x[n]+MR, chk->y[n]+MR);
			rnd_fprintf(stderr, "line %#mm %#mm %#mm %#mm\n", chk->x[n]-MR, chk->y[n]+MR, chk->x[n]+MR, chk->y[n]-MR);
		}
	}

	if ((chk != NULL) && (chk->lines > 0)) {
		int n;
		fprintf(stderr, "color #990000\n");
		for(n = 0; n < chk->lines; n++)
			rnd_fprintf(stderr, "line %#mm %#mm %#mm %#mm\n", chk->x1[n], chk->y1[n], chk->x2[n], chk->y2[n]);
	}

	fprintf(stderr, "flush\n");
	fprintf(stderr, "!!!animator end\n");

#undef update_minmax
}
#endif


rnd_bool rnd_poly_valid(rnd_polyarea_t * p)
{
	rnd_pline_t *c;
	pa_chk_res_t chk;

	if ((p == NULL) || (p->contours == NULL)) {
#if 0
(disabled for too many false positive)
#ifndef NDEBUG
		rnd_fprintf(stderr, "Invalid polyarea: no contours\n");
#endif
#endif
		return rnd_false;
	}

	if (p->contours->Flags.orient == RND_PLF_INV) {
#ifndef NDEBUG
		rnd_fprintf(stderr, "Invalid Outer rnd_pline_t: failed orient\n");
		rnd_poly_valid_report(p->contours, p->contours->head, NULL);
#endif
		return rnd_false;
	}

	if (rnd_polyarea_contour_check_(p->contours, &chk)) {
#ifndef NDEBUG
		rnd_fprintf(stderr, "Invalid Outer rnd_pline_t: failed contour check\n");
		rnd_poly_valid_report(p->contours, p->contours->head, &chk);
#endif
		return rnd_false;
	}

	for (c = p->contours->next; c != NULL; c = c->next) {
		if (c->Flags.orient == RND_PLF_DIR) {
#ifndef NDEBUG
			rnd_fprintf(stderr, "Invalid Inner: rnd_pline_t orient = %d\n", c->Flags.orient);
			rnd_poly_valid_report(c, c->head, NULL);
#endif
			return rnd_false;
		}
		if (rnd_polyarea_contour_check_(c, &chk)) {
#ifndef NDEBUG
			rnd_fprintf(stderr, "Invalid Inner: failed contour check\n");
			rnd_poly_valid_report(c, c->head, &chk);
#endif
			return rnd_false;
		}
		if (!rnd_poly_contour_in_contour(p->contours, c)) {
#ifndef NDEBUG
			rnd_fprintf(stderr, "Invalid Inner: overlap with outer\n");
			rnd_poly_valid_report(c, c->head, NULL);
#endif
			return rnd_false;
		}
	}
	return rnd_true;
}


rnd_vector_t rnd_vect_zero = { (long) 0, (long) 0 };

/*********************************************************************/
/*             L o n g   V e c t o r   S t u f f                     */
/*********************************************************************/

void vect_init(rnd_vector_t v, double x, double y)
{
	v[0] = (long) x;
	v[1] = (long) y;
}																/* vect_init */

#define Vzero(a)   ((a)[0] == 0. && (a)[1] == 0.)

#define Vsub(a,b,c) {(a)[0]=(b)[0]-(c)[0];(a)[1]=(b)[1]-(c)[1];}

int vect_equal(rnd_vector_t v1, rnd_vector_t v2)
{
	return (v1[0] == v2[0] && v1[1] == v2[1]);
}																/* vect_equal */


void vect_sub(rnd_vector_t res, rnd_vector_t v1, rnd_vector_t v2)
{
Vsub(res, v1, v2)}							/* vect_sub */

void vect_min(rnd_vector_t v1, rnd_vector_t v2, rnd_vector_t v3)
{
	v1[0] = (v2[0] < v3[0]) ? v2[0] : v3[0];
	v1[1] = (v2[1] < v3[1]) ? v2[1] : v3[1];
}																/* vect_min */

void vect_max(rnd_vector_t v1, rnd_vector_t v2, rnd_vector_t v3)
{
	v1[0] = (v2[0] > v3[0]) ? v2[0] : v3[0];
	v1[1] = (v2[1] > v3[1]) ? v2[1] : v3[1];
}																/* vect_max */

double rnd_vect_len2(rnd_vector_t v)
{
	return ((double) v[0] * v[0] + (double) v[1] * v[1]);	/* why sqrt? only used for compares */
}

double rnd_vect_dist2(rnd_vector_t v1, rnd_vector_t v2)
{
	double dx = v1[0] - v2[0];
	double dy = v1[1] - v2[1];

	return (dx * dx + dy * dy);		/* why sqrt */
}

/* value has sign of angle between vectors */
double rnd_vect_det2(rnd_vector_t v1, rnd_vector_t v2)
{
	return (((double) v1[0] * v2[1]) - ((double) v2[0] * v1[1]));
}

static double vect_m_dist(rnd_vector_t v1, rnd_vector_t v2)
{
	double dx = v1[0] - v2[0];
	double dy = v1[1] - v2[1];
	double dd = (dx * dx + dy * dy);	/* sqrt */

	if (dx > 0)
		return +dd;
	if (dx < 0)
		return -dd;
	if (dy > 0)
		return +dd;
	return -dd;
}																/* vect_m_dist */

/*
vect_inters2
 (C) 1993 Klamer Schutte
 (C) 1997 Michael Leonov, Alexey Nikitin
*/

int rnd_vect_inters2(rnd_vector_t p1, rnd_vector_t p2, rnd_vector_t q1, rnd_vector_t q2, rnd_vector_t S1, rnd_vector_t S2)
{
	double s, t, deel;
	double rpx, rpy, rqx, rqy;

	if (max(p1[0], p2[0]) < min(q1[0], q2[0]) ||
			max(q1[0], q2[0]) < min(p1[0], p2[0]) || max(p1[1], p2[1]) < min(q1[1], q2[1]) || max(q1[1], q2[1]) < min(p1[1], p2[1]))
		return 0;

	rpx = p2[0] - p1[0];
	rpy = p2[1] - p1[1];
	rqx = q2[0] - q1[0];
	rqy = q2[1] - q1[1];

	deel = rpy * rqx - rpx * rqy;	/* -vect_det(rp,rq); */

	/* coordinates are 30-bit integers so deel will be exactly zero
	 * if the lines are parallel
	 */

	if (deel == 0) {							/* parallel */
		double dc1, dc2, d1, d2, h;	/* Check to see whether p1-p2 and q1-q2 are on the same line */
		rnd_vector_t hp1, hq1, hp2, hq2, q1p1, q1q2;

		Vsub2(q1p1, q1, p1);
		Vsub2(q1q2, q1, q2);


		/* If this product is not zero then p1-p2 and q1-q2 are not on same line! */
		if (rnd_vect_det2(q1p1, q1q2) != 0)
			return 0;
		dc1 = 0;										/* m_len(p1 - p1) */

		dc2 = vect_m_dist(p1, p2);
		d1 = vect_m_dist(p1, q1);
		d2 = vect_m_dist(p1, q2);

/* Sorting the independent points from small to large */
		Vcpy2(hp1, p1);
		Vcpy2(hp2, p2);
		Vcpy2(hq1, q1);
		Vcpy2(hq2, q2);
		if (dc1 > dc2) {						/* hv and h are used as help-variable. */
			Vswp2(hp1, hp2);
			h = dc1, dc1 = dc2, dc2 = h;
		}
		if (d1 > d2) {
			Vswp2(hq1, hq2);
			h = d1, d1 = d2, d2 = h;
		}

/* Now the line-pieces are compared */

		if (dc1 < d1) {
			if (dc2 < d1)
				return 0;
			if (dc2 < d2) {
				Vcpy2(S1, hp2);
				Vcpy2(S2, hq1);
			}
			else {
				Vcpy2(S1, hq1);
				Vcpy2(S2, hq2);
			};
		}
		else {
			if (dc1 > d2)
				return 0;
			if (dc2 < d2) {
				Vcpy2(S1, hp1);
				Vcpy2(S2, hp2);
			}
			else {
				Vcpy2(S1, hp1);
				Vcpy2(S2, hq2);
			};
		}
		return (Vequ2(S1, S2) ? 1 : 2);
	}
	else {												/* not parallel */
		/*
		 * We have the lines:
		 * l1: p1 + s(p2 - p1)
		 * l2: q1 + t(q2 - q1)
		 * And we want to know the intersection point.
		 * Calculate t:
		 * p1 + s(p2-p1) = q1 + t(q2-q1)
		 * which is similar to the two equations:
		 * p1x + s * rpx = q1x + t * rqx
		 * p1y + s * rpy = q1y + t * rqy
		 * Multiplying these by rpy resp. rpx gives:
		 * rpy * p1x + s * rpx * rpy = rpy * q1x + t * rpy * rqx
		 * rpx * p1y + s * rpx * rpy = rpx * q1y + t * rpx * rqy
		 * Subtracting these gives:
		 * rpy * p1x - rpx * p1y = rpy * q1x - rpx * q1y + t * ( rpy * rqx - rpx * rqy )
		 * So t can be isolated:
		 * t = (rpy * ( p1x - q1x ) + rpx * ( - p1y + q1y )) / ( rpy * rqx - rpx * rqy )
		 * and s can be found similarly
		 * s = (rqy * (q1x - p1x) + rqx * (p1y - q1y))/( rqy * rpx - rqx * rpy)
		 */

		if (Vequ2(q1, p1) || Vequ2(q1, p2)) {
			S1[0] = q1[0];
			S1[1] = q1[1];
		}
		else if (Vequ2(q2, p1) || Vequ2(q2, p2)) {
			S1[0] = q2[0];
			S1[1] = q2[1];
		}
		else {
			s = (rqy * (p1[0] - q1[0]) + rqx * (q1[1] - p1[1])) / deel;
			if (s < 0 || s > 1.)
				return 0;
			t = (rpy * (p1[0] - q1[0]) + rpx * (q1[1] - p1[1])) / deel;
			if (t < 0 || t > 1.)
				return 0;

			S1[0] = q1[0] + ROUND(t * rqx);
			S1[1] = q1[1] + ROUND(t * rqy);
		}
		return 1;
	}
}																/* vect_inters2 */

/*
 * rnd_pline_isect_line()
 * (C) 2017, 2018 Tibor 'Igor2' Palinkas
*/

typedef struct {
	rnd_vector_t l1, l2;
	rnd_coord_t cx, cy;
} pline_isect_line_t;

static rnd_r_dir_t pline_isect_line_cb(const rnd_box_t * b, void *cl)
{
	pline_isect_line_t *ctx = (pline_isect_line_t *)cl;
	struct seg *s = (struct seg *)b;
	rnd_vector_t S1, S2;

	if (rnd_vect_inters2(s->v->point, s->v->next->point, ctx->l1, ctx->l2, S1, S2)) {
		ctx->cx = S1[0];
		ctx->cy = S1[1];
		return RND_R_DIR_CANCEL; /* found */
	}

	return RND_R_DIR_NOT_FOUND;
}

rnd_bool rnd_pline_isect_line(rnd_pline_t *pl, rnd_coord_t lx1, rnd_coord_t ly1, rnd_coord_t lx2, rnd_coord_t ly2, rnd_coord_t *cx, rnd_coord_t *cy)
{
	pline_isect_line_t ctx;
	rnd_box_t lbx;
	ctx.l1[0] = lx1; ctx.l1[1] = ly1;
	ctx.l2[0] = lx2; ctx.l2[1] = ly2;
	lbx.X1 = MIN(lx1, lx2);
	lbx.Y1 = MIN(ly1, ly2);
	lbx.X2 = MAX(lx1, lx2);
	lbx.Y2 = MAX(ly1, ly2);

	if (pl->tree == NULL)
		pl->tree = (rnd_rtree_t *) rnd_poly_make_edge_tree(pl);

	if (rnd_r_search(pl->tree, &lbx, NULL, pline_isect_line_cb, &ctx, NULL) == RND_R_DIR_CANCEL) {
		if (cx != NULL) *cx = ctx.cx;
		if (cy != NULL) *cy = ctx.cy;
		return rnd_true;
	}
	return rnd_false;
}

/*
 * rnd_pline_isect_circle()
 * (C) 2017 Tibor 'Igor2' Palinkas
*/

typedef struct {
	rnd_coord_t cx, cy, r;
	double r2;
} pline_isect_circ_t;

static rnd_r_dir_t pline_isect_circ_cb(const rnd_box_t * b, void *cl)
{
	pline_isect_circ_t *ctx = (pline_isect_circ_t *)cl;
	struct seg *s = (struct seg *)b;
	rnd_vector_t S1, S2;
	rnd_vector_t ray1, ray2;
	double ox, oy, dx, dy, l;

	/* Cheap: if either line endpoint is within the circle, we sure have an intersection */
	if ((RND_SQUARE(s->v->point[0] - ctx->cx) + RND_SQUARE(s->v->point[1] - ctx->cy)) <= ctx->r2)
		return RND_R_DIR_CANCEL; /* found */
	if ((RND_SQUARE(s->v->next->point[0] - ctx->cx) + RND_SQUARE(s->v->next->point[1] - ctx->cy)) <= ctx->r2)
		return RND_R_DIR_CANCEL; /* found */

	dx = s->v->point[0] - s->v->next->point[0];
	dy = s->v->point[1] - s->v->next->point[1];
	l = sqrt(RND_SQUARE(dx) + RND_SQUARE(dy));
	ox = -dy / l * (double)ctx->r;
	oy = dx / l * (double)ctx->r;

	ray1[0] = ctx->cx - ox; ray1[1] = ctx->cy - oy;
	ray2[0] = ctx->cx + ox; ray2[1] = ctx->cy + oy;

	if (rnd_vect_inters2(s->v->point, s->v->next->point, ray1, ray2, S1, S2))
		return RND_R_DIR_CANCEL; /* found */

	return RND_R_DIR_NOT_FOUND;
}

rnd_bool rnd_pline_isect_circ(rnd_pline_t *pl, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r)
{
	pline_isect_circ_t ctx;
	rnd_box_t cbx;
	ctx.cx = cx; ctx.cy = cy;
	ctx.r = r; ctx.r2 = (double)r * (double)r;
	cbx.X1 = cx - r; cbx.Y1 = cy - r;
	cbx.X2 = cx + r; cbx.Y2 = cy + r;

	if (pl->tree == NULL)
		pl->tree = (rnd_rtree_t *) rnd_poly_make_edge_tree(pl);

	return rnd_r_search(pl->tree, &cbx, NULL, pline_isect_circ_cb, &ctx, NULL) == RND_R_DIR_CANCEL;
}


/*
 * rnd_pline_embraces_circle()
 * If the circle does not intersect the polygon (the caller needs to check this)
 * return whether the circle is fully within the polygon or not.
 * Shoots a ray to the right from center+radius, then one to the left from
 * center-radius; if both ray cross odd number of pline segments, we are in
 * (or intersecting).
 * (C) 2017 Tibor 'Igor2' Palinkas
*/
typedef struct {
	int cnt;
	rnd_coord_t cx, cy;
	int dx;
} pline_embrace_t;

static rnd_r_dir_t pline_embraces_circ_cb(const rnd_box_t * b, void *cl)
{
	pline_embrace_t *e = cl;
	struct seg *s = (struct seg *)b;
	double dx;
	rnd_coord_t lx;
	rnd_coord_t x1 = s->v->point[0];
	rnd_coord_t y1 = s->v->point[1];
	rnd_coord_t x2 = s->v->next->point[0];
	rnd_coord_t y2 = s->v->next->point[1];

	/* ray is below or above the line - no chance */
	if ((e->cy < y1) && (e->cy < y2))
		return RND_R_DIR_NOT_FOUND;
	if ((e->cy >= y1) && (e->cy >= y2))
		return RND_R_DIR_NOT_FOUND;

	/* calculate line's X for e->cy */
	dx = (double)(x2 - x1) / (double)(y2 - y1);
	lx = rnd_round((double)x1 + (double)(e->cy - y1) * dx);
	if (e->dx < 0) { /* going left */
		if (lx < e->cx)
			e->cnt++;
	}
	else { /* going right */
		if (lx > e->cx)
			e->cnt++;
	}

	return RND_R_DIR_NOT_FOUND;
}

rnd_bool rnd_pline_embraces_circ(rnd_pline_t *pl, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r)
{
	rnd_box_t bx;
	pline_embrace_t e;

	bx.Y1 = cy; bx.Y2 = cy+1;
	e.cy = cy;
	if (pl->tree == NULL)
		pl->tree = (rnd_rtree_t *) rnd_poly_make_edge_tree(pl);

	/* ray to the right */
	bx.X1 = e.cx = cx + r;
	bx.X2 = RND_COORD_MAX;
	e.dx = +1;
	e.cnt = 0;
	rnd_r_search(pl->tree, &bx, NULL, pline_embraces_circ_cb, &e, NULL);
	if ((e.cnt % 2) == 0)
		return rnd_false;

	/* ray to the left */
	bx.X1 = -RND_COORD_MAX;
	bx.X2 = e.cx = cx - r;
	e.dx = -1;
	e.cnt = 0;
	rnd_r_search(pl->tree, &bx, NULL, pline_embraces_circ_cb, &e, NULL);
	if ((e.cnt % 2) == 0)
		return rnd_false;

	return rnd_true;
}

/*
 * rnd_pline_isect_circle()
 * (C) 2017 Tibor 'Igor2' Palinkas
*/
rnd_bool rnd_pline_overlaps_circ(rnd_pline_t *pl, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r)
{
	rnd_box_t cbx, pbx;
	cbx.X1 = cx - r; cbx.Y1 = cy - r;
	cbx.X2 = cx + r; cbx.Y2 = cy + r;
	pbx.X1 = pl->xmin; pbx.Y1 = pl->ymin;
	pbx.X2 = pl->xmax; pbx.Y2 = pl->ymax;

	/* if there's no overlap in bounding boxes, don't do any expensive calc */
	if (!(rnd_box_intersect(&cbx, &pbx)))
		return rnd_false;

	if (pl->tree == NULL)
		pl->tree = (rnd_rtree_t *) rnd_poly_make_edge_tree(pl);

	if (rnd_pline_isect_circ(pl, cx, cy, r))
		return rnd_true;

	return rnd_pline_embraces_circ(pl, cx, cy, r);
}


/*
 * rnd_is_point_in_convex_quad()
 * (C) 2017 Tibor 'Igor2' Palinkas
*/
rnd_bool_t rnd_is_point_in_convex_quad(rnd_vector_t p, rnd_vector_t *q)
{
	return point_in_triangle(q[0], q[1], q[2], p) || point_in_triangle(q[0], q[3], q[2], p);
}


/*
 * rnd_polyarea_move()
 * (C) 2017 Tibor 'Igor2' Palinkas
*/
void rnd_polyarea_move(rnd_polyarea_t *pa1, rnd_coord_t dx, rnd_coord_t dy)
{
	int cnt;
	rnd_polyarea_t *pa;

	for (pa = pa1, cnt = 0; pa != NULL; pa = pa->f) {
		rnd_pline_t *pl;
		if (pa == pa1) {
			cnt++;
			if (cnt > 1)
				break;
		}
		if (pa->contour_tree != NULL)
			rnd_r_destroy_tree(&pa->contour_tree);
		pa->contour_tree = rnd_r_create_tree();
		for(pl = pa->contours; pl != NULL; pl = pl->next) {
			rnd_vnode_t *v;
			int cnt2 = 0;
			for(v = pl->head; v != NULL; v = v->next) {
				if (v == pl->head) {
					cnt2++;
					if (cnt2 > 1)
						break;
				}
				v->point[0] += dx;
				v->point[1] += dy;
			}
			pl->xmin += dx;
			pl->ymin += dy;
			pl->xmax += dx;
			pl->ymax += dy;
			if (pl->tree != NULL) {
				rnd_r_free_tree_data(pl->tree, free);
				rnd_r_destroy_tree(&pl->tree);
			}
			pl->tree = (rnd_rtree_t *)rnd_poly_make_edge_tree(pl);

			rnd_r_insert_entry(pa->contour_tree, (rnd_box_t *)pl);
		}
	}
}

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
