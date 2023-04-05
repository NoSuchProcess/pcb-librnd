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
   fixed point numbers with widths chosen so that they never overflow and do
   not lose precision (assigns different coords to different intersection
   points even if they are very close) */

#include <librnd/config.h>

#include "pa_math.c"

/*** configure fixed point numbers ***/


typedef rnd_ucoord_t big_word;

#define PA_BIGCOORD_ISC
#include "pa.h"

#include <genfip/big.h>


/*** implementation ***/

#define W PA_BIGCRD_WIDTH
#define W2 (PA_BIGCRD_WIDTH*2)
#define W3 (PA_BIGCRD_WIDTH*3)

#include "big_coord_conv.c"
#include "big_coord_vect.c"
#include "big_coord_isc.c"
#include "big_coord_ang.c"


/* suppress warnings for unused inlines; do not call this function, it's a nop */
void pa_big_coord_warning_suppressor(void)
{
	(void)big_signed_tostr;
	(void)big_signed_fromstr;
	(void)big_sub1;
	(void)big_add1;
	(void)big_free;
	(void)big_init;
}
