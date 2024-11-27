/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2017, 2023, 2024 Tibor 'Igor2' Palinkas
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

/* 2D vector utilities */

/* These work only because rnd_vector_t is an array so it is packed. */
#define Vcpy2(dst, src)   memcpy((dst), (src), sizeof(rnd_vector_t))
#define Vequ2(a,b)       (memcmp((a),   (b),   sizeof(rnd_vector_t)) == 0)

#define Vsub2(r, a, b) \
	do { \
		(r)[0] = (a)[0] - (b)[0]; \
		(r)[1] = (a)[1] - (b)[1]; \
	} while(0)


#define Vswp2(a,b) \
	do { \
		rnd_coord_t t; \
		t = (a)[0]; (a)[0] = (b)[0]; (b)[0] = t; \
		t = (a)[1]; (a)[1] = (b)[1]; (b)[1] = t; \
	} while(0)

#define Vzero2(a)   ((a)[0] == 0 && (a)[1] == 0)

const rnd_vector_t rnd_vect_zero = {0, 0};

/* Distance square between v and 0 (length of v), in double to avoid overflow */
double rnd_vect_len2(rnd_vector_t v)
{
	double x = v[0], y = v[1];
	return x*x + y*y;
}

/* Distance square between v1 and v2, in double to avoid overflow */
double rnd_vect_dist2(rnd_vector_t v1, rnd_vector_t v2)
{
	double dx = v1[0] - v2[0], dy = v1[1] - v2[1];
	return dx*dx + dy*dy;
}

/* Cross product of v1 and v2: if the angle from v1 to v2 is between 0 an 90
   the cross product increases, then from 90 to 180 decreases back to 0 then
   from 180 to 270 decreases further into negative and from 270 to 360 increases
   back to 0. Maximum is when v1-to-v2 is 90 degrees; minimum is when v1-to-v2 is
   270 degrees (-90 degrees).
   Visual example: https://web.archive.org/web/20211026000030/https://engineeringstatics.org/cross-product-math.html */
double rnd_vect_det2(rnd_vector_t v1, rnd_vector_t v2)
{
	return (((double)v1[0] * (double)v2[1]) - ((double)v2[0] * (double)v1[1]));
}

#include "pa_vect_inline.c"

/* for the API */
int rnd_vect_inters2(rnd_vector_t p1, rnd_vector_t p2, rnd_vector_t q1, rnd_vector_t q2, rnd_vector_t S1, rnd_vector_t S2)
{
	return pa_vect_inters2(p1, p2, q1, q2, S1, S2, 1);
}


/* Compute whether point is inside a triangle formed by 3 other points
   Algorithm from http://www.blackpawn.com/texts/pointinpoly/default.html */
RND_INLINE int point_in_triangle(rnd_vector_t A, rnd_vector_t B, rnd_vector_t C, rnd_vector_t P)
{
	rnd_vector_t v0, v1, v2;
	double dot00, dot01, dot02, dot11, dot12;
	double inv_denom;
	double u, v;

	/* Compute vectors */
	v0[0] = C[0] - A[0];    v0[1] = C[1] - A[1];
	v1[0] = B[0] - A[0];    v1[1] = B[1] - A[1];
	v2[0] = P[0] - A[0];    v2[1] = P[1] - A[1];

	/* Compute dot products */
	dot00 = Vdot2(v0, v0);
	dot01 = Vdot2(v0, v1);
	dot02 = Vdot2(v0, v2);
	dot11 = Vdot2(v1, v1);
	dot12 = Vdot2(v1, v2);

	/* Compute barycentric coordinates */
	inv_denom = 1.0 / (dot00 * dot11 - dot01 * dot01);
	u = (dot11 * dot02 - dot01 * dot12) * inv_denom;
	v = (dot00 * dot12 - dot01 * dot02) * inv_denom;

	/* Check if point is in triangle */
	return (u > 0.0) && (v > 0.0) && ((u + v) < 1.0);
}

/* wrapper to keep the original name short and original function inline */
int rnd_point_in_triangle(rnd_vector_t A, rnd_vector_t B, rnd_vector_t C, rnd_vector_t P)
{
	return point_in_triangle(A, B, C, P);
}


/* Returns the dot product of vector A->B, and a vector
   orthogonal to rnd_vector_t C->D. The result is not normalised, so will be
   weighted by the magnitude of the C->D vector. */
