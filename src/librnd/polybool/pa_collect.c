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

/* collect the result */

typedef enum pa_direction_e { PA_FORWARD, PA_BACKWARD } pa_direction_t;
typedef int (*pa_start_rule_t)(rnd_vnode_t *, pa_direction_t *);
typedef int (*pa_jump_rule_t)(char poly, rnd_vnode_t *, pa_direction_t *);

/*** Per operation start/jump rules ***/

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
		return !marked;
	}

	/* cross-vertex means we are at an intersection and have to decide which
	   edge to continue at */
#ifdef DEBUG_JUMP
	DEBUGP("jump entering node at %$mD\n", (*cur)->point[0], (*cur)->point[1]);
#endif

	start = d = (*cdir == PA_FORWARD) ? (*cur)->cvclst_prev->prev : (*cur)->cvclst_next->prev;
	do {
		pa_direction_t newdir = *cdir;
		rnd_vnode_t *e = d->parent;

		if (d->side == 'P')
			e = e->prev;

		if (!e->flg.mark && rule(d->poly, e, &newdir)) {
			if (((d->side == 'N') && (newdir == PA_FORWARD)) || ((d->side == 'P') && (newdir == PA_BACKWARD))) {
#ifdef DEBUG_JUMP
				{
					rnd_vnode_t *nnd = (newdir == PA_FORWARD) ? e->next : e;
					DEBUGP("jump leaving node at %#mD\n", nnd->point[0], nnd->point[1]);
				}
#endif
				*cur = d->parent;
				*cdir = newdir;
				return rnd_true;
			}
		}
	} while((d = d->prev) != start);

	return rnd_false;
}

#define PA_NEXT_NODE(nd, dir) (((dir) == PA_FORWARD) ? (nd)->next : (nd)->prev)

RND_INLINE int pa_coll_gather(rnd_vnode_t *start, rnd_pline_t **result, pa_jump_rule_t v_rule, pa_direction_t dir)
{
	rnd_vnode_t *nd, *newnd;

#ifdef DEBUG_GATHER
	DEBUGP("gather direction = %d\n", dir);
#endif
	*result = NULL;

	/* Run nd from start hopping to next node */
	for(nd = start; pa_coll_jump(&nd, &dir, v_rule); nd = PA_NEXT_NODE(nd, dir)) {

		/* add edge to pline */
		if (*result == NULL) { /* create first */
			*result = pa_pline_new(nd->point);
			if (*result == NULL)
				return pa_err_no_memory;
		}
		else { /* insert subsequent */
			newnd = rnd_poly_node_create(nd->point);
			if (newnd == NULL)
				return pa_err_no_memory;
			rnd_poly_vertex_include((*result)->head->prev, newnd);
		}

#ifdef DEBUG_GATHER
		DEBUGP("gather vertex at %#mD\n", nd->point[0], nd->point[1]);
#endif

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
#ifdef DEBUG_GATHER
		DEBUGP("adding contour with %d vertices and direction %c\n", pl->Count, (pl->flg.orient ? 'F' : 'B'));
#endif
		put_contour(e, pl, contours, holes, NULL, NULL, NULL);
	}
	else {
#ifdef DEBUG_GATHER
		DEBUGP("Bad contour! Not enough points!!\n");
#endif
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
		switch(op) {
			case RND_PBO_UNITE: return pa_collect_pline(e, *A, contours, holes, pa_rule_unite_start, pa_rule_unite_jump);
			case RND_PBO_ISECT: return pa_collect_pline(e, *A, contours, holes, pa_rule_isect_start, pa_rule_isect_jump);
			case RND_PBO_XOR:   return pa_collect_pline(e, *A, contours, holes, pa_rule_xor_start, pa_rule_xor_jump);
			case RND_PBO_SUB:   return pa_collect_pline(e, *A, contours, holes, pa_rule_sub_start, pa_rule_sub_jump);
		}
	}
	else {
		switch (op) {
			case RND_PBO_ISECT:
				if ((*A)->flg.llabel == PA_PLL_INSIDE)
					return pa_coll_move_cnt(e, A, contours, holes, old_parent, NULL, NULL, 0);
				break;
			case RND_PBO_XOR:
				if ((*A)->flg.llabel == PA_PLL_INSIDE)
					return pa_coll_move_cnt(e, A, contours, holes, old_parent, NULL, NULL, 1);
				/* else: fall through to collect all outside areas */
			case RND_PBO_UNITE:
			case RND_PBO_SUB:
				if ((*A)->flg.llabel == PA_PLL_OUTSIDE)
					return pa_coll_move_cnt(e, A, contours, holes, old_parent, parent, parent_contour, 0);
				break;
		}
	}

	return rnd_false;
}

