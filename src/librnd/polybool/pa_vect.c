/*
       Copyright (C) 2006 harry eaton

   based on:
       poly_Boolean: a polygon clip library
       Copyright (C) 1997  Alexey Nikitin, Michael Leonov
       (also the authors of the paper describing the actual algorithm)
       leonov@propro.iis.nsk.su

   in turn based on:
       nclip: a polygon clip library
       Copyright (C) 1993  Klamer Schutte
 
       This program is free software; you can redistribute it and/or
       modify it under the terms of the GNU General Public
       License as published by the Free Software Foundation; either
       version 2 of the License, or (at your option) any later version.
 
       This program is distributed in the hope that it will be useful,
       but WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
       General Public License for more details.
 
       You should have received a copy of the GNU General Public
       License along with this program; if not, write to the Free
       Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 
      polygon1.c
      (C) 1997 Alexey Nikitin, Michael Leonov
      (C) 1993 Klamer Schutte

      all cases where original (Klamer Schutte) code is present
      are marked
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

/* value has sign of angle between vectors */
double rnd_vect_det2(rnd_vector_t v1, rnd_vector_t v2)
{
	return (((double)v1[0] * (double)v2[1]) - ((double) v2[0] * (double)v1[1]));
}

/* Signed distance square between v1 and v2; positive if v1.x > v2.x or
   when x components are equal, if v1.y > v2.y */
static double vect_m_dist(rnd_vector_t v1, rnd_vector_t v2)
{
	double dx = v1[0] - v2[0];
	double dy = v1[1] - v2[1];
	double dd = (dx * dx + dy * dy);

	if (dx > 0)    return +dd;
	if (dx < 0)    return -dd;
	if (dy > 0)    return +dd;
	/* (dy < 0) */ return -dd;
}

/*
vect_inters2
 (C) 1993 Klamer Schutte
 (C) 1997 Michael Leonov, Alexey Nikitin
*/

int rnd_vect_inters2(rnd_vector_t p1, rnd_vector_t p2, rnd_vector_t q1, rnd_vector_t q2, rnd_vector_t S1, rnd_vector_t S2)
{
	double s, t, deel;
	double rpx, rpy, rqx, rqy;

	if (max(p1[0], p2[0]) < min(q1[0], q2[0]) ||
			max(q1[0], q2[0]) < min(p1[0], p2[0]) || max(p1[1], p2[1]) < min(q1[1], q2[1]) || max(q1[1], q2[1]) < min(p1[1], p2[1]))
		return 0;

	rpx = p2[0] - p1[0];
	rpy = p2[1] - p1[1];
	rqx = q2[0] - q1[0];
	rqy = q2[1] - q1[1];

	deel = rpy * rqx - rpx * rqy;	/* -vect_det(rp,rq); */

	/* coordinates are 30-bit integers so deel will be exactly zero
	 * if the lines are parallel
	 */

	if (deel == 0) {							/* parallel */
		double dc1, dc2, d1, d2, h;	/* Check to see whether p1-p2 and q1-q2 are on the same line */
		rnd_vector_t hp1, hq1, hp2, hq2, q1p1, q1q2;

		Vsub2(q1p1, q1, p1);
		Vsub2(q1q2, q1, q2);


		/* If this product is not zero then p1-p2 and q1-q2 are not on same line! */
		if (rnd_vect_det2(q1p1, q1q2) != 0)
			return 0;
		dc1 = 0;										/* m_len(p1 - p1) */

		dc2 = vect_m_dist(p1, p2);
		d1 = vect_m_dist(p1, q1);
		d2 = vect_m_dist(p1, q2);

/* Sorting the independent points from small to large */
		Vcpy2(hp1, p1);
		Vcpy2(hp2, p2);
		Vcpy2(hq1, q1);
		Vcpy2(hq2, q2);
		if (dc1 > dc2) {						/* hv and h are used as help-variable. */
			Vswp2(hp1, hp2);
			h = dc1, dc1 = dc2, dc2 = h;
		}
		if (d1 > d2) {
			Vswp2(hq1, hq2);
			h = d1, d1 = d2, d2 = h;
		}

/* Now the line-pieces are compared */

		if (dc1 < d1) {
			if (dc2 < d1)
				return 0;
			if (dc2 < d2) {
				Vcpy2(S1, hp2);
				Vcpy2(S2, hq1);
			}
			else {
				Vcpy2(S1, hq1);
				Vcpy2(S2, hq2);
			};
		}
		else {
			if (dc1 > d2)
				return 0;
			if (dc2 < d2) {
				Vcpy2(S1, hp1);
				Vcpy2(S2, hp2);
			}
			else {
				Vcpy2(S1, hp1);
				Vcpy2(S2, hq2);
			};
		}
		return (Vequ2(S1, S2) ? 1 : 2);
	}
	else {												/* not parallel */
		/*
		 * We have the lines:
		 * l1: p1 + s(p2 - p1)
		 * l2: q1 + t(q2 - q1)
		 * And we want to know the intersection point.
		 * Calculate t:
		 * p1 + s(p2-p1) = q1 + t(q2-q1)
		 * which is similar to the two equations:
		 * p1x + s * rpx = q1x + t * rqx
		 * p1y + s * rpy = q1y + t * rqy
		 * Multiplying these by rpy resp. rpx gives:
		 * rpy * p1x + s * rpx * rpy = rpy * q1x + t * rpy * rqx
		 * rpx * p1y + s * rpx * rpy = rpx * q1y + t * rpx * rqy
		 * Subtracting these gives:
		 * rpy * p1x - rpx * p1y = rpy * q1x - rpx * q1y + t * ( rpy * rqx - rpx * rqy )
		 * So t can be isolated:
		 * t = (rpy * ( p1x - q1x ) + rpx * ( - p1y + q1y )) / ( rpy * rqx - rpx * rqy )
		 * and s can be found similarly
		 * s = (rqy * (q1x - p1x) + rqx * (p1y - q1y))/( rqy * rpx - rqx * rpy)
		 */

		if (Vequ2(q1, p1) || Vequ2(q1, p2)) {
			S1[0] = q1[0];
			S1[1] = q1[1];
		}
		else if (Vequ2(q2, p1) || Vequ2(q2, p2)) {
			S1[0] = q2[0];
			S1[1] = q2[1];
		}
		else {
			s = (rqy * (p1[0] - q1[0]) + rqx * (q1[1] - p1[1])) / deel;
			if (s < 0 || s > 1.)
				return 0;
			t = (rpy * (p1[0] - q1[0]) + rpx * (q1[1] - p1[1])) / deel;
			if (t < 0 || t > 1.)
				return 0;

			S1[0] = q1[0] + ROUND(t * rqx);
			S1[1] = q1[1] + ROUND(t * rqy);
		}
		return 1;
	}
}																/* vect_inters2 */

