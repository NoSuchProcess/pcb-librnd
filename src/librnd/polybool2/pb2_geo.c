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


/* returns 1 if sa and sb are in full overlap */
RND_INLINE int seg_seg_olap(pb2_seg_t *sa, pb2_seg_t *sb)
{
	TODO("arc: this is only for line at the moment");
	if (Vequ2(sa->start, sb->start) && Vequ2(sa->end, sb->end))
		return 1;
	if (Vequ2(sa->start, sb->end) && Vequ2(sa->end, sb->start))
		return 1;
	return 0;
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
		(iscpt[0])[0] = ip[0].x; \
		(iscpt[0])[1] = ip[0].y; \
	} \
	if (num > 1) { \
		(iscpt[1])[0] = ip[1].x; \
		(iscpt[1])[1] = ip[1].y; \
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
	arc->bbox.x1 = bb.p1.x; arc->bbox.y1 = bb.p1.y;
	arc->bbox.x2 = bb.p2.x; arc->bbox.y2 = bb.p2.y;
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
