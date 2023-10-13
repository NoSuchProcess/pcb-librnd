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

/* collect the resulting plines and polyareas */

#define PB_OPTIMIZE

typedef enum pa_direction_e { PA_FORWARD, PA_BACKWARD } pa_direction_t;
typedef int (*pa_start_rule_t)(rnd_vnode_t *, pa_direction_t *);
typedef int (*pa_jump_rule_t)(char poly, rnd_vnode_t *, pa_direction_t *);

/*** Per operation start/jump rules ***/

/* As per table 3 in the original paper */

static int pa_rule_unite_start(rnd_vnode_t *cur, pa_direction_t *initdir)
{
	*initdir = PA_FORWARD;
	return (cur->flg.plabel == PA_PTL_OUTSIDE) || (cur->flg.plabel == PA_PTL_SHARED);
}

static int pa_rule_unite_jump(char p, rnd_vnode_t *v, pa_direction_t *cdir)
{
	assert(*cdir == PA_FORWARD);
	return (v->flg.plabel == PA_PTL_OUTSIDE) || (v->flg.plabel == PA_PTL_SHARED);
}

static int pa_rule_isect_start(rnd_vnode_t *cur, pa_direction_t *initdir)
{
	*initdir = PA_FORWARD;
	return (cur->flg.plabel == PA_PTL_INSIDE) || (cur->flg.plabel == PA_PTL_SHARED);
}

static int pa_rule_isect_jump(char p, rnd_vnode_t *v, pa_direction_t *cdir)
{
	assert(*cdir == PA_FORWARD);
	return (v->flg.plabel == PA_PTL_INSIDE) || (v->flg.plabel == PA_PTL_SHARED);
}

static int pa_rule_sub_start(rnd_vnode_t *cur, pa_direction_t *initdir)
{
	/* this is called only on poly A (in Table 3) */
	*initdir = PA_FORWARD;
	return (cur->flg.plabel == PA_PTL_OUTSIDE) || (cur->flg.plabel == PA_PTL_SHARED2);
}

static int pa_rule_sub_jump(char p, rnd_vnode_t *v, pa_direction_t *cdir)
{
	if ((p == 'B') && (v->flg.plabel == PA_PTL_INSIDE)) {
		*cdir = PA_BACKWARD;
		return rnd_true;
	}

	if ((p == 'A') && (v->flg.plabel == PA_PTL_OUTSIDE)) {
		*cdir = PA_FORWARD;
		return rnd_true;
	}

	if (v->flg.plabel == PA_PTL_SHARED2) {
		*cdir = (p == 'A') ? PA_FORWARD : PA_BACKWARD;
		return rnd_true;
	}

	return rnd_false;
}

static int pa_rule_xor_start(rnd_vnode_t *cur, pa_direction_t *initdir)
{
	if (cur->flg.plabel == PA_PTL_INSIDE) {
		*initdir = PA_BACKWARD;
		return rnd_true;
	}

	if (cur->flg.plabel == PA_PTL_OUTSIDE) {
		*initdir = PA_FORWARD;
		return rnd_true;
	}

	return rnd_false;
}

static int pa_rule_xor_jump(char p, rnd_vnode_t *v, pa_direction_t *cdir)
{
	if (v->flg.plabel == PA_PTL_INSIDE) {
		*cdir = PA_BACKWARD;
		return rnd_true;
	}

	if (v->flg.plabel == PA_PTL_OUTSIDE) {
		*cdir = PA_FORWARD;
		return rnd_true;
	}

	return rnd_false;
}


/* Jump to next edge to consider; returns whether an edge is found.
   (For direction PA_BACKWARD, return the next vertex so that prev vertex has
   the flags for the edge.) */
