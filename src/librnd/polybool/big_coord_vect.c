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

/* Distance square between v1 and v2 in big coord (signed) */
void rnd_vect_m_dist2_big(pa_big2_coord_t dst, pa_big_vector_t v1, pa_big_vector_t v2)
{
	pa_big_coord_t dx, dy;
	pa_big2_coord_t a, b;
	int dxs, dys;

	big_subn(dx, v1.x, v2.x, W, 0);
	big_subn(dy, v1.y, v2.y, W, 0);

	big_signed_mul(a, W2, dx, dx, W);
	big_signed_mul(b, W2, dy, dy, W);

	big_addn(dst, a, b, W2, 0);

	dxs = big_sgn(dx, W);
	if (dxs > 0)    {                        return /*+dd*/; }
	if (dxs < 0)    { big_neg(dst, dst, W2); return /*-dd*/; }

	dys = big_sgn(dy, W);
	if (dys > 0)    {                        return /*+dd*/; }
	if (dys < 0)    { big_neg(dst, dst, W2); return /*-dd*/; }
}

/* Distance square between v1 and v2 in big coord *unsigned) */
void rnd_vect_u_dist2_big(pa_big2_coord_t dst, pa_big_vector_t v1, pa_big_vector_t v2)
{
	pa_big_coord_t dx, dy;
	pa_big2_coord_t a, b;

	big_subn(dx, v1.x, v2.x, W, 0);
	big_subn(dy, v1.y, v2.y, W, 0);

	big_signed_mul(a, W2, dx, dx, W);
	big_signed_mul(b, W2, dy, dy, W);

	big_addn(dst, a, b, W2, 0);
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

#define pa_big_vect_equ(a, b) ((big_signed_cmpn((a).x, (b).x, W) == 0) && (big_signed_cmpn((a).y, (b).y, W) == 0))

/* Returns if c is +-1 bit near zero */
RND_INLINE int pa_big_coord_near_zero(pa_big_coord_t c)
{
	static const pa_big_coord_t p1 = {1, 0, 0, 0, 0, 0};
	int is_zero = 1, is_p1 = 1, is_n1 = 1;
	int n;

	/* search c from LSB and exclude is_zero, is_p1 or is_n1 if the number
	   can not be 0, +1 (bit) or -1 (bit). A number is zero only if it's
	   all 0, -1 if it's all BIG_IMAX; p1 is the tricky one. */
	for(n = 0; n < W; n++) {
		if (is_zero && (c[n] != 0)) is_zero = 0;
		if (is_p1 && (c[n] != p1[n])) is_p1 = 0;
		if (is_n1 && (c[n] != BIG_UMAX)) is_n1 = 0;
		if (!is_zero && !is_p1 && !is_n1)
			return 0;
	}
	return 1;
}

/* Compares pt's point to vn and returns 1 if they match (0 otherwise). If
   vn has a cvc, use its high precision coords */
RND_INLINE int pa_big_vnode_vect_equ(rnd_vnode_t *vn, pa_big_vector_t pt)
{
	/* high prec if available */
	if (vn->cvclst_prev != NULL)
		return pa_big_vect_equ(pt, vn->cvclst_prev->isc);

	/* if pt is high precision but vn point is not, and pt is not an integer
	   coord then they must differ (in fractions) */
	if (!pa_big_vect_is_int(pt))
		return 0;

	return (pa_big_to_coord(pt.x) == vn->point[0]) && (pa_big_to_coord(pt.y) == vn->point[1]);
}

#define Vequ2(a,b)       (memcmp((a),   (b),   sizeof(rnd_vector_t)) == 0)
int pa_big_vnode_vnode_equ(rnd_vnode_t *vna, rnd_vnode_t *vnb)
{
	/* all high prec */
	if ((vna->cvclst_prev != NULL) && (vnb->cvclst_prev != NULL))
		return pa_big_vect_equ(vna->cvclst_prev->isc, vnb->cvclst_prev->isc);

	/* neither high prec */
	if ((vna->cvclst_prev == NULL) && (vnb->cvclst_prev == NULL))
		return Vequ2(vna->point, vnb->point);

	/* mixed */
	if (vna->cvclst_prev == NULL)
		return pa_big_vnode_vect_equ(vna, vnb->cvclst_prev->isc);

	assert(vnb->cvclst_prev == NULL);
	return pa_big_vnode_vect_equ(vnb, vna->cvclst_prev->isc);
}

int pa_big_vnode_vnode_cross_sgn(pa_big_vector_t sv, pa_big_vector_t sv_next, pa_big_vector_t pt)
{
	pa_big_vector_t v1, v2;
	pa_big2_coord_t m1, m2, cross;

	big_subn(v1.x, sv_next.x, sv.x, W, 0);
	big_subn(v1.y, sv_next.y, sv.y, W, 0);
	big_subn(v2.x, pt.x, sv.x, W, 0);
	big_subn(v2.y, pt.y, sv.y, W, 0);
	big_signed_mul(m1, W2, v1.x, v2.y, W);
	big_signed_mul(m2, W2, v2.x, v1.y, W);

	big_subn(cross, m1, m2, W2, 0);
	if (big_is_neg(cross, W2))
		return -1;
	if (big_is_zero(cross, W2))
		return 0;
	return +1;
}

int pa_big_coord_cmp(pa_big_coord_t a, pa_big_coord_t b)
{
	return big_signed_cmpn(a, b, W);
}

int pa_big2_coord_cmp(pa_big2_coord_t a, pa_big2_coord_t b)
{
	return big_signed_cmpn(a, b, W2);
}

int pa_small_big_xy_eq(rnd_coord_t smallx, rnd_coord_t smally, pa_big_coord_t bigx, pa_big_coord_t bigy)
{
	int n;

	/* if big is not round, they can not be equal */
	for(n = 0; n < W/2; n++)
		if ((bigx[n] != 0) || (bigy[n] != 0))
			return 0;

	/* if big is larger than a single word, they can not be equal */
	for(n = W/2+1; n < W; n++)
		if ((bigx[n] != 0) || (bigy[n] != 0))
			return 0;

	return (smallx == bigx[W/2]) && (smally == bigy[W/2]);
}


int pa_big_vmandist_small(pa_big_vector_t *a, rnd_vnode_t *vnode, int copy)
{
	pa_big_coord_t dx, dy;
	pa_big_vector_t b;

	rnd_pa_big_load_cvc(&b, vnode);

	big_subn(dx, a->x, b.x, W, 0);
	if (!pa_big_coord_near_zero(dx))
		return 0;

	big_subn(dy, a->y, b.y, W, 0);
	if (!pa_big_coord_near_zero(dy))
		return 0;


	if (copy)
		*a = b;

	return 1;
}
