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
 *  This file is new in librnd an implements a more efficient special case
 *  solution to the no-hole dicing problem. Special casing rationale: this
 *  runs for rendering at every frame at least in sw-render.
 *
 *  The code is not reentrant: it temporary modifies pline flags of the input.
 */

#include "pa_dicer.h"
#include <librnd/core/vtc0.h>

/*** clipper ***/

typedef enum pa_dic_pline_label_e { /* pline's flg.label */
	PA_PLD_UNKNWN  = 0,
	PA_PLD_INSIDE  = 1,    /* pline is fully inside the box */
	PA_PLD_WRAPPER = 2,    /* the box is fully included within the pline */
	PA_PLD_ISECTED = 3,    /* they are crossing */
	PA_PLD_AWAY = 4        /* pline is fully outside but not wrapping the box */
} pa_dic_pline_label_t;

struct pa_dic_isc_s {
	rnd_vnode_t *vn;
	rnd_pline_t *pl;

	rnd_coord_t x, y;      /* corners have vn==NULL pl==NULL but we still need to remember the coords */

	unsigned temporary:1;  /* inserted by the dicer code, shall be removed at the end */
	unsigned coax:1;       /* set if line is coaxial (may overlap with) the edge */
	unsigned pcollected:1; /* already emitted the outgoing pline (->next) in the output */
	unsigned ecollected:1; /* already emitted the outgoing clipbox edge in the output */
	pa_dic_isc_t *next;
};

TODO("cache allocations");
RND_INLINE pa_dic_isc_t *pa_dic_isc_alloc(pa_dic_ctx_t *ctx)
{
	return malloc(sizeof(pa_dic_isc_t));
}

RND_INLINE void pa_dic_isc_free(pa_dic_ctx_t *ctx, pa_dic_isc_t *isc, int destroy)
{
	free(isc);
}

RND_INLINE void pa_dic_reset_ctx_pa_(pa_dic_ctx_t *ctx, int destroy)
{
	int sd, m;

	for(sd = 0; sd < PA_DIC_sides; sd++)
		for(m = 0; m < ctx->side[sd].used; m++)
			pa_dic_isc_free(ctx, ctx->side[sd].array[m], destroy);


	if (destroy) {
		/* free cached side[] vectors */
		for(sd = 0; sd < PA_DIC_sides; sd++)
			vtp0_uninit(&ctx->side[sd]);
	}
	else {
		for(sd = 0; sd < PA_DIC_sides; sd++)
			ctx->side[sd].used = 0;
	}


	/* free ISCs and temporary pline nodes */
	if (ctx->head != NULL) {
		pa_dic_isc_t *i, *next;

		i = ctx->head;
		do {
			next = i->next;
			if (i->temporary) {
				rnd_poly_vertex_exclude(i->pl, i->vn);
				free(i->vn);
			}
			pa_dic_isc_free(ctx, i, destroy);
		} while((i = next) != ctx->head);

		ctx->head = NULL;
	}
}

RND_INLINE void pa_dic_reset_ctx_pa(pa_dic_ctx_t *ctx)
{
	pa_dic_reset_ctx_pa_(ctx, 0);
}


RND_INLINE void pa_dic_free_ctx(pa_dic_ctx_t *ctx)
{
	pa_dic_reset_ctx_pa_(ctx, 1);
}


/* Returns whether c is in between low and high without being equal to either */
RND_INLINE int crd_in_between(rnd_coord_t c, rnd_coord_t low, rnd_coord_t high)
{
	return (c > low) && (c < high);
}

/* Returns whether c is in between low and high, inclusive, fix order of
   low,high if needed */
RND_INLINE int crd_in_between_auto(rnd_coord_t c, rnd_coord_t low, rnd_coord_t high)
{
	if (low > high)
		rnd_swap(rnd_coord_t, low, high);
	return (c >= low) && (c <= high);
}

/* Reuse a flag of vnode for indiciating temporary nodes */
#define TEMPORARY mark

/* insert a new temporary node in seg */
RND_INLINE rnd_vnode_t *pa_dic_split_seg(pa_dic_ctx_t *ctx, pa_seg_t *seg, rnd_coord_t x, rnd_coord_t y)
{
	rnd_vnode_t *after, *next, *newnd;

	if ((x == seg->v->point[0]) && (y == seg->v->point[1]))
		return seg->v;

	for(after = seg->v;;after = next) {
		next = after->next;
		if ((after != seg->v) && !after->flg.TEMPORARY) {
			assert(!"ran out of the seg without finding where to insert");
		}
		if ((x == next->point[0]) && (y == next->point[1]))
			return next;
		if (crd_in_between_auto(x, after->point[0], next->point[0]) && crd_in_between_auto(y, after->point[1], next->point[1])) {
			/* insert a new temporayr node between after and next */
			newnd = calloc(sizeof(rnd_vnode_t), 1);
			newnd->point[0] = x;
			newnd->point[1] = y;
			newnd->flg.TEMPORARY = 1;
			rnd_poly_vertex_include_force(after, newnd);
			seg->p->Count++; /* vertex exclude will decrease it back at the end when we are removing temporaries */

			/* NOTE: segment and rtree are intentionally not updated as this node is temporary only */

			return newnd;
		}
	}

}

/* Record an intersection, potentially creating a new temporary node in seg;
   first and second are the first and second real (non-temporary) node of seg */
RND_INLINE pa_dic_isc_t *pa_dic_isc(pa_dic_ctx_t *ctx, pa_seg_t *seg, pa_dic_side_t side, rnd_coord_t x, rnd_coord_t y, int *iscs, int coax, rnd_vnode_t *first, rnd_vnode_t *second)
{
	pa_dic_isc_t *isc = pa_dic_isc_alloc(ctx);
	rnd_vnode_t *nd = NULL;

	if (iscs != NULL) {
		assert(*iscs < 2);
	}


	if (seg != NULL) {
		if ((first->point[0] == x) && (first->point[1] == y))
			nd = first;
		else if ((second->point[0] == x) && (second->point[1] == y))
			nd = second;
		else
			nd = pa_dic_split_seg(ctx, seg, x, y);
		isc->vn = nd;
		isc->pl = seg->p;
		isc->temporary = nd->flg.TEMPORARY;
	}
	else {
		/* creating the box */
		isc->vn = NULL;
		isc->pl = NULL;
		isc->temporary = 0;
	}

	isc->coax = coax;
	isc->pcollected = 0;
	isc->ecollected = 0;
	isc->x = x;
	isc->y = y;

	vtp0_append(&ctx->side[side], isc);

	if (iscs != NULL)
		(*iscs)++;

	return isc;
}

