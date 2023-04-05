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
#if 0
	pa_big_angle_t ang, dx, dy;

figure high prec points
	if (side == 'P') /* previous */
		Vsub2(v, pt->prev->point, pt->point);
	else /* next */
		Vsub2(v, pt->next->point, pt->point);

	assert(!Vequ2(v, rnd_vect_zero));

	dx = fabs((double)v[0]);
	dy = fabs((double)v[1]);
	ang = dy / (dy + dx);

	/* now move to the actual quadrant */
	if ((v[0] < 0) && (v[1] >= 0))         cd->angle = 2.0 - ang; /* 2nd quadrant */
	else if ((v[0] < 0) && (v[1] < 0))     cd->angle = 2.0 + ang; /* 3rd quadrant */
	else if ((v[0] >= 0) && (v[1] < 0))    cd->angle = 4.0 - ang; /* 4th quadrant */
	else                                   cd->angle = ang;       /* 1st quadrant */

	assert((ang >= 0.0) && (ang <= 4.0));

	DEBUG_ANGLE("point on %c at %$mD assigned angle %.08f on side %c\n", poly, pt->point[0], pt->point[1], ang, side);
#endif
}
