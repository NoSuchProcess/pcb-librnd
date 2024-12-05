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

#include "pb2_geo_raw.h"

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
	double mid1, mid2;

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

			/* also need to check that mid angles are the same because of the () case */
			mid1 = sa->shape.arc.start + sa->shape.arc.delta/2.0;
			mid2 = sb->shape.arc.start + sb->shape.arc.delta/2.0;
			if (fabs(mid1 - mid2) > 0.001)
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

	switch(s->shape_type) {
		case RND_VNODE_LINE:
			break;
		case RND_VNODE_ARC:
			{
				double ea = s->shape.arc.start + s->shape.arc.delta;
				if (ea < 0) ea += 2 * G2D_PI;
				else if (ea > 2 * G2D_PI) ea -= 2 * G2D_PI;
				s->shape.arc.start = ea;
				s->shape.arc.delta = -s->shape.arc.delta;
				s->shape.arc.adir = !s->shape.arc.adir;
			}
			break;
	}
}

/* not normalized */
RND_INLINE void pb2_line_tangent_from(rnd_vector_t dst, pb2_seg_t *seg, rnd_vector_t from)
{
	if (Vequ2(seg->start, from)) {
		dst[0] = seg->end[0] - from[0];
		dst[1] = seg->end[1] - from[1];
	}
	else {
		dst[0] = seg->start[0] - from[0];
		dst[1] = seg->start[1] - from[1];
	}
}

RND_INLINE void pb2_line_angles_from(pa_angle_t *dst1, pa_angle_t *dst2, pb2_seg_t *seg, rnd_vector_t from)
{
	TODO("bignum: this is not correct: for 64 bits we need higher precision angles than doubles");
	if (Vequ2(seg->start, from))
		pa_calc_angle_nn(dst1, from, seg->end);
	else
		pa_calc_angle_nn(dst1, from, seg->start);
	*dst2 = -4242; /* shall not be used */
}

/* vector slope used to compare direction vector to segment in step 3 point-in-poly */
typedef struct {
	rnd_coord_t dx, dy;
	double s;
} pb2_slope_t;

RND_INLINE int PB2_SLOPE_LT(pb2_slope_t a, pb2_slope_t b)
{
	/* slope value is invalid for vertical vectors */
	assert(a.dx != 0);
	assert(b.dx != 0);
	return a.s < b.s;
}

/*** line ***/

RND_INLINE pb2_slope_t pb2_slope_line(const rnd_vector_t origin, const rnd_vector_t other)
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


/* Returns 1 if p1 is closer to startpoint of line than p2 */
RND_INLINE int pb2_line_points_ordered(pb2_seg_t *line, rnd_vector_t p1, rnd_vector_t p2)
{
	return (rnd_vect_dist2(line->start, p1) < rnd_vect_dist2(line->start, p2));
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

#define SEG2CARC_ORIGC(carc, seg) \
do { \
	pb2_seg_t *__seg__ = seg; \
	carc.c.x = __seg__->shape.arc.cx; carc.c.y = __seg__->shape.arc.cy; \
	carc.r = __seg__->shape.arc.r; \
	carc.start = __seg__->shape.arc.start_origc; \
	carc.delta = __seg__->shape.arc.delta_origc; \
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
		SEG2CARC_ORIGC(carc1, arc1);
		SEG2CARC_ORIGC(carc2, arc2);
		carc1.r = carc2.r = (arc1->shape.arc.r + arc2->shape.arc.r)/2.0;
	}
	else {
		SEG2CARC(carc1, arc1);
		SEG2CARC(carc2, arc2);
	}

	num = g2d_iscp_carc_carc(&carc1, &carc2,  ip,  of);
	ISC_OUT(num);
	return num;
}