TODO("rewrite these with big_coords to make the 64-bit-coord safe");
rnd_coord_t pa_line_x_for_y(rnd_coord_t lx1, rnd_coord_t ly1, rnd_coord_t lx2, rnd_coord_t ly2, rnd_coord_t y)
{
	double dx = (double)(lx2 - lx1) / (double)(ly2 - ly1);
	return rnd_round(lx1 + (y - ly1) * dx);
}

rnd_coord_t pa_line_y_for_x(rnd_coord_t lx1, rnd_coord_t ly1, rnd_coord_t lx2, rnd_coord_t ly2, rnd_coord_t x)
{
	double dy = (double)(ly2 - ly1) / (double)(lx2 - lx1);
	return rnd_round(ly1 + (x - lx1) * dy);
}


/* Horizontal box edge intersection with seg */
static int pa_dic_isc_h(pa_dic_ctx_t *ctx, pa_seg_t *seg, pa_dic_side_t side, rnd_coord_t y)
{
	rnd_coord_t lx1, ly1, lx2, ly2;
	int iscs = 0;
	rnd_vnode_t *first, *second;

	/* remember the two real endpoints of the segment; for the second endpoint
	   we need to skip through the temporary points already inserted */
	first = seg->v;
	for(second = first->next; second->flg.TEMPORARY; second = second->next) ;

	TODO("arc: needs special code here");

	lx1 = first->point[0]; ly1 = first->point[1];
	lx2 = second->point[0]; ly2 = second->point[1];

	if (ly1 == ly2) {
		/* special case: horizontal line, may overlap */
		if (ly1 != y)
			return 0;
		if (lx1 > lx2)
			rnd_swap(rnd_coord_t, lx1, lx2);
		if ((lx1 > ctx->clip.X2) || (lx2 < ctx->clip.X1))
			return 0;

		if (crd_in_between(lx1, ctx->clip.X1, ctx->clip.X2))
			pa_dic_isc(ctx, seg, side, lx1, y, &iscs, 1, first, second);
		if (crd_in_between(lx2, ctx->clip.X1, ctx->clip.X2))
			pa_dic_isc(ctx, seg, side, lx2, y, &iscs, 1, first, second);
		if (crd_in_between(ctx->clip.X1, lx1, lx2))
			pa_dic_isc(ctx, seg, side, ctx->clip.X1, y, &iscs, 1, first, second);
		if (crd_in_between(ctx->clip.X2, lx1, lx2))
			pa_dic_isc(ctx, seg, side, ctx->clip.X2, y, &iscs, 1, first, second);
		if (ctx->clip.X1 == lx1)
			pa_dic_isc(ctx, seg, side, lx1, y, &iscs, 1, first, second);
		if (ctx->clip.X2 == lx2)
			pa_dic_isc(ctx, seg, side, lx2, y, &iscs, 1, first, second);
	}
	else {
		/* normal case: sloped line */
		rnd_coord_t x;

		if (ly1 > ly2) {
			rnd_swap(rnd_coord_t, ly1, ly2);
			rnd_swap(rnd_coord_t, lx1, lx2);
		}

		if (y == ly1)
			x = lx1;
		else if (y == ly2)
			x = lx2;
		else if (crd_in_between(y, ly1, ly2))
			x = pa_line_x_for_y(lx1, ly1, lx2, ly2, y);
		else
			return 0;
		if ((x >= ctx->clip.X1) && (x <= ctx->clip.X2))
			pa_dic_isc(ctx, seg, side, x, y, &iscs, 0, first, second);
	}

	return iscs;
}

/* Vertical box edge intersection with seg */
static int pa_dic_isc_v(pa_dic_ctx_t *ctx, pa_seg_t *seg, pa_dic_side_t side, rnd_coord_t x)
{
	rnd_coord_t lx1, ly1, lx2, ly2;
	int iscs = 0;

	rnd_vnode_t *first, *second;

	/* remember the two real endpoints of the segment; for the second endpoint
	   we need to skip through the temporary points already inserted */
	first = seg->v;
	for(second = first->next; second->flg.TEMPORARY; second = second->next) ;

	TODO("arc: needs special code here");

	lx1 = first->point[0]; ly1 = first->point[1];
	lx2 = second->point[0]; ly2 = second->point[1];


	if (lx1 == lx2) {
		/* special case: vertical line, may overlap */
		if (lx1 != x)
			return 0;
		if (ly1 > ly2)
			rnd_swap(rnd_coord_t, ly1, ly2);

		if (crd_in_between(ly1, ctx->clip.Y1, ctx->clip.Y2))
			pa_dic_isc(ctx, seg, side, x, ly1, &iscs, 1, first, second);
		if (crd_in_between(ly2, ctx->clip.Y1, ctx->clip.Y2))
			pa_dic_isc(ctx, seg, side, x, ly2, &iscs, 1, first, second);
		if (crd_in_between(ctx->clip.Y1, ly1, ly2))
			pa_dic_isc(ctx, seg, side, x, ctx->clip.Y1, &iscs, 1, first, second);
		if (crd_in_between(ctx->clip.Y2, ly1, ly2))
			pa_dic_isc(ctx, seg, side, x, ctx->clip.Y2, &iscs, 1, first, second);
		if (ctx->clip.Y1 == ly1)
			pa_dic_isc(ctx, seg, side, x, ly1, &iscs, 1, first, second);
		if (ctx->clip.Y2 == ly2)
			pa_dic_isc(ctx, seg, side, x, ly2, &iscs, 1, first, second);
	}
	else {
		/* normal case: sloped line */
		rnd_coord_t y;

		if (lx1 > lx2) {
			rnd_swap(rnd_coord_t, lx1, lx2);
			rnd_swap(rnd_coord_t, ly1, ly2);
		}

		if (x == lx1)
			y = ly1;
		else if (x == lx2)
			y = ly2;
		else if (crd_in_between(x, lx1, lx2))
			y = pa_line_y_for_x(lx1, ly1, lx2, ly2, x);
		else
			return 0;
		if ((y >= ctx->clip.Y1) && (y <= ctx->clip.Y2))
			pa_dic_isc(ctx, seg, side, x, y, &iscs, 0, first, second);
	}

	return iscs;
}

/* Consider a ray specified as a box of rx*;ry* and compute all intersections;
   if there is any intersection mark the pline PA_PLL_ISECTED */