static double dot(rnd_vector_t A, rnd_vector_t B)
{
	return (double) A[0] * (double) B[0] + (double) A[1] * (double) B[1];
}

/* Compute whether point is inside a triangle formed by 3 other points */
/* Algorithm from http://www.blackpawn.com/texts/pointinpoly/default.html */
static int point_in_triangle(rnd_vector_t A, rnd_vector_t B, rnd_vector_t C, rnd_vector_t P)
{
	rnd_vector_t v0, v1, v2;
	double dot00, dot01, dot02, dot11, dot12;
	double invDenom;
	double u, v;

	/* Compute vectors */
	v0[0] = C[0] - A[0];
	v0[1] = C[1] - A[1];
	v1[0] = B[0] - A[0];
	v1[1] = B[1] - A[1];
	v2[0] = P[0] - A[0];
	v2[1] = P[1] - A[1];

	/* Compute dot products */
	dot00 = dot(v0, v0);
	dot01 = dot(v0, v1);
	dot02 = dot(v0, v2);
	dot11 = dot(v1, v1);
	dot12 = dot(v1, v2);

	/* Compute barycentric coordinates */
	invDenom = 1. / (dot00 * dot11 - dot01 * dot01);
	u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	/* Check if point is in triangle */
	return (u > 0.0) && (v > 0.0) && (u + v < 1.0);
}

/* wrapper to keep the original name short and original function inline */
int rnd_point_in_triangle(rnd_vector_t A, rnd_vector_t B, rnd_vector_t C, rnd_vector_t P)
{
	return point_in_triangle(A, B, C, P);
}


/* Returns the dot product of rnd_vector_t A->B, and a vector
 * orthogonal to rnd_vector_t C->D. The result is not normalised, so will be
 * weighted by the magnitude of the C->D vector.
 */
static double dot_orthogonal_to_direction(rnd_vector_t A, rnd_vector_t B, rnd_vector_t C, rnd_vector_t D)
{
	rnd_vector_t l1, l2, l3;
	l1[0] = B[0] - A[0];
	l1[1] = B[1] - A[1];
	l2[0] = D[0] - C[0];
	l2[1] = D[1] - C[1];

	l3[0] = -l2[1];
	l3[1] = l2[0];

	return dot(l1, l3);
}

/*
 * rnd_is_point_in_convex_quad()
 * (C) 2017 Tibor 'Igor2' Palinkas
*/
rnd_bool_t rnd_is_point_in_convex_quad(rnd_vector_t p, rnd_vector_t *q)
{
	return point_in_triangle(q[0], q[1], q[2], p) || point_in_triangle(q[0], q[3], q[2], p);
}
