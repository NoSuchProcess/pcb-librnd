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

/*** configure bigint and rational numbers ***/

#define big_local      RND_INLINE
#define RATIONAL_API   RND_INLINE
#define RATIONAL_IMPL  RND_INLINE

#define FROM_ISC_RATIONAL_C

typedef rnd_ucoord_t big_word;


#define RATIONAL_OP_ADD(r,a,b) big_addn((r), (a), (b), RND_BIGCRD_WIDTH, 0)
#define RATIONAL_OP_SUB(r,a,b) big_subn((r), (a), (b), RND_BIGCRD_WIDTH, 0)
#define RATIONAL_OP_MUL(r,a,b) big_mul((r), RND_BIGCRD_WIDTH, (a), (b), RND_BIGCRD_WIDTH)
#define RATIONAL_OP_DIV(r,a,b) \
do { \
	big_word __d__[RND_BIGCRD_WIDTH], __tmp__[RND_BIGCRD_WIDTH]; \
	big_zero((r), RND_BIGCRD_WIDTH); \
	big_zero(__d__, RND_BIGCRD_WIDTH); \
	big_div((r), __d__, (a), RND_BIGCRD_WIDTH, (b), RND_BIGCRD_WIDTH, __tmp__); \
} while(0)

/* returns 0 if a is 0, +1 if a is positive and -1 if a is negative */
#define RATIONAL_OP_SGN(a)     (big_sgn((a), RND_BIGCRD_WIDTH))
#define RATIONAL_OP_LESS(a, b) (big_signed_cmpn((a), (b), RND_BIGCRD_WIDTH) < 0)
#define RATIONAL_OP_EQU(a, b)  (big_cmpn((a), (b), RND_BIGCRD_WIDTH) == 0)
#define RATIONAL_OP_GT0(a)     (big_sgn((a), RND_BIGCRD_WIDTH) > 0)
#define RATIONAL_OP_LT0(a)     (big_is_neg((a), RND_BIGCRD_WIDTH))
#define RATIONAL_OP_NEG(a)     (big_neg((a), (a), RND_BIGCRD_WIDTH))
#define RATIONAL_OP_SWAP(a, b) (big_swap((a), (b), RND_BIGCRD_WIDTH))
#define RATIONAL_OP_CPY(dst, src) big_copy(*(dst), *(src), RND_BIGCRD_WIDTH)

#include "isc_rational.h"

/*** implementation ***/

#define W RND_BIGCRD_WIDTH
#define W2 RND_BIGCRD_WIDTH2

#define Vcpy2(dst, src)   memcpy((dst), (src), sizeof(rnd_vector_t))
#define Vsub2(r, a, b) \
	do { \
		(r)[0] = (a)[0] - (b)[0]; \
		(r)[1] = (a)[1] - (b)[1]; \
	} while(0)

RND_INLINE void load_big(rnd_big_coord_t dst, rnd_coord_t src)
{
	int n;

	dst[0] = src;
	if (src > 0) {
		for(n = 1; n < W; n++)
			dst[n] = 0;
	}
	else {
		for(n = 1; n < W; n++)
			dst[n] = -1;
	}
}

/* Distance square between v1 and v2 in big coord */
RND_INLINE void rnd_vect_m_dist2_big(rnd_big_coord_t dst, rnd_vector_t v1, rnd_vector_t v2)
{
	rnd_big_coord_t dx = {0}, dy = {0}, a, b;
	rnd_coord_t vdx = v1[0] - v2[0];
	rnd_coord_t vdy = v1[1] - v2[1];

	load_big(dx, vdx);
	load_big(dy, vdy);
	big_mul(a, W, dx, dx, W);
	big_mul(b, W, dy, dy, W);

	big_addn(dst, a, b, W, 0);

	if (vdx > 0)    {                       return /*+dd*/; }
	if (vdx < 0)    { big_neg(dst, dst, W); return /*-dd*/; }
	if (vdy > 0)    {                       return /*+dd*/; }
	/* (vdy < 0) */ { big_neg(dst, dst, W); return /*-dd*/; }
}