RND_INLINE void pa_dic_pline_label_side(pa_dic_ctx_t *ctx, rnd_pline_t *pl, pa_dic_side_t side, int (*iscf)(pa_dic_ctx_t*, pa_seg_t*, pa_dic_side_t, rnd_coord_t), rnd_coord_t scrd, rnd_coord_t rx1, rnd_coord_t ry1, rnd_coord_t rx2, rnd_coord_t ry2)
{
	int isected = 0;
	rnd_rtree_box_t ray;
	rnd_rtree_it_t it;
	pa_seg_t *sg;

	ray.x1 = rx1; ray.y1 = ry1;
	ray.x2 = rx2; ray.y2 = ry2;

	for(sg = rnd_rtree_first(&it, pl->tree, &ray); sg != NULL; sg = rnd_rtree_next(&it))
		isected |= iscf(ctx, (pa_seg_t *)sg, side, scrd);

	if (isected)
		pl->flg.llabel = PA_PLD_ISECTED;
}

typedef enum pa_dic_pt_box_relation_e { /* bitfield */
	PA_DPT_INSIDE = 1,
	PA_DPT_OUTSIDE = 2,
	PA_DPT_ON_EDGE = 3
} pa_dic_pt_box_relation_t;

RND_INLINE pa_dic_pt_box_relation_t pa_dic_pt_in_box(rnd_coord_t ptx, rnd_coord_t pty, rnd_box_t *box)
{
	if (((ptx == box->X1) || (ptx == box->X2)) && (pty >= box->Y1) && (pty <= box->Y2))
		return PA_DPT_ON_EDGE;
	if (((pty == box->Y1) || (pty == box->Y2)) && (ptx >= box->X1) && (ptx <= box->X2))
		return PA_DPT_ON_EDGE;

	if (crd_in_between(ptx, box->X1, box->X2) && crd_in_between(pty, box->Y1, box->Y2))
		return PA_DPT_INSIDE;

	return PA_DPT_OUTSIDE;
}

RND_INLINE int pa_dic_pt_inside_pl(rnd_coord_t ptx, rnd_coord_t pty, rnd_pline_t *pl)
{
	rnd_vector_t pt;

	pt[0] = ptx; pt[1] = pty;

	return pa_pline_is_point_inside(pl, pt);
}

/* Compute all intersections between the label and pl, allocate pa_dic_isc_t *
   for them and append them in ctx->side[] in random order. Also set
   pl->flg.llabel */
RND_INLINE void pa_dic_pline_label(pa_dic_ctx_t *ctx, rnd_pline_t *pl)
{
	pa_dic_pt_box_relation_t rel;

	pl->flg.llabel = PA_PTL_UNKNWN;

	/* Cheap bbox test: if the bbox of pl is fully within the clipbox, it's surely inside */
	if ((pl->xmin > ctx->clip.X1) && (pl->ymin > ctx->clip.Y1) && (pl->xmax < ctx->clip.X2) && (pl->ymax < ctx->clip.Y2)) {
		pl->flg.llabel = PA_PLD_INSIDE;
		return;
	}

	/* Cheap bbox test: fully outside and not wrapping */
	if ((pl->xmax < ctx->clip.X1) || (pl->xmin > ctx->clip.X2) || (pl->ymax < ctx->clip.Y1) || (pl->ymin > ctx->clip.Y2)) {
		pl->flg.llabel = PA_PLD_AWAY;
		return;
	}

	/* Note: can't cheap-bbox-test for pl wrapping clipbox because they may intersect */

	/* Edge intersection tests */
	pa_dic_pline_label_side(ctx, pl, PA_DIC_H1, pa_dic_isc_h, ctx->clip.Y1, ctx->clip.X1, ctx->clip.Y1, ctx->clip.X2, ctx->clip.Y1);
	pa_dic_pline_label_side(ctx, pl, PA_DIC_V1, pa_dic_isc_v, ctx->clip.X2, ctx->clip.X2, ctx->clip.Y1, ctx->clip.X2, ctx->clip.Y2);
	pa_dic_pline_label_side(ctx, pl, PA_DIC_H2, pa_dic_isc_h, ctx->clip.Y2, ctx->clip.X1, ctx->clip.Y2, ctx->clip.X2, ctx->clip.Y2);
	pa_dic_pline_label_side(ctx, pl, PA_DIC_V2, pa_dic_isc_v, ctx->clip.X1, ctx->clip.X1, ctx->clip.Y1, ctx->clip.X1, ctx->clip.Y2);
	if (pl->flg.llabel == PA_PLD_ISECTED)
		return;

	/* Now that we know there's no intersection, things are fully inside/outside
	   or are far away */

	/* Cheap: if any point of the pline is inside the box, the whole pline is
	   inside as there was no intersection */
	rel = pa_dic_pt_in_box(pl->head->point[0], pl->head->point[1], &ctx->clip);
	assert(rel != PA_DPT_ON_EDGE); /* would have to be labelled ISECTED */
	if (rel == PA_DPT_INSIDE) {
		pl->flg.llabel = PA_PLD_INSIDE;
		return;
	}

	/* If all four corners of the box are inside the pline, the box is inside
	   the pline */
	if (pa_dic_pt_inside_pl(ctx->clip.X1, ctx->clip.Y1, pl) &&
		pa_dic_pt_inside_pl(ctx->clip.X2, ctx->clip.Y1, pl) &&
		pa_dic_pt_inside_pl(ctx->clip.X2, ctx->clip.Y2, pl) &&
		pa_dic_pt_inside_pl(ctx->clip.X1, ctx->clip.Y2, pl)) {
			pl->flg.llabel = PA_PLD_WRAPPER;
			return;
	}

	/* The only remaining case is the expensive-away case: there was no
	   intersection or wrapping so pl has an overlapping bbox but is fully
	   outside */
	pl->flg.llabel = PA_PLD_AWAY;
}

/* Multiple iscs in the same point are sorted by pointer value so that
   redundant entries are adjacent after the sort (the filter depends on it).
   In theory a by-angle-sort would be needed here on vn->next, but as
   the walkaround is following pline ->next it is taking the right turn anyway.
   Test case: clip27a. */
static int cmp_xmin(const void *A, const void *B)
{
	const pa_dic_isc_t * const *a = A, * const *b = B;
	if ((*a)->x == (*b)->x) return ((*a)->vn < (*b)->vn ? -1 : +1);
	return ((*a)->x < (*b)->x) ? -1 : +1;
}

static int cmp_ymin(const void *A, const void *B)
{
	const pa_dic_isc_t * const *a = A, * const *b = B;
	if ((*a)->y == (*b)->y) return ((*a)->vn < (*b)->vn ? -1 : +1);
	return ((*a)->y < (*b)->y) ? -1 : +1;
}

