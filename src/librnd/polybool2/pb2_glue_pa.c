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
				pb2_1_map_pline(ctx, pl, poly_id);
		}
	} while((pa = pa->f) != start);
}



/*** Input pa optimalization: run the expensive pb2 algo only on A-B bbox overlaps ***/

/* Return whether a and b has any chance to intersect comparing their bboxes */
RND_INLINE int pa_pa_overlap(rnd_polyarea_t *a, rnd_polyarea_t *b)
{
	if (a->contours->xmax < b->contours->xmin) return 0;
	if (a->contours->ymax < b->contours->ymin) return 0;
	if (a->contours->xmin > b->contours->xmax) return 0;
	if (a->contours->ymin > b->contours->ymax) return 0;

	return 1;
}


void pb2_pa_clear_overlaps(rnd_polyarea_t *start)
{
	rnd_polyarea_t *pa = start;
	do {
		pa->overlap = 0;
	} while((pa = pa->f) != start);
}

void pb2_pa_map_overlaps(rnd_polyarea_t *A, rnd_polyarea_t *B)
{
	rnd_polyarea_t *a, *b;

	a = A;
	do {
		b = B;
		do {
			if (pa_pa_overlap(a, b))
				a->overlap = b->overlap = 1;
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
}