/* Corner case handler: when p1..p2 and q1..q2 are parallel */
RND_INLINE int rnd_big_coord_isc_par(rnd_bcr_t x[2], rnd_bcr_t y[2], rnd_vector_t p1, rnd_vector_t p2, rnd_vector_t q1, rnd_vector_t q2)
{
	rnd_big_coord_t dc1, dc2, d1, d2;
	rnd_vector_t tmp1, tmp2, tmq1, tmq2;

	/* to easy conversion of coords to big coords - results are always on input coords */
	memset(x, 0, sizeof(rnd_bcr_t) * 2);
	memset(y, 0, sizeof(rnd_bcr_t) * 2);
	x[0].denom[0] = 1; x[1].denom[0] = 1;
	y[0].denom[0] = 1; y[1].denom[0] = 1;

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
	rnd_vect_m_dist2_big(dc2, p1, q2);

	rnd_vect_m_dist2_big(d1, p1, q1);
	rnd_vect_m_dist2_big(d2, p1, q2);

	/* Make sure dc1 is always the smaller one (always on the left);
	   depends on p1..p2 direction */
	if (big_signed_cmpn(dc1, dc2, W) > 0) { /* dc1 > dc2 */
		Vcpy2(tmp1, p2);
		Vcpy2(tmp2, p1);
		big_swap(dc1, dc2, W);
	}
	else {
		Vcpy2(tmp1, p1);
		Vcpy2(tmp2, p2);
	}

	/* Make sure d1 is always the smaller one (always on the left);
	   depends on q1..q2 direction */
	if (big_signed_cmpn(d1, d2, W) > 0) { /* d1 > d2 */
		Vcpy2(tmq1, q2);
		Vcpy2(tmq2, q1);
		big_swap(d1, d2, W);
	}
	else {
		Vcpy2(tmq1, q1);
		Vcpy2(tmq2, q2);
	}

	/* by now tmp* and tmq* are ordered p1..p2 and q1..q2 */

	/* Compare distances to figure what overlaps */
	if (big_signed_cmpn(dc1, d1, W) < 0) { /* (dc1 < d1) */
		if (big_signed_cmpn(dc2, d1, W) < 0) /* (dc2 < d1) */
			return 0;
		if (big_signed_cmpn(dc2, d2, W) < 0) { /* (dc2 < d2) */
			load_big(x[0].num, tmp2[0]); load_big(y[0].num, tmp2[1]);
			load_big(x[1].num, tmq1[0]); load_big(y[1].num, tmq1[1]);
		}
		else {
			load_big(x[0].num, tmq1[0]); load_big(y[0].num, tmq1[1]);
			load_big(x[1].num, tmq2[0]); load_big(y[1].num, tmq2[1]);
		}
	}
	else {
		if (big_signed_cmpn(dc1, d2, W) > 0) /* (dc1 > d2) */
			return 0;
		if (big_signed_cmpn(dc2, d2, W) < 0) { /* (dc2 < d2) */
			load_big(x[0].num, tmp1[0]); load_big(y[0].num, tmp1[1]);
			load_big(x[1].num, tmp2[0]); load_big(y[1].num, tmp2[1]);
		}
		else {
			load_big(x[0].num, tmp1[0]); load_big(y[0].num, tmp1[1]);
			load_big(x[1].num, tmq2[0]); load_big(y[1].num, tmq2[1]);
		}
	}

	/* if the two intersections are the same, return only one; denominators are
	   always 1, do not compare them */
	if ((big_signed_cmpn(x[0].num, x[1].num, W) == 0) && (big_cmpn(y[0].num, y[1].num, W) == 0))
		return 1;

	return 2;
}

void pa_div_to_big2(rnd_big2_coord_t dst, rnd_big_coord_t n, rnd_big_coord_t d)
{
	rnd_big2_coord_t N, D;
	rnd_big_coord_t r;

	memset(((rnd_big_coord_t *)N)+0, (big_is_neg(n, W) ? 0xff : 0x00), sizeof(rnd_big_coord_t));
	memset(((rnd_big_coord_t *)D)+1, (big_is_neg(d, W) ? 0xff : 0x00), sizeof(rnd_big_coord_t));
	memcpy(((rnd_big_coord_t *)N)+1, n, sizeof(rnd_big_coord_t));
	memcpy(D, d, sizeof(rnd_big_coord_t));

	big_zero(dst, W2);
	big_signed_div(dst, r, N, W2, D, W2, NULL);
}