static int cmp_xmax(const void *A, const void *B)
{
	const pa_dic_isc_t * const *a = A, * const *b = B;
	if ((*a)->x == (*b)->x) return ((*a)->vn < (*b)->vn ? -1 : +1);
	return ((*a)->x > (*b)->x) ? -1 : +1;
}

static int cmp_ymax(const void *A, const void *B)
{
	const pa_dic_isc_t * const *a = A, * const *b = B;
	if ((*a)->y == (*b)->y) return ((*a)->vn < (*b)->vn ? -1 : +1);
	return ((*a)->y > (*b)->y) ? -1 : +1;
}

RND_INLINE void pa_dic_sort_sides(pa_dic_ctx_t *ctx)
{
	int sd;
	long m;
	pa_dic_isc_t *last = NULL, *i, *next, *prev;

	/* create dummy intersetions for corners for easier walkarounds */
	ctx->corner[0] = pa_dic_isc(ctx, NULL, PA_DIC_H1, ctx->clip.X1, ctx->clip.Y1, NULL, 0, NULL, NULL);
	ctx->corner[1] = pa_dic_isc(ctx, NULL, PA_DIC_V1, ctx->clip.X2, ctx->clip.Y1, NULL, 0, NULL, NULL);
	ctx->corner[2] = pa_dic_isc(ctx, NULL, PA_DIC_H2, ctx->clip.X2, ctx->clip.Y2, NULL, 0, NULL, NULL);
	ctx->corner[3] = pa_dic_isc(ctx, NULL, PA_DIC_V2, ctx->clip.X1, ctx->clip.Y2, NULL, 0, NULL, NULL);

	qsort(ctx->side[PA_DIC_H1].array, ctx->side[PA_DIC_H1].used, sizeof(void *), cmp_xmin);
	qsort(ctx->side[PA_DIC_V1].array, ctx->side[PA_DIC_V1].used, sizeof(void *), cmp_ymin);
	qsort(ctx->side[PA_DIC_H2].array, ctx->side[PA_DIC_H2].used, sizeof(void *), cmp_xmax);
	qsort(ctx->side[PA_DIC_V2].array, ctx->side[PA_DIC_V2].used, sizeof(void *), cmp_ymax);

	/* link ordered iscs into a cyclic list */
	for(sd = 0; sd < PA_DIC_sides; sd++) {
		for(m = 0; m < ctx->side[sd].used; m++) {
			pa_dic_isc_t *isc = ctx->side[sd].array[m];
			if (last != NULL)
				last->next = isc;
			last = isc;
		}
	}

	last->next = ctx->corner[0];
	ctx->head = ctx->corner[0];

#if DEBUG_CLIP_DUMP_LOOP != 0
	rnd_trace("isc loop #1:\n");
	i = ctx->head;
	do {
		if (i->vn != NULL)
			rnd_trace(" %ld;%ld .. %ld;%ld vn=%p pl=%p\n", i->x, i->y, i->vn->next->point[0], i->vn->next->point[1], i->vn, i->pl);
		else
			rnd_trace(" %ld;%ld (E)\n", i->x, i->y);
	} while((i = i->next) != ctx->head);
#endif

	/* merge adjacent iscs if the are the same; last is still the last on the list */
	restart:;
	prev = last;
	i = ctx->head;
	do {
		next = i->next;
		if ((i->vn == prev->vn) && (i->vn != NULL))
			goto del;
		else if ((i->x == prev->x) && (i->y == prev->y) && (i->vn == NULL)) {
			/* remove dummy corner if there's a real node on it as well */
			del:;
			prev->next = next;
			pa_dic_isc_free(ctx, i, 1);
			if (i == ctx->head) {
				ctx->head = next;
				goto restart;
			}
			i = prev;
		}
		prev = i;
	} while((i = next) != ctx->head);

#if DEBUG_CLIP_DUMP_LOOP != 0
	rnd_trace("isc loop #2:\n");
	i = ctx->head;
	do {
		if (i->vn != NULL)
			rnd_trace(" %ld;%ld .. %ld;%ld vn=%p pl=%p\n", i->x, i->y, i->vn->next->point[0], i->vn->next->point[1], i->vn, i->pl);
		else
			rnd_trace(" %ld;%ld (E)\n", i->x, i->y);
	} while((i = i->next) != ctx->head);
#endif

	/* reset sides but keep the allocation as cache */
	for(sd = 0; sd < PA_DIC_sides; sd++)
		for(m = 0; m < ctx->side[sd].used; m++)
			ctx->side[sd].used = 0;
}

RND_INLINE void pa_dic_append(pa_dic_ctx_t *ctx, rnd_coord_t x, rnd_coord_t y)
{
	ctx->num_emits++;

	if (ctx->first) {
		ctx->first_x = ctx->last_x = x;
		ctx->first_y = ctx->last_y = y;
		ctx->first = 0;
		ctx->has_coord = 1;
		return;
	}

	/* delay printing the coords; always print the last one that's not yet printed;
	   this allows pa_dic_end() to print the last coords and can omit them if
	   they match the first */
	if (ctx->has_coord) {
		ctx->append_coord(ctx, ctx->last_x, ctx->last_y);
		ctx->has_coord = 0;
	}

	if ((ctx->last_x != x) || (ctx->last_y != y)) {
		ctx->last_x = x;
		ctx->last_y = y;
		ctx->has_coord = 1;
	}
}


RND_INLINE void pa_dic_begin(pa_dic_ctx_t *ctx)
{
	ctx->begin_pline(ctx);
	ctx->first = 1;
	ctx->has_coord = 0;
}

RND_INLINE void pa_dic_end(pa_dic_ctx_t *ctx)
{
	if (ctx->has_coord && ((ctx->last_x != ctx->first_x) || (ctx->last_y != ctx->first_y)))
		ctx->append_coord(ctx, ctx->last_x, ctx->last_y);

	ctx->first = 0;
	ctx->has_coord = 0;

	ctx->end_pline(ctx);
}

RND_INLINE void pa_dic_emit_whole_pline(pa_dic_ctx_t *ctx, rnd_pline_t *pl)
{
	rnd_vnode_t *vn;
	pa_dic_begin(ctx);

	vn = pl->head;
	do {
		pa_dic_append(ctx, vn->point[0], vn->point[1]);
	} while((vn = vn->next) != pl->head);

	pa_dic_end(ctx);
}

