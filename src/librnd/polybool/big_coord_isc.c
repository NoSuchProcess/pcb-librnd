/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2023 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2023)
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
 */
/* Corner case handler: when p1..p2 and q1..q2 are parallel */
RND_INLINE int pa_big_coord_isc_par(pa_big_vector_t res[2], pa_big_vector_t p1, pa_big_vector_t p2, pa_big_vector_t q1, pa_big_vector_t q2)
{
	pa_big2_coord_t dc1, dc2, d1, d2;
	pa_big_vector_t tmp1, tmp2, tmq1, tmq2;

	/* to easy conversion of coords to big coords - results are always on input coords */
	memset(res, 0, sizeof(pa_big_vector_t) * 2);

	/* Figure overlaps by distances:
	
	       p1-------------------p2
	            q1--------q2
	
	   Distance vectors:
	       dc1
	       -------------------->dc2
	       ---->d1
	       -------------->d2
	*/

	memset(dc1, 0, sizeof(dc1));
	rnd_vect_m_dist2_big(dc2, p1, p2);

	rnd_vect_m_dist2_big(d1, p1, q1);
	rnd_vect_m_dist2_big(d2, p1, q2);

	/* Make sure dc1 is always the smaller one (always on the left);
	   depends on p1..p2 direction */
	if (big_signed_cmpn(dc1, dc2, W2) > 0) { /* dc1 > dc2 */
		memcpy(&tmp1, &p2, sizeof(tmp1));
		memcpy(&tmp2, &p1, sizeof(tmp2));
		big_swap(dc1, dc2, W2);
	}
	else {
		memcpy(&tmp1, &p1, sizeof(tmp1));
		memcpy(&tmp2, &p2, sizeof(tmp2));
	}

	/* Make sure d1 is always the smaller one (always on the left);
	   depends on q1..q2 direction */
	if (big_signed_cmpn(d1, d2, W2) > 0) { /* d1 > d2 */
		memcpy(&tmq1, &q2, sizeof(tmq1));
		memcpy(&tmq2, &q1, sizeof(tmq1));
		big_swap(d1, d2, W2);
	}
	else {
		memcpy(&tmq1, &q1, sizeof(tmq1));
		memcpy(&tmq2, &q2, sizeof(tmq1));
	}

	/* by now tmp* and tmq* are ordered p1..p2 and q1..q2 */

	/* Compare distances to figure what overlaps */
	if (big_signed_cmpn(dc1, d1, W2) < 0) { /* (dc1 < d1) */
		if (big_signed_cmpn(dc2, d1, W2) < 0) /* (dc2 < d1) */
			return 0;
		if (big_signed_cmpn(dc2, d2, W2) < 0) { /* (dc2 < d2) */
			memcpy(&res[0], &tmp2, sizeof(tmp2));
			memcpy(&res[1], &tmq1, sizeof(tmq2));
		}
		else {
			memcpy(&res[0], &tmq1, sizeof(tmq1));
			memcpy(&res[1], &tmq2, sizeof(tmq2));
		}
	}
	else {
		if (big_signed_cmpn(dc1, d2, W2) > 0) /* (dc1 > d2) */
			return 0;
		if (big_signed_cmpn(dc2, d2, W2) < 0) { /* (dc2 < d2) */
			memcpy(&res[0], &tmp1, sizeof(tmp1));
			memcpy(&res[1], &tmp2, sizeof(tmp2));
		}
		else {
			memcpy(&res[0], &tmp1, sizeof(tmp1));
			memcpy(&res[1], &tmq2, sizeof(tmq2));
		}
	}

	/* if the two intersections are the same, return only one; denominators are
	   always 1, do not compare them */
	if ((big_signed_cmpn(res[0].x, res[1].x, W) == 0) && (big_cmpn(res[0].y, res[1].y, W) == 0))
		return 1;

	return 2;
}

