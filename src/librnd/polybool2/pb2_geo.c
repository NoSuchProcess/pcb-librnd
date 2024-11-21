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

#include <stdlib.h>
#include <librnd/rnd_config.h>

#define G2D_INLINE RND_INLINE

TODO("bignum: calc_t should be bignum, not double, especially with 64 bit coords")
#include "typecfg_coord_double.h"
#include <gengeo2d/cline.h>
#include <gengeo2d/carc.h>
#include <gengeo2d/intersect.h>

/* returns 1 if sa's and sb's endpoints match (even if sa or sb is reversed) */
RND_INLINE int seg_seg_end_match(pb2_seg_t *sa, pb2_seg_t *sb)
{
	if (Vequ2(sa->start, sb->start) && Vequ2(sa->end, sb->end))
		return 1;
	if (Vequ2(sa->start, sb->end) && Vequ2(sa->end, sb->start))
		return 1;
	return 0;
}

/* returns 1 if sa and sb are in full overlap */
RND_INLINE int seg_seg_olap(pb2_seg_t *sa, pb2_seg_t *sb)
{
	if (sa->shape_type != sb->shape_type)
		return 0;

	/* endpoints must match, regardless of the shape */
	if (!seg_seg_end_match(sa, sb))
		return 0;

	switch(sa->shape_type) {
		case RND_VNODE_LINE: break; /*matching endpoints is enough to check */
		case RND_VNODE_ARC:
			/* need to have same endpoints, same center */
			if (fabs(sa->shape.arc.cx - sb->shape.arc.cx) > 0.5)
				return 0;
			if (fabs(sa->shape.arc.cy - sb->shape.arc.cy) > 0.5)
				return 0;
			break;
	}

	return 1;
}

/* swap start and end of a segment (reverse orientation); rtree doesn't change */
RND_INLINE void seg_reverse(pb2_seg_t *s)
{
	pa_swap(rnd_coord_t, s->start[0], s->end[0]);
	pa_swap(rnd_coord_t, s->start[1], s->end[1]);
	TODO("arc: also need to swap angles");
}

RND_INLINE void seg_angle_from(pa_angle_t *dst, pb2_seg_t *seg, rnd_vector_t from)
{
	TODO("bignum: this is not correct: for 64 bits we need higher precision angles than doubles");
	if (Vequ2(seg->start, from))
		pa_calc_angle_nn(dst, from, seg->end);
	else
		pa_calc_angle_nn(dst, from, seg->start);
}

/* vector slope used to compare direction vector to segment in step 3 point-in-poly */
typedef struct {
	rnd_coord_t dx, dy;
	double s;
} pb2_slope_t;

RND_INLINE pb2_slope_t pb2_slope(const rnd_vector_t origin, const rnd_vector_t other)
{
	pb2_slope_t slp;

	slp.dx = (other[0] - origin[0]);
	slp.dy = (other[1] - origin[1]);
	TODO("bignum: double is not enough for 64 bit coords; this large number should be inf really");
	if (slp.dx != 0)
		slp.s = (double)slp.dy / (double)slp.dx;
	else
		slp.s = 0;
	return slp;
}

RND_INLINE int PB2_SLOPE_LT(pb2_slope_t a, pb2_slope_t b)
{
	/* slope value is invalid for vertical vectors */
	assert(a.dx != 0);
	assert(b.dx != 0);
	return a.s < b.s;
}


/*** arc ***/
#define SEG2CLINE(cline, seg) \
do { \
	pb2_seg_t *__seg__ = seg; \
	cline.p1.x = __seg__->start[0]; cline.p1.y = __seg__->start[1]; \
	cline.p2.x = __seg__->end[0]; cline.p2.y = __seg__->end[1]; \
} while(0)

#define SEG2CARC(carc, seg) \
do { \
	pb2_seg_t *__seg__ = seg; \
	carc.c.x = __seg__->shape.arc.cx; carc.c.y = __seg__->shape.arc.cy; \
	carc.r = __seg__->shape.arc.r; \
	carc.start = __seg__->shape.arc.start; \
	carc.delta = __seg__->shape.arc.delta; \
} while(0)

/* copy gengeo2d results to rnd outpout */
#define ISC_OUT(num) \
do { \
	if (num > 0) { \
		(iscpt[0])[0] = rnd_round(ip[0].x); \
		(iscpt[0])[1] = rnd_round(ip[0].y); \
	} \
	if (num > 1) { \
		(iscpt[1])[0] = rnd_round(ip[1].x); \
		(iscpt[1])[1] = rnd_round(ip[1].y); \
	} \
	if (offs != NULL) { \
		offs[0] = of[0]; \
		offs[1] = of[1]; \
	} \
} while(0)