static int pa_coll_jump(rnd_vnode_t **cur, pa_direction_t *cdir, pa_jump_rule_t rule)
{
	pa_conn_desc_t *d, *start;

	if ((*cur)->cvclst_prev == NULL) { /* not a cross-vertex, proceed normally */
		int marked = ((*cdir == PA_FORWARD) ? (*cur)->flg.mark : (*cur)->prev->flg.mark);
		return !marked; /* This is the loop terminating condition in Collect() of the original paper */
	}

	/* cross-vertex means we are at an intersection and have to decide which
	   edge to continue at */
	DEBUG_JUMP("jump entering node at %$mD\n", (*cur)->point[0], (*cur)->point[1]);

	start = d = (*cdir == PA_FORWARD) ? (*cur)->cvclst_prev->prev : (*cur)->cvclst_next->prev;
	do {
		pa_direction_t newdir = *cdir;
		rnd_vnode_t *e = d->parent;

		if (d->side == 'P')
			e = e->prev;

		if (!e->flg.mark && rule(d->poly, e, &newdir)) {
			if (((d->side == 'N') && (newdir == PA_FORWARD)) || ((d->side == 'P') && (newdir == PA_BACKWARD))) {
				rnd_vnode_t *nnd = (newdir == PA_FORWARD) ? e->next : e;
				DEBUG_JUMP("jump leaving node at %$mD\n", nnd->point[0], nnd->point[1]);

				*cur = d->parent;
				*cdir = newdir;
				return rnd_true;
			}
		}
	} while((d = d->prev) != start);

	return rnd_false;
}

/* Returns 1 if bigcoord is enabled and nd is not on an exact integer coordinate */
RND_INLINE int pa_is_node_coords_non_integer(rnd_vnode_t *nd)
{
/* If we have an intersection, we have a high resolution coord; if that coord
   is not integer, we had to do a rounding to get the output integer coord */
#ifdef PA_BIGCOORD_ISC
	if (nd->cvclst_next == NULL)
		return 0;

	return !pa_big_vect_is_int(nd->cvclst_next->isc);
#endif

	return 0;
}

#define PA_NEXT_NODE(nd, dir) (((dir) == PA_FORWARD) ? (nd)->next : (nd)->prev)

/* This is Collect() in the original paper */
RND_INLINE int pa_coll_gather(rnd_vnode_t *start, rnd_pline_t **result, pa_jump_rule_t v_rule, pa_direction_t dir)
{
	rnd_vnode_t *nd, *newnd;

	DEBUG_GATHER("gather direction = %d\n", dir);
	*result = NULL;

	/* Run nd from start hopping to next node */
	for(nd = start; pa_coll_jump(&nd, &dir, v_rule); nd = PA_NEXT_NODE(nd, dir)) {

		/* add edge to pline */
		if (*result == NULL) { /* create first */
			*result = pa_pline_new(nd->point);
			if (*result == NULL)
				return pa_err_no_memory;
			newnd = (*result)->head;
			newnd->flg.rounded = nd->flg.rounded | pa_is_node_coords_non_integer(nd);
		}
		else { /* insert subsequent */
			newnd = rnd_poly_node_create(nd->point);
			if (newnd == NULL)
				return pa_err_no_memory;
			newnd->flg.rounded = nd->flg.rounded | pa_is_node_coords_non_integer(nd);
			rnd_poly_vertex_include((*result)->head->prev, newnd);
		}

		DEBUG_GATHER("gather vertex at %$mD\n", nd->point[0], nd->point[1]);

		/* mark the edge as included; mark both if SHARED edge */
		newnd = (dir == PA_FORWARD) ? nd : nd->prev;
		newnd->flg.mark = 1;
		if (newnd->shared != NULL)
			newnd->shared->flg.mark = 1;
	}

	return pa_err_ok;
}

RND_INLINE void pa_collect_gather(jmp_buf *e, rnd_vnode_t *cur, pa_direction_t dir, rnd_polyarea_t ** contours, rnd_pline_t **holes, pa_jump_rule_t j_rule)
{
	rnd_vnode_t *vn;
	rnd_pline_t *pl;
	int res;

	/* gather a polyline in pl */
	vn = (dir == PA_FORWARD) ? cur : cur->next;
	res = pa_coll_gather(vn, &pl, j_rule, dir);
	if (res != pa_err_ok) {
		pa_pline_free(&pl);
		pa_error(res);
	}

	if (pl == NULL)
		return;

	pa_pline_update(pl, rnd_true);

	if (pl->Count > 2) {
		DEBUG_GATHER("adding contour with %d vertices and direction %c\n", pl->Count, (pl->flg.orient ? 'F' : 'B'));
		put_contour(e, pl, contours, holes, NULL, NULL, NULL);
	}
	else {
		DEBUG_GATHER("Bad contour! Not enough points!!\n");
		pa_pline_free(&pl);
	}
}

