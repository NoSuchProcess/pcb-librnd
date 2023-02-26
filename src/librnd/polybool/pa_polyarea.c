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

/* rnd_polyarea_t internal/low level */

/* Return whether a and b has any chance to intersect comparing their bboxes */
RND_INLINE int pa_polyarea_box_overlap(rnd_polyarea_t *a, rnd_polyarea_t *b)
{
	if (a->contours->xmax < b->contours->xmin) return 0;
	if (a->contours->ymax < b->contours->ymin) return 0;
	if (a->contours->xmin > b->contours->xmax) return 0;
	if (a->contours->ymin > b->contours->ymax) return 0;

	return 1;
}

static rnd_r_dir_t count_contours_i_am_inside(const rnd_box_t * b, void *cl)
{
	rnd_pline_t *me = (rnd_pline_t *) cl;
	rnd_pline_t *check = (rnd_pline_t *) b;

	if (rnd_poly_contour_in_contour(check, me))
		return RND_R_DIR_FOUND_CONTINUE;
	return RND_R_DIR_NOT_FOUND;
}

/* pa_is_pline_in_polyarea
returns poly is inside outfst ? rnd_true : rnd_false */
static int pa_is_pline_in_polyarea(rnd_pline_t * poly, rnd_polyarea_t * outfst, rnd_bool test)
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
}																/* pa_is_pline_in_polyarea */