RND_INLINE void pa_dic_emit_clipbox(pa_dic_ctx_t *ctx)
{
	ctx->begin_pline(ctx);
	ctx->append_coord(ctx, ctx->clip.X1, ctx->clip.Y1);
	ctx->append_coord(ctx, ctx->clip.X2, ctx->clip.Y1);
	ctx->append_coord(ctx, ctx->clip.X2, ctx->clip.Y2);
	ctx->append_coord(ctx, ctx->clip.X1, ctx->clip.Y2);
	ctx->end_pline(ctx);
}

RND_INLINE pa_dic_isc_t *pa_dic_find_isc_for_node(pa_dic_ctx_t *ctx, rnd_vnode_t *vn)
{
	pa_dic_isc_t *i;
	i = ctx->head;
	do {
		if (i->vn == vn)
			return i;
	} while((i = i->next) != ctx->head);
	return NULL;
}

/* Start from a node that's on the edge; go as far as needed to find the first
   node that's not on edge and return the relation of that node to the box. */
RND_INLINE pa_dic_pt_box_relation_t pa_dic_emit_island_predict(pa_dic_ctx_t *ctx, rnd_vnode_t *start, rnd_pline_t *pl)
{
	rnd_vnode_t *n;

	for(n = start->next; n != start; n = n->next)  {
		pa_dic_pt_box_relation_t dir = pa_dic_pt_in_box(n->point[0], n->point[1], &ctx->clip);
		if (dir == PA_DPT_INSIDE)
			return PA_DPT_INSIDE; /* going inside the box: always accept */
		if (dir == PA_DPT_OUTSIDE)
			return PA_DPT_OUTSIDE; /* going outside the box: always refuse */
		if (dir == PA_DPT_ON_EDGE) {
			pa_dic_isc_t *isc;
			rnd_coord_t midx = (n->point[0] + n->prev->point[0]) / 2;
			rnd_coord_t midy = (n->point[1] + n->prev->point[1]) / 2;


			TODO("Corner case: if difference is only 1 above, the midpoint is rounded badly - use bigcoord");
			/* special case: a pline seg sloping across two edge iscs, test case 
			   clip01b; if the center of the line is inside the box (not on edge)
			   we are good to go */
			if (crd_in_between(midx, ctx->clip.X1, ctx->clip.X2) && crd_in_between(midy, ctx->clip.Y1, ctx->clip.Y2))
				return PA_DPT_INSIDE;

			/* special case: for a coaxial overlapping segment: if pline direction
			   matches clip box edge direction, the polyline surely has points within
			   the box in the svg-right-hand-side neighborhood */
			if ((n->point[0] == n->prev->point[0]) && (n->point[1] != n->prev->point[1])) {
				/* vertical */

				if (pl->flg.orient == RND_PLF_INV)
					return PA_DPT_OUTSIDE; /* hole edge overlap: always refuse; test case clip21c */

				isc = pa_dic_find_isc_for_node(ctx, n);
				if (isc->pcollected)
					return PA_DPT_OUTSIDE; /* don't start collecting something that's already collected; test case: clip25c 50;30 -> 50;70 */

				if (n->point[0] == ctx->clip.X1) {
					if (n->point[1] < n->prev->point[1])
						return PA_DPT_INSIDE;
				}
				else if (n->point[0] == ctx->clip.X2) {
					if (n->point[1] > n->prev->point[1])
						return PA_DPT_INSIDE;
				}
			}
			if ((n->point[0] != n->prev->point[0]) && (n->point[1] == n->prev->point[1])) {
				/* horizontal */
				if (pl->flg.orient == RND_PLF_INV)
					return PA_DPT_OUTSIDE; /* hole edge overlap: always refuse; test case clip21c */

				isc = pa_dic_find_isc_for_node(ctx, n);
				if (isc->pcollected)
					return PA_DPT_OUTSIDE; /* don't start collecting something that's already collected; test case: clip25c 50;30 -> 50;70 */

				if (n->point[1] == ctx->clip.Y1) {
					if (n->prev->point[0] < n->point[0])
						return PA_DPT_INSIDE;
				}
				else if (n->point[1] == ctx->clip.Y2) {
					if (n->prev->point[0] > n->point[0])
						return PA_DPT_INSIDE;
				}
			}
		}
	}

	return PA_DPT_ON_EDGE; /* arrived back to start which is surely on the edge */
}

/* en is on the edge at intersection point si; return 1 if n->next is taking the
   wrong turn (see test case clip26*) */
RND_INLINE int pa_dic_wrong_turn(pa_dic_ctx_t *ctx, rnd_vnode_t *en, pa_dic_isc_t *si)
{
	static const pa_big_angle_t zero = {0,0,0,0,0,0};
	static const pa_big_angle_t one  = {0,0,0,0,1,0};
	static const pa_big_angle_t four = {0,0,0,0,4,0};
	pa_big_angle_t incoming, outgoing;

	pa_big_calc_angle_nn(&incoming, en, en->prev);
	pa_big_calc_angle_nn(&outgoing, en, en->next);

	if (si->y == ctx->clip.Y1) {
		if (pa_angle_gte(outgoing, incoming))
			return 1;
	}

	if (si->x == ctx->clip.X2) {
		if (pa_angle_gte(outgoing, incoming))
			return 1;
	}

	if (si->y == ctx->clip.Y2) {
		/* range of angles is 2..4, but 0 should be counted as 4 */
		if (pa_angle_equ(outgoing, zero))
			memcpy(outgoing, four, sizeof(four));

		if (pa_angle_equ(incoming, zero))
			memcpy(incoming, four, sizeof(four));

		if (pa_angle_gte(outgoing, incoming))
			return 1;
	}

	if (si->x == ctx->clip.X1) {
		/* range of angles is 0..1 and 3..4; for the comparison normal the 3..4
		   angles back to the -1..0 region */
		if (pa_angle_gt(outgoing, one))
			pa_angle_sub(outgoing, outgoing, four);

		if (pa_angle_gt(incoming, one))
			pa_angle_sub(incoming, incoming, four);

		if (pa_angle_gte(outgoing, incoming))
			return 1;
	}

	return 0;
}

/* Emit pline vnodes as long as they are all inside the box. Return the
   intersection where the edge went outside */
