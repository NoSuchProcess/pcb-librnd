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

/* Calculate and track intersections and edge angles using multi-precision
   rational numbers with widths chosen so that they never overflow and do
   not lose precision (assigns different coords to different intersection
   points even if they are very close) */

#include <librnd/config.h>

#include "pa_math.c"

/*** configure fixed point numbers ***/


typedef rnd_ucoord_t big_word;

#define PB_RATIONAL_ISC
#include "pa.h"

#include <genfip/big.h>


/*** implementation ***/

#define W PA_BIGCRD_WIDTH
#define W2 (PA_BIGCRD_WIDTH*2)
#define W3 (PA_BIGCRD_WIDTH*3)

void pa_big_load(pa_big_coord_t dst, rnd_coord_t src)
{
	TODO("sign extend");
	memset(dst, 0, PA_BIGCOORD_SIZEOF);
	dst[W/2] = src;
}

/* Convert a fixed point big_coord to a small one using floor() */
RND_INLINE rnd_coord_t pa_big2small(pa_big_coord_t src) { return src[W/2]; }


/* Distance square between v1 and v2 in big coord */
RND_INLINE void rnd_vect_m_dist2_big(pa_big_coord_t dst, pa_big_vector_t v1, pa_big_vector_t v2)
{
	pa_big_coord_t a, b, dx, dy;
	int dxs, dys;

	big_subn(dx, v1.x, v2.x, W, 0);
	big_subn(dy, v1.y, v2.y, W, 0);

	big_mul(a, W, dx, dx, W);
	big_mul(b, W, dy, dy, W);

	big_addn(dst, a, b, W, 0);

	dxs = big_sgn(dx, W);
	if (dxs > 0)    {                       return /*+dd*/; }
	if (dxs < 0)    { big_neg(dst, dst, W); return /*-dd*/; }

	dys = big_sgn(dy, W);
	if (dys > 0)    {                       return /*+dd*/; }
	if (dys < 0)    { big_neg(dst, dst, W); return /*-dd*/; }
}

