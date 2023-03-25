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

#	if RND_COORD_MAX == ((1UL<<31)-1)
#		define BIG_BITS 32
#		define BIG_DECIMAL_DIGITS 9
#		define BIG_DECIMAL_BASE 1000000000UL
#		define BIG_NEG_BASE 0x80000000UL
#	elif RND_COORD_MAX == ((1ULL<<63)-1)
#		define BIG_BITS 64
#		define BIG_DECIMAL_DIGITS 19
#		define BIG_DECIMAL_BASE 10000000000000000000UL
#		define BIG_NEG_BASE 0x8000000000000000UL
#	else
#		error "unsupported system: rnd_coord has to be 32 or 64 bits wide (checked: RND_COORD_MAX)"
#	endif

#define RND_BIGCRD_WIDTH 3
typedef rnd_coord_t big_word;
typedef rnd_coord_t rnd_big_coord_t[RND_BIGCRD_WIDTH];

#include <genfip/big.h>


#define RATIONAL(x) rnd_bcr_ ## x
#define RATIONAL_INT rnd_big_coord_t

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

#include <genfip/rational.h>

/*** implementation ***/
