/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2023,2024 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2023,2024)
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

/* functions for generating polygons or polygon slices */

#include <librnd/rnd_config.h>

#include "pa_config.h"

#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include <librnd/core/global_typedefs.h>
#include "polyarea.h"

#include "pa_math.c"

#include "polygon1_gen.h"

#define PA_M180                 (M_PI/180.0)
static double rotate_circle_seg_pos[4], rotate_circle_seg_neg[4];
int rotate_circle_seg_inited = 0;

static void init_rotate_cache(void)
{
	if (!rotate_circle_seg_inited) {
		double cos_ang, sin_ang;

		cos_ang = cos(2.0 * M_PI / RND_POLY_CIRC_SEGS_F);
		sin_ang = sin(2.0 * M_PI / RND_POLY_CIRC_SEGS_F);

		rotate_circle_seg_pos[0] = cos_ang;
		rotate_circle_seg_pos[1] = -sin_ang;
		rotate_circle_seg_pos[2] = sin_ang;
		rotate_circle_seg_pos[3] = cos_ang;

		cos_ang = cos(-2.0 * M_PI / RND_POLY_CIRC_SEGS_F);
		sin_ang = sin(-2.0 * M_PI / RND_POLY_CIRC_SEGS_F);

		rotate_circle_seg_neg[0] = cos_ang;
		rotate_circle_seg_neg[1] = -sin_ang;
		rotate_circle_seg_neg[2] = sin_ang;
		rotate_circle_seg_neg[3] = cos_ang;

		rotate_circle_seg_inited = 1;
	}
}

static void rnd_poly_frac_circle_(rnd_pline_t * c, rnd_coord_t cx, rnd_coord_t cy, rnd_vector_t v, int range, int add_last, rnd_poly_fct_t how)
{
	double ex, ey, rad1_x = (v[0]-cx), rad1_y = (v[1]-cy), *rotate_circle_seg;
	int n, end;

	init_rotate_cache();

	rotate_circle_seg = (how & RND_POLY_FCT_REVERSE) ? rotate_circle_seg_neg : rotate_circle_seg_pos;

	rnd_poly_vertex_include(c->head->prev, rnd_poly_node_create(v));

	/* move vector to origin */
	ex = (v[0]-cx) * RND_POLY_CIRC_RADIUS_ADJ; ey = (v[1]-cy) * RND_POLY_CIRC_RADIUS_ADJ;

	end = RND_POLY_CIRC_SEGS / range - 1; /* -1 so the last vertex is not added in the loop */
	for(n = 0; n < end; n++) {
		double tmp;

		/* rotate e */
		tmp = rotate_circle_seg[0] * ex + rotate_circle_seg[1] * ey;
		ey = rotate_circle_seg[2] * ex + rotate_circle_seg[3] * ey;
		ex = tmp;

		v[0] = cx + PA_ROUND(ex); v[1] = cy + PA_ROUND(ey);
		rnd_poly_vertex_include(c->head->prev, rnd_poly_node_create(v));
	}

	if (add_last && (range == 4)) {
		v[0] = cx - PA_ROUND(rad1_y); v[1] = cy + PA_ROUND(rad1_x);
		rnd_poly_vertex_include(c->head->prev, rnd_poly_node_create(v));
	}
}

/* Return v's double-quarter ID in rnd (screen) coord system:
        6

        |
     5  |  7
        |
 4 -----+----->  0
        |     x
     3  |  1
        |
        v y

        2
*/
RND_INLINE int dquarter(rnd_vector_t v)
{
	if (v[0] == 0) {
		if (v[1] == 0) return -1;
		if (v[1] > 0) return 2;
		/*if (v[1] < 0)*/ return 6;
	}
	if (v[1] == 0) {
		if (v[0] == 0) return -1;
		if (v[0] > 0) return 0;
		/*if (v[0] < 0)*/ return 4;
	}
	if (v[0] > 0) {
		if (v[1] > 0) return 1;
		/*if (v[1] < 0)*/ return 7;
	}
	/*if (v[0] < 0)*/ {
		if (v[1] > 0) return 3;
		/*if (v[1] < 0)*/ return 5;
	}
}

/* Return 1 if v reached or webt beyond end, assuming CW direction in the
   rnd (screen) coord system */
