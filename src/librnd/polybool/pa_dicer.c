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

typedef enum pa_dic_pline_label_e { /* pline's flg.label */
	PA_PLD_UNKNWN  = 0,
	PA_PLD_INSIDE  = 1,    /* pline is fully inside the box */
	PA_PLD_AROUND  = 2,    /* the box is fully included within the pline */
	PA_PLD_ISECTED = 3,    /* they are crossing */
	PA_PLD_AWAY = 4        /* pline is fully outside but not wrapping the box */
} pa_dic_pline_label_t;


typedef enum pa_dic_side_s {
	PA_DIC_H1 = 0, /* in pcb-rnd/svg coord system: north; in doc coord system: south */
	PA_DIC_V1,     /* in pcb-rnd/svg coord system: east; in doc coord system: east */
	PA_DIC_H2,     /* in pcb-rnd/svg coord system: south; in doc coord system: north */
	PA_DIC_V2,     /* in pcb-rnd/svg coord system: west; in doc coord system: west */
	PA_DIC_sides
} pa_dic_side_t;

typedef struct pa_dic_isc_s {
	pa_seg_t *seg;
	rnd_coord_t x, y;
	int isc_idx;     /* first or second, from the perspective of the clip box line; second means overlapping line or arc */
	unsigned coax:1; /* set if line is coaxial (may overlap with) the edge */
} pa_dic_isc_t;

typedef struct pa_dic_ctx_s {
	rnd_box_t clip;
	vtp0_t side[PA_DIC_sides];
} pa_dic_ctx_t;

/* Returns whether c is in between low and high without being equal to either */
RND_INLINE int crd_in_between(rnd_coord_t c, rnd_coord_t low, rnd_coord_t high)
{
	return (c > low) && (c < high);
}

/* Record an intersection */
RND_INLINE void pa_dic_isc(pa_dic_ctx_t *ctx, pa_seg_t *seg, pa_dic_side_t side, rnd_coord_t isc_x, rnd_coord_t isc_y, int *iscs, int coax)
{
	pa_dic_isc_t *isc = malloc(sizeof(pa_dic_isc_t));

	assert(*iscs < 2);

	isc->seg = seg;
	isc->x = isc_x;
	isc->y = isc_y;
	isc->isc_idx = *iscs;
	isc->coax = coax;

	vtp0_append(&ctx->side[side], isc);

	(*iscs)++;
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

/* Compute all intersections between the label and pl, allocate pa_dic_isc_t *
   for them and append them in ctx->side[] in random order. Also set
   pl->flg.llabel */
RND_INLINE void pa_dic_pline_label(pa_dic_ctx_t *ctx, rnd_pline_t *pl)
{
	pl->flg.llabel = PA_PTL_UNKNWN;

	TODO("Cheap tests for pl bbox fully inside or fully away");

	pa_dic_pline_label_side(ctx, pl, PA_DIC_H1, pa_dic_isc_h, ctx->clip.Y1, ctx->clip.X1, ctx->clip.Y1, ctx->clip.X2, ctx->clip.Y1);
	pa_dic_pline_label_side(ctx, pl, PA_DIC_V1, pa_dic_isc_v, ctx->clip.X1, ctx->clip.X2, ctx->clip.Y1, ctx->clip.X2, ctx->clip.Y2);
	pa_dic_pline_label_side(ctx, pl, PA_DIC_H2, pa_dic_isc_h, ctx->clip.Y2, ctx->clip.X1, ctx->clip.Y2, ctx->clip.X2, ctx->clip.Y2);
	pa_dic_pline_label_side(ctx, pl, PA_DIC_V2, pa_dic_isc_v, ctx->clip.X2, ctx->clip.X1, ctx->clip.Y1, ctx->clip.X1, ctx->clip.Y2);


	if (pl->flg.llabel == PA_PLD_ISECTED)
		return;

	TODO("Figure if PA_PLD_INSIDE, PA_PLD_AROUND or PA_PLD_AWAY");
}

