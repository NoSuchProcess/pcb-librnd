/*
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

/* Vector utilities */

rnd_vector_t rnd_vect_zero = { (long) 0, (long) 0 };

#define Vzero(a)   ((a)[0] == 0. && (a)[1] == 0.)

#define Vsub(a,b,c) {(a)[0]=(b)[0]-(c)[0];(a)[1]=(b)[1]-(c)[1];}

int vect_equal(rnd_vector_t v1, rnd_vector_t v2)
{
	return (v1[0] == v2[0] && v1[1] == v2[1]);
}																/* vect_equal */


void vect_sub(rnd_vector_t res, rnd_vector_t v1, rnd_vector_t v2)
{
Vsub(res, v1, v2)}							/* vect_sub */

double rnd_vect_len2(rnd_vector_t v)
{
	return ((double) v[0] * v[0] + (double) v[1] * v[1]);
}

double rnd_vect_dist2(rnd_vector_t v1, rnd_vector_t v2)
{
	double dx = v1[0] - v2[0];
	double dy = v1[1] - v2[1];

	return (dx * dx + dy * dy);		/* why sqrt */
}

/* value has sign of angle between vectors */
double rnd_vect_det2(rnd_vector_t v1, rnd_vector_t v2)
{
	return (((double) v1[0] * v2[1]) - ((double) v2[0] * v1[1]));
}

static double vect_m_dist(rnd_vector_t v1, rnd_vector_t v2)
{
	double dx = v1[0] - v2[0];
	double dy = v1[1] - v2[1];
	double dd = (dx * dx + dy * dy);	/* sqrt */

	if (dx > 0)
		return +dd;
	if (dx < 0)
		return -dd;
	if (dy > 0)
		return +dd;
	return -dd;
}																/* vect_m_dist */

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