RND_INLINE int went_beyond_end_pos(int start_dq, rnd_vector_t end, int end_dq, int rollover, rnd_vector_t v, int *was_in_same)
{
	int dq = dquarter(v);

	if (rollover) {
		if (dq < start_dq)
			dq += 8;
	}

/* rnd_printf("        %.9mm;%.9mm %d end: %.9mm;%.9mm %d \n", v[0], v[1], dq, end[0], end[1], end_dq); */

	if (dq > end_dq)
		return 1;

	if (dq == end_dq) { /* in the same dquarter */
		*was_in_same = 1;
		switch (dq % 8) {
			/* axis aligned end vector */
			case 0:
			case 2:
			case 4:
			case 6:
				return 1;

			/* diagonal end vector */
			case 1: if (v[1] >= end[1]) return 1; break;
			case 3: if (v[1] <= end[1]) return 1; break;
			case 5: if (v[1] <= end[1]) return 1; break;
			case 7: if (v[1] >= end[1]) return 1; break;
		}
	}
	else if (*was_in_same) {
		/* if we were in the same quarter as end previously and left the quarter,
		   we are surely beyond end; test case: pcb02 */
		return 1;
	}

	return 0;
}

/* Return 1 if v reached or webt beyond end, assuming CCW direction in the
   rnd (screen) coord system */
RND_INLINE int went_beyond_end_neg(int start_dq, rnd_vector_t end, int end_dq, int rollover, rnd_vector_t v, int *was_in_same)
{
	int dq = dquarter(v);

	if (rollover) {
		if (dq < start_dq)
			dq += 8;
	}

	if (dq > end_dq)
		return 1;

	if (dq == end_dq) { /* in the same dquarter */
		*was_in_same = 1;
		switch (dq % 8) {
			/* axis aligned end vector */
			case 0:
			case 2:
			case 4:
			case 6:
				return 1;

			/* diagonal end vector */
			case 1: if (v[1] <= end[1]) return 1; break;
			case 3: if (v[1] >= end[1]) return 1; break;
			case 5: if (v[1] >= end[1]) return 1; break;
			case 7: if (v[1] <= end[1]) return 1; break;
		}
	}
	else if (*was_in_same) {
		/* if we were in the same quarter as end previously and left the quarter,
		   we are surely beyond end; test case: pcb02 */
		return 1;
	}

	return 0;
}

void rnd_poly_frac_circle_to(rnd_pline_t *c, rnd_vnode_t *insert_after, rnd_coord_t cx, rnd_coord_t cy, const rnd_vector_t start, const rnd_vector_t end, rnd_poly_fct_t how)
{
	double ex, ey, *rotate_circle_seg;
	int n, start_dq, end_dq, rollover, was_in_same = 0;
	rnd_vector_t rel_s, rel_e, v;
	rnd_vnode_t *new_node;

	if ((start[0] == end[0]) && (start[1] == end[1]))
		return;

	rotate_circle_seg = (how & RND_POLY_FCT_REVERSE) ? rotate_circle_seg_neg : rotate_circle_seg_pos;

	rel_s[0] = start[0] - cx;
	rel_s[1] = start[1] - cy;
	rel_e[0] = end[0] - cx;
	rel_e[1] = end[1] - cy;
	start_dq = dquarter(rel_s);
	end_dq = dquarter(rel_e);

	/* degenerate case: zero length vector */
	if (end_dq < 0)
		return;

	/* dq must increase from start to end */
	rollover = (start_dq > end_dq);
	if (rollover)
		end_dq += 8;

	init_rotate_cache();

	/* move vector to origin */
	v[0] = start[0]; v[1] = start[1];
	ex = (v[0]-cx) * RND_POLY_CIRC_RADIUS_ADJ; ey = (v[1]-cy) * RND_POLY_CIRC_RADIUS_ADJ;

	if (how & RND_POLY_FCT_SKIP_TINY) {
		double dist2 = rnd_vect_dist2(rel_s, rel_e), mindist;
		mindist = (ex + ey) * M_PI / RND_POLY_CIRC_SEGS_F;
		if (dist2 < 4*mindist*mindist)
			return;
	}



/*
rnd_printf("frac circ at %mm;%mm: %mm;%mm %d .. %mm;%mm %d\n",
	cx, cy, start[0], start[1], start_dq, end[0], end[1], end_dq);
*/

	for(n = 0; n <= RND_POLY_CIRC_SEGS_F; n++) { /* defensive programming: avoid infinite loop to full circle */
		double tmp;
		rnd_vector_t er;

		/* rotate e */
		tmp = rotate_circle_seg[0] * ex + rotate_circle_seg[1] * ey;
		ey = rotate_circle_seg[2] * ex + rotate_circle_seg[3] * ey;
		ex = tmp;

		er[0] = PA_ROUND(ex);
		er[1] = PA_ROUND(ey);

		/* stop if went beyond end */
		if (how & RND_POLY_FCT_REVERSE) {
			if (went_beyond_end_neg(start_dq, rel_e, end_dq, rollover, er, &was_in_same))
				break;
		}
		else {
			if (went_beyond_end_pos(start_dq, rel_e, end_dq, rollover, er, &was_in_same))
				break;
		}

		v[0] = cx + er[0]; v[1] = cy + er[1];
		new_node = rnd_poly_node_create(v);
		rnd_poly_vertex_include(insert_after, new_node);
		insert_after = new_node;
	}
}


