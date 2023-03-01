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

/* routines for temporary storing resulting contours */


RND_INLINE void insert_pline(jmp_buf *e, rnd_polyarea_t **dst, rnd_pline_t *pl)
{
	rnd_polyarea_t *newp = malloc(sizeof(rnd_polyarea_t));

	if (RND_UNLIKELY(newp == NULL))
		pa_error(pa_err_no_memory);

	if (*dst == NULL) {
		*dst = newp;
		(*dst)->f = (*dst)->b = *dst;
	}
	else {
		newp->f = *dst;
		newp->b = (*dst)->b;
		newp->f->b = newp->b->f = newp;
	}

	newp->contours = pl;
	newp->contour_tree = rnd_r_create_tree();
	rnd_r_insert_entry(newp->contour_tree, (rnd_box_t *)pl);
	pl->next = NULL;
}

/* Put pl on a list:
    - if it's an outer contour, on contours;
    - if it's a hole with a known new_parent, on the pline list of the new parent
    - if it's a hole with a unknown new_parent, on the temporary holes list
   Arguments:
    - old_parent is the current parent of pl (or NULL).
    - new_parent is where pl is inserted (or NULL)
    - new_parent_contour is the outer contour of new_parent (or NULL)
*/
static void put_contour(jmp_buf *e, rnd_pline_t *pl, rnd_polyarea_t **contours, rnd_pline_t **holes, rnd_polyarea_t *old_parent, rnd_polyarea_t *new_parent, rnd_pline_t *new_parent_contour)
{
	assert((pl != NULL) && (pl->Count > 2));

	pl->next = NULL; /* unlink from its original list of plines */

	if (pl->flg.orient == RND_PLF_DIR) { /* outer contour */
		if (old_parent != NULL)
			rnd_r_delete_entry(old_parent->contour_tree, (rnd_box_t *)pl);
		insert_pline(e, contours, pl);
	}
	else { /* inner: hole */
		if (new_parent_contour != NULL) { /* known parent */
			pl->next = new_parent_contour->next;
			new_parent_contour->next = pl;
			if (old_parent != new_parent) { /* parent changed, move from one rtree to another */
				if (old_parent != NULL)
					rnd_r_delete_entry(old_parent->contour_tree, (rnd_box_t *)pl);
				rnd_r_insert_entry(new_parent->contour_tree, (rnd_box_t *)pl);
			}
		}
		else { /* no known parent - put hole into temporary list */
			/* prepend pl in holes */
			pl->next = *holes;
			*holes = pl;
			/* don't insert temporary holes into an r-tree, just on a linked list */
			if (old_parent != NULL)
				rnd_r_delete_entry(old_parent->contour_tree, (rnd_box_t *)pl);
		}
	}
}

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
	if (check->flg.orient == RND_PLF_DIR) {
		return RND_R_DIR_NOT_FOUND;
	}
	if (pa_pline_inside_pline(info->want_inside, check)) {
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
		pa_error(pa_err_bad_parm);				/* empty contour list */

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
			pa_poly_dump(dest);
#endif
#endif
			pa_pline_free(&curh);
			pa_error(pa_err_bad_parm);
		}
		/* Now search the heap for the container. If there was only one item
		 * in the heap, assume it is the container without the expense of
		 * proving it.
		 */
		pa_info = (struct polyarea_info *) rnd_heap_remove_smallest(heap);
		if (rnd_heap_is_empty(heap)) {	/* only one possibility it must be the right one */
			assert(pa_pline_inside_pline(pa_info->pa->contours, curh));
			container = pa_info->pa->contours;
		}
		else {
			do {
				if (pa_pline_inside_pline(pa_info->pa->contours, curh)) {
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
			pa_poly_dump(dest);
#endif
#endif
			curh->next = NULL;
			pa_pline_free(&curh);
			pa_error(pa_err_bad_parm);
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
