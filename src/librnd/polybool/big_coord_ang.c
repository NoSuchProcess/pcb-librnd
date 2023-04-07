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

int pa_angle_equ(pa_big_angle_t a, pa_big_angle_t b)
{
	return big_signed_cmpn(a, b, W) == 0;
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

int pa_angle_valid(pa_big_angle_t a)
{
	static const pa_big_angle_t a4 = {0, 0, 0, 4, 0, 0};
	return !big_is_neg(a, W) && (big_signed_cmpn(a, (big_word *)a4, W) <= 0);
}


void pa_big_calc_angle(pa_conn_desc_t *cd, rnd_vnode_t *pt, char poly, char side, rnd_vector_t v)
{
	pa_big_vector_t D, PT, OTHER;
	pa_big_coord_t dxdy, tmp;
	pa_big2_coord_t dy2, ang2, ang2tmp, *ang2res;
	int xneg = 0, yneg = 0;
	static const pa_big_angle_t a2 = {0, 0, 0, 2, 0, 0};
	static const pa_big_angle_t a4 = {0, 0, 0, 4, 0, 0};

	pa_big_load_cvc(&PT, pt);

	if (side == 'P') /* previous */
		pa_big_load_cvc(&OTHER, pt->prev);
	else /* next */
		pa_big_load_cvc(&OTHER, pt->next);

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

	big_zero(ang2, W2);
	big_signed_div(ang2, tmp, dy2, W2, dxdy, W, NULL);

	/* now move to the actual quadrant */
	ang2res = &ang2tmp;
	if (xneg && !yneg)         big_subn(ang2tmp, (big_word *)a2, ang2, W, 0);  /* 2nd quadrant */
	else if (xneg && yneg)     big_addn(ang2tmp, (big_word *)a2, ang2, W, 0);  /* 3rd quadrant */
	else if (!xneg && yneg)    big_subn(ang2tmp, (big_word *)a4, ang2, W, 0);  /* 4th quadrant */
	else                       ang2res = &ang2;                                /* 1st quadrant */

	memcpy(cd->angle, ang2res, sizeof(cd->angle)); /* truncate */

	assert(pa_angle_valid(cd->angle));

#if DEBUG_ANGLE
	DEBUG_ANGLE("point on %c at %$mD assigned angle ", poly, pt->point[0], pt->point[1]);
	pa_debug_print_angle(cd->angle);
	DEBUG_ANGLE(" on side %c\n", side);
#endif
}