RND_INLINE pa_dic_isc_t *pa_dic_gather_pline(pa_dic_ctx_t *ctx, rnd_vnode_t *start, pa_dic_isc_t *start_isc, pa_dic_isc_t *term)
{
	pa_dic_pt_box_relation_t state = PA_DPT_ON_EDGE, dir;
	rnd_vnode_t *prev = NULL;
	rnd_vnode_t *n;
	pa_dic_isc_t *si, *last_si = NULL, *pending_si = NULL;

	assert(start_isc->vn != NULL); /* need to have a pline to start with */

	n = start;
	do {

		dir = pa_dic_pt_in_box(n->point[0], n->point[1], &ctx->clip);
		if (dir == PA_DPT_OUTSIDE) {
			DEBUG_CLIP("        break: going outside\n");
			return last_si;
		}

		if (pending_si != NULL) {
			pending_si->pcollected = 1;
			pending_si = NULL;
		}

		if (dir == PA_DPT_ON_EDGE) {
			si = pa_dic_find_isc_for_node(ctx, n);

			if (si == term) {
				DEBUG_CLIP("        break: term\n");
				return term;
			}

			if (pa_dic_wrong_turn(ctx, n, si)) {
				if ((n->point[0] != term->x) || (n->point[1] != term->y)) {
					/* Append the last valid point before the wrong turn is made
					   so we can pass on to the edge tracer; do not append
					   when not returned to the starting point (test case: clip09) */
					pa_dic_append(ctx, n->point[0], n->point[1]);
				}
				DEBUG_CLIP("        break: wrong turn\n");
				return si;
			}

			/* break at hole/island overlapping edge; test case: clip21c/clip25c */
			if (pa_dic_emit_island_predict(ctx, n, si->pl) == PA_DPT_OUTSIDE) {
				pa_dic_append(ctx, n->point[0], n->point[1]);
				DEBUG_CLIP("        break: overlapping edg\n");
				return si;
			}

			last_si = si;
			pending_si = si; /* mark it later, only if this is the last si before the pline goes outside */
		}

		pa_dic_append(ctx, n->point[0], n->point[1]);
		DEBUG_CLIP("       append: %ld;%ld\n", (long)n->point[0], (long)n->point[1]);
		prev = n;
		n = n->next;
	} while(n != start);

	return start_isc;
}

/* Emit edge points; stop after reaching one that's already collected
   (arrived back at start) or reaching one that's going inside. Return
   this intersection */
RND_INLINE pa_dic_isc_t *pa_dic_gather_edge(pa_dic_ctx_t *ctx, pa_dic_isc_t *start_isc, pa_dic_isc_t *term)
{
	pa_dic_isc_t *i;
	for(i = start_isc->next;; i = i->next) {
		if (i == term) {
			if (i == start_isc->next)
				pa_dic_append(ctx, start_isc->x, start_isc->y); /* test case: clip26, 45;80..55;70..65;80 */
			DEBUG_CLIP("        break: term\n");
			break;
		}
		if (i->ecollected) {
			DEBUG_CLIP("        break: already ecollected\n");
			break;
		}
		pa_dic_append(ctx, i->x, i->y);
		DEBUG_CLIP("       append: %ld;%ld\n", (long)i->x, (long)i->y);
		if ((i->vn != NULL) && (pa_dic_emit_island_predict(ctx, i->vn, i->pl) == PA_DPT_INSIDE)) {
			DEBUG_CLIP("        break: pline going inside\n");
			break;
		}
		i->ecollected = 1;
	}
	return i;
}

RND_INLINE void pa_dic_emit_island_collect_from(pa_dic_ctx_t *ctx, pa_dic_isc_t *from)
{
	pa_dic_pt_box_relation_t ptst;
	rnd_vnode_t *vn;
	pa_dic_isc_t *i;

	if (from->vn == NULL)
		return;

	DEBUG_CLIP("     collect from: %ld;%ld\n", (long)from->x, (long)from->y);

	/* Check where we can get from this intersection */
	ptst = pa_dic_emit_island_predict(ctx, from->vn, from->pl);

	if (ptst == PA_DPT_ON_EDGE) {
		/* corner case: all nodes of the pline are on the clipbox but not in
		   the right direction to have point inside the box -> impossible;
		   it's a safe bet to return empty */
		return;
	}

	if (ptst != PA_DPT_INSIDE)
		return; /* ignore intersection that has an edge going outside */

	/* it's safe to start here, next deviation is going inside */
	pa_dic_begin(ctx);
	pa_dic_append(ctx, from->x, from->y);
	DEBUG_CLIP("       append: %ld;%ld\n", (long)from->x, (long)from->y);
	from->pcollected = 1;

	i = from;
	do {
		assert(i->vn != NULL); /* we need a pline intersection to start from */
		vn = i->vn->next;
		DEBUG_CLIP("      gather pline from: %ld;%ld (%ld;%ld -> %ld;%ld)\n", (long)i->x, (long)i->y, (long)vn->point[0], (long)vn->point[1], (long)vn->next->point[0], (long)vn->next->point[1]);
		i = pa_dic_gather_pline(ctx, vn, i, from);
		if (i == from)
			break;
		i->ecollected = 1;
		DEBUG_CLIP("      gather edge from: %ld;%ld\n", (long)i->x, (long)i->y);
		i = pa_dic_gather_edge(ctx, i, from);
		if (i == from)
			break;
		i->pcollected = 1;
	} while(1);

	pa_dic_end(ctx);
}

RND_INLINE void pa_dic_emit_island_common(pa_dic_ctx_t *ctx, rnd_polyarea_t *pa)
{
	pa_dic_isc_t *i;
	for(i = ctx->head->next; i != ctx->head; i = i->next) {
		if ((i->vn != NULL) && (!i->pcollected))
			pa_dic_emit_island_collect_from(ctx, i);
	}
}

/* In this case the box is filled and holes are cut out */
RND_INLINE void pa_dic_emit_island_inverted(pa_dic_ctx_t *ctx, rnd_polyarea_t *pa)
{
	long num_pts = ctx->num_emits;
	TODO("This is the same as the normal case... maybe just merge them");
	DEBUG_CLIP("    emit island inverted\n");
	pa_dic_emit_island_common(ctx, pa);
	if (num_pts == ctx->num_emits) {
		/* Special case: technically there was an intersection but the walk-around
		   decided not to include anything so our output is empty; in an inverted
		   situation this means the whole box should be filled. Test case: clip21d */
		pa_dic_emit_clipbox(ctx);
	}
}

/* The box cuts into the outer contour of the island; we are basically
   drawing the contour of the island except for the box sections */
RND_INLINE void pa_dic_emit_island_normal(pa_dic_ctx_t *ctx, rnd_polyarea_t *pa)
{
	DEBUG_CLIP("    emit island normal\n");
	pa_dic_emit_island_common(ctx, pa);
}

