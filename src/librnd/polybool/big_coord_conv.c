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


/*

Numbers are stored in LSB so it's fraction.integer, or f.i, where both f an i
are expressed as number of rnd_ucoord_t words. We have one native coord type
and three static width bignums:

  i.f   width  usage
  ------------------------------------------------------------------
  1     -      normal resolution coord (input, output): rnd_coord_t
  3.3   W      stored high resolution coordinates
  6.6   W2     result of a W*W
  9.9   W3     result of a W*W*W (or more often a W2*W)
*/

void pa_big_load(pa_big_coord_t dst, rnd_coord_t src)
{
	memset(dst, 0, PA_BIGCOORD_SIZEOF);
	dst[W/2] = src;
	if (src < 0) /* sign extend */
		memset(&dst[W/2+1], 0xff, sizeof(dst[0]) * (W/2 - 1));
}

/* Convert a fixed point big_coord to a small one using floor() */
RND_INLINE rnd_coord_t pa_big2small(pa_big_coord_t src) { return src[W/2]; }

RND_INLINE void pa_big_to_big2(pa_big2_coord_t dst, pa_big_coord_t src)
{
	int s, d;

	for(d = 0; d < W/2; d++)
		dst[d] = 0;
	for(s = 0; s < W; s++,d++)
		dst[d] = src[s];

	memset(&dst[d], big_is_neg(src, W) ? 0xff : 0x00, sizeof(dst[0]) * W/2);
}

RND_INLINE void pa_big2_to_big3(pa_big3_coord_t dst, pa_big2_coord_t src)
{
	int s, d;

	for(d = 0; d < W/2; d++)
		dst[d] = 0;

	for(s = 0; s < W2; s++,d++)
		dst[d] = src[s];

	memset(&dst[d], big_is_neg(src, W2) ? 0xff : 0x00, sizeof(dst[0]) * W/2);
}

RND_INLINE void pa_big2_to_big(pa_big_coord_t dst, pa_big2_coord_t src)
{
	memcpy(dst, &src[W/2], sizeof(dst[0]) * W);
}

RND_INLINE double pa_big_double_(pa_big_coord_t crd)
{
	double n = (double)crd[W/2] + (double)crd[W/2+1] * BIG_DBL_MULT + (double)crd[W/2+2] * BIG_DBL_MULT * BIG_DBL_MULT;
	double d = (double)crd[2] / BIG_DBL_MULT + (double)crd[1] / (BIG_DBL_MULT * BIG_DBL_MULT) + (double)crd[0] / (BIG_DBL_MULT * BIG_DBL_MULT * BIG_DBL_MULT);
	return n + d;
}

double pa_big_double(pa_big_coord_t crd)
{
	if (big_is_neg(crd, W)) {
		pa_big_coord_t tmp;
		big_neg(tmp, crd, W);
		return -pa_big_double_(tmp);
	}
	else
		return pa_big_double_(crd);
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

/* Load the coords of src into dst; if there is high resolution coord
   available because the src is an intersection, use the high res isc
   coords, else use the input integer coords */
RND_INLINE void pa_big_load_cvc(pa_big_vector_t dst, rnd_vnode_t *src)
{
	if (src->cvclst_next == NULL) {
		pa_big_load(dst.x, src->point[0]);
		pa_big_load(dst.y, src->point[1]);
	}
	else
		memcpy(&dst, &src->cvclst_next->isc, sizeof(pa_big_vector_t));
}