static void M_B_AREA_Collect(jmp_buf * e, rnd_polyarea_t * bfst, rnd_polyarea_t ** contours, rnd_pline_t ** holes, int action)
{
	rnd_polyarea_t *b = bfst;
	rnd_pline_t **cur, **next, *tmp;

	assert(b != NULL);
	do {
		for (cur = &b->contours; *cur != NULL; cur = next) {
			next = &((*cur)->next);
			if ((*cur)->flg.llabel == PA_PLL_ISECTED)
				continue;

			if ((*cur)->flg.llabel == PA_PLL_INSIDE)
				switch (action) {
				case RND_PBO_XOR:
				case RND_PBO_SUB:
					pa_pline_invert(*cur);
				case RND_PBO_ISECT:
					tmp = *cur;
					*cur = tmp->next;
					next = cur;
					tmp->next = NULL;
					tmp->flg.llabel = PA_PLL_UNKNWN;
					put_contour(e, tmp, contours, holes, b, NULL, NULL);
					break;
				case RND_PBO_UNITE:
					break;								/* nothing to do - already included */
				}
			else if ((*cur)->flg.llabel == PA_PLL_OUTSIDE)
				switch (action) {
				case RND_PBO_XOR:
				case RND_PBO_UNITE:
					/* include */
					tmp = *cur;
					*cur = tmp->next;
					next = cur;
					tmp->next = NULL;
					tmp->flg.llabel = PA_PLL_UNKNWN;
					put_contour(e, tmp, contours, holes, b, NULL, NULL);
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
			int isect_contour = (curc->flg.llabel == PA_PLL_ISECTED);

			next = curc->next;

			if (isect_contour || hole_contour) {

				/* Reset the intersection flags, since we keep these pieces */
				if (curc->flg.llabel != PA_PLL_ISECTED)
					curc->flg.llabel = PA_PLL_UNKNWN;

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
					pa_polyarea_free_all(&a);				/* NB: Sets a to NULL */
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
	if (check->flg.orient == RND_PLF_DIR)
		return RND_R_DIR_NOT_FOUND;
	/* Don't look at contours marked as being intersected */
	if (check->flg.llabel == PA_PLL_ISECTED)
		return RND_R_DIR_NOT_FOUND;
	if (pa_is_pline_in_polyarea(check, info->want_inside, rnd_false)) {
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
			if (a->contours->flg.llabel != PA_PLL_ISECTED &&
					/* Pre-filter on bounding box */
					((a->contours->xmin >= box.X1) && (a->contours->ymin >= box.Y1)
					 && (a->contours->xmax <= box.X2)
					 && (a->contours->ymax <= box.Y2)) &&
					/* Then test properly */
					pa_is_pline_in_polyarea(a->contours, bpa, rnd_false)) {

				/* Delete this contour, all children -> holes queue */

				/* Delete the outer contour */
				curc = a->contours;
				remove_contour(a, NULL, curc, rnd_false);	/* Rtree deleted in poly_Free below */
				/* a->contours now points to the remaining holes */
				pa_pline_free(&curc);

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
				pa_polyarea_free_all(&a);					/* NB: Sets a to NULL */

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
				pa_pline_free(&info.result);
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
				del_contour = curc->flg.llabel != PA_PLL_ISECTED && !pa_is_pline_in_polyarea(curc, bpa, rnd_false);

			/* Skip intersected contours */
			if (curc->flg.llabel == PA_PLL_ISECTED) {
				prev = curc;
				continue;
			}

			/* Reset the intersection flags, since we keep these pieces */
			curc->flg.llabel = PA_PLL_UNKNWN;

			if (del_contour || hole_contour) {

				remove_contour(a, prev, curc, !(is_first && is_last));

				if (del_contour) {
					/* Delete the contour */
					pa_pline_free(&curc);	/* NB: Sets curc to NULL */
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
					pa_polyarea_free_all(&a);				/* NB: Sets a to NULL */
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
		if (pa_collect_contour(e, cur, contours, holes, action, NULL, NULL, NULL))
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
		if (maybe && a->contours->flg.llabel != PA_PLL_ISECTED)
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
			if (pa_collect_contour(e, cur, contours, holes, action, a, NULL, NULL)) {
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
			if (pa_collect_contour(e, cur, contours, holes, action, a, parent, parent_contour))
				next = cur;
		}
	}
	while ((a = a->f) != afst);
}