RND_INLINE rnd_bool pa_collect_pline(jmp_buf *e, rnd_pline_t *a, rnd_polyarea_t **contours, rnd_pline_t **holes, pa_start_rule_t s_rule, pa_jump_rule_t j_rule)
{
	rnd_vnode_t *vn, *other;
	pa_direction_t dir;

	vn = a->head;
	do {
		if (s_rule(vn, &dir) && (vn->flg.mark == 0))
			pa_collect_gather(e, vn, dir, contours, holes, j_rule);

		other = vn;
		if ((other->cvclst_prev && pa_coll_jump(&other, &dir, j_rule)))
			pa_collect_gather(e, other, dir, contours, holes, j_rule);
	} while((vn = vn->next) != a->head);

	return rnd_false;
}

/* Remove head of *pl and add it in contours or holes; if invert is true
   invert direction before adding. Always returns true. */
RND_INLINE rnd_bool pa_coll_move_cnt(jmp_buf *e, rnd_pline_t **pl, rnd_polyarea_t **contours, rnd_pline_t **holes, rnd_polyarea_t *old_parent, rnd_polyarea_t *new_parent, rnd_pline_t *new_parent_contour, rnd_bool invert)
{
	rnd_pline_t *tmp = *pl;

	/* remove head of pl to put it in contours or holes (rtree entry removed in put_contour) */
	*pl = tmp->next;
	tmp->next = NULL;

	if (invert)
		pa_pline_invert(tmp);

	put_contour(e, tmp, contours, holes, old_parent, NULL, NULL);
	return rnd_true;
}


RND_INLINE int pa_collect_contour(jmp_buf *e, rnd_pline_t **A, rnd_polyarea_t **contours, rnd_pline_t **holes, int op, rnd_polyarea_t *old_parent, rnd_polyarea_t *parent, rnd_pline_t *parent_contour)
{
	if ((*A)->flg.llabel == PA_PLL_ISECTED) {
		/* As per table 3. in the original paper */
		switch(op) {
			case RND_PBO_UNITE: return pa_collect_pline(e, *A, contours, holes, pa_rule_unite_start, pa_rule_unite_jump);
			case RND_PBO_ISECT: return pa_collect_pline(e, *A, contours, holes, pa_rule_isect_start, pa_rule_isect_jump);
			case RND_PBO_XOR:   return pa_collect_pline(e, *A, contours, holes, pa_rule_xor_start, pa_rule_xor_jump);
			case RND_PBO_SUB:   return pa_collect_pline(e, *A, contours, holes, pa_rule_sub_start, pa_rule_sub_jump);
		}
	}
	else {
		/* As per table 2. in the original paper */
		switch (op) {
			case RND_PBO_ISECT:
				if ((*A)->flg.llabel == PA_PLL_INSIDE)
					return pa_coll_move_cnt(e, A, contours, holes, old_parent, NULL, NULL, 0);
				break;
			case RND_PBO_UNITE:
				if ((*A)->flg.llabel == PA_PLL_OUTSIDE)
					return pa_coll_move_cnt(e, A, contours, holes, old_parent, parent, parent_contour, 0);
				break;
			case RND_PBO_SUB:
				if ((*A)->flg.llabel == PA_PLL_OUTSIDE)
					return pa_coll_move_cnt(e, A, contours, holes, old_parent, parent, parent_contour, 0);
				/* the second rule that deals with the edge being in B is not implemented here as we are iterating over A */
				break;
			case RND_PBO_XOR:
				if ((*A)->flg.llabel == PA_PLL_INSIDE)
					return pa_coll_move_cnt(e, A, contours, holes, old_parent, NULL, NULL, 1);
				else
					return pa_coll_move_cnt(e, A, contours, holes, old_parent, parent, parent_contour, 0);
				break;
		}
	}

	return rnd_false;
}

/* remove pl's head and add it in the contours/holes list; returns next pline
   of pl (new head) */
RND_INLINE rnd_pline_t **pa_coll_b_area_add(jmp_buf *e, rnd_pline_t **pl, rnd_polyarea_t **contours, rnd_pline_t **holes, rnd_polyarea_t *old_parent)
{
	rnd_pline_t *tmp = *pl, **next;

	*pl = tmp->next;
	next = pl;
	tmp->next = NULL;
	tmp->flg.llabel = PA_PLL_UNKNWN;
	put_contour(e, tmp, contours, holes, old_parent, NULL, NULL);

	return next;
}

