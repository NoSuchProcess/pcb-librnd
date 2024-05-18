#include "polyarea.h"
#include "pa.h"

/* place all segments from pline into ctx, splitting segments on intersections */
RND_INLINE void pb2_1_map_pline(pb2_ctx_t *ctx, const rnd_pline_t *pline, char poly_id)
{
	rnd_vnode_t *vn = pline->head;
	do {
		pb2_1_map_seg_line(ctx, vn->point, vn->next->point, poly_id, 0);
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

RND_INLINE void pa_include_if(rnd_polyarea_t **res, rnd_polyarea_t *start, int overlap_val)
{
	rnd_polyarea_t *pa = start;
	do {
		if (pa->overlap == overlap_val) {
			rnd_polyarea_t *newpa = rnd_polyarea_dup(pa);
			rnd_polyarea_m_include(res, newpa);
		}
	} while((pa = pa->f) != start);
}

/* Put back an 'A' hole into res; since the hole is not overlapping with 'B',
   it is untouched by pb2, it surely isn't cut in half. The host island of
   A may have been removed tho (e.g. if operation was an isect). Search
   each island of res and find the first one pl fits into. */
RND_INLINE pa_reinstall_hole(rnd_polyarea_t **res, rnd_pline_t *pl)
{
	rnd_polyarea_t *pa = *res;
	do {

		if (pa->contours->area < pl->area)
			continue; /* cheap test: won't fit */

		if (!pl_pl_overlap(pl, pa->contours))
			continue; /* cheap test: bbox not overlapping */

		/* have to check two adjacent nodes: rare corner case is when two islands
		   touch and a hole in one of them has a point on the boundary; but the hole
		   can not have two points on the same boundary without making the input invalid */
		if (pa_pline_is_vnode_inside(pa->contours, pl->head, 1) && pa_pline_is_vnode_inside(pa->contours, pl->head->next, 1)) {
			rnd_pline_t *newpl = pa_pline_dup(pl);
			pa_polyarea_insert_pline(pa, newpl);
			return;
		}
	} while((pa = pa->f) != *res);
}

/* Take all islands of A that was marked overlapping; they may have
   non-overlapping holes that were not participating in pb2 (to save CPU).
   Put those non-overlapping holes back in res. */
RND_INLINE void pa_reinstall_nonolap_holes(rnd_polyarea_t **res, rnd_polyarea_t *start)
{
	rnd_polyarea_t *pa = start;
	rnd_pline_t *pl;

	do {
		if (pa->overlap == 1)
			for(pl = pa->contours; pl != NULL; pl = pl->next)
				if (pl->flg.overlap == 0)
					pa_reinstall_hole(res, pl);
	} while((pa = pa->f) != start);
}

void pb2_pa_apply_nonoverlaps(rnd_polyarea_t **res, rnd_polyarea_t *A, rnd_polyarea_t *B, int op)
{
	switch (op) {
		case RND_PBO_XOR:
		case RND_PBO_UNITE:
			pa_include_if(res, A, 0);
			pa_include_if(res, B, 0);
			break;
		case RND_PBO_SUB:
			pa_include_if(res, A, 0);
			break;
		case RND_PBO_ISECT:
			break;
	}

	pa_reinstall_nonolap_holes(res, A);
}

