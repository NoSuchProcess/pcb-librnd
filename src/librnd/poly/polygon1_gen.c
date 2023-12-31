/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,2010 Thomas Nau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include <librnd/rnd_config.h>

#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include <librnd/core/global_typedefs.h>
#include <librnd/poly/polyarea.h>
#include <librnd/core/math_helper.h>
#include <librnd/core/unit.h>

#include <librnd/poly/polygon1_gen.h>

/* kept to ensure nanometer compatibility */
#define ROUND(x) ((long)(((x) >= 0 ? (x) + 0.5  : (x) - 0.5)))

static double rotate_circle_seg[4];
int rotate_circle_seg_inited = 0;

static void init_rotate_cache(void)
{
	if (!rotate_circle_seg_inited) {
		double cos_ang = cos(2.0 * M_PI / RND_POLY_CIRC_SEGS_F);
		double sin_ang = sin(2.0 * M_PI / RND_POLY_CIRC_SEGS_F);

		rotate_circle_seg[0] = cos_ang;
		rotate_circle_seg[1] = -sin_ang;
		rotate_circle_seg[2] = sin_ang;
		rotate_circle_seg[3] = cos_ang;
		rotate_circle_seg_inited = 1;
	}
}

rnd_polyarea_t *rnd_poly_from_contour_nochk(rnd_pline_t *contour)
{
	rnd_polyarea_t *p;
	rnd_poly_contour_pre(contour, rnd_true);
	assert(contour->Flags.orient == RND_PLF_DIR);
	if (!(p = rnd_polyarea_create()))
		return NULL;
	rnd_polyarea_contour_include(p, contour);
	return p;
}

rnd_polyarea_t *rnd_poly_from_contour(rnd_pline_t *contour)
{
	rnd_polyarea_t *p = rnd_poly_from_contour_nochk(contour);
	assert(rnd_poly_valid(p));
	return p;
}


rnd_polyarea_t *rnd_poly_from_contour_autoinv(rnd_pline_t *contour)
{
	rnd_polyarea_t *p;
	rnd_poly_contour_pre(contour, rnd_true);
	if (contour->Flags.orient != RND_PLF_DIR)
		rnd_poly_contour_inv(contour);
	if (!(p = rnd_polyarea_create()))
		return NULL;
	rnd_polyarea_contour_include(p, contour);
	assert(rnd_poly_valid(p));
	return p;
}


#define ARC_ANGLE 5
static rnd_polyarea_t *ArcPolyNoIntersect(rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t astart, rnd_angle_t adelta, rnd_coord_t thick, int end_caps)
{
	rnd_pline_t *contour = NULL;
	rnd_polyarea_t *np = NULL;
	rnd_vector_t v, v2;
	int i, segs;
	double ang, da, rx, ry;
	long half;
	double radius_adj;
	rnd_coord_t edx, edy, endx1, endx2, endy1, endy2;

	if (thick <= 0)
		return NULL;
	if (adelta < 0) {
		astart += adelta;
		adelta = -adelta;
	}
	half = (thick + 1) / 2;

	rnd_arc_get_endpt(cx, cy, width, height, astart, adelta, 0, &endx1, &endy1);
	rnd_arc_get_endpt(cx, cy, width, height, astart, adelta, 1, &endx2, &endy2);

	/* start with inner radius */
	rx = MAX(width - half, 0);
	ry = MAX(height - half, 0);
	segs = 1;
	if (thick > 0)
		segs = MAX(segs, adelta * M_PI / 360 *
							 sqrt(sqrt((double) rx * rx + (double) ry * ry) / RND_POLY_ARC_MAX_DEVIATION / 2 / thick));
	segs = MAX(segs, adelta / ARC_ANGLE);

	ang = astart;
	da = (1.0 * adelta) / segs;
	radius_adj = (M_PI * da / 360) * (M_PI * da / 360) / 2;
	v[0] = cx - rx * cos(ang * RND_M180);
	v[1] = cy + ry * sin(ang * RND_M180);
	if ((contour = rnd_poly_contour_new(v)) == NULL)
		return 0;
	for (i = 0; i < segs - 1; i++) {
		ang += da;
		v[0] = cx - rx * cos(ang * RND_M180);
		v[1] = cy + ry * sin(ang * RND_M180);
		rnd_poly_vertex_include(contour->head->prev, rnd_poly_node_create(v));
	}
	/* find last point */
	ang = astart + adelta;
	v[0] = cx - rx * cos(ang * RND_M180) * (1 - radius_adj);
	v[1] = cy + ry * sin(ang * RND_M180) * (1 - radius_adj);

	/* add the round cap at the end */
	if (end_caps)
		rnd_poly_frac_circle(contour, endx2, endy2, v, 2);

	/* and now do the outer arc (going backwards) */
	rx = (width + half) * (1 + radius_adj);
	ry = (width + half) * (1 + radius_adj);
	da = -da;
	for (i = 0; i < segs; i++) {
		v[0] = cx - rx * cos(ang * RND_M180);
		v[1] = cy + ry * sin(ang * RND_M180);
		rnd_poly_vertex_include(contour->head->prev, rnd_poly_node_create(v));
		ang += da;
	}

	/* explicitly draw the last point if the manhattan-distance is large enough */
	ang = astart;
	v2[0] = cx - rx * cos(ang * RND_M180) * (1 - radius_adj);
	v2[1] = cy + ry * sin(ang * RND_M180) * (1 - radius_adj);
	edx = (v[0] - v2[0]);
	edy = (v[1] - v2[1]);
	if (edx < 0) edx = -edx;
	if (edy < 0) edy = -edy;
	if (edx+edy > RND_MM_TO_COORD(0.001))
		rnd_poly_vertex_include(contour->head->prev, rnd_poly_node_create(v2));


	/* now add other round cap */
	if (end_caps)
		rnd_poly_frac_circle(contour, endx1, endy1, v2, 2);

	/* now we have the whole contour */
	if (!(np = rnd_poly_from_contour(contour)))
		return NULL;
	return np;
}

