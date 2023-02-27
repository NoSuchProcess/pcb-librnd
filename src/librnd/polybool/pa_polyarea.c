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

/* Internal callback for counting countours the pline cl is inside */
static rnd_r_dir_t pa_pline_count_inside_cb(const rnd_box_t *b, void *cl)
{
	rnd_pline_t *me = (rnd_pline_t *)cl;
	rnd_pline_t *check = (rnd_pline_t *)b;

	if (pa_pline_inside_pline(check, me))
		return RND_R_DIR_FOUND_CONTINUE;

	return RND_R_DIR_NOT_FOUND;
}

/* Returns whether pl is inside pa; if first_only is set, only the first island
   of pa is considered */
static int pa_is_pline_in_polyarea(rnd_pline_t *pl, rnd_polyarea_t *pa, rnd_bool first_only)
{
	rnd_polyarea_t *outer = pa;
	rnd_heap_t *heap = rnd_heap_create();

	assert(pl != NULL);
	assert(outer != NULL);

	do {
		if (pa_pline_box_inside(pl, outer->contours))
			rnd_heap_insert(heap, outer->contours->area, (void *)outer);
	}	while(!first_only && (outer = outer->f) != pa);

	/* start checking from smallest to largest island whether pl is within
	   that island; if it's within 2 contours that means it's in a hole */
	while(!rnd_heap_is_empty(heap)) {
		int cnt;

		/* count how many positive and negative contours are around pl in the smallest island */
		outer = (rnd_polyarea_t *)rnd_heap_remove_smallest(heap);
		rnd_r_search(outer->contour_tree, (rnd_box_t *)pl, NULL, pa_pline_count_inside_cb, pl, &cnt);

		switch (cnt) {
			case 0: break;            /* pl is outside of any contour of this island */
			case 1: goto inside;     /* pl is inside an island (+1), and not any of its holes */
			case 2: goto not_inside; /* pl is inside a hole (+1) in the smallest island (+1) so far. No need to check the other polygons */
			default:
				fprintf(stderr, "pa_is_pline_in_polyarea() internal error - please report this bug\n");
				assert(0);
				break;
		}
	}

	not_inside:;
	rnd_heap_destroy(&heap);
	return rnd_false;

	inside:;
	rnd_heap_destroy(&heap);
	return rnd_true;
}
