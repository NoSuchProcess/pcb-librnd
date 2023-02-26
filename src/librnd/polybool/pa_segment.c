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

/* some structures for handling segment intersections using the rtrees */

/* A segment is an object of the pline between ->v and ->v->next, upgraded
   with a bounding box so that it can be added to an rtree. Also remembers
   its parent pline, plus random temporary fields used in the calculations */
typedef struct pa_seg_s {
	rnd_box_t box;
	rnd_vnode_t *v;
	rnd_pline_t *p;
	int intersected;
} pa_seg_t;

/* used by pcb-rnd */
rnd_vnode_t *rnd_pline_seg2vnode(void *box)
{
	pa_seg_t *seg = box;
	return seg->v;
}

/* Recalculate bbox of the segment from current coords */
RND_INLINE void pa_seg_update_bbox(pa_seg_t *s)
{
	TODO("arc: the code below works only for lines");
	s->box.X1 = pa_min(s->v->point[0], s->v->next->point[0]);
	s->box.X2 = pa_max(s->v->point[0], s->v->next->point[0]) + 1;
	s->box.Y1 = pa_min(s->v->point[1], s->v->next->point[1]);
	s->box.Y2 = pa_max(s->v->point[1], s->v->next->point[1]) + 1;
}

/* Replace sg with two new segments in tree ; called after a vertex has
   been added. Return 0 on success. */
static int pa_adjust_tree(rnd_rtree_t *tree, pa_seg_t *sg)
{
	pa_seg_t *newseg;
	rnd_vnode_t *sg_v_next = sg->v->next; /* remember original sg field because sg will be repurposed */

	newseg = malloc(sizeof(pa_seg_t));
	if (newseg == NULL)
		return 1;

	/* remove as we are replacing it with two objects */
	rnd_r_delete_entry(tree, (const rnd_box_t *)sg);

	/* init and insert newly allocated seg from v */
	newseg->intersected = 0;
	newseg->v = sg->v;
	newseg->p = sg->p;
	pa_seg_update_bbox(newseg);
	rnd_r_insert_entry(tree, (const rnd_box_t *)newseg);

	/* instead of free(sg) and malloc() newseg, repurpose sg as newseg for v->next */
	newseg = sg;
	newseg->intersected = 0;
	newseg->v = sg_v_next;
/*	newseg->p = sg->p; - it's still the same*/
	pa_seg_update_bbox(newseg);
	rnd_r_insert_entry(tree, (const rnd_box_t *)newseg);

	return 0;
}

void *rnd_poly_make_edge_tree(rnd_pline_t *pl)
{
	rnd_vnode_t *bv = pl->head;
	rnd_rtree_t *res = rnd_r_create_tree();

	do {
		pa_seg_t *s = malloc(sizeof(pa_seg_t));
		s->intersected = 0;
		s->v = bv;
		s->p = pl;
		pa_seg_update_bbox(s);
		rnd_r_insert_entry(res, (const rnd_box_t *)s);
	} while ((bv = bv->next) != pl->head);

	return (void *)res;
}

/*** seg-in-seg search helpers */

typedef struct pa_insert_node_task_s pa_insert_node_task_t;
struct pa_insert_node_task_s {
	pa_insert_node_task_t *next;
	pa_seg_t *node_seg;
	rnd_vnode_t *new_node;
};

typedef struct pa_seg_seg_s {
	double m, b;
	rnd_rtree_t *tree;
	rnd_vnode_t *v;
	pa_seg_t *s;
	jmp_buf *env, sego, *touch;
	int need_restart;
	pa_insert_node_task_t *node_insert_list;
} pa_seg_seg_t;

/* Prune the search for boxes that don't intersect the segment's box. */
static rnd_r_dir_t seg_in_region_cb(const rnd_box_t *b, void *cl)
{
	pa_seg_seg_t *ctx = (pa_seg_seg_t *)cl;
	double y1, y2;

	/* zero slope means axis aligned so it is already pruned */
	if (ctx->m == 0.0)         return RND_R_DIR_FOUND_CONTINUE;

	y1 = ctx->m * b->X1 + ctx->b;
	y2 = ctx->m * b->X2 + ctx->b;

	/* check if y1 or y2 is out of range */
	if (pa_min(y1, y2) >= b->Y2)  return RND_R_DIR_NOT_FOUND;
	if (pa_max(y1, y2) < b->Y1)   return RND_R_DIR_NOT_FOUND;

	return RND_R_DIR_FOUND_CONTINUE;
}

/* Allocate and prepend a deferred node-insertion task to a task list */
RND_INLINE pa_insert_node_task_t *prepend_node_task(pa_insert_node_task_t *list, pa_seg_t *seg, rnd_vnode_t *new_node)
{
	pa_insert_node_task_t *res = malloc(sizeof(pa_insert_node_task_t));

	res->next = list;
	res->node_seg = seg;
	res->new_node = new_node;

	return res;
}

/* Execute pending tasks from the tasklist (creating new nodes) and free
   tasklist. Return number of tasks executed. */