/* Collect contours of polygon 'B' in some binop */
RND_INLINE void pa_collect_b_area(jmp_buf *e, rnd_polyarea_t *B, rnd_polyarea_t **contours, rnd_pline_t **holes, int op)
{
	rnd_polyarea_t *pa = B;

	assert(pa != NULL);

	do {
		rnd_pline_t **pl, **next;

		for(pl = &pa->contours; *pl != NULL; pl = next) {
			next = &((*pl)->next);
			if ((*pl)->flg.llabel == PA_PLL_ISECTED)
				continue;

			if ((*pl)->flg.llabel == PA_PLL_INSIDE)
				switch (op) {
				case RND_PBO_XOR:
				case RND_PBO_SUB:
					pa_pline_invert(*pl);
					/* fall thru to add */

				case RND_PBO_ISECT:
					next = pa_coll_b_area_add(e, pl, contours, holes, pa);
					break;
				case RND_PBO_UNITE:
					break; /* do not add, already included */
				}
			else if ((*pl)->flg.llabel == PA_PLL_OUTSIDE)
				switch (op) {
				case RND_PBO_XOR:
				case RND_PBO_UNITE:
					next = pa_coll_b_area_add(e, pl, contours, holes, pa);
					break;
				case RND_PBO_ISECT:
				case RND_PBO_SUB:
					break; /* do not add, not to be included */
				}
		}
	} while((pa = pa->f) != B);
}

/* Prepend pl in list */
RND_INLINE void pa_coll_sepisc_link_in(rnd_pline_t *pl, rnd_pline_t **list)
{
	pl->next = *list;
	*list = pl;
}

/* Move out intersected polylines of islands (among with their holes) into
   isected/holes */
RND_INLINE void pa_polyarea_separate_intersected(jmp_buf *e, rnd_polyarea_t **islands, rnd_pline_t **holes, rnd_pline_t **isected)
{
	rnd_polyarea_t *pa, *panext;
	int finished = 0;

	for(pa = *islands; !finished && (*islands != NULL); pa = panext) {
		rnd_pline_t *pl, *plnext, *plprev = NULL;
		int hole_contour = 0, is_outline = 1;

		panext = pa->f;
		finished = (panext == *islands); /* reached back to the starting point, exit before next iteration */

		for(pl = pa->contours; pl != NULL; pl = plnext, is_outline = 0) {
			int is_first = pa_pline_is_first(pa, pl), is_last = pa_pline_is_last(pl);
			int isect_contour = (pl->flg.llabel == PA_PLL_ISECTED);

			plnext = pl->next;

			if (isect_contour || hole_contour) {
				if (pl->flg.llabel != PA_PLL_ISECTED)
					pl->flg.llabel = PA_PLL_UNKNWN;

				remove_contour(pa, plprev, pl, !(is_first && is_last));

				if (isect_contour)       pa_coll_sepisc_link_in(pl, isected);
				else if (hole_contour)   pa_coll_sepisc_link_in(pl, holes);

				/* pl was the only polyline in pa, so pa must be empty now that pl is removed */
				if (is_first && is_last) {
					pa_polyarea_unlink(islands, pa);
					pa_polyarea_free_all(&pa);
				}
			}
			else {
				/* Optimization: remember previous line on the singly linked list
				   (needed in removal) */
				plprev = pl;
			}

			/* When moving or deleting an outer contour, also move related holes */
			if (is_outline && isect_contour)
				hole_contour = 1;
		}
	}
}

/*** update primary ***/

typedef struct pa_find_pl_inside_pa_s {
	jmp_buf jb;
	rnd_polyarea_t *want_inside;
	rnd_pline_t *result;
} pa_find_pl_inside_pa_t;