void rnd_poly_frac_circle(rnd_pline_t *c, rnd_coord_t cx, rnd_coord_t cy, rnd_vector_t v, int range)
{
	rnd_poly_frac_circle_(c, cx, cy, v, range, 0, 0);
}

void rnd_poly_frac_circle_end(rnd_pline_t *c, rnd_coord_t cx, rnd_coord_t cy, rnd_vector_t v, int range)
{
	rnd_poly_frac_circle_(c, cx, cy, v, range, 1, 0);
}

rnd_polyarea_t *rnd_poly_from_contour_nochk(rnd_pline_t *pl)
{
	rnd_polyarea_t *pa;

	assert(pl->flg.orient == RND_PLF_DIR);

	pa = pa_polyarea_alloc();
	if (pa == NULL)
		return NULL;

	pa_polyarea_insert_pline(pa, pl);


	return pa;
}

rnd_polyarea_t *rnd_poly_from_contour(rnd_pline_t *pl)
{
	rnd_polyarea_t *pa;
	pa_pline_update(pl, rnd_true);
	pa = rnd_poly_from_contour_nochk(pl);
	assert(rnd_poly_valid(pa));
	return pa;
}

rnd_polyarea_t *rnd_poly_from_contour_autoinv(rnd_pline_t *pl)
{
	rnd_polyarea_t *pa;

	pa_pline_update(pl, rnd_true);

	if (pl->flg.orient != RND_PLF_DIR)
		pa_pline_invert(pl);

	pa = pa_polyarea_alloc();
	if (pa == NULL)
		return NULL;

	pa_polyarea_insert_pline(pa, pl);

	assert(rnd_poly_valid(pa));

	return pa;
}

static void pa_poly_from_arc_stroke(rnd_pline_t *pl, rnd_coord_t cx, rnd_coord_t cy, double rx, double ry, double ang, double da, long num_segs)
{
	long n;

	for(n = 0; n < num_segs; n++, ang += da) {
		rnd_vector_t v;
		v[0] = cx - rx * cos(ang * PA_M180);
		v[1] = cy + ry * sin(ang * PA_M180);
		rnd_poly_vertex_include(pl->head->prev, rnd_poly_node_create(v));
	}
}

#ifdef RND_API_VER
#	include <librnd/core/unit.h>
#	define LAST_POINT_FAR_ENOUGH RND_MM_TO_COORD(0.001)
#else
#	define LAST_POINT_FAR_ENOUGH 1000.0
#endif

#define PA_MAX(a,b)  ((a) > (b) ? (a) : (b))