RND_INLINE long pa_exec_node_tasks(pa_insert_node_task_t *tasklist)
{
	pa_insert_node_task_t *t, *next;
	long modified = 0;

	for(t = tasklist; t != NULL; t = next) {
		next = t->next;

		/* perform the insertion */
		t->new_node->prev = t->node_seg->v;
		t->new_node->next = t->node_seg->v->next;
		t->node_seg->v->next->prev = t->new_node;
		t->node_seg->v->next = t->new_node;
		t->node_seg->p->Count++;

		pa_pline_box_bump(t->node_seg->p, t->new_node->point);

		if (pa_adjust_tree(t->node_seg->p->tree, t->node_seg) != 0)
			assert(0); /* failed memory allocation */

		modified++;

		free(t);
	}

	return modified;
}


/* This callback checks if the segment "s" in the tree intersect
   the search segment "i". If it does, both plines are marked as intersected
   and the point is marked for the connectivity list. If the point is not
   already a vertex, a new vertex is inserted and the search for intersections
   starts over at the beginning.

   (That is potentially a significant time penalty, but it does solve the snap
   rounding problem. There are efficient algorithms for finding intersections
   with snap rounding, but I don't have time to implement them right now.)
 */
static rnd_r_dir_t seg_in_seg_cb(const rnd_box_t *b, void *cl)
{
	pa_seg_seg_t *ctx = (pa_seg_seg_t *)cl;
	pa_seg_t *s = (pa_seg_t *)b;
	rnd_vector_t isc1, isc2;
	int num_isc;

	/* When new nodes are added at the end of a pass due to an intersection
	   the segments may be altered. If either segment we're looking at has
	   already been intersected this pass, skip it until the next pass. */
	if (s->intersected || ctx->s->intersected)
		return RND_R_DIR_NOT_FOUND;

	TODO("arc: this is where an arc-arc or line-arc or arc-line intersection would be detected then new point added");
	num_isc = rnd_vect_inters2(s->v->point, s->v->next->point, ctx->v->point, ctx->v->next->point, isc1, isc2);
	if (!num_isc)
		return RND_R_DIR_NOT_FOUND;

 /* if checking touches the first intersection decides the result */
	if (ctx->touch != NULL)
		longjmp(*ctx->touch, PA_ISC_TOUCHES);

	ctx->s->p->flg.llabel = PA_PLL_ISECTED;
	s->p->flg.llabel = PA_PLL_ISECTED;

	for (; num_isc > 0; num_isc--) {
		rnd_bool done_insert_on_i = rnd_false;
		rnd_vector_t *my_s = (num_isc == 1 ? &isc1 : &isc2);
		rnd_vnode_t *new_node;

		/* add new node on "i" */
		new_node = pa_ensure_point_and_reset_cvc(ctx->v, *my_s);
		if (new_node != NULL) {
#ifdef DEBUG_INTERSECT
			DEBUGP("new intersection on segment \"i\" at %#mD\n", (*my_s)[0], (*my_s)[1]);
#endif
			ctx->node_insert_list = prepend_node_task(ctx->node_insert_list, ctx->s, new_node);
			ctx->s->intersected = 1;
			done_insert_on_i = rnd_true;
		}

		/* add new node on "s" */
		new_node = pa_ensure_point_and_reset_cvc(s->v, *my_s);
		if (new_node != NULL) {
#ifdef DEBUG_INTERSECT
			DEBUGP("new intersection on segment \"s\" at %#mD\n", (*my_s)[0], (*my_s)[1]);
#endif
			ctx->node_insert_list = prepend_node_task(ctx->node_insert_list, s, new_node);
			s->intersected = 1;
			return RND_R_DIR_NOT_FOUND; /* Keep looking for intersections with segment "i" */
		}

		/* Skip any remaining r_search hits against segment "i", as any further
		   intersections will be rejected until the next pass anyway. */
		if (done_insert_on_i)
			longjmp(*ctx->env, 1);
	}
	return RND_R_DIR_NOT_FOUND;
}

/* Cancels the search and sets ctx->s if b is the same as ctx->v. Usefule
   for finding the segment for a node */
static rnd_r_dir_t pa_get_seg_cb(const rnd_box_t *b, void *ctx_)
{
	pa_seg_seg_t *ctx = (pa_seg_seg_t *)ctx_;
	pa_seg_t *s = (pa_seg_t *)b;

	if (ctx->v == s->v) {
		ctx->s = s;
		return RND_R_DIR_CANCEL; /* found */
	}
	return RND_R_DIR_NOT_FOUND;
}

/* Find the segment for pt in tree; result is filled in segctx. Caller should
   initialize the following fields of segctx: env, touch, need_restart,
   node_insert_list */
RND_INLINE void pa_find_seg_for_pt(rnd_rtree_t *tree, rnd_vnode_t *pt, pa_seg_seg_t *segctx)
{
	rnd_r_dir_t rres;
	rnd_box_t box;
	double dx = pt->next->point[0] - pt->point[0]; /* compute the slant for region trimming */

	segctx->v = pt;
	if (dx != 0) {
		segctx->m = (pt->next->point[1] - pt->point[1]) / dx;
		segctx->b = pt->point[1] - segctx->m * pt->point[0];
	}
	else
		segctx->m = 0;

	box.X1 = pt->point[0];    box.Y1 = pt->point[1];
	box.X2 = box.X1 + 1;      box.Y2 = box.Y1 + 1;
	rres = rnd_r_search(tree, &box, NULL, pa_get_seg_cb, segctx, NULL);
	assert(rres == RND_R_DIR_CANCEL);
}