int rnd_big_coord_isc(pa_big_vector_t res[2], pa_big_vector_t p1, pa_big_vector_t p2, pa_big_vector_t q1, pa_big_vector_t q2)
{
	pa_big_coord_t dx1, dy1, dx3, dy3;
	pa_big2_coord_t tmp1, tmp2, a, b, denom, r, d2x1, d2y1, d2x3, d2y3;
	pa_big3_coord_t TMP3, A, B, DENOM;

	/* if bounding boxes don't overlap, no need to check, they can't intersect */
	if (big_signed_cmpn(pa_big_max(p1.x, p2.x), pa_big_min(q1.x, q2.x), W) < 0) return 0;
	if (big_signed_cmpn(pa_big_max(q1.x, q2.x), pa_big_min(p1.x, p2.x), W) < 0) return 0;
	if (big_signed_cmpn(pa_big_max(p1.y, p2.y), pa_big_min(q1.y, q2.y), W) < 0) return 0;
	if (big_signed_cmpn(pa_big_max(q1.y, q2.y), pa_big_min(p1.y, p2.y), W) < 0) return 0;

/* https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection as of 2023 March */
#define X1 (p1.x)
#define Y1 (p1.y)
#define X2 (p2.x)
#define Y2 (p2.y)
#define X3 (q1.x)
#define Y3 (q1.y)
#define X4 (q2.x)
#define Y4 (q2.y)

	big_subn(dx1, X1, X2, W, 0);
	big_subn(dy1, Y1, Y2, W, 0);
	big_subn(dx3, X3, X4, W, 0);
	big_subn(dy3, Y3, Y4, W, 0);

	/* denom = dx1 * dy3 - dy1 * dx3 */
	big_signed_mul(a, W2, dx1, dy3, W);
	big_signed_mul(b, W2, dy1, dx3, W);
	big_subn(denom, a, b, W2, 0);

	if (big_is_zero(denom, W2)) {
		pa_big_coord_t dx0, dy0;

		/* parallel, check if X1;Y1 is on the infinite line of q (dx3;dy3)
		   Method:
		   dx3=X3-X4; dy3=Y3-Y4
		   dx0=X3-X1; dy0=Y3-Y1
		   if point is on line, the three points form two similar triangles:
		
		   dx0/dy0 == dx3/dy3
		
		   or in other words:
		
		   dx0*dy3 == dx3*dy0
		*/
		big_subn(dx0, X3, X1, W, 0);
		big_subn(dy0, Y3, Y1, W, 0);
		big_signed_mul(a, W2, dx0, dy3, W);
		big_signed_mul(b, W2, dy0, dx3, W);
		if (big_signed_cmpn(a, b, W2) != 0)
			return 0; /* point not on line */

		return pa_big_coord_isc_par(res, p1, p2, q1, q2);
	}

	pa_big_to_big2(d2x1, dx1);
	pa_big_to_big2(d2y1, dy1);
	pa_big_to_big2(d2x3, dx3);
	pa_big_to_big2(d2y3, dy3);
	pa_big2_to_big3(DENOM, denom);

	/* tmp1 = x1*y2 - y1*x2 */
	big_signed_mul(a, W2, X1, Y2, W);
	big_signed_mul(b, W2, Y1, X2, W);
	big_subn(tmp1, a, b, W2, 0);

	/* tmp2 = x3*y4 - y3*x4 */
	big_signed_mul(a, W2, X3, Y4, W);
	big_signed_mul(b, W2, Y3, X4, W);
	big_subn(tmp2, a, b, W2, 0);

	/* Px = (tmp1 * dx3 - tmp2 * dx1)  /  denom */
	big_signed_mul(A, W3, tmp1, d2x3, W2);
	big_signed_mul(B, W3, tmp2, d2x1, W2);
	big_subn(TMP3, A, B, W3, 0);
	big_zero(res[0].x, W3);
	big_signed_div(res[0].x, r, TMP3, W3, DENOM, W3, NULL); /* result length is 1 */
	if (!pa_big_in_between(pa_big_less(X1, X2), X1, X2, res[0].x)) return 0;
	if (!pa_big_in_between(pa_big_less(X3, X4), X3, X4, res[0].x)) return 0;

	/* Py = (tmp1 * dy3 - tmp2 * dy1) / denom */
	big_signed_mul(A, W3, tmp1, d2y3, W2);
	big_signed_mul(B, W3, tmp2, d2y1, W2);
	big_subn(TMP3, A, B, W3, 0);
	big_zero(res[0].y, W3);
	big_signed_div(res[0].y, r, TMP3, W3, DENOM, W3, NULL); /* result length is 1 */
	if (!pa_big_in_between(pa_big_less(Y1, Y2), Y1, Y2, res[0].y)) return 0;
	if (!pa_big_in_between(pa_big_less(Y3, Y4), Y3, Y4, res[0].y)) return 0;
	return 1;
}

int pa_big_inters2(rnd_vnode_t *v1a, rnd_vnode_t *v1b, rnd_vnode_t *v2a, rnd_vnode_t *v2b, pa_big_vector_t *isc1, pa_big_vector_t *isc2)
{
	pa_big_vector_t tmp[2], V1A, V1B, V2A, V2B;
	int res;

	pa_big_load_cvc(V1A, v1a);
	pa_big_load_cvc(V1B, v1b);
	pa_big_load_cvc(V2A, v2a);
	pa_big_load_cvc(V2B, v2b);

	res = rnd_big_coord_isc(tmp, V1A, V1B, V2A, V2B);
	if (res > 0) memcpy(isc1, &tmp[0], sizeof(pa_big_vector_t));
	if (res > 1) memcpy(isc2, &tmp[1], sizeof(pa_big_vector_t));

	return res;
}

rnd_vnode_t *pa_big_node_add_single(rnd_vnode_t *dst, pa_big_vector_t ptv)
{
	rnd_vnode_t *newnd;
	rnd_vector_t small;

  /* no new allocation for redundant node around dst */
	if (pa_big_vnode_vect_equ(dst, ptv))        return dst;
	if (pa_big_vnode_vect_equ(dst->next, ptv))  return dst->next;

	/* have to allocate a new node */
	small[0] = pa_big2small(ptv.x);
	small[1] = pa_big2small(ptv.y);
	newnd = rnd_poly_node_create(small);
	if (newnd != NULL)
		newnd->flg.plabel = PA_PTL_UNKNWN;

	return newnd;
}