#define ARC_ANGLE 5
static rnd_polyarea_t *pa_poly_from_arc_no_isc(rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t astart, rnd_angle_t adelta, rnd_coord_t thick, int end_caps)
{
	rnd_pline_t *pl = NULL;
	rnd_vector_t v, v2;
	long segs, half = (thick + 1) / 2;
	double ang, da, rx, ry, radius_adj;
	rnd_coord_t edx, edy, endx1, endx2, endy1, endy2;

	if (thick <= 0)
		return NULL;

	if (adelta < 0) {
		astart += adelta;
		adelta = -adelta;
	}

	rnd_arc_get_endpt(cx, cy, width, height, astart, adelta, 0, &endx1, &endy1);
	rnd_arc_get_endpt(cx, cy, width, height, astart, adelta, 1, &endx2, &endy2);

	/* start with inner radius */
	rx = PA_MAX(width - half, 0);
	ry = PA_MAX(height - half, 0);

	segs = 1;
	if (thick > 0)
		segs = PA_MAX(segs, adelta * M_PI / 360 * sqrt(sqrt((double)rx * (double)rx + (double)ry * (double)ry) / RND_POLY_ARC_MAX_DEVIATION / 2 / thick));
	segs = PA_MAX(segs, adelta / ARC_ANGLE);

	ang = astart;
	da = (double)adelta / (double)segs;
	radius_adj = (M_PI * da / 360) * (M_PI * da / 360) / 2;

	v[0] = cx - rx * cos(ang * PA_M180);
	v[1] = cy + ry * sin(ang * PA_M180);

	pl = pa_pline_new(v);
	if (pl == NULL)
		return 0;

	pa_poly_from_arc_stroke(pl, cx, cy, rx, ry, ang+da, da, segs - 1);

	/* find last point */
	ang = astart + adelta;
	v[0] = cx - rx * cos(ang * PA_M180) * (1 - radius_adj);
	v[1] = cy + ry * sin(ang * PA_M180) * (1 - radius_adj);

	/* add the round cap at the end */
	if (end_caps)
		rnd_poly_frac_circle(pl, endx2, endy2, v, 2);

	/* and now do the outer arc (going backwards) */
	rx = (width + half) * (1 + radius_adj);
	ry = (width + half) * (1 + radius_adj);
	pa_poly_from_arc_stroke(pl, cx, cy, rx, ry, ang, -da, segs);


	/* explicitly draw the last point if the manhattan-distance is large enough */
	ang = astart;
	v2[0] = cx - rx * cos(ang * PA_M180) * (1 - radius_adj);
	v2[1] = cy + ry * sin(ang * PA_M180) * (1 - radius_adj);
	edx = (v[0] - v2[0]);
	edy = (v[1] - v2[1]);
	if (edx < 0) edx = -edx;
	if (edy < 0) edy = -edy;
	if (edx+edy > LAST_POINT_FAR_ENOUGH)
		rnd_poly_vertex_include(pl->head->prev, rnd_poly_node_create(v2));

	/* now add other round cap */
	if (end_caps)
		rnd_poly_frac_circle(pl, endx1, endy1, v2, 2);

	return rnd_poly_from_contour(pl);
}

#define MIN_CLEARANCE_BEFORE_BISECT 10.0
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

		tmp_arc = pa_poly_from_arc_no_isc(cx, cy, width, height, astart, adelta, thick, 0);

		rnd_arc_get_endpt(cx, cy, width, height, astart, adelta, 0, &lx1, &ly1);
		tmp1 = rnd_poly_from_line(lx1, ly1, lx1, ly1, thick, 0);

		rnd_arc_get_endpt(cx, cy, width, height, astart, adelta, 1, &lx1, &ly1);
		tmp2 = rnd_poly_from_line(lx1, ly1, lx1, ly1, thick, 0);

		rnd_polyarea_boolean_free(tmp1, tmp2, &ends, RND_PBO_UNITE);
		rnd_polyarea_boolean_free(ends, tmp_arc, &res, RND_PBO_UNITE);
		return res;
	}

	/* If the arc segment would self-intersect, build it as the union of
	   two non-intersecting segments */
	if (2 * M_PI * width * (1.0 - (double)delta / 360.0) - thick < MIN_CLEARANCE_BEFORE_BISECT) {
		rnd_polyarea_t *tmp1, *tmp2, *res;
		int half_delta = adelta / 2;

		tmp1 = pa_poly_from_arc_no_isc(cx, cy, width, height, astart, half_delta, thick, 1);
		tmp2 = pa_poly_from_arc_no_isc(cx, cy, width, height, astart+half_delta, adelta-half_delta, thick, 1);
		rnd_polyarea_boolean_free(tmp1, tmp2, &res, RND_PBO_UNITE);
		return res;
	}

	return pa_poly_from_arc_no_isc(cx, cy, width, height, astart, adelta, thick, 1);
}