/* Dice up a single island that is intersected or has holes.
   The contour is already labelled, but holes are not */
RND_INLINE void pa_dic_emit_island_expensive(pa_dic_ctx_t *ctx, rnd_polyarea_t *pa)
{
	rnd_pline_t *pl;

	DEBUG_CLIP("   emit island expensive\n");
	/* label all holes */
	for(pl = pa->contours->next; pl != NULL; pl = pl->next) {
		pa_dic_pline_label(ctx, pl);
		if (pl->flg.llabel == PA_PLD_WRAPPER)
			return; /* if the box is within a hole, it surely won't contain anything */
	}

	pa_dic_sort_sides(ctx);

	pl = pa->contours;
	if (pl->flg.llabel == PA_PLD_WRAPPER)
		pa_dic_emit_island_inverted(ctx, pa);
	else
		pa_dic_emit_island_normal(ctx, pa);
}

/* Emit a rectangular area of a single island (potentially with holes) */
RND_INLINE void pa_dic_emit_island(pa_dic_ctx_t *ctx, rnd_polyarea_t *pa)
{
	rnd_pline_t *pl;

	pl = pa->contours;
	DEBUG_CLIP("  emit island: %ld;%ld\n", (long)pl->head->point[0], (long)pl->head->point[1]);
	pa_dic_pline_label(ctx, pl);

	/* first handle a few cheap common cases */
	switch(pl->flg.llabel) {
		case PA_PLD_UNKNWN:
			assert(!"dic label failed");
			goto fin;
		case PA_PLD_AWAY:
			goto fin; /* no need to emit anything */

		case PA_PLD_INSIDE:
			if (pl->next == NULL) {
				/* there are no holes and the whole thig is inside -> plain emit */
				pa_dic_emit_whole_pline(ctx, pl);
				goto fin;
			}
			break;
		case PA_PLD_WRAPPER:
			if (pl->next == NULL) {
				/* there are no holes and it covers the whole area: emit the box */
				pa_dic_emit_clipbox(ctx);
				goto fin;
			}
			break;

		case PA_PLD_ISECTED:
			break;
	}

	/* have to do the full thing */
	pa_dic_emit_island_expensive(ctx, pa);

	fin:;
	/* Reset the only flag we changed in the input */
	for(pl = pa->contours; pl != NULL; pl = pl->next)
		pl->flg.llabel = PA_PLL_UNKNWN;
}

RND_INLINE void pa_dic_emit_pa(pa_dic_ctx_t *ctx, rnd_polyarea_t *start)
{
	rnd_polyarea_t *pa;

	pa = start;
	do {
		pa_dic_reset_ctx_pa(ctx);
		pa_dic_emit_island(ctx, pa);
	} while((pa = pa->f) != start);
	pa_dic_free_ctx(ctx);
}

/*** slicer ***/
typedef struct pa_slc_endp_s {
	rnd_pline_t *pl;   /* the hole creating this segment */
	rnd_coord_t x;     /* clipped into ctx clipping area */
	long height;       /* stack height after (to the right of) this endpoint */
	unsigned side:1;   /* 0 for start/left, 1 for end/right */
	unsigned sliced:1; /* only when side == 0 */
} pa_slc_endp_t;

#define GVT(x) vtslc_ ## x
#define GVT_ELEM_TYPE pa_slc_endp_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 4096
#define GVT_START_SIZE 128
#define GVT_FUNC
#define GVT_SET_NEW_BYTES_TO 0

#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
#include <genvector/genvector_impl.c>
#include <genvector/genvector_undef.h>

typedef struct pa_slc_ctx_s {
	/* input */
	rnd_polyarea_t *pa;
	vtslc_t v;
	rnd_coord_t minx, miny, maxx, maxy;

	/* output */
	vtc0_t cuts;
} pa_slc_ctx_t;

/* map endpoints of the holes of a single contour */
RND_INLINE void pa_slc_map_pline_holes(pa_slc_ctx_t *ctx, rnd_pline_t *contour)
{
	rnd_pline_t *hole;
	for(hole = contour->next; hole != NULL; hole = hole->next) {
		pa_slc_endp_t *ep;

		if ((hole->xmin > ctx->maxx) || (hole->xmax < ctx->minx) || (hole->ymin > ctx->maxy) || (hole->ymax < ctx->miny))
			continue; /* ignore holes that are fully outside of the clipping area */

		ep = vtslc_alloc_append(&ctx->v, 2);
		ep[0].pl = hole;
		ep[0].x = RND_MAX(hole->xmin, ctx->minx);
		ep[0].side = 0;
		ep[1].pl = hole;
		ep[1].x = RND_MIN(hole->xmax, ctx->maxx);
		ep[1].side = 1;
		hole->flg.sliced = 0;
	}
}

/* map endpoints of all islands */
RND_INLINE void pa_slc_map_pline_pa(pa_slc_ctx_t *ctx)
{
	rnd_polyarea_t *pa = ctx->pa;
	do {
		pa_slc_map_pline_holes(ctx, pa->contours);
	} while((pa = pa->f) != ctx->pa);
}

static int cmp_slc_endp(const void *a_, const void *b_)
{
	pa_slc_endp_t *a = a_, *b = b_;
	return (a->x < b->x) ? -1 : +1;
}

/* sort endpoints from left to right and fill in ->height */
RND_INLINE void pa_slc_sort_and_compute_heights(pa_slc_ctx_t *ctx)
{
	long n, h;
	pa_slc_endp_t *ep;

	qsort(ctx->v.array, ctx->v.used, sizeof(pa_slc_endp_t), cmp_slc_endp);
	for(n = h = 0, ep = ctx->v.array; n < ctx->v.used; n++,ep++) {
		if (ep->side == 0)
			h++;
		else
			h--;
		ep->height = h;
	}
	assert(h == 0);
}

static int pa_slc_cmp_coord(const void *a_, const void *b_)
{
	const rnd_coord_t *a = a_, *b = b_;
	return (*a < *b) ? -1 : +1;
}

RND_INLINE void pa_slc_dump(pa_slc_ctx_t *ctx)
{
#ifdef WANT_DEBUG_SLICE
	long n;
	pa_slc_endp_t *ep;

	for(n = 0, ep = ctx->v.array; n < ctx->v.used; n++,ep++)
		DEBUG_SLICE(" %c @%d ^%d", ep->side==0 ? '{' : '}', ep->x, ep->height);

	DEBUG_SLICE("\n");
#endif
}