rnd_polyarea_t *rnd_poly_from_rect(rnd_coord_t x1, rnd_coord_t x2, rnd_coord_t y1, rnd_coord_t y2)
{
	rnd_pline_t *contour = NULL;
	rnd_vector_t v;

	/* Return NULL for zero or negatively sized rectangles */
	if (x2 <= x1 || y2 <= y1)
		return NULL;

	v[0] = x1;
	v[1] = y1;
	if ((contour = rnd_poly_contour_new(v)) == NULL)
		return NULL;
	v[0] = x2;
	v[1] = y1;
	rnd_poly_vertex_include(contour->head->prev, rnd_poly_node_create(v));
	v[0] = x2;
	v[1] = y2;
	rnd_poly_vertex_include(contour->head->prev, rnd_poly_node_create(v));
	v[0] = x1;
	v[1] = y2;
	rnd_poly_vertex_include(contour->head->prev, rnd_poly_node_create(v));
	return rnd_poly_from_contour(contour);
}

static void rnd_poly_frac_circle_(rnd_pline_t * c, rnd_coord_t X, rnd_coord_t Y, rnd_vector_t v, int range, int add_last)
{
	double oe1, oe2, e1, e2, t1;
	int i, orange = range;

	init_rotate_cache();

	oe1 = (v[0] - X);
	oe2 = (v[1] - Y);

	rnd_poly_vertex_include(c->head->prev, rnd_poly_node_create(v));

	/* move vector to origin */
	e1 = (v[0] - X) * RND_POLY_CIRC_RADIUS_ADJ;
	e2 = (v[1] - Y) * RND_POLY_CIRC_RADIUS_ADJ;

	/* NB: the caller adds the last vertex, hence the -1 */
	range = RND_POLY_CIRC_SEGS / range - 1;
	for (i = 0; i < range; i++) {
		/* rotate the vector */
		t1 = rotate_circle_seg[0] * e1 + rotate_circle_seg[1] * e2;
		e2 = rotate_circle_seg[2] * e1 + rotate_circle_seg[3] * e2;
		e1 = t1;
		v[0] = X + ROUND(e1);
		v[1] = Y + ROUND(e2);
		rnd_poly_vertex_include(c->head->prev, rnd_poly_node_create(v));
	}

	if ((add_last) && (orange == 4)) {
		v[0] = X - ROUND(oe2);
		v[1] = Y + ROUND(oe1);
		rnd_poly_vertex_include(c->head->prev, rnd_poly_node_create(v));
	}
}


/* add vertices in a fractional-circle starting from v
 * centered at X, Y and going counter-clockwise
 * does not include the first point
 * last argument is 1 for a full circle
 * 2 for a half circle
 * or 4 for a quarter circle
 */
void rnd_poly_frac_circle(rnd_pline_t * c, rnd_coord_t X, rnd_coord_t Y, rnd_vector_t v, int range)
{
	rnd_poly_frac_circle_(c, X, Y, v, range, 0);
}

/* same but adds the last vertex */
void rnd_poly_frac_circle_end(rnd_pline_t * c, rnd_coord_t X, rnd_coord_t Y, rnd_vector_t v, int range)
{
	rnd_poly_frac_circle_(c, X, Y, v, range, 1);
}