/* check if a polyline is within the swarch polyarea */
static rnd_r_dir_t pa_find_pl_inside_pa_cb(const rnd_box_t *b, void *cl)
{
	pa_find_pl_inside_pa_t *info = (pa_find_pl_inside_pa_t *)cl;
	rnd_pline_t *pl = (rnd_pline_t *)b;

	/* ignore main contour (positive means outer), ignore intersecteds */
	if ((pl->flg.orient == RND_PLF_DIR) || (pl->flg.llabel == PA_PLL_ISECTED))
		return RND_R_DIR_NOT_FOUND;

	if (pa_is_pline_in_polyarea(pl, info->want_inside, rnd_false)) {
		info->result = pl;
		return RND_R_DIR_CANCEL;
	}

	return RND_R_DIR_NOT_FOUND;
}

/* returns whether pl overlaps box */
RND_INLINE rnd_bool pa_contour_in_box(rnd_pline_t *pl, rnd_box_t box)
{
	if (pl->xmin < box.X1) return 0;
	if (pl->ymin < box.Y1) return 0;
	if (pl->xmax > box.X2) return 0;
	if (pl->ymax > box.Y2) return 0;
	return 1;
}

RND_INLINE void pa_polyarea_update_primary_del_inside(jmp_buf *e, rnd_box_t box, rnd_polyarea_t **islands, rnd_pline_t **holes, rnd_polyarea_t *bpa)
{
	rnd_polyarea_t *pa, *panext;
	rnd_pline_t *pl;
	int finished = 0;

	for(pa = *islands; !finished && (*islands != NULL); pa = panext) {
		panext = pa->f;
		finished = (panext == *islands); /* reached where we started, stop next iteration */

		/* Test the outer contour first, as we may need to remove all children;
		   ignore intersected contours (bbox check is an optimization) */
		if ((pa->contours->flg.llabel != PA_PLL_ISECTED) && pa_contour_in_box(pa->contours, box) && pa_is_pline_in_polyarea(pa->contours, bpa, rnd_false)) {
			/* Delete this contour, move all children to the pending holes list */

			pl = pa->contours;
			remove_contour(pa, NULL, pl, rnd_false);
			pa_pline_free(&pl);
			/* rtree deleted in pa_polyarea_free_all() */

			/* a->contours now points to the remaining holes */
			if (pa->contours != NULL) {
				rnd_pline_t *end;
	
				/* Find the end of the list of holes so holes cna be prepended in *holes */
				for(end = pa->contours; end->next != NULL; end = end->next) ;

				end->next = *holes;
				*holes = pa->contours;
				pa->contours = NULL;
			}

			pa_polyarea_unlink(islands, pa);
			pa_polyarea_free_all(&pa);
			continue;
		}

		/* Loop whilst we find INSIDE contours to delete */
		for(;;) {
			pa_find_pl_inside_pa_t ictx;
			rnd_pline_t *prev;

			ictx.want_inside = bpa;
			ictx.result = NULL;

			/* find a hole that's inside bpa */
			rnd_r_search(pa->contour_tree, &box, NULL, pa_find_pl_inside_pa_cb, &ictx, NULL);
			if (ictx.result == NULL)
				break; /* nothing found */

			/* singly linked list; find ->prev */
			for(prev = pa->contours; prev->next != ictx.result; prev = prev->next) ;

			/* Remove hole from the contour */
			remove_contour(pa, prev, ictx.result, rnd_true);
			pa_pline_free(&ictx.result);
		}
	}
}

/* prepend pl in list */
RND_INLINE void pa_prim_delo_link_in(rnd_pline_t *pl, rnd_pline_t **list)
{
	/* prepend in holes */
	pl->next = *list;
	*list = pl;
}