/* Corner case handler: when p1..p2 and q1..q2 are parallel */
RND_INLINE int rnd_big_coord_isc_par(pa_big_vector_t res[2], pa_big_vector_t p1, pa_big_vector_t p2, pa_big_vector_t q1, pa_big_vector_t q2)
{
	pa_big_coord_t dc1, dc2, d1, d2;
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
	if (big_signed_cmpn(dc1, dc2, W) > 0) { /* dc1 > dc2 */
		memcpy(&tmp1, &p2, sizeof(tmp1));
		memcpy(&tmp2, &p1, sizeof(tmp2));
		big_swap(dc1, dc2, W);
	}
	else {
		memcpy(&tmp1, &p1, sizeof(tmp1));
		memcpy(&tmp2, &p2, sizeof(tmp2));
	}

	/* Make sure d1 is always the smaller one (always on the left);
	   depends on q1..q2 direction */
	if (big_signed_cmpn(d1, d2, W) > 0) { /* d1 > d2 */
		memcpy(&tmq1, &q2, sizeof(tmq1));
		memcpy(&tmq2, &q1, sizeof(tmq1));
		big_swap(d1, d2, W);
	}
	else {
		memcpy(&tmq1, &q1, sizeof(tmq1));
		memcpy(&tmq2, &q2, sizeof(tmq1));
	}

	/* by now tmp* and tmq* are ordered p1..p2 and q1..q2 */

	/* Compare distances to figure what overlaps */
	if (big_signed_cmpn(dc1, d1, W) < 0) { /* (dc1 < d1) */
		if (big_signed_cmpn(dc2, d1, W) < 0) /* (dc2 < d1) */
			return 0;
		if (big_signed_cmpn(dc2, d2, W) < 0) { /* (dc2 < d2) */
			memcpy(&res[0], &tmp2, sizeof(tmp2));
			memcpy(&res[1], &tmq1, sizeof(tmq2));
		}
		else {
			memcpy(&res[0], &tmq1, sizeof(tmq1));
			memcpy(&res[1], &tmq2, sizeof(tmq2));
		}
	}
	else {
		if (big_signed_cmpn(dc1, d2, W) > 0) /* (dc1 > d2) */
			return 0;
		if (big_signed_cmpn(dc2, d2, W) < 0) { /* (dc2 < d2) */
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

/* Returns if num is between A and B */
RND_INLINE int pa_big_in_between(int ordered, pa_big_coord_t A, pa_big_coord_t B, pa_big_coord_t num)
{
	if (ordered) {
		if (big_signed_cmpn(num, A, W) < 0) return 0;
		if (big_signed_cmpn(num, B, W) > 0) return 0;
	}
	else {
		/* swap so A < B */
		if (big_signed_cmpn(num, B, W) < 0) return 0;
		if (big_signed_cmpn(num, A, W) > 0) return 0;
	}

	return 1;
}

#define pa_big_min(a, b)  ((big_signed_cmpn((a), (b), W) <= 0) ? (a) : (b))
#define pa_big_max(a, b)  ((big_signed_cmpn((a), (b), W) >= 0) ? (a) : (b))
#define pa_big_less(a, b) (big_signed_cmpn((a), (b), W) < 0)

RND_INLINE void pa_big_to_big2(pa_big2_coord_t dst, pa_big_coord_t src)
{
	int s, d;

	for(d = 0; d < W/2; d++)
		dst[d] = 0;
	for(s = 0; s < W; s++,d++)
		dst[d] = src[s];

	memset(&dst[d], big_is_neg(src, W) ? 0xff : 0x00, sizeof(dst[0]) * W/2);
}

RND_INLINE void pa_big_to_from2(pa_big_coord_t dst, pa_big2_coord_t src)
{
	memcpy(dst, &src[W/2], sizeof(dst[0]) * W);
}

int rnd_big_coord_isc(pa_big_vector_t res[2], pa_big_vector_t p1, pa_big_vector_t p2, pa_big_vector_t q1, pa_big_vector_t q2)
{
	pa_big_coord_t dx1, dy1, dx3, dy3;
	pa_big2_coord_t tmp1, tmp2, a, b, denom, r, d2x1, d2y1, d2x3, d2y3, rtmp;
	pa_big3_coord_t TMP3, A, B;

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
	big_mul(a, W2, dx1, dy3, W);
	big_mul(b, W2, dy1, dx3, W);
	big_subn(denom, a, b, W2, 0);

	if (big_is_zero(denom, W2))
		return rnd_big_coord_isc_par(res, p1, p2, q1, q2);

	pa_big_to_big2(d2x1, dx1);
	pa_big_to_big2(d2y1, dy1);
	pa_big_to_big2(d2x3, dx3);
	pa_big_to_big2(d2y3, dy3);

	/* tmp1 = x1*y2 - y1*x2 */
	big_mul(a, W2, X1, Y2, W);
	big_mul(b, W2, Y1, X2, W);
	big_subn(tmp1, a, b, W2, 0);

	/* tmp2 = x3*y4 - y3*x4 */
	big_mul(a, W2, X3, Y4, W);
	big_mul(b, W2, Y3, X4, W);
	big_subn(tmp2, a, b, W2, 0);

	/* Px = (tmp1 * dx3 - tmp2 * dx1)  /  denom */
	big_mul(A, W3, tmp1, d2x3, W2);
	big_mul(B, W3, tmp2, d2x1, W2);
	big_subn(TMP3, A, B, W3, 0);
	big_signed_div(rtmp, r, TMP3, W3, denom, W2, NULL);
	pa_big_to_from2(res[0].x, rtmp);
	if (!pa_big_in_between(pa_big_less(X1, X2), X1, X2, res[0].x)) return 0;
	if (!pa_big_in_between(pa_big_less(X3, X4), X3, X4, res[0].x)) return 0;

	/* Py = (tmp1 * dy3 - tmp2 * dy1) / denom */
	big_mul(A, W3, tmp1, d2y3, W2);
	big_mul(B, W3, tmp2, d2y1, W2);
	big_subn(TMP3, A, B, W3, 0);
	big_signed_div(rtmp, r, TMP3, W3, denom, W2, NULL);
	pa_big_to_from2(res[0].y, rtmp);
	if (!pa_big_in_between(pa_big_less(Y1, Y2), Y1, Y2, res[0].y)) return 0;
	if (!pa_big_in_between(pa_big_less(Y3, Y4), Y3, Y4, res[0].y)) return 0;
	return 1;
}

int pa_big_inters2(rnd_vnode_t *v1a, rnd_vnode_t *v1b, rnd_vnode_t *v2a, rnd_vnode_t *v2b, pa_big_vector_t *isc1, pa_big_vector_t *isc2)
{
	pa_big_vector_t tmp[2], V1A, V1B, V2A, V2B;
	int res;

	pa_big_load(V1A.x, v1a->point[0]); pa_big_load(V1A.y, v1a->point[1]);
	pa_big_load(V1B.x, v1b->point[0]); pa_big_load(V1B.y, v1b->point[1]);
	pa_big_load(V2A.x, v2a->point[0]); pa_big_load(V2A.y, v2a->point[1]);
	pa_big_load(V2B.x, v2b->point[0]); pa_big_load(V2B.y, v2b->point[1]);

	res = rnd_big_coord_isc(tmp, V1A, V1B, V2A, V2B);
	if (res > 0) memcpy(isc1, &tmp[0], sizeof(pa_big_vector_t));
	if (res > 1) memcpy(isc2, &tmp[1], sizeof(pa_big_vector_t));

	return res;
}

#define pa_big_vect_equ(a, b) ((big_signed_cmpn((a).x, (b).x, W) == 0) && (big_signed_cmpn((a).y, (b).y, W) == 0))

/* Compares pt's point to vn and returns 1 if they match (0 otherwise). If
   vn has a cvc, use its high precision coords */
RND_INLINE int pa_big_vnode_vect_equ(rnd_vnode_t *vn, pa_big_vector_t pt)
{
	/* high prec if available */
	if (vn->cvclst_prev != NULL)
		return pa_big_vect_equ(pt, vn->cvclst_prev->isc);

	return (pa_big2small(pt.x) == vn->point[0]) && (pa_big2small(pt.y) == vn->point[1]);
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


	/* suppress warnings for unused inlines */
	(void)big_signed_tostr;
	(void)big_signed_fromstr;
	(void)big_sub1;
	(void)big_add1;
	(void)big_free;
	(void)big_init;
}


double pa_big_double(pa_big_coord_t crd)
{
	double n = (double)crd[W/2] + (double)crd[W/2+1] * BIG_DBL_MULT + (double)crd[W/2+2] * BIG_DBL_MULT * BIG_DBL_MULT;
	double d = (double)crd[0] / BIG_DBL_MULT + (double)crd[1] / (BIG_DBL_MULT * BIG_DBL_MULT);
	return n/d;
}

double pa_big_vnxd(rnd_vnode_t *vn)
{
	if (vn->cvclst_prev != NULL)
		return pa_big_double(vn->cvclst_prev->isc.x);
	return (double)vn->point[0];
}

double pa_big_vnyd(rnd_vnode_t *vn)
{
	if (vn->cvclst_prev != NULL)
		return pa_big_double(vn->cvclst_prev->isc.y);
	return (double)vn->point[1];
}
