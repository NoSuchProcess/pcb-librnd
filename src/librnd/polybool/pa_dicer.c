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

typedef enum pa_dic_pline_label_e { /* pline's flg.label */
	PA_PLD_UNKNWN  = 0,
	PA_PLD_INSIDE  = 1,    /* pline is fully inside the box */
	PA_PLD_WRAPPER = 2,    /* the box is fully included within the pline */
	PA_PLD_ISECTED = 3,    /* they are crossing */
	PA_PLD_AWAY = 4        /* pline is fully outside but not wrapping the box */
} pa_dic_pline_label_t;

struct pa_dic_isc_s {
	pa_seg_t *seg;
	rnd_coord_t x, y;
	int isc_idx;     /* first or second, from the perspective of the clip box line; second means overlapping line or arc */
	unsigned coax:1; /* set if line is coaxial (may overlap with) the edge */
	unsigned collected:1; /* already emitted in the output */
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
	int sd;
	long m;

	for(sd = 0; sd < PA_DIC_sides; sd++) {
		for(m = 0; m < ctx->side[sd].used; m++)
			pa_dic_isc_free(ctx, ctx->side[sd].array[m], destroy);
		ctx->side[sd].used = 0;
		if (destroy)
			vtp0_uninit(&ctx->side[sd]);
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

/* Record an intersection */
RND_INLINE pa_dic_isc_t *pa_dic_isc(pa_dic_ctx_t *ctx, pa_seg_t *seg, pa_dic_side_t side, rnd_coord_t isc_x, rnd_coord_t isc_y, int *iscs, int coax)
{
	pa_dic_isc_t *isc = pa_dic_isc_alloc(ctx);

	if (iscs != NULL) {
		assert(*iscs < 2);
	}

	isc->seg = seg;
	isc->x = isc_x;
	isc->y = isc_y;
	isc->isc_idx = (iscs == NULL ? 0 : *iscs);
	isc->coax = coax;
	isc->collected = 0;

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

	TODO("arc: needs special code here");

	lx1 = seg->v->point[0]; ly1 = seg->v->point[1];
	lx2 = seg->v->next->point[0]; ly2 = seg->v->next->point[1];

	if (ly1 == ly2) {
		/* special case: horizontal line, may overlap */
		if (ly1 != y)
			return 0;
		if (lx1 > lx2)
			rnd_swap(rnd_coord_t, lx1, lx2);
		if ((lx1 > ctx->clip.X2) || (lx2 < ctx->clip.X1))
			return 0;

		if (crd_in_between(lx1, ctx->clip.X1, ctx->clip.X2))
			pa_dic_isc(ctx, seg, side, lx1, y, &iscs, 1);
		if (crd_in_between(lx2, ctx->clip.X1, ctx->clip.X2))
			pa_dic_isc(ctx, seg, side, lx2, y, &iscs, 1);
		if (crd_in_between(ctx->clip.X1, lx1, lx2))
			pa_dic_isc(ctx, seg, side, ctx->clip.X1, y, &iscs, 1);
		if (crd_in_between(ctx->clip.X2, lx1, lx2))
			pa_dic_isc(ctx, seg, side, ctx->clip.X2, y, &iscs, 1);
		if (ctx->clip.X1 == lx1)
			pa_dic_isc(ctx, seg, side, lx1, y, &iscs, 1);
		if (ctx->clip.X2 == lx2)
			pa_dic_isc(ctx, seg, side, lx2, y, &iscs, 1);
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
		pa_dic_isc(ctx, seg, side, x, y, &iscs, 0);
	}

	return iscs;
}

/* Vertical box edge intersection with seg */
static int pa_dic_isc_v(pa_dic_ctx_t *ctx, pa_seg_t *seg, pa_dic_side_t side, rnd_coord_t x)
{
	rnd_coord_t lx1, ly1, lx2, ly2;
	int iscs = 0;

	TODO("arc: needs special code here");

	lx1 = seg->v->point[0]; ly1 = seg->v->point[1];
	lx2 = seg->v->next->point[0]; ly2 = seg->v->next->point[1];


	if (lx1 == lx2) {
		/* special case: vertical line, may overlap */
		if (lx1 != x)
			return 0;
		if (ly1 > ly2)
			rnd_swap(rnd_coord_t, ly1, ly2);

		if (crd_in_between(ly1, ctx->clip.Y1, ctx->clip.Y2))
			pa_dic_isc(ctx, seg, side, x, ly1, &iscs, 1);
		if (crd_in_between(ly2, ctx->clip.Y1, ctx->clip.Y2))
			pa_dic_isc(ctx, seg, side, x, ly2, &iscs, 1);
		if (crd_in_between(ctx->clip.Y1, ly1, ly2))
			pa_dic_isc(ctx, seg, side, x, ctx->clip.Y1, &iscs, 1);
		if (crd_in_between(ctx->clip.Y2, ly1, ly2))
			pa_dic_isc(ctx, seg, side, x, ctx->clip.Y2, &iscs, 1);
		if (ctx->clip.Y1 == ly1)
			pa_dic_isc(ctx, seg, side, x, ly1, &iscs, 1);
		if (ctx->clip.Y2 == ly2)
			pa_dic_isc(ctx, seg, side, x, ly2, &iscs, 1);
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
		pa_dic_isc(ctx, seg, side, x, y, &iscs, 0);
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
	pa_dic_pline_label_side(ctx, pl, PA_DIC_V1, pa_dic_isc_v, ctx->clip.X1, ctx->clip.X2, ctx->clip.Y1, ctx->clip.X2, ctx->clip.Y2);
	pa_dic_pline_label_side(ctx, pl, PA_DIC_H2, pa_dic_isc_h, ctx->clip.Y2, ctx->clip.X1, ctx->clip.Y2, ctx->clip.X2, ctx->clip.Y2);
	pa_dic_pline_label_side(ctx, pl, PA_DIC_V2, pa_dic_isc_v, ctx->clip.X2, ctx->clip.X1, ctx->clip.Y1, ctx->clip.X1, ctx->clip.Y2);
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

static int cmp_xmin(const void *A, const void *B)
{
	const pa_dic_isc_t * const *a = A, * const *b = B;
	return ((*a)->x < (*b)->x) ? -1 : +1;
}

static int cmp_ymin(const void *A, const void *B)
{
	const pa_dic_isc_t * const *a = A, * const *b = B;
	return ((*a)->y < (*b)->y) ? -1 : +1;
}

static int cmp_xmax(const void *A, const void *B)
{
	const pa_dic_isc_t * const *a = A, * const *b = B;
	return ((*a)->x > (*b)->x) ? -1 : +1;
}

static int cmp_ymax(const void *A, const void *B)
{
	const pa_dic_isc_t * const *a = A, * const *b = B;
	return ((*a)->y > (*b)->y) ? -1 : +1;
}

RND_INLINE void pa_dic_sort_sides(pa_dic_ctx_t *ctx)
{
	int sd;
	long m;
	pa_dic_isc_t *last = NULL;

	/* create dummy intersetions for corners for easier walkarounds */
	ctx->corner[0] = pa_dic_isc(ctx, NULL, PA_DIC_H1, ctx->clip.X1, ctx->clip.Y1, NULL, 0);
	ctx->corner[1] = pa_dic_isc(ctx, NULL, PA_DIC_V1, ctx->clip.X2, ctx->clip.Y1, NULL, 0);
	ctx->corner[2] = pa_dic_isc(ctx, NULL, PA_DIC_H2, ctx->clip.X2, ctx->clip.Y2, NULL, 0);
	ctx->corner[3] = pa_dic_isc(ctx, NULL, PA_DIC_V2, ctx->clip.X1, ctx->clip.Y2, NULL, 0);

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
}

RND_INLINE void pa_dic_append(pa_dic_ctx_t *ctx, rnd_coord_t x, rnd_coord_t y)
{
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

#define PA_DIC_STEP(n, dir) n = (dir == 'N' ? n->next : n->prev)

/* Start from a node that's on the edge; go as far as needed to find the first
   node that's not on edge and return the relation of that node to the box.
   Dir is either N or P for ->next or ->prev traversal */
RND_INLINE pa_dic_pt_box_relation_t pa_dic_emit_island_predict(pa_dic_ctx_t *ctx, rnd_vnode_t *start, char dir)
{
	rnd_vnode_t *n;

	for(n = start->next; n != start; PA_DIC_STEP(n, dir))  {
		pa_dic_pt_box_relation_t dir = pa_dic_pt_in_box(n->point[0], n->point[1], &ctx->clip);
		if (dir != PA_DPT_ON_EDGE)
			return dir;
	}

	return PA_DPT_ON_EDGE; /* arrived back to start which is surely on the edge */
}

RND_INLINE char pa_dic_pline_walkdir(rnd_pline_t *pl)
{
	return pl->flg.orient == RND_PLF_INV ? 'P' : 'N';
}

RND_INLINE pa_dic_isc_t *pa_dic_find_isc_for_node(pa_dic_ctx_t *ctx, rnd_vnode_t *vn)
{
	pa_dic_isc_t *i;
	i = ctx->head;
	do {
		if ((i->seg != NULL) && (i->seg->v == vn))
			return i;
	} while((i = i->next) != ctx->head);
	return NULL;
}

/* Emit pline vnodes as long as they are all inside the box. Return the
   intersection where the edge went outside */
RND_INLINE pa_dic_isc_t *pa_dic_gather_pline(pa_dic_ctx_t *ctx, rnd_vnode_t *start, pa_dic_isc_t *start_isc)
{
	pa_dic_pt_box_relation_t state = PA_DPT_ON_EDGE, dir;
	rnd_vnode_t *prev = NULL;
	rnd_vnode_t *n;
	pa_dic_isc_t *si;
	char walkdir;

	assert(start_isc->seg != NULL); /* need to have a pline to start with */

	walkdir = pa_dic_pline_walkdir(start_isc->seg->p);

	n = start;
	do {
		dir = pa_dic_pt_in_box(n->point[0], n->point[1], &ctx->clip);
		if (dir == PA_DPT_OUTSIDE) {
			TODO("Handle overlap on box corner: the only overlapping case is when one of the corners is on the seg?");
			si = pa_dic_find_isc_for_node(ctx, prev);
			pa_dic_append(ctx, si->x, si->y);
			si->collected = 1;
			return si;
		}
		if ((dir == PA_DPT_ON_EDGE) && (prev != NULL)) {
			/* arrived back on an edge; there may be a double isc here if the box crosses
			   a node, see test case clip04; mark the second isc */
			si = pa_dic_find_isc_for_node(ctx, prev);
			si->collected = 1;
		}

		pa_dic_append(ctx, n->point[0], n->point[1]);
		prev = n;
		PA_DIC_STEP(n, walkdir);
	} while(n != start);

	/* arrived back; there may be a double isc here if the box crosses
	   a node, see test case clip04; mark the second isc */
	si = pa_dic_find_isc_for_node(ctx, prev);
	si->collected = 1;

	return start_isc;
}

/* Emit edge points; stop after reaching one that's already collected
   (arrived back at start) or reaching one that's going inside. Return
   this intersection */
RND_INLINE pa_dic_isc_t *pa_dic_gather_edge(pa_dic_ctx_t *ctx, pa_dic_isc_t *start_isc)
{
	pa_dic_isc_t *i;
	for(i = start_isc->next;; i = i->next) {
		if (i->collected)
			break;
		pa_dic_append(ctx, i->x, i->y);
		if ((i->seg != NULL) && (pa_dic_emit_island_predict(ctx, i->seg->v, pa_dic_pline_walkdir(i->seg->p)) == PA_DPT_INSIDE))
			break;
	}
	return i;
}

RND_INLINE void pa_dic_emit_island_collect_from(pa_dic_ctx_t *ctx, pa_dic_isc_t *from)
{
	pa_dic_pt_box_relation_t ptst;
	rnd_vnode_t *vn;
	pa_dic_isc_t *i;
	char walkdir;
	int sd;
	long m;

	if (from->seg == NULL)
		return;

	/* Check where we can get from this intersection */
	walkdir = pa_dic_pline_walkdir(from->seg->p);
	ptst = pa_dic_emit_island_predict(ctx, from->seg->v, walkdir);

	if (ptst == PA_DPT_ON_EDGE) {
		/* corner case: all nodes of the pline are on the clipbox but it could take
		   shortcuts - emit the pline and mark all of its segs collected */
		pa_dic_emit_whole_pline(ctx, from->seg->p);
		for(sd = 0; sd < PA_DIC_sides; sd++) {
			for(m = 0; m < ctx->side[sd].used; m++) {
				pa_dic_isc_t *isc = ctx->side[sd].array[m];
				if ((isc->seg != NULL) && (isc->seg->p == from->seg->p))
					isc->collected = 1;
			}
		}
		return;
	}

	if (ptst != PA_DPT_INSIDE)
		return; /* ignore intersection that has an edge going outside */

	/* it's safe to start here, next deviation is going inside */
	pa_dic_begin(ctx);
	pa_dic_append(ctx, from->x, from->y);
	from->collected = 1;

	/* also mark the edge coming from outside (test case clip09) so we are not
	   gathering the same shape again */
	i = pa_dic_find_isc_for_node(ctx, (walkdir == 'N') ? from->seg->v->prev : from->seg->v->next);
	if ((i != NULL) && (i->x == from->x) && (i->y == from->y))
		i->collected = 1;


	i = from;
	do {
		assert(i->seg != NULL); /* we need a pline intersection to start from */
		vn = i->seg->v;
		PA_DIC_STEP(vn, walkdir);
		i = pa_dic_gather_pline(ctx, vn, i);
		if (i->collected)
			break;
		i = pa_dic_gather_edge(ctx, i);
		if (i->collected)
			break;
	} while(0);

	pa_dic_end(ctx);
}

/* In this case the box is filled and holes are cut out */
RND_INLINE void pa_dic_emit_island_inverted(pa_dic_ctx_t *ctx, rnd_polyarea_t *pa)
{
	TODO("implement me");
	assert("!implement me");
}

/* The box cuts into the outer contour of the island; we are basically
   drawing the contour of the island except for the box sections */
RND_INLINE void pa_dic_emit_island_normal(pa_dic_ctx_t *ctx, rnd_polyarea_t *pa)
{
	pa_dic_isc_t *i;
	for(i = ctx->head->next; i != ctx->head; i = i->next) {
		if ((i->seg != NULL) && (!i->collected))
			pa_dic_emit_island_collect_from(ctx, i);
	}
}

/* Dice up a single island that is intersected or has holes.
   The contour is already labelled, but holes are not */
RND_INLINE void pa_dic_emit_island_expensive(pa_dic_ctx_t *ctx, rnd_polyarea_t *pa)
{
	rnd_pline_t *pl;

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

/* API */
void rnd_polyarea_clip_box_emit(pa_dic_ctx_t *ctx, rnd_polyarea_t *pa)
{
	pa_dic_emit_pa(ctx, pa);
}