RND_INLINE int pb2_isc_line_arc(pb2_seg_t *line, pb2_seg_t *arc, rnd_vector_t iscpt[], double offs[], int offs_on_arc)
{
	g2d_cline_t cline;
	g2d_carc_t carc;
	int num;
	g2d_vect_t ip[2];
	g2d_offs_t of[2];

	SEG2CLINE(cline, line);
	SEG2CARC(carc, arc);

	num = g2d_iscp_cline_carc(&cline, &carc,  ip, of, offs_on_arc);
	ISC_OUT(num);
	return num;
}

/* offs[] is on arc1 */
RND_INLINE int pb2_isc_arc_arc(pb2_seg_t *arc1, pb2_seg_t *arc2, rnd_vector_t iscpt[], double offs[])
{
	g2d_carc_t carc1, carc2;
	int num;
	g2d_vect_t ip[2];
	g2d_offs_t of[2];

	SEG2CARC(carc1, arc1);
	SEG2CARC(carc2, arc2);

	/* special case: arcs on the same circle; ignore high resolution centers in that case */
	if (Vequ2(arc1->shape.arc.center, arc2->shape.arc.center) && (fabs(arc1->shape.arc.r - arc2->shape.arc.r) < 0.5)) {
		carc1.c.x = carc2.c.x = arc1->shape.arc.center[0];
		carc1.c.y = carc2.c.y = arc1->shape.arc.center[1];
		carc1.r = carc2.r = arc1->shape.arc.r;
	}

	num = g2d_iscp_carc_carc(&carc1, &carc2,  ip,  of);
	ISC_OUT(num);
	return num;
}


RND_INLINE void pb2_arc_bbox(pb2_seg_t *arc)
{
	g2d_carc_t carc;
	g2d_box_t bb;

	SEG2CARC(carc, arc);
	bb = g2d_carc_bbox(&carc);
	arc->bbox.x1 = floor(bb.p1.x); arc->bbox.y1 = floor(bb.p1.y);
	arc->bbox.x2 = ceil(bb.p2.x); arc->bbox.y2 = ceil(bb.p2.y);
}

/* Make sure *ang is between start and start+delta of arc; returns 0 on
   success */
RND_INLINE int pb2_arc_angle_clamp(double *ang, pb2_seg_t *arc)
{
	double da = arc->shape.arc.delta, sa = arc->shape.arc.start, ea = sa + da;

	if (da > 0) {
		if (*ang < sa) *ang += 2*G2D_PI;
		else if (*ang > ea) *ang -= 2*G2D_PI;
	}
	else {
		if (*ang > sa) *ang -= 2*G2D_PI;
		else if (*ang < ea) *ang += 2*G2D_PI;
	}

	if ((*ang < sa) || (*ang > ea))
		return -1;
	return 0;
}

RND_INLINE double pb2_arc_angle_dist(double ang, pb2_seg_t *arc)
{
	double da = arc->shape.arc.delta, sa = arc->shape.arc.start;
	if (da > 0)
		return ang - sa;
	return sa - ang;
}

RND_INLINE void pb2_arc_get_midpoint_dbl(pb2_seg_t *arc, double *mx, double *my)
{
	double ang = arc->shape.arc.start + arc->shape.arc.delta/2.0;
	*mx = arc->shape.arc.cx + cos(ang) * arc->shape.arc.r;
	*my = arc->shape.arc.cy + sin(ang) * arc->shape.arc.r;
}

/* Compute endpoint with an offset from a given endpoint (1 or 2) toward the
   mid of the arc */
RND_INLINE void pb2_arc_shift_end_dbl(pb2_seg_t *arc, int end_idx, double offs, double *xout, double *yout)
{
	double da, ang, mid_ang = arc->shape.arc.start + arc->shape.arc.delta/2.0;
	switch(end_idx) {
		case 1: ang = arc->shape.arc.start; da = 1; break;
		case 2: ang = arc->shape.arc.start + arc->shape.arc.delta; da = -1; break;
	}
	if (arc->shape.arc.delta < 0)
		da = -da;
	ang += da * offs/arc->shape.arc.r;

	*xout = arc->shape.arc.cx + cos(ang) * arc->shape.arc.r;
	*yout = arc->shape.arc.cy + sin(ang) * arc->shape.arc.r;
}
