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


/*** configure bigint and rational numbers ***/

#define big_local      RND_INLINE
#define RATIONAL_API   RND_INLINE
#define RATIONAL_IMPL  RND_INLINE

#define FROM_ISC_RATIONAL_C

typedef rnd_coord_t big_word;


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
#define RATIONAL_OP_LESS(a, b) (big_cmpn((a), (b), RND_BIGCRD_WIDTH) < 0)
#define RATIONAL_OP_EQU(a, b)  (big_cmpn((a), (b), RND_BIGCRD_WIDTH) == 0)
#define RATIONAL_OP_GT0(a)     (big_sgn((a), RND_BIGCRD_WIDTH) > 0)
#define RATIONAL_OP_LT0(a)     (big_is_neg((a), RND_BIGCRD_WIDTH))
#define RATIONAL_OP_NEG(a)     (big_neg((a), (a), RND_BIGCRD_WIDTH))
#define RATIONAL_OP_SWAP(a, b) (big_swap((a), (b), RND_BIGCRD_WIDTH))
#define RATIONAL_OP_CPY(dst, src) big_copy(*(dst), *(src), RND_BIGCRD_WIDTH)

#include "isc_rational.h"

/*** implementation ***/

#define W RND_BIGCRD_WIDTH

int rnd_big_coord_isc(rnd_bcr_t *x, rnd_bcr_t *y, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t x3, rnd_coord_t y3, rnd_coord_t x4, rnd_coord_t y4)
{
	rnd_big_coord_t tmp1, tmp2, dx1, dy1, dx3, dy3, a, b;
	rnd_big_coord_t X1 = {0}, Y1 = {0}, X2 = {0}, Y2 = {0};
	rnd_big_coord_t X3 = {0}, Y3 = {0}, X4 = {0}, Y4 = {0};

	X1[0] = x1; Y1[0] = y1; X2[0] = x2; Y2[0] = y2;
	X3[0] = x3; Y3[0] = y3; X4[0] = x4; Y4[0] = y4;

/* https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection as of 2023 March */

	big_subn(dx1, X1, X2, W, 0);
	big_subn(dy1, Y1, Y2, W, 0);
	big_subn(dx3, X3, X4, W, 0);
	big_subn(dy3, Y3, Y4, W, 0);

	/* tmp1 = x1*y2 - y1*x2 */
	big_mul(a, W, X1, Y2, W);
	big_mul(b, W, Y1, X2, W);
	big_subn(tmp1, a, b, W, 0);

	/* tmp2 = x3*y4 - y3*x4 */
	big_mul(a, W, X3, Y4, W);
	big_mul(b, W, Y3, X4, W);
	big_subn(tmp2, a, b, W, 0);

	/* denom = dx1 * dy3 - dy1 * dx3 */
	big_mul(a, W, dx1, dy3, W);
	big_mul(b, W, dy1, dx3, W);
	big_subn(x->denom, a, b, W, 0);
	big_copy(y->denom, x->denom, W);

	if (big_is_zero(x->denom, W))
		return -1;

	/* Px = (tmp1 * dx3 - tmp2 * dx1)  /  denom */
	big_mul(a, W, tmp1, dx3, W);
	big_mul(b, W, tmp2, dx1, W);
	big_subn(x->num, a, b, W, 0);

	/* Py = (tmp1 * dy3 - tmp2 * dy1) / denom */
	big_mul(a, W, tmp1, dx3, W);
	big_mul(b, W, tmp2, dx1, W);
	big_subn(y->num, a, b, W, 0);

	return 0;
}
