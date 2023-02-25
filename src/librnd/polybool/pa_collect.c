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
	return (cur->flg.plabel == PA_PTL_OUTSIDE) || (cur->flg.plabel == PA_PTL_SHARED);
}

static int IsectS_Rule(rnd_vnode_t * cur, DIRECTION * initdir)
{
	*initdir = FORW;
	return (cur->flg.plabel == PA_PTL_INSIDE) || (cur->flg.plabel == PA_PTL_SHARED);
}

static int SubS_Rule(rnd_vnode_t * cur, DIRECTION * initdir)
{
	*initdir = FORW;
	return (cur->flg.plabel == PA_PTL_OUTSIDE) || (cur->flg.plabel == PA_PTL_SHARED2);
}

static int XorS_Rule(rnd_vnode_t * cur, DIRECTION * initdir)
{
	if (cur->flg.plabel == PA_PTL_INSIDE) {
		*initdir = BACKW;
		return rnd_true;
	}
	if (cur->flg.plabel == PA_PTL_OUTSIDE) {
		*initdir = FORW;
		return rnd_true;
	}
	return rnd_false;
}

static int IsectJ_Rule(char p, rnd_vnode_t * v, DIRECTION * cdir)
{
	assert(*cdir == FORW);
	return (v->flg.plabel == PA_PTL_INSIDE || v->flg.plabel == PA_PTL_SHARED);
}

static int UniteJ_Rule(char p, rnd_vnode_t * v, DIRECTION * cdir)
{
	assert(*cdir == FORW);
	return (v->flg.plabel == PA_PTL_OUTSIDE || v->flg.plabel == PA_PTL_SHARED);
}

static int XorJ_Rule(char p, rnd_vnode_t * v, DIRECTION * cdir)
{
	if (v->flg.plabel == PA_PTL_INSIDE) {
		*cdir = BACKW;
		return rnd_true;
	}
	if (v->flg.plabel == PA_PTL_OUTSIDE) {
		*cdir = FORW;
		return rnd_true;
	}
	return rnd_false;
}

static int SubJ_Rule(char p, rnd_vnode_t * v, DIRECTION * cdir)
{
	if (p == 'B' && v->flg.plabel == PA_PTL_INSIDE) {
		*cdir = BACKW;
		return rnd_true;
	}
	if (p == 'A' && v->flg.plabel == PA_PTL_OUTSIDE) {
		*cdir = FORW;
		return rnd_true;
	}
	if (v->flg.plabel == PA_PTL_SHARED2) {
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
		if (*cdir == FORW ? (*cur)->flg.mark : (*cur)->prev->flg.mark)
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
		if (!e->flg.mark && rule(d->poly, e, &newone)) {
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
		newn->flg.mark = 1;
		/* for SHARED edge mark both */
		if (newn->shared)
			newn->shared->flg.mark = 1;

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
		DEBUGP("adding contour with %d vertices and direction %c\n", p->Count, p->flg.orient ? 'F' : 'B');
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
		if (s_rule(cur, &dir) && cur->flg.mark == 0)
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

	if ((*A)->flg.llabel == PA_PLL_ISECTED) {
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
			if ((*A)->flg.llabel == PA_PLL_INSIDE) {
				tmprev = *A;
				/* disappear this contour (rtree entry removed in PutContour) */
				*A = tmprev->next;
				tmprev->next = NULL;
				PutContour(e, tmprev, contours, holes, owner, NULL, NULL);
				return rnd_true;
			}
			break;
		case RND_PBO_XOR:
			if ((*A)->flg.llabel == PA_PLL_INSIDE) {
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
			if ((*A)->flg.llabel == PA_PLL_OUTSIDE) {
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
			if ((*cur)->flg.llabel == PA_PLL_ISECTED)
				continue;

			if ((*cur)->flg.llabel == PA_PLL_INSIDE)
				switch (action) {
				case RND_PBO_XOR:
				case RND_PBO_SUB:
					rnd_poly_contour_inv(*cur);
				case RND_PBO_ISECT:
					tmp = *cur;
					*cur = tmp->next;
					next = cur;
					tmp->next = NULL;
					tmp->flg.llabel = PA_PLL_UNKNWN;
					PutContour(e, tmp, contours, holes, b, NULL, NULL);
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
	if (check->flg.orient == RND_PLF_DIR)
		return RND_R_DIR_NOT_FOUND;
	/* Don't look at contours marked as being intersected */
	if (check->flg.llabel == PA_PLL_ISECTED)
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
			if (a->contours->flg.llabel != PA_PLL_ISECTED &&
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
				del_contour = curc->flg.llabel != PA_PLL_ISECTED && !cntr_in_M_rnd_polyarea_t(curc, bpa, rnd_false);

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
