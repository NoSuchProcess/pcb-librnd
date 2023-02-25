/*
       Copyright (C) 2006 harry eaton

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

	TODO("arc: this is where an arc-arc or line-arc or arc-line intersection would be detected then new point added");
	cnt = rnd_vect_inters2(s->v->point, s->v->next->point, i->v->point, i->v->next->point, s1, s2);
	if (!cnt)
		return RND_R_DIR_NOT_FOUND;
	if (i->touch)									/* if checking touches one find and we're done */
		longjmp(*i->touch, TOUCHES);
	i->s->p->Flags.lstatus = PA_PLS_ISECTED;
	s->p->Flags.lstatus = PA_PLS_ISECTED;
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
