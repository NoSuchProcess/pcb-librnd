/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2024)
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
 */

#include "polyarea.h"
#include "pa.h"

/* place all segments from pline into ctx, splitting segments on intersections */
RND_INLINE void pb2_1_map_pline(pb2_ctx_t *ctx, const rnd_pline_t *pline, char poly_id)
{
	rnd_vnode_t *vn = pline->head;
	do {
		switch(vn->flg.curve_type) {
			case RND_VNODE_ARC: pb2_1_map_seg_arc(ctx, vn->point, vn->next->point, vn->curve.arc.center, vn->curve.arc.adir, poly_id); break;

			case RND_VNODE_LINE:
			default:  pb2_1_map_seg_line(ctx, vn->point, vn->next->point, poly_id); break;

		}
	} while((vn = vn->next) != pline->head);
}

/* place all segments from polyarea into ctx, splitting segments on intersections */
void pb2_pa_map_polyarea(pb2_ctx_t *ctx, const rnd_polyarea_t *start, char poly_id, int force)
{
	const rnd_polyarea_t *pa = start;
	do {
		if (force || pa->overlap) {
			rnd_pline_t *pl;
			for(pl = pa->contours; pl != NULL; pl = pl->next)
				if (force || (poly_id == 'B') || pl->flg.overlap)
					pb2_1_map_pline(ctx, pl, poly_id);
		}
	} while((pa = pa->f) != start);
}



/*** Input pa optimalization: run the expensive pb2 algo only on A-B bbox overlaps ***/

/* Return whether a and b has any chance to intersect comparing their bboxes */
RND_INLINE int pl_pl_overlap(rnd_pline_t *a, rnd_pline_t *b)
{
	if (a->xmax < b->xmin) return 0;
	if (a->ymax < b->ymin) return 0;
	if (a->xmin > b->xmax) return 0;
	if (a->ymin > b->ymax) return 0;

	return 1;
}

/* Return whether a and b has any chance to intersect comparing their bboxes */
RND_INLINE int pa_pa_overlap(rnd_polyarea_t *a, rnd_polyarea_t *b)
{
	return pl_pl_overlap(a->contours, b->contours);
}


void pb2_pa_clear_overlaps(rnd_polyarea_t *start)
{
	rnd_polyarea_t *pa = start;
	rnd_pline_t *pl;
	do {
		pa->overlap = 0;
		for(pl = pa->contours; pl != NULL; pl = pl->next)
			pl->flg.overlap = 0;
	} while((pa = pa->f) != start);
}

/* Mark all holes in insland 'a' that has a bbox overlapping with contour of
   island 'b' */
RND_INLINE void pa_pa_olap_mark_holes(rnd_polyarea_t *a, rnd_polyarea_t *b)
{
	rnd_pline_t *pl;
	for(pl = a->contours; pl != NULL; pl = pl->next)
		if (pl_pl_overlap(pl, b->contours))
			pl->flg.overlap = 1;
}

void pb2_pa_map_overlaps(rnd_polyarea_t *A, rnd_polyarea_t *B)
{
	rnd_polyarea_t *a, *b;

	a = A;
	do {
		b = B;
		do {
			if (pa_pa_overlap(a, b)) {
				a->overlap = b->overlap = 1;
				pa_pa_olap_mark_holes(a, b);
			}
		} while((b = b->f) != B);
	} while((a = a->f) != A);
}

RND_INLINE void pa_include_if(rnd_polyarea_t **res, rnd_polyarea_t **start, int overlap_val, rnd_bool preserve)
{
	rnd_polyarea_t *pa, *paf;

	restart:;
	pa = *start;
	do {
		paf = pa->f;
		if (pa->overlap == overlap_val) {
			if (preserve) {
				rnd_polyarea_t *newpa = rnd_polyarea_dup(pa);
				rnd_polyarea_m_include(res, newpa);
			}
			else {
				int removed_start = (pa == *start), only1;

				/* check if there is only one polyarea left in start */
				only1 = (removed_start && (*start == paf));

				pa_polyarea_unlink(start, pa);
				rnd_polyarea_m_include(res, pa);

				if (only1)
					return;
				if (removed_start) {
					*start = paf;
					goto restart;
				}
				
			}
		}
	} while((pa = paf) != *start);
}

/* Put back an 'A' hole into res; since the hole is not overlapping with 'B',
   it is untouched by pb2, it surely isn't cut in half. The host island of
   A may have been removed tho (e.g. if operation was an isect). Search
   each island of res and find the smallest one pl fits into. Returns 1 if
   reinstalled. */
RND_INLINE int pa_reinstall_hole(rnd_polyarea_t **res, rnd_polyarea_t *plpa, rnd_pline_t *prev, rnd_pline_t *pl, rnd_bool preserve)
{
	rnd_polyarea_t *pa = *res, *smallest = NULL;

	if (pl->tree == NULL)
		pa_pline_update(pl, 0);

	/* find the smallest island of *res the hole is inside; this matters in
	   case of pos-neg-pos stacking where the hole needs to be inserted
	   in the bottom pos */
	do {
		if (pa->contours->area < pl->area)
			continue; /* cheap test: won't fit */

		if (!pl_pl_overlap(pl, pa->contours))
			continue; /* cheap test: bbox not overlapping */

		if (pa->contours->tree == NULL)
			pa_pline_update(pa->contours, 0);

		/* have to check two adjacent nodes: rare corner case is when two islands
		   touch and a hole in one of them has a point on the boundary; but the hole
		   can not have two points on the same boundary without making the input invalid */
		if (pa_pline_is_vnode_inside(pa->contours, pl->head, 1) && pa_pline_is_vnode_inside(pa->contours, pl->head->next, 1))
			if ((smallest == NULL) || (pa->contours->area < smallest->contours->area))
				smallest = pa;
	} while((pa = pa->f) != *res);

	if (smallest != NULL) {
		if (preserve) {
			rnd_pline_t *newpl = pa_pline_dup(pl);
			pa_polyarea_insert_pline(smallest, newpl);
		}
		else {
			pa_pline_unlink(plpa, prev, pl);
			pa_polyarea_insert_pline(smallest, pl);
		}
		return 1;
	}

	return 0;
}

/* Take all islands of A that was marked overlapping; they may have
   non-overlapping holes that were not participating in pb2 (to save CPU).
   Put those non-overlapping holes back in res. */
RND_INLINE void pa_reinstall_nonolap_holes(rnd_polyarea_t **res, rnd_polyarea_t *start, rnd_bool preserve)
{
	rnd_polyarea_t *pa = start;
	rnd_pline_t *pl, *prev, *next;

	do {
		if (pa->overlap == 1) {
			prev = pa->contours;
			for(pl = prev->next; pl != NULL; pl = next) {
				next = pl->next;
				if (pl->flg.overlap == 0) {
					if (!pa_reinstall_hole(res, pa, prev, pl, preserve))
						prev = pl;
				}
				else
					prev = pl;
			}
		}
	} while((pa = pa->f) != start);
}

void pb2_pa_apply_nonoverlaps(rnd_polyarea_t **res, rnd_polyarea_t **A, rnd_polyarea_t **B, int op, rnd_bool preserve)
{
	switch (op) {
		case RND_PBO_XOR:
		case RND_PBO_UNITE:
			pa_include_if(res, A, 0, preserve);
			pa_include_if(res, B, 0, preserve);
			break;
		case RND_PBO_SUB:
			pa_include_if(res, A, 0, preserve);
			break;
		case RND_PBO_ISECT:
			break;
	}

	if (*A != NULL)
		pa_reinstall_nonolap_holes(res, *A, preserve);
}