rnd_polyarea_t *rnd_poly_from_rect(rnd_coord_t x1, rnd_coord_t x2, rnd_coord_t y1, rnd_coord_t y2)
{
	rnd_pline_t *pl = NULL;
	rnd_vector_t v;

	/* refuse zero or negative size to avoid self-intersecting polygons */
	if ((x2 <= x1) || (y2 <= y1))
		return NULL;

	v[0] = x1; v[1] = y1;
	pl = pa_pline_new(v);
	if (pl == NULL)
		return NULL;

	v[0] = x2; v[1] = y1;
	rnd_poly_vertex_include(pl->head->prev, rnd_poly_node_create(v));

	v[0] = x2; v[1] = y2;
	rnd_poly_vertex_include(pl->head->prev, rnd_poly_node_create(v));

	v[0] = x1; v[1] = y2;
	rnd_poly_vertex_include(pl->head->prev, rnd_poly_node_create(v));

	return rnd_poly_from_contour(pl);
}

rnd_polyarea_t *rnd_poly_from_circle(rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r)
{
	rnd_pline_t *pl;
	rnd_vector_t v;

	if (r <= 0)
		return NULL;

/* v is the start point */
	v[0] = cx + r; v[1] = cy;

	pl = pa_pline_new(v);
	if (pl == NULL)
		return NULL;

	rnd_poly_frac_circle(pl, cx, cy, v, 1);

	return rnd_poly_from_contour(pl);
}

rnd_polyarea_t *rnd_poly_from_round_rect(rnd_coord_t x1, rnd_coord_t x2, rnd_coord_t y1, rnd_coord_t y2, rnd_coord_t t)
{
	rnd_pline_t *pl = NULL;
	rnd_vector_t v;

	/* refuse zero or negative size to avoid self-intersecting polygons */
	if ((x2 <= x1) || (y2 <= y1))
		return NULL;

	v[0] = x1 - t; v[1] = y1;
	pl = pa_pline_new(v);
	if (pl == NULL)
		return NULL;

	rnd_poly_frac_circle_end(pl, x1, y1, v, 4);

	v[0] = x2; v[1] = y1 - t;
	rnd_poly_vertex_include(pl->head->prev, rnd_poly_node_create(v));
	rnd_poly_frac_circle_end(pl, x2, y1, v, 4);

	v[0] = x2 + t; v[1] = y2;
	rnd_poly_vertex_include(pl->head->prev, rnd_poly_node_create(v));
	rnd_poly_frac_circle_end(pl, x2, y2, v, 4);

	v[0] = x1; v[1] = y2 + t;
	rnd_poly_vertex_include(pl->head->prev, rnd_poly_node_create(v));
	rnd_poly_frac_circle_end(pl, x1, y2, v, 4);

	return rnd_poly_from_contour(pl);
}


rnd_polyarea_t *rnd_poly_from_line(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t thick, rnd_bool square)
{
	rnd_pline_t *pl = NULL;
	rnd_vector_t v;
	double d, dx, dy;
	rnd_coord_t half = (thick + 1) / 2;

	if (thick <= 0)
		return NULL;

	dx = x2 - x1; dy = y1 - y2;
	d = dx * dx + dy * dy;

	if (!square && (d == 0)) /* degenerate: single point line */
			return rnd_poly_from_circle(x1, y1, half);

	if (d != 0) {
		double nx, ny;
		d = half / sqrt(d);
		nx = dy * d; ny = dx * d;
		dx = nx; dy = ny;
	}
	else {
		dx = half; dy = 0;
	}

	if (square) { /* adjust (make line longer) for quarey end caps */
		x1 -= dy; y1 += dx;
		x2 += dy; y2 -= dx;
	}

	v[0] = x1 - dx; v[1] = y1 - dy;
	pl = pa_pline_new(v);
	if (pl == NULL)
		return 0;

	v[0] = x2 - dx; v[1] = y2 - dy;
	if (square)
		rnd_poly_vertex_include(pl->head->prev, rnd_poly_node_create(v));
	else
		rnd_poly_frac_circle(pl, x2, y2, v, 2);

	v[0] = x2 + dx; v[1] = y2 + dy;
	rnd_poly_vertex_include(pl->head->prev, rnd_poly_node_create(v));

	v[0] = x1 + dx; v[1] = y1 + dy;
	if (square)
		rnd_poly_vertex_include(pl->head->prev, rnd_poly_node_create(v));
	else
		rnd_poly_frac_circle(pl, x1, y1, v, 2);

	return rnd_poly_from_contour(pl);
}