RND_INLINE void pa_polyarea_update_primary_del_outside(jmp_buf *e, rnd_box_t box, rnd_polyarea_t **islands, rnd_pline_t **holes, rnd_polyarea_t *bpa, int del_outside)
{
	rnd_polyarea_t *pa, *panext;
	rnd_pline_t *pl, *plnext, *plprev;
	int finished = 0;

	for(pa = *islands; !finished && (*islands != NULL); pa = panext) {
		int hole_contour = 0, is_outline = 1;

		panext = pa->f;
		finished = (panext == *islands); /* reached where we started, stop next iteration */

		plprev = NULL;
		for(pl = pa->contours; pl != NULL; pl = plnext, is_outline = 0) {
			int is_first = pa_pline_is_first(pa, pl), is_last = pa_pline_is_last(pl);
			int del_contour = 0;

			plnext = pl->next;

			if (del_outside) {
#ifdef PB_OPTIMIZE
				del_contour = (pl->flg.llabel != PA_PLL_ISECTED) && (!pa_contour_in_box(pa->contours, box) || !pa_is_pline_in_polyarea(pl, bpa, rnd_false));
#else
				del_contour = (pl->flg.llabel != PA_PLL_ISECTED) && !pa_is_pline_in_polyarea(pl, bpa, rnd_false);
#endif
			}

			/* ignore intersected contours */
			if (pl->flg.llabel == PA_PLL_ISECTED) {
				plprev = pl;
				continue;
			}

			/* Reset the intersection flags, (going to keep these islands) */
			pl->flg.llabel = PA_PLL_UNKNWN;

			if (del_contour || hole_contour) {
				remove_contour(pa, plprev, pl, !(is_first && is_last));

				if (del_contour)       pa_pline_free(&pl);
				else if (hole_contour) pa_prim_delo_link_in(pl, holes);

				if (is_first && is_last) { /* was the only item on the list */
					pa_polyarea_unlink(islands, pa);
					pa_polyarea_free_all(&pa);
				}
			}
			else
				plprev = pl; /* optimization: singly linked list, remember ->prev so it won't need to be recalculated */

			/* When moving or deleting an outer contour, also move related holes */
			if (is_outline && del_contour)
				hole_contour = 1;
		}
	}
}

RND_INLINE void pa_polyarea_update_primary(jmp_buf *e, rnd_polyarea_t **islands, rnd_pline_t **holes, int op, rnd_polyarea_t *bpa)
{
	rnd_box_t box;

	if (*islands == NULL)
		return;

	pa_polyarea_bbox(&box, bpa);

	switch(op) {
		case RND_PBO_ISECT:
			pa_polyarea_update_primary_del_outside(e, box, islands, holes, bpa, 1);
			return;

		case RND_PBO_UNITE:
		case RND_PBO_SUB:
			pa_polyarea_update_primary_del_inside(e, box, islands, holes, bpa);
			return;

		case RND_PBO_XOR:
			/* not implemented or used (yet); would call
			pa_polyarea_update_primary_del_outside() with del_outside == 0?
			Also inv_inside? */
			break;
	}

	/* invalid op */
	assert(0);
}

RND_INLINE void pa_polyarea_collect_separated(jmp_buf *e, rnd_pline_t *A, rnd_polyarea_t **contours, rnd_pline_t **holes, int op, rnd_bool maybe)
{
	rnd_pline_t **pl, **next;

	for(pl = &A; *pl != NULL; pl = next) {
		next = &((*pl)->next);
		if (pa_collect_contour(e, pl, contours, holes, op, NULL, NULL, NULL))
			next = pl; /* if a contour is removed, don't advance "next" twice */
	}
}

static void pa_polyarea_collect(jmp_buf *e, rnd_polyarea_t *A, rnd_polyarea_t **contours, rnd_pline_t **holes, int op, rnd_bool maybe)
{
	rnd_polyarea_t *pa = A, *parent = NULL;
	rnd_pline_t **pl, **plnext, *parent_contour;

	assert(pa != NULL);

#ifndef PB_OPTIMIZE
	/* verify that pa is on a cyclic list */
	while((pa = pa->f) != A);
#endif

	/* collect non-intersect parts in "holes" */
	do {
		if (maybe && (pa->contours->flg.llabel != PA_PLL_ISECTED))
			parent_contour = pa->contours;
		else
			parent_contour = NULL;

		/* first contour: may be possible to shortcut reparenting some of its children */
		pl = &pa->contours;
		if (*pl != NULL) {
			plnext = &((*pl)->next);
			
			if (pa_collect_contour(e, pl, contours, holes, op, pa, NULL, NULL)) {
				parent = (*contours)->b; /* inserted behind the head */
				plnext = pl; /* removed a contour, don't advance "plnext" twice */
			}
			else
				parent = pa;
			pl = plnext;
		}

		for(; *pl != NULL; pl = plnext) {
			plnext = &((*pl)->next);
			if (pa_collect_contour(e, pl, contours, holes, op, pa, parent, parent_contour))
				plnext = pl; /* removed a contour, don't advance "plnext" twice */
		}
	}
	while((pa = pa->f) != A);

	pa_debug_dump(stderr, "collected", A, PA_DBG_DUMP_CVC);
}