/* Returns if num is between a*denom and b*denom */
RND_INLINE int pa_big_in_between(int ordered, rnd_big_coord_t a, rnd_big_coord_t b, rnd_big_coord_t num, rnd_big_coord_t denom)
{
	rnd_big_coord_t A, B;
	if (ordered) {
		big_mul(A, W, a, denom, W);
		big_mul(B, W, b, denom, W);
	}
	else {
		/* swap so A < B */
		big_mul(B, W, a, denom, W);
		big_mul(A, W, b, denom, W);
	}

	if (big_signed_cmpn(num, A, W) < 0) return 0;
	if (big_signed_cmpn(num, B, W) > 0) return 0;
	return 1;
}

int rnd_big_coord_isc(rnd_bcr_t x[2], rnd_bcr_t y[2], rnd_vector_t p1, rnd_vector_t p2, rnd_vector_t q1, rnd_vector_t q2)
{
	rnd_coord_t x1 = p1[0], y1 = p1[1], x2 = p2[0], y2 = p2[1];
	rnd_coord_t x3 = q1[0], y3 = q1[1], x4 = q2[0], y4 = q2[1];
	rnd_big_coord_t tmp1, tmp2, dx1, dy1, dx3, dy3, a, b;
	rnd_big_coord_t X1, Y1, X2, Y2, X3, Y3, X4, Y4;

	/* if bounding boxes don't overlap, no need to check, they can't intersect */
	if (pa_max(p1[0], p2[0]) < pa_min(q1[0], q2[0])) return 0;
	if (pa_max(q1[0], q2[0]) < pa_min(p1[0], p2[0])) return 0;
	if (pa_max(p1[1], p2[1]) < pa_min(q1[1], q2[1])) return 0;
	if (pa_max(q1[1], q2[1]) < pa_min(p1[1], p2[1])) return 0;


	load_big(X1, x1); load_big(Y1, y1);
	load_big(X2, x2); load_big(Y2, y2);
	load_big(X3, x3); load_big(Y3, y3);
	load_big(X4, x4); load_big(Y4, y4);

/* https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection as of 2023 March */

	big_subn(dx1, X1, X2, W, 0);
	big_subn(dy1, Y1, Y2, W, 0);
	big_subn(dx3, X3, X4, W, 0);
	big_subn(dy3, Y3, Y4, W, 0);

	/* denom = dx1 * dy3 - dy1 * dx3 */
	big_mul(a, W, dx1, dy3, W);
	big_mul(b, W, dy1, dx3, W);
	big_subn(x->denom, a, b, W, 0);
	big_copy(y->denom, x->denom, W);

	if (big_is_zero(x->denom, W))
		return rnd_big_coord_isc_par(x, y, p1, p2, q1, q2);

	/* tmp1 = x1*y2 - y1*x2 */
	big_mul(a, W, X1, Y2, W);
	big_mul(b, W, Y1, X2, W);
	big_subn(tmp1, a, b, W, 0);

	/* tmp2 = x3*y4 - y3*x4 */
	big_mul(a, W, X3, Y4, W);
	big_mul(b, W, Y3, X4, W);
	big_subn(tmp2, a, b, W, 0);

	/* Px = (tmp1 * dx3 - tmp2 * dx1)  /  denom */
	big_mul(a, W, tmp1, dx3, W);
	big_mul(b, W, tmp2, dx1, W);
	big_subn(x->num, a, b, W, 0);
	if (!pa_big_in_between(x1 < x2, X1, X2, x->num, x->denom)) return 0;
	if (!pa_big_in_between(x3 < x4, X3, X4, x->num, x->denom)) return 0;

	/* Py = (tmp1 * dy3 - tmp2 * dy1) / denom */
	big_mul(a, W, tmp1, dy3, W);
	big_mul(b, W, tmp2, dy1, W);
	big_subn(y->num, a, b, W, 0);
	if (!pa_big_in_between(y1 < y2, Y1, Y2, y->num, y->denom)) return 0;
	if (!pa_big_in_between(y3 < y4, Y3, Y4, y->num, y->denom)) return 0;

	return 1;
}
