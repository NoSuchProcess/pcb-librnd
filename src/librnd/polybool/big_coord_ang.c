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

#if DEBUG_ANGLE
#include <stdio.h>
#include <librnd/core/rnd_printf.h>
void pa_debug_print_angle(pa_big_angle_t a);
#endif


int pa_angle_equ(pa_big_angle_t a, pa_big_angle_t b)
{
	return big_signed_cmpn(a, b, W) == 0;
}

int pa_angle_gt(pa_big_angle_t a, pa_big_angle_t b)
{
	return big_signed_cmpn(a, b, W) > 0;
}

int pa_angle_lt(pa_big_angle_t a, pa_big_angle_t b)
{
	return big_signed_cmpn(a, b, W) < 0;
}

int pa_angle_gte(pa_big_angle_t a, pa_big_angle_t b)
{
	return big_signed_cmpn(a, b, W) >= 0;
}

int pa_angle_lte(pa_big_angle_t a, pa_big_angle_t b)
{
	return big_signed_cmpn(a, b, W) <= 0;
}

void pa_angle_sub(pa_big_angle_t res, pa_big_angle_t a, pa_big_angle_t b)
{
	big_subn(res, a, b, W, 0);
}


int pa_angle_valid(pa_big_angle_t a)
{
	static const pa_big_angle_t a4 = {0, 0, 0, 0, 4, 0};
	return !big_is_neg(a, W) && (big_signed_cmpn(a, (big_word *)a4, W) <= 0);
}

RND_INLINE void pa_big_calc_angle_(pa_big_angle_t *dst, pa_big_vector_t PT, pa_big_vector_t OTHER)
{
	pa_big_vector_t D;
	pa_big_coord_t dxdy, tmp;
	pa_big2_coord_t dy2, ang2, ang2tmp, *ang2res;
	pa_big3_coord_t ang3, dy3;
	int xneg = 0, yneg = 0, n;
	static const pa_big_angle_t a2 = {0, 0, 0, 0, 2, 0};
	static const pa_big_angle_t a4 = {0, 0, 0, 0, 4, 0};

	big_subn(D.x, OTHER.x, PT.x, W, 0);
	big_subn(D.y, OTHER.y, PT.y, W, 0);

	assert(!big_is_zero(D.x, W) || !big_is_zero(D.y, W));

	if (big_is_neg(D.x, W)) {
		big_neg(D.x, D.x, W);
		xneg = 1;
	}
	if (big_is_neg(D.y, W)) {
		big_neg(D.y, D.y, W);
		yneg = 1;
	}

	big_addn(dxdy, D.x, D.y, W, 0);

	pa_big_to_big2(dy2, D.y);


	/* Originally we did this here:
	
	     big_signed_div(ang2, tmp, dy2, W2, dxdy, W, NULL);
	
	   But we need more than 6 decimal words when two high-resolution (3.3)
	   lines participate. Especially is one of them is real short, the final
	   result in a W2 (6.6) is not wide enough and two very close but not
	   overlapping lines may end up having the same angle. What we really need
	   is 1.7 for the division and 1.4 for the final unique ang2 value, but we
	   don't have that.
	
	   Test case: gixedi
	
	   The trick is to do the division in 9.9 and then convert it back to 6.6
	   but with a shift up one word so we really have 2.4 stored in a 3.3.
	   This works because the actual value of the angle doesn't matter, it
	   just need to be different for different angles.
	
	   Example:
	    division result is: 1 2 3 4 5 6 7 8 9 . a b c d e f g h i
	    output ang2 is            3 4 5 6 7 8 . 9 a b c d e
	    Note: b = c = d = e = 0; the extra info we get by the shift is '3'
	*/
	pa_big2_to_big3(dy3, dy2);
	big_zero(ang3, W3);
	big_signed_div(ang3, tmp, dy3, W3, dxdy, W, NULL);
	for(n = 0; n < W2; n++)
		ang2[n] = ang3[n+(W/2-1)]; /* if W is 6, that's 3.3 and this shifts 2, discarding the 2 lowest decimals */

	/* now move to the actual quadrant */
	ang2res = &ang2tmp;
	if (xneg && !yneg)         big_subn(ang2tmp, (big_word *)a2, ang2, W, 0);  /* 2nd quadrant */
	else if (xneg && yneg)     big_addn(ang2tmp, (big_word *)a2, ang2, W, 0);  /* 3rd quadrant */
	else if (!xneg && yneg)    big_subn(ang2tmp, (big_word *)a4, ang2, W, 0);  /* 4th quadrant */
	else                       ang2res = &ang2;                                /* 1st quadrant */

	memcpy(dst, ang2res, sizeof(pa_big_angle_t)); /* truncate */

	assert(pa_angle_valid(*dst));
}

void pa_big_calc_angle_nn(pa_big_angle_t *dst, rnd_vnode_t *nfrom, rnd_vnode_t *nto)
{
	pa_big_vector_t NF, NT;
	pa_big_load_cvc(&NF, nfrom);
	pa_big_load_cvc(&NT, nto);


	pa_big_calc_angle_(dst, NF, NT);
}

void pa_big_calc_angle(pa_conn_desc_t *cd, rnd_vnode_t *pt, char poly, char side)
{
	pa_big_vector_t PT, OTHER;

#if DEBUG_ANGLE
	static int cnt;
	cnt++;
/*	rnd_fprintf(stderr, "ANG [%d] ---------------\n", cnt);*/
#endif
	pa_big_load_cvc(&PT, pt);


	if (side == 'P') /* previous */
		pa_big_load_cvc(&OTHER, pt->prev);
	else /* next */
		pa_big_load_cvc(&OTHER, pt->next);

	pa_big_calc_angle_(&cd->angle, PT, OTHER);

#if DEBUG_ANGLE
	rnd_fprintf(stderr, "Angle [%d]: %c %.19f;%.19f %.19f;%.19f assigned angle ", cnt, poly,
		pa_big_double(PT.x),pa_big_double(PT.y),
		pa_big_double(OTHER.x),pa_big_double(OTHER.y)
		);
	pa_debug_print_angle(cd->angle);
	rnd_fprintf(stderr, " on side %c\n", side);
#endif
}