/* create a circle approximation from lines */
rnd_polyarea_t *rnd_poly_from_circle(rnd_coord_t x, rnd_coord_t y, rnd_coord_t radius)
{
	rnd_pline_t *contour;
	rnd_vector_t v;

	if (radius <= 0)
		return NULL;
	v[0] = x + radius;
	v[1] = y;
	if ((contour = rnd_poly_contour_new(v)) == NULL)
		return NULL;
	rnd_poly_frac_circle(contour, x, y, v, 1);
	contour->is_round = rnd_true;
	contour->cx = x;
	contour->cy = y;
	contour->radius = radius;
	return rnd_poly_from_contour(contour);
}

/* make a rounded-corner rectangle with radius t beyond x1,x2,y1,y2 rectangle */
rnd_polyarea_t *rnd_poly_from_round_rect(rnd_coord_t x1, rnd_coord_t x2, rnd_coord_t y1, rnd_coord_t y2, rnd_coord_t t)
{
	rnd_pline_t *contour = NULL;
	rnd_vector_t v;

	assert(x2 > x1);
	assert(y2 > y1);
	v[0] = x1 - t;
	v[1] = y1;
	if ((contour = rnd_poly_contour_new(v)) == NULL)
		return NULL;
	rnd_poly_frac_circle_end(contour, x1, y1, v, 4);
	v[0] = x2;
	v[1] = y1 - t;
	rnd_poly_vertex_include(contour->head->prev, rnd_poly_node_create(v));
	rnd_poly_frac_circle_end(contour, x2, y1, v, 4);
	v[0] = x2 + t;
	v[1] = y2;
	rnd_poly_vertex_include(contour->head->prev, rnd_poly_node_create(v));
	rnd_poly_frac_circle_end(contour, x2, y2, v, 4);
	v[0] = x1;
	v[1] = y2 + t;
	rnd_poly_vertex_include(contour->head->prev, rnd_poly_node_create(v));
	rnd_poly_frac_circle_end(contour, x1, y2, v, 4);
	return rnd_poly_from_contour(contour);
}


rnd_polyarea_t *rnd_poly_from_line(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t thick, rnd_bool square)
{
	rnd_pline_t *contour = NULL;
	rnd_polyarea_t *np = NULL;
	rnd_vector_t v;
	double d, dx, dy;
	long half;

	if (thick <= 0)
		return NULL;
	half = (thick + 1) / 2;
	d = sqrt(RND_SQUARE(x1 - x2) + RND_SQUARE(y1 - y2));
	if (!square)
		if (d == 0)									/* line is a point */
			return rnd_poly_from_circle(x1, y1, half);
	if (d != 0) {
		d = half / d;
		dx = (y1 - y2) * d;
		dy = (x2 - x1) * d;
	}
	else {
		dx = half;
		dy = 0;
	}
	if (square) {	/* take into account the ends */
		x1 -= dy;
		y1 += dx;
		x2 += dy;
		y2 -= dx;
	}
	v[0] = x1 - dx;
	v[1] = y1 - dy;
	if ((contour = rnd_poly_contour_new(v)) == NULL)
		return 0;
	v[0] = x2 - dx;
	v[1] = y2 - dy;
	if (square)
		rnd_poly_vertex_include(contour->head->prev, rnd_poly_node_create(v));
	else
		rnd_poly_frac_circle(contour, x2, y2, v, 2);
	v[0] = x2 + dx;
	v[1] = y2 + dy;
	rnd_poly_vertex_include(contour->head->prev, rnd_poly_node_create(v));
	v[0] = x1 + dx;
	v[1] = y1 + dy;
	if (square)
		rnd_poly_vertex_include(contour->head->prev, rnd_poly_node_create(v));
	else
		rnd_poly_frac_circle(contour, x1, y1, v, 2);
	/* now we have the line contour */
	if (!(np = rnd_poly_from_contour(contour)))
		return NULL;
	return np;
}

