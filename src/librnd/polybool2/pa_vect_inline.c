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

/* Signed distance square between v1 and v2; positive if v1.x > v2.x or
   when x components are equal, if v1.y > v2.y */
static double vect_m_dist2(rnd_vector_t v1, rnd_vector_t v2)
{
	double dx = v1[0] - v2[0];
	double dy = v1[1] - v2[1];
	double dd = (dx * dx + dy * dy);

	if (dx > 0)    return +dd;
	if (dx < 0)    return -dd;
	if (dy > 0)    return +dd;
	/* (dy < 0) */ return -dd;
}

RND_INLINE double Vdot2(rnd_vector_t A, rnd_vector_t B)
{
	return (double)A[0] * (double)B[0] + (double)A[1] * (double)B[1];
}

#define SWAP(type, a, b) \
do { \
	type __tmp__ = a; \
	a = b; \
	b = __tmp__; \
} while(0)


/* Corner case handler: when p1..p2 and q1..q2 are parallel */
RND_INLINE int rnd_vect_inters2_par(rnd_vector_t p1, rnd_vector_t p2, rnd_vector_t q1, rnd_vector_t q2, rnd_vector_t S1, rnd_vector_t S2)
{
	double dc1, dc2, d1, d2;
	rnd_vector_t tmp1, tmp2, tmq1, tmq2, q1p1, q1q2;

	Vsub2(q1p1, q1, p1);
	Vsub2(q1q2, q1, q2);

	/* check if p1..p2 and q1..q2 are not on same line (no intersections then) */
	if (rnd_vect_det2(q1p1, q1q2) != 0)
		return 0;

	/* Figure overlaps by distances:
	
	       p1-------------------p2
	            q1--------q2
	
	   Distance vectors:
	       dc1
	       -------------------->dc2
	       ---->d1
	       -------------->d2
	*/

	dc1 = 0;
	dc2 = vect_m_dist2(p1, p2);
	d1 = vect_m_dist2(p1, q1);
	d2 = vect_m_dist2(p1, q2);

	/* Make sure dc1 is always the smaller one (always on the left);
	   depends on p1..p2 direction */
	if (dc1 > dc2) {
		Vcpy2(tmp1, p2);
		Vcpy2(tmp2, p1);
		SWAP(double, dc1, dc2);
	}
	else {
		Vcpy2(tmp1, p1);
		Vcpy2(tmp2, p2);
	}

	/* Make sure d1 is always the smaller one (always on the left);
	   depends on q1..q2 direction */
	if (d1 > d2) {
		Vcpy2(tmq1, q2);
		Vcpy2(tmq2, q1);
		SWAP(double, d1, d2);
	}
	else {
		Vcpy2(tmq1, q1);
		Vcpy2(tmq2, q2);
	}

	/* by now tmp* and tmq* are ordered p1..p2 and q1..q2 */

	/* Compare distances to figure what overlaps */
	if (dc1 < d1) {
		if (dc2 < d1)
			return 0;
		if (dc2 < d2) {
			Vcpy2(S1, tmp2);
			Vcpy2(S2, tmq1);
		}
		else {
			Vcpy2(S1, tmq1);
			Vcpy2(S2, tmq2);
		}
	}
	else {
		if (dc1 > d2)
			return 0;
		if (dc2 < d2) {
			Vcpy2(S1, tmp1);
			Vcpy2(S2, tmp2);
		}
		else {
			Vcpy2(S1, tmp1);
			Vcpy2(S2, tmq2);
		}
	}

	return Vequ2(S1, S2) ? 1 : 2;
}

RND_INLINE int pa_vect_inters2(rnd_vector_t p1, rnd_vector_t p2, rnd_vector_t q1, rnd_vector_t q2, rnd_vector_t S1, rnd_vector_t S2, int check_bbox)
{
	double s, t, divider;
	double pdx, pdy, qdx, qdy;

	if (check_bbox) {
		/* if bounding boxes don't overlap, no need to check, they can't intersect */
		if (pa_max(p1[0], p2[0]) < pa_min(q1[0], q2[0])) return 0;
		if (pa_max(q1[0], q2[0]) < pa_min(p1[0], p2[0])) return 0;
		if (pa_max(p1[1], p2[1]) < pa_min(q1[1], q2[1])) return 0;
		if (pa_max(q1[1], q2[1]) < pa_min(p1[1], p2[1])) return 0;
	}

	/* calculate and cache delta_x/delta_y for p and q */
	pdx = p2[0] - p1[0]; pdy = p2[1] - p1[1];
	qdx = q2[0] - q1[0]; qdy = q2[1] - q1[1];

	/* We are looking for the intersection point of these two non-parallel lines:
	line #1: p1 + s*(p2 - p1)
	line #2: q1 + t*(q2 - q1)
	
	The intersection is:
	p1 + s*(p2-p1) = q1 + t*(q2-q1)
	
	Substituting vectors:
	p1.x + s * pdx = q1.x + t * qdx
	p1.y + s * pdy = q1.y + t * qdy
	
	Multiplying these by pdy resp. pdx gives:
	pdy * p1.x + s * pdx * pdy = pdy * q1.x + t * pdy * qdx
	pdx * p1.y + s * pdx * pdy = pdx * q1.y + t * pdx * qdy
	
	Subtracting these gives:
	pdy * p1x - pdx * p1y = pdy * q1x - pdx * q1y + t * (pdy*qdx - pdx*qdy)
	
	So t can be isolated:
	t = (pdy * (p1x - q1x) + pdx * (-p1y+q1y))/(pdy*qdx - pdx*qdy)
	and s can be found similarly
	s = (qdy * (q1x - p1x) + qdx * (+p1y-q1y))/(qdy*pdx - qdx*pdy)
	
	Cache (qdy*pdx - qdx*pdy) as divider. */

	divider = pdy * qdx - pdx * qdy;
	if (divider == 0) /* divider is zero if the lines are parallel */
		return rnd_vect_inters2_par(p1, p2, q1, q2, S1, S2);

	/* degenerate cases: endpoint on the other line */
	if (Vequ2(q1, p1) || Vequ2(q1, p2)) {
		S1[0] = q1[0]; S1[1] = q1[1];
		return 1;
	}
	if (Vequ2(q2, p1) || Vequ2(q2, p2)) {
		S1[0] = q2[0]; S1[1] = q2[1];
		return 1;
	}

	s = (qdy * (p1[0] - q1[0]) + qdx * (q1[1] - p1[1])) / divider;
	if (s < 0.0 || s > 1.0)
		return 0;
	t = (pdy * (p1[0] - q1[0]) + pdx * (q1[1] - p1[1])) / divider;
	if (t < 0.0 || t > 1.0)
		return 0;

	S1[0] = q1[0] + PA_ROUND(t * qdx);
	S1[1] = q1[1] + PA_ROUND(t * qdy);
	return 1;
}
