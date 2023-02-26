/*
       Copyright (C) 2006 harry eaton

   based on:
       poly_Boolean: a polygon clip library
       Copyright (C) 1997  Alexey Nikitin, Michael Leonov
       (also the authors of the paper describing the actual algorithm)
       leonov@propro.iis.nsk.su

   in turn based on:
       nclip: a polygon clip library
       Copyright (C) 1993  Klamer Schutte
 
       This program is free software; you can redistribute it and/or
       modify it under the terms of the GNU General Public
       License as published by the Free Software Foundation; either
       version 2 of the License, or (at your option) any later version.
 
       This program is distributed in the hope that it will be useful,
       but WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
       General Public License for more details.
 
       You should have received a copy of the GNU General Public
       License along with this program; if not, write to the Free
       Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 
      polygon1.c
      (C) 1997 Alexey Nikitin, Michael Leonov
      (C) 1993 Klamer Schutte

      all cases where original (Klamer Schutte) code is present
      are marked
*/

/* rnd_pline_t low level and internal */

/* Allocate a new node for a given location. Returns NULL if allocationf fails. */
rnd_vnode_t *rnd_poly_node_create(rnd_vector_t v)
{
	rnd_vnode_t *res = calloc(sizeof(rnd_vnode_t), 1);

	assert(v != NULL);
	if (res == NULL)
		return NULL;

	Vcpy2(res->point, v);
	return res;
}

/* Return node for a point next to dst. Returns existing node on coord
   match else allocates a new node. Returns NULL if new node can't be
   allocated. */
rnd_vnode_t *rnd_poly_node_add_single(rnd_vnode_t *dst, rnd_vector_t ptv)
{
	rnd_vnode_t *newnd;

  /* no new allocation for redundant node around dst */
	if (Vequ2(ptv, dst->point))        return dst;
	if (Vequ2(ptv, dst->next->point))  return dst->next;

	/* have to allocate a new node */
	newnd = rnd_poly_node_create(ptv);
	if (newnd != NULL)
		newnd->flg.plabel = PA_PTL_UNKNWN;

	return newnd;
}

/* Return a new node for a point next to dst, or NULL if pt falls on an existing
   node. Crashes/asserts if allocation fails.
   Side effect: cvclst of the node at pt is reset, even for existing nodes. */
static rnd_vnode_t *pa_ensure_point_and_reset_cvc(rnd_vnode_t *dst, rnd_vector_t pt)
{
	rnd_vnode_t *dnext = dst->next, *newnd;

	newnd = rnd_poly_node_add_single(dst, pt);
	assert(newnd != NULL);

	newnd->cvclst_prev = newnd->cvclst_next = PA_CONN_DESC_INVALID;

	if ((newnd == dst) || (newnd == dnext))
		return NULL; /* no new allocation */

	return newnd;
}


static inline int cntrbox_inside(rnd_pline_t * c1, rnd_pline_t * c2)
{
	assert(c1 != NULL && c2 != NULL);
	return ((c1->xmin >= c2->xmin) && (c1->ymin >= c2->ymin) && (c1->xmax <= c2->xmax) && (c1->ymax <= c2->ymax));
}

static inline void cntrbox_adjust(rnd_pline_t * c, const rnd_vector_t p)
{
	c->xmin = pa_min(c->xmin, p[0]);
	c->xmax = pa_max(c->xmax, p[0] + 1);
	c->ymin = pa_min(c->ymin, p[1]);
	c->ymax = pa_max(c->ymax, p[1] + 1);
}