#define MIN_CLEARANCE_BEFORE_BISECT 10.
rnd_polyarea_t *rnd_poly_from_arc(rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t astart, rnd_angle_t adelta, rnd_coord_t thick)
{
	double delta;
	rnd_coord_t half;

	delta = (adelta < 0) ? -adelta : adelta;

	half = (thick + 1) / 2;

	/* corner case: can't even calculate the end cap properly because radius
	   is so small that there's no inner arc of the clearance */
	if ((width - half <= 0) || (height - half <= 0)) {
		rnd_coord_t lx1, ly1;
		rnd_polyarea_t *tmp_arc, *tmp1, *tmp2, *res, *ends;

		tmp_arc = ArcPolyNoIntersect(cx, cy, width, height, astart, adelta, thick, 0);

		rnd_arc_get_endpt(cx, cy, width, height, astart, adelta, 0, &lx1, &ly1);
		tmp1 = rnd_poly_from_line(lx1, ly1, lx1, ly1, thick, 0);

		rnd_arc_get_endpt(cx, cy, width, height, astart, adelta, 1, &lx1, &ly1);
		tmp2 = rnd_poly_from_line(lx1, ly1, lx1, ly1, thick, 0);

		rnd_polyarea_boolean_free(tmp1, tmp2, &ends, RND_PBO_UNITE);
		rnd_polyarea_boolean_free(ends, tmp_arc, &res, RND_PBO_UNITE);
		return res;
	}

	/* If the arc segment would self-intersect, we need to construct it as the union of
	   two non-intersecting segments */
	if (2 * M_PI * width * (1. - (double) delta / 360.) - thick < MIN_CLEARANCE_BEFORE_BISECT) {
		rnd_polyarea_t *tmp1, *tmp2, *res;
		int half_delta = adelta / 2;

		tmp1 = ArcPolyNoIntersect(cx, cy, width, height, astart, half_delta, thick, 1);
		tmp2 = ArcPolyNoIntersect(cx, cy, width, height, astart+half_delta, adelta-half_delta, thick, 1);
		rnd_polyarea_boolean_free(tmp1, tmp2, &res, RND_PBO_UNITE);
		return res;
	}

	return ArcPolyNoIntersect(cx, cy, width, height, astart, adelta, thick, 1);
}

/* NB: This function will free the passed rnd_polyarea_t.
       It must only be passed a single rnd_polyarea_t (pa->f == pa->b == pa) */
static void r_NoHolesPolygonDicer(rnd_polyarea_t * pa, void (*emit) (rnd_pline_t *, void *), void *user_data)
{
	rnd_pline_t *p = pa->contours;

	if (!pa->contours->next) {		/* no holes */
		pa->contours = NULL;				/* The callback now owns the contour */
		/* Don't bother removing it from the rnd_polyarea_t's rtree
		   since we're going to free the rnd_polyarea_t below anyway */
		emit(p, user_data);
		rnd_polyarea_free(&pa);
		return;
	}
	else {
		rnd_polyarea_t *poly2, *left, *right;

		/* make a rectangle of the left region slicing through the middle of the first hole */
		poly2 = rnd_poly_from_rect(p->xmin, (p->next->xmin + p->next->xmax) / 2, p->ymin, p->ymax);
		rnd_polyarea_and_subtract_free(pa, poly2, &left, &right);
		if (left) {
			rnd_polyarea_t *cur, *next;
			cur = left;
			do {
				next = cur->f;
				cur->f = cur->b = cur;	/* Detach this polygon piece */
				r_NoHolesPolygonDicer(cur, emit, user_data);
				/* NB: The rnd_polyarea_t was freed by its use in the recursive dicer */
			}
			while ((cur = next) != left);
		}
		if (right) {
			rnd_polyarea_t *cur, *next;
			cur = right;
			do {
				next = cur->f;
				cur->f = cur->b = cur;	/* Detach this polygon piece */
				r_NoHolesPolygonDicer(cur, emit, user_data);
				/* NB: The rnd_polyarea_t was freed by its use in the recursive dicer */
			}
			while ((cur = next) != right);
		}
	}
}

void rnd_polyarea_no_holes_dicer(rnd_polyarea_t *main_contour, rnd_coord_t clipX1, rnd_coord_t clipY1, rnd_coord_t clipX2, rnd_coord_t clipY2, void (*emit)(rnd_pline_t *, void *), void *user_data)
{
	rnd_polyarea_t *cur, *next;

	/* clip to the bounding box */
	if ((clipX1 != clipX2) || (clipY1 != clipY2)) {
		rnd_polyarea_t *cbox = rnd_poly_from_rect(clipX1, clipX2, clipY1, clipY2);
		rnd_polyarea_boolean_free(main_contour, cbox, &main_contour, RND_PBO_ISECT);
	}
	if (main_contour == NULL)
		return;
	/* Now dice it up.
	 * NB: Could be more than one piece (because of the clip above) */
	cur = main_contour;
	do {
		next = cur->f;
		cur->f = cur->b = cur;			/* Detach this polygon piece */
		r_NoHolesPolygonDicer(cur, emit, user_data);
		/* NB: The rnd_polyarea_t was freed by its use in the recursive dicer */
	}
	while ((cur = next) != main_contour);
}