/* figure a relatively low number of cuts */
RND_INLINE void pa_slc_find_cuts(pa_slc_ctx_t *ctx)
{
	long n, m;
	long remaining; /* number of plines still waiting for a cut */
	rnd_coord_t x1, x2, xc;
	pa_slc_endp_t *ep, *ep2;

	pa_slc_sort_and_compute_heights(ctx);
	for(remaining = ctx->v.used/2; remaining > 0; ) {
		long best_n, best_h = -1;

		DEBUG_SLICE(" slc [%d] ", remaining);
		pa_slc_dump(ctx);

		/* find highest tower to slice */
		for(n = 0, ep = ctx->v.array; n < ctx->v.used-1; n++,ep++) {
			if (ep->height > best_h) {
				best_h = ep->height;
				best_n = n;
			}
		}

		assert(best_h > 0);

		/* insert a cut halfway in between best_n and the next endpoint */
		x1 = ctx->v.array[best_n].x;
		x2 = ctx->v.array[best_n+1].x;
		xc = (x1+x2)/2;
		vtc0_append(&ctx->cuts, xc);

		DEBUG_SLICE("best: @%d ^%d cut at %d\n", best_n, best_h, xc);

		/* mark all affected plines already sliced and decrease heights and remaining */
		for(n = 0, ep = ctx->v.array; n < ctx->v.used-1; n++,ep++) {
			if ((ep->side == 0) && !ep->pl->flg.sliced && (xc >= ep->pl->xmin) && (xc <= ep->pl->xmax)) {
				ep->pl->flg.sliced = 1;
				remaining--;
				DEBUG_SLICE(" remove %d..%d\n", ep->pl->xmin, ep->pl->xmax);
				/* decrease height over this pline */
				for(m = n, ep2 = ep; (m < ctx->v.used) && (ep2->x < ep->pl->xmax); m++,ep2++)
					ep2->height--;
			}
		}
	}


	DEBUG_SLICE("slc post ");
	pa_slc_dump(ctx);

	/* reset pline flags */
	for(n = 0, ep = ctx->v.array; n < ctx->v.used-1; n++,ep++)
		ep->pl->flg.sliced = 0;

	/* sort and remove redundancies */
	qsort(ctx->cuts.array, ctx->cuts.used, sizeof(rnd_coord_t), pa_slc_cmp_coord);
	for(n = 0; n < ctx->cuts.used-1; n++) {
		if (ctx->cuts.array[n] == ctx->cuts.array[n+1]) {
			vtc0_remove(&ctx->cuts, n, 1);
			n--;
		}
	}
}

RND_INLINE void pa_slc_slice(pa_slc_ctx_t *ctx)
{

}

/*** API ***/
void rnd_polyarea_clip_box_emit(pa_dic_ctx_t *ctx, rnd_polyarea_t *pa)
{
	pa_dic_emit_pa(ctx, pa);
}

/* New, emit API; overwrites ctx->clip */
void rnd_polyarea_slice_noholes(pa_dic_ctx_t *ctx, rnd_polyarea_t *pa)
{
	pa_slc_ctx_t slc = {0};
	long n;

	slc.minx = ctx->clip.X1; slc.miny = ctx->clip.Y1;
	slc.maxx = ctx->clip.X2; slc.maxy = ctx->clip.Y2;
	slc.pa = pa;

	/* so that edges are no special case */
	vtc0_append(&slc.cuts, ctx->clip.X1);
	vtc0_append(&slc.cuts, ctx->clip.X2);

	pa_slc_map_pline_pa(&slc);
	pa_slc_find_cuts(&slc);
	pa_slc_slice(&slc);

	for(n = 1; n < slc.cuts.used; n++) {
		ctx->clip.X1 = slc.cuts.array[n-1];
		ctx->clip.X2 = slc.cuts.array[n];
		pa_dic_emit_pa(ctx, pa);
	}

	vtslc_uninit(&slc.v);
	vtc0_uninit(&slc.cuts);
}


#if PA_USE_NEW_DICER
/* Old, compatibility API */

typedef struct {
	/* user config: */
	void (*emit_pline)(rnd_pline_t *, void *);
	void *user_data;

	/* transient/cache: */
	rnd_pline_t *pl;
} pa_nhdic_ctx_t;

static void pa_nhdic_begin_pline(pa_dic_ctx_t *ctx)
{
	pa_nhdic_ctx_t *cctx = ctx->user_data;
	assert(cctx->pl == NULL);
}

static void pa_nhdic_append_coord(pa_dic_ctx_t *ctx, rnd_coord_t x, rnd_coord_t y)
{
	pa_nhdic_ctx_t *cctx = ctx->user_data;
	rnd_vector_t v;

	v[0] = x;
	v[1] = y;

	if (cctx->pl == NULL)
		cctx->pl = rnd_poly_contour_new(v);
	else
		rnd_poly_vertex_include(cctx->pl->head->prev, rnd_poly_node_create(v));
}

static void pa_nhdic_end_pline(pa_dic_ctx_t *ctx)
{
	rnd_polyarea_t *new_pa;
	pa_nhdic_ctx_t *cctx = ctx->user_data;
	assert(cctx->pl != NULL);

	pa_pline_update(cctx->pl, 0);

	cctx->emit_pline(cctx->pl, cctx->user_data);

	/* cctx->pl ownership has been passed over to emit_pline() above */
	cctx->pl = NULL;
}



void rnd_polyarea_no_holes_dicer(rnd_polyarea_t *pa, rnd_coord_t clipX1, rnd_coord_t clipY1, rnd_coord_t clipX2, rnd_coord_t clipY2, void (*emit)(rnd_pline_t *, void *), void *user_data)
{
	pa_dic_ctx_t ctx = {0};;
	pa_nhdic_ctx_t nh = {0};

	nh.emit_pline = emit;
	nh.user_data = user_data;

	if ((clipX1 == clipX2) && (clipY1 == clipY2)) {
		clipX1 = clipY1 = -RND_COORD_MAX;
		clipX2 = clipY2 = +RND_COORD_MAX;
	}

	ctx.clip.X1 = clipX1; ctx.clip.Y1 = clipY1;
	ctx.clip.X2 = clipX2; ctx.clip.Y2 = clipY2;
	ctx.begin_pline = pa_nhdic_begin_pline;
	ctx.append_coord = pa_nhdic_append_coord;
	ctx.end_pline = pa_nhdic_end_pline;
	ctx.user_data = &nh;

	rnd_polyarea_slice_noholes(&ctx, pa);
	pa_polyarea_free_all(&pa);
	pa_dic_free_ctx(&ctx);
}
#endif