static double dot_orthogonal_to_direction(rnd_vector_t A, rnd_vector_t B, rnd_vector_t C, rnd_vector_t D)
{
	rnd_vector_t l1, l2, ort;

	/* input line vectors */
	l1[0] = B[0] - A[0];     l1[1] = B[1] - A[1];
	l2[0] = D[0] - C[0];     l2[1] = D[1] - C[1];

	/* orthogonal vector of l2 */
	ort[0] = -l2[1];         ort[1] = l2[0];

	return Vdot2(l1, ort);
}

rnd_bool_t rnd_is_point_in_convex_quad(rnd_vector_t p, rnd_vector_t *q)
{
	return point_in_triangle(q[0], q[1], q[2], p) || point_in_triangle(q[0], q[3], q[2], p);
}

rnd_bool_t pa_is_node_on_line(rnd_vnode_t *node, rnd_vnode_t *l1, rnd_vnode_t *l2)
{
	TODO("arc: Implement me");
	return 0;
}

rnd_bool_t pa_is_node_on_arc(rnd_vnode_t *node, rnd_vnode_t *arc)
{
	return pb2_raw_pt_on_arc(node->point, arc->point, arc->next->point, arc->curve.arc.center, arc->curve.arc.adir);
}

rnd_bool_t pa_is_node_on_curve(rnd_vnode_t *node, rnd_vnode_t *seg)
{
	switch(seg->flg.curve_type) {
		case RND_VNODE_LINE: return pa_is_node_on_line(node, seg, seg->next);
		case RND_VNODE_ARC: return pa_is_node_on_arc(node, seg);
	}
	return rnd_false;
}

RND_INLINE int pa_curve_isc_arc_line(rnd_vnode_t *arc, rnd_vnode_t *line, rnd_vector_t i1, rnd_vector_t i2)
{
	return pb2_raw_isc_arc_line(
		arc->point, arc->next->point, arc->curve.arc.center, arc->curve.arc.adir,
		line->point, line->next->point,
		i1, i2);
}

RND_INLINE int pa_curve_isc_arc_arc(rnd_vnode_t *arc1, rnd_vnode_t *arc2, rnd_vector_t i1, rnd_vector_t i2)
{
	return pb2_raw_isc_arc_arc(
		arc1->point, arc1->next->point, arc1->curve.arc.center, arc1->curve.arc.adir,
		arc2->point, arc2->next->point, arc2->curve.arc.center, arc2->curve.arc.adir,
		i1, i2);
}

RND_INLINE int pa_curves_isc(rnd_vnode_t *a1, rnd_vnode_t *a2, rnd_vector_t i1, rnd_vector_t i2)
{
	switch(a1->flg.curve_type) {
		case RND_VNODE_LINE:
			switch(a2->flg.curve_type) {
				case RND_VNODE_LINE: return rnd_vect_inters2(a1->point, a1->next->point, a2->point, a2->next->point, i1, i2);
				case RND_VNODE_ARC: return pa_curve_isc_arc_line(a2, a1, i1, i2);
			}
			break;

		case RND_VNODE_ARC:
			switch(a2->flg.curve_type) {
				case RND_VNODE_LINE: return pa_curve_isc_arc_line(a1, a2, i1, i2);
				case RND_VNODE_ARC: return pa_curve_isc_arc_arc(a1, a2, i1, i2);
			}
			break;
	}

	return 0; /* invalid curve type */
}



void pa_calc_angle_nn(pa_angle_t *dst, rnd_vector_t PT, rnd_vector_t OTHER)
{
	double dx, dy;
	rnd_coord_t dxdy;
	pa_angle_t ang2;
	int xneg = 0, yneg = 0;

	dx = OTHER[0] - PT[0];
	dy = OTHER[1] - PT[1];

	assert((dx != 0) || (dy != 0));

	if (dx < 0) {
		dx = -dx;
		xneg = 1;
	}
	if (dy < 0) {
		dy = -dy;
		yneg = 1;
	}

	dxdy = dx + dy;
	ang2 = dy / dxdy;

	/* now move to the actual quadrant */
	if (xneg && !yneg)         *dst = 2.0 - ang2;  /* 2nd quadrant */
	else if (xneg && yneg)     *dst = 2.0 + ang2;  /* 3rd quadrant */
	else if (!xneg && yneg)    *dst = 4.0 - ang2;  /* 4th quadrant */
	else                       *dst = ang2;       /* 1st quadrant */

	assert(pa_angle_valid(*dst));
}