/* return 1 if a horizontal ray from pt to the right crosses arc */
RND_INLINE int pb2_arc_hray_isect(const pb2_seg_t *arc, const rnd_vector_t pt)
{
	g2d_carc_t carc;
	SEG2CARC(carc, arc);
	return g2d__iscp_carc_hcline(&carc, pt[1], pt[0], RND_COORD_MAX, NULL, NULL) > 0;
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

	if (da < 0)
		pa_swap(double, sa, ea);

	if (*ang < sa) *ang += 2*G2D_PI;
	else if (*ang > ea) *ang -= 2*G2D_PI;

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
	double da, ang;
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

/* Returns 1 if p1 is closer to startpoint of arc than p2 */
RND_INLINE int pb2_arc_points_ordered(pb2_seg_t *arc, rnd_vector_t p1, rnd_vector_t p2)
{
	pa_angle_t as, a1, a2;

	pa_calc_angle_nn(&as, arc->shape.arc.center, arc->start);
	pa_calc_angle_nn(&a1, arc->shape.arc.center, p1);
	pa_calc_angle_nn(&a2, arc->shape.arc.center, p2);

	if (arc->shape.arc.delta >= 0) {
		if (a1 < as)
			a1 += 4;
		if (a2 < as)
			a2 += 4;
	}
	else {
		if (a1 > as)
			a1 -= 4;
		if (a2 > as)
			a2 -= 4;
	}

	return (a1 - as) < (a2 - as);
}

/* Return tangential vector at endpoint "from" (not normalized); tangent is
   pointing in opposite direction of the line that would continue the contour
   at the end of the arc. In other words the tangent from an endpoint shows
   the direction of the arc going out from that endpoint. */
RND_INLINE void pb2_arc_tangent_from(rnd_vector_t dst, pb2_seg_t *seg, rnd_vector_t from)
{
	rnd_vector_t spoke;
	int inv;

	/* radial direction */
	spoke[0] = from[0] - seg->shape.arc.center[0];
	spoke[1] = from[1] - seg->shape.arc.center[1];

	/* inverse direction on CCW */
	inv = (seg->shape.arc.adir == 0);

	/* inverse direction when computing for endpoint */
	if (Vequ2(from, seg->end))
		inv = !inv;

	/* normal vector of radial direction */
	if (inv) {
		dst[0] = +spoke[1];
		dst[1] = -spoke[0];
	}
	else {
		dst[0] = -spoke[1];
		dst[1] = spoke[0];
	}
}

/* Return tangential "angle" at endpoint "from" */
RND_INLINE void pb2_arc_angles_from(pa_angle_t *dst1, pa_angle_t *dst2, pb2_seg_t *seg, rnd_vector_t from)
{
	rnd_vector_t norm, tang, radial, between;

	pb2_arc_tangent_from(norm, seg, from);

	tang[0] = from[0] + norm[0];
	tang[1] = from[1] + norm[1];

	/* first angle is tangential */
	pa_calc_angle_nn(dst1, from, tang);

	/* second angle is something in between tangential and radial, always pointing inside the arc area */
	radial[0] = from[0] - seg->shape.arc.center[0];
	radial[1] = from[1] - seg->shape.arc.center[1];
	between[0] = (radial[0] + tang[0])/2;
	between[1] = (radial[1] + tang[1])/2;
	pa_calc_angle_nn(dst2, from, between);
}


RND_INLINE pb2_slope_t pb2_slope_arc(pb2_seg_t *seg, rnd_vector_t from)
{
	pb2_slope_t slp;
	rnd_vector_t tmp;

	pb2_arc_tangent_from(tmp, seg, from);

	slp.dx = tmp[0];
	slp.dy = tmp[1];
	TODO("bignum: double is not enough for 64 bit coords; this large number should be inf really");
	if (slp.dx != 0)
		slp.s = (double)slp.dy / (double)slp.dx;
	else
		slp.s = 0;
	return slp;
}


RND_INLINE void pb2_seg_arc_update_cache(pb2_ctx_t *ctx, pb2_seg_t *seg)
{
	double sa, ea, r1, r2, ravg, posdelta;
	int side;

	r1 = rnd_vect_dist2(seg->start, seg->shape.arc.center);
	if (r1 != 0)
		r1 = sqrt(r1);

	if (!Vequ2(seg->start, seg->end)) {
		r2 = rnd_vect_dist2(seg->end, seg->shape.arc.center);
		if (r2 != 0)
			r2 = sqrt(r2);
	}

	/* endpoints may not be on the arc with the original center due to rounding
	   errors on intersections that created the new integer endpoints;
	   calculate a new accurate center */
	if (fabs(r1-r2) > 0.001) {
		double dx, dy, dl, hdl, nx, ny, mx, my, h;

		ravg = (r1+r2)/2.0;

		dx = seg->end[0] - seg->start[0]; dy = seg->end[1] - seg->start[1];
		dl = sqrt(dx*dx + dy*dy); hdl = dl/2.0;
		dx /= dl; dy /= dl;
		mx = (double)(seg->end[0] + seg->start[0])/2.0; my = (double)(seg->end[1] + seg->start[1])/2.0;

		nx = -dy;
		ny = dx;
		h = sqrt(ravg*ravg - hdl*hdl); /* distance from midpoint to center */

		/* ccw: mirror normal by mirroring h distance */
		if (seg->shape.arc.adir == 0)
			h = -h;

		seg->shape.arc.r = ravg;
		seg->shape.arc.cx = mx + nx*h;
		seg->shape.arc.cy = my + ny*h;

#if 0
		pa_trace(" Uarc S", Plong(PB2_UID_GET(seg)), " ", Pvect(seg->start), " -> ", Pvect(seg->end), 0);
		pa_trace(" r ", Pdouble(r1), " ", Pdouble(r2), " ", Pdouble(ravg), 0);
		pa_trace(" mid: ", Pdouble(mx), " ", Pdouble(my), " n: ", Pdouble(nx), " ", Pdouble(ny), " h: ", Pdouble(h), 0);
		pa_trace(" cent: ", Pdouble(seg->shape.arc.cx), " ", Pdouble(seg->shape.arc.cy), "\n",  0);
#endif
	}
	else {
		/* original integer center is accurate enough */
		seg->shape.arc.r = r1;
		seg->shape.arc.cx = seg->shape.arc.center[0];
		seg->shape.arc.cy = seg->shape.arc.center[1];
		ravg = r1; /* used in area computation below */

#if 0
		pa_trace(" uarc S", Plong(PB2_UID_GET(seg)), " ", Pvect(seg->start), " -> ", Pvect(seg->end), 0);
		pa_trace(" r ", Pdouble(r1), 0);
		pa_trace(" cent: ", Pdouble(seg->shape.arc.cx), " ", Pdouble(seg->shape.arc.cy), "\n",  0);
#endif
	}

	sa = atan2((double)seg->start[1] - seg->shape.arc.cy, (double)seg->start[0] - seg->shape.arc.cx);
	ea = atan2((double)seg->end[1] - seg->shape.arc.cy, (double)seg->end[0] - seg->shape.arc.cx);
	seg->shape.arc.start = sa;
	if (seg->shape.arc.adir) {
		/* Positive delta; CW in svg; CCW in gengeo and C */
		if (ea < sa)
			ea += 2 * G2D_PI;
		seg->shape.arc.delta = ea - sa;
	}
	else {
		/* Negative delta; CCW in svg; CW in gengeo and C */
		if (ea > sa)
			ea -= 2 * G2D_PI;
		seg->shape.arc.delta = ea - sa;
	}

	/* compute the same thing for the original center as well, for the coaxial arc-arc intersection case */
	sa = atan2(seg->start[1] - seg->shape.arc.center[1], seg->start[0] - seg->shape.arc.center[0]);
	ea = atan2(seg->end[1] - seg->shape.arc.center[1], seg->end[0] - seg->shape.arc.center[0]);
	seg->shape.arc.start_origc = sa;
	if (seg->shape.arc.adir) {
		/* Positive delta; CW in svg; CCW in gengeo and C */
		if (ea < sa)
			ea += 2 * G2D_PI;
		seg->shape.arc.delta_origc = ea - sa;
	}
	else {
		/* Negative delta; CCW in svg; CW in gengeo and C */
		if (ea > sa)
			ea -= 2 * G2D_PI;
		seg->shape.arc.delta_origc = ea - sa;
	}

	/* sector area is used to compensate polyline area which is computed as if
	   there was a straight line between start and end;  */
	posdelta = seg->shape.arc.delta > 0 ? seg->shape.arc.delta : -seg->shape.arc.delta;
	seg->shape.arc.sect_area = ravg*ravg/2 * (posdelta - sin(posdelta));

	/* CCW always means it's cutting into the poly -> negative area */
	if (seg->shape.arc.adir == 0)
		seg->shape.arc.sect_area = -seg->shape.arc.sect_area;

#if 0
	rnd_trace("arc ends: %ld;%ld    %f;%f\n", seg->start[0], seg->start[1], seg->shape.arc.cx + cos(sa) * seg->shape.arc.r, seg->shape.arc.cy + sin(sa) * seg->shape.arc.r);
	rnd_trace("          %ld;%ld    %f;%f\n", seg->end[0], seg->end[1], seg->shape.arc.cx + cos(ea) * seg->shape.arc.r, seg->shape.arc.cy + sin(ea) * seg->shape.arc.r);
#endif
}

/*** common ***/

/* Returns 1 if p1 is closer to startpoint of seg than p2 */
RND_INLINE int pb2_seg_points_ordered(pb2_seg_t *seg, rnd_vector_t p1, rnd_vector_t p2)
{
	switch(seg->shape_type) {
		case RND_VNODE_LINE: return pb2_line_points_ordered(seg, p1, p2);
		case RND_VNODE_ARC: return pb2_arc_points_ordered(seg, p1, p2);
	}
	abort(); /* internal error: invalid seg shape_type */
	return -1;
}

/* Return tangential vector at endpoint "from" (not normalized) */
RND_INLINE void pb2_seg_tangent_from(rnd_vector_t dst, pb2_seg_t *seg, rnd_vector_t from)
{
	switch(seg->shape_type) {
		case RND_VNODE_LINE: pb2_line_tangent_from(dst, seg, from); return;
		case RND_VNODE_ARC: pb2_arc_tangent_from(dst, seg, from); return;
	}
	abort(); /* internal error: invalid seg shape_type */
}


/* Fill in dst with the slope "angle" of the segment at its endpoint "from" */
RND_INLINE void seg_angles_from(pa_angle_t *dst1, pa_angle_t *dst2, pb2_seg_t *seg, rnd_vector_t from)
{
	switch(seg->shape_type) {
		case RND_VNODE_LINE: pb2_line_angles_from(dst1, dst2, seg, from); return;
		case RND_VNODE_ARC: pb2_arc_angles_from(dst1, dst2, seg, from); return;
	}
	abort(); /* internal error: invalid seg shape_type */
}

RND_INLINE pb2_slope_t pb2_slope(pb2_seg_t *seg, rnd_vector_t origin, rnd_vector_t other)
{
	switch(seg->shape_type) {
		case RND_VNODE_LINE: return pb2_slope_line(origin, other);
		case RND_VNODE_ARC:  return pb2_slope_arc(seg, origin);
	}
	abort(); /* invalid shape type */
}


/* recompute cache fields for a seg using input/config fields of the seg */
RND_INLINE void pb2_seg_update_cache(pb2_ctx_t *ctx, pb2_seg_t *seg)
{
	switch(seg->shape_type) {
		case RND_VNODE_LINE: break;
		case RND_VNODE_ARC: pb2_seg_arc_update_cache(ctx, seg); break;
	}
}

RND_INLINE void pb2_seg_bbox(pb2_seg_t *seg)
{
	switch(seg->shape_type) {
		case RND_VNODE_LINE:
			seg->bbox.x1 = pa_min(seg->start[0], seg->end[0]);   seg->bbox.y1 = pa_min(seg->start[1], seg->end[1]);
			seg->bbox.x2 = pa_max(seg->start[0], seg->end[0])+1; seg->bbox.y2 = pa_max(seg->start[1], seg->end[1])+1;
			break;
		case RND_VNODE_ARC:
			pb2_arc_bbox(seg);
			break;
	}
}


/*** helpers for pa ***/
void pb2_raw_arc_bbox(rnd_rtree_box_t *dst, rnd_vector_t p1, rnd_vector_t p2, rnd_vector_t center, int adir)
{
	pb2_seg_t seg;
	g2d_carc_t carc;
	g2d_box_t bb;

	seg.shape_type = RND_VNODE_ARC;
	seg.start[0] = p1[0]; seg.start[1] = p1[1];
	seg.end[0] = p2[0]; seg.end[1] = p2[1];
	seg.shape.arc.center[0] = center[0]; seg.shape.arc.center[1] = center[1];
	seg.shape.arc.adir = adir;

	pb2_seg_arc_update_cache(NULL, &seg);
	SEG2CARC(carc, &seg);

	bb = g2d_carc_bbox(&carc);
	dst->x1 = floor(bb.p1.x); dst->y1 = floor(bb.p1.y);
	dst->x2 = ceil(bb.p2.x); dst->y2 = ceil(bb.p2.y);
}

int pb2_raw_pt_on_arc(rnd_vector_t pt,  rnd_vector_t p1, rnd_vector_t p2, rnd_vector_t center, int adir)
{
	pb2_seg_t seg;
	g2d_carc_t carc;
	g2d_cvect_t gpt;
	g2d_angle_t pang;
	double pr, sa, ea;

	/* cheap common case: endpoint match */
	if (Vequ2(pt, p1) || Vequ2(pt, p2))
		return 0;

	seg.shape_type = RND_VNODE_ARC;
	seg.start[0] = p1[0]; seg.start[1] = p1[1];
	seg.end[0] = p2[0]; seg.end[1] = p2[1];
	seg.shape.arc.center[0] = center[0]; seg.shape.arc.center[1] = center[1];
	seg.shape.arc.adir = adir;

	pb2_seg_arc_update_cache(NULL, &seg);
	SEG2CARC(carc, &seg);

	pr = rnd_vect_dist2(pt, center);
	if (pr != 0)
		pr = sqrt(pr);

	/* check if point is on the circle */
	if (fabs(pr - seg.shape.arc.r) > 0.5)
		return 0;

	/* figure point's angle */
	gpt.x = pt[0]; gpt.y = pt[1];
	pang = g2d__ang_carc_pt(&carc, gpt);

	/* figure ordered start and end angle */
	sa = carc.start;
	ea = carc.start + carc.delta;
	if (sa > ea) {
		double tmp = sa;
		sa = ea;
		ea = tmp;
	}

	return g2d_is_between_angles(pang, sa, ea);
}

/* copy gengeo2d results to rnd outpout */
#undef ISC_OUT
#define ISC_OUT(num) \
do { \
	if (num > 0) { \
		isc_out1[0] = rnd_round(ip[0].x); \
		isc_out1[1] = rnd_round(ip[0].y); \
	} \
	if (num > 1) { \
		isc_out2[0] = rnd_round(ip[1].x); \
		isc_out2[1] = rnd_round(ip[1].y); \
	} \
} while(0)

int pb2_raw_isc_arc_line(rnd_vector_t ap1, rnd_vector_t ap2, rnd_vector_t center, int adir, rnd_vector_t lp1, rnd_vector_t lp2, rnd_vector_t isc_out1, rnd_vector_t isc_out2)
{
	pb2_seg_t aseg, lseg;
	g2d_cline_t cline;
	g2d_carc_t carc;
	int num;
	g2d_vect_t ip[2];

	aseg.shape_type = RND_VNODE_ARC;
	aseg.start[0] = ap1[0]; aseg.start[1] = ap1[1];
	aseg.end[0] = ap2[0]; aseg.end[1] = ap2[1];
	aseg.shape.arc.center[0] = center[0]; aseg.shape.arc.center[1] = center[1];
	aseg.shape.arc.adir = adir;
	pb2_seg_arc_update_cache(NULL, &aseg);

	lseg.shape_type = RND_VNODE_LINE;
	lseg.start[0] = lp1[0]; lseg.start[1] = lp1[1];
	lseg.end[0] = lp2[0]; lseg.end[1] = lp2[1];

	SEG2CARC(carc, &aseg);
	SEG2CLINE(cline, &lseg);

	num = g2d_iscp_cline_carc(&cline, &carc,  ip, NULL, 0);
	ISC_OUT(num);
	return num;
}

int pb2_raw_isc_arc_arc(rnd_vector_t a1p1, rnd_vector_t a1p2, rnd_vector_t a1center, int a1adir, rnd_vector_t a2p1, rnd_vector_t a2p2, rnd_vector_t a2center, int a2adir, rnd_vector_t isc_out1, rnd_vector_t isc_out2)
{
	pb2_seg_t a1seg, a2seg;
	g2d_carc_t carc1, carc2;
	int num;
	g2d_vect_t ip[2];

	a1seg.shape_type = RND_VNODE_ARC;
	a1seg.start[0] = a1p1[0]; a1seg.start[1] = a1p1[1];
	a1seg.end[0] = a1p2[0]; a1seg.end[1] = a1p2[1];
	a1seg.shape.arc.center[0] = a1center[0]; a1seg.shape.arc.center[1] = a1center[1];
	a1seg.shape.arc.adir = a1adir;
	pb2_seg_arc_update_cache(NULL, &a1seg);

	a2seg.shape_type = RND_VNODE_ARC;
	a2seg.start[0] = a2p1[0]; a2seg.start[1] = a2p1[1];
	a2seg.end[0] = a2p2[0]; a2seg.end[1] = a2p2[1];
	a2seg.shape.arc.center[0] = a2center[0]; a2seg.shape.arc.center[1] = a2center[1];
	a2seg.shape.arc.adir = a2adir;
	pb2_seg_arc_update_cache(NULL, &a2seg);

	if (Vequ2(a1seg.shape.arc.center, a2seg.shape.arc.center) && (fabs(a1seg.shape.arc.r - a2seg.shape.arc.r) < 0.5)) {
		SEG2CARC_ORIGC(carc1, &a1seg);
		SEG2CARC_ORIGC(carc2, &a2seg);
		carc1.r = carc2.r = (a1seg.shape.arc.r + a2seg.shape.arc.r)/2.0;
	}
	else {
		SEG2CARC(carc1, &a1seg);
		SEG2CARC(carc2, &a2seg);
	}

	num = g2d_iscp_carc_carc(&carc1, &carc2,  ip, NULL);
	ISC_OUT(num);
	return num;
}

void pb2_raw_tangent_from_arc(rnd_vector_t dst, rnd_vector_t ap1, rnd_vector_t ap2, rnd_vector_t center, int adir, rnd_vector_t from)
{
	pb2_seg_t aseg;

	aseg.shape_type = RND_VNODE_ARC;
	aseg.start[0] = ap1[0]; aseg.start[1] = ap1[1];
	aseg.end[0] = ap2[0]; aseg.end[1] = ap2[1];
	aseg.shape.arc.center[0] = center[0]; aseg.shape.arc.center[1] = center[1];
	aseg.shape.arc.adir = adir;
	pb2_seg_arc_update_cache(NULL, &aseg);

	pb2_arc_tangent_from(dst, &aseg, from);
}
