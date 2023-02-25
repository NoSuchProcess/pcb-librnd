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

/* rnd_pline_t internal/low level */

/*
node_add
 (C) 1993 Klamer Schutte
 (C) 1997 Alexey Nikitin, Michael Leonov
 (C) 2006 harry eaton
*/
rnd_vnode_t *rnd_poly_node_add_single(rnd_vnode_t *dest, rnd_vector_t po)
{
	rnd_vnode_t *p;

	if (vect_equal(po, dest->point))
		return dest;
	if (vect_equal(po, dest->next->point))
		return dest->next;
	p = rnd_poly_node_create(po);
	if (p == NULL)
		return NULL;
	p->cvc_prev = p->cvc_next = NULL;
	p->Flags.plabel = PA_PTL_UNKNWN;
	return p;
}																/* node_add */

/*
node_add_point
 (C) 1993 Klamer Schutte
 (C) 1997 Alexey Nikitin, Michael Leonov

 return 1 if new node in b, 2 if new node in a and 3 if new node in both
*/

static rnd_vnode_t *node_add_single_point(rnd_vnode_t * a, rnd_vector_t p)
{
	rnd_vnode_t *next_a, *new_node;

	next_a = a->next;

	new_node = rnd_poly_node_add_single(a, p);
	assert(new_node != NULL);

	new_node->cvc_prev = new_node->cvc_next = (rnd_cvc_list_t *) - 1;

	if (new_node == a || new_node == next_a)
		return NULL;

	return new_node;
}																/* node_add_point */


static inline int cntrbox_inside(rnd_pline_t * c1, rnd_pline_t * c2)
{
	assert(c1 != NULL && c2 != NULL);
	return ((c1->xmin >= c2->xmin) && (c1->ymin >= c2->ymin) && (c1->xmax <= c2->xmax) && (c1->ymax <= c2->ymax));
}

static inline void cntrbox_adjust(rnd_pline_t * c, const rnd_vector_t p)
{
	c->xmin = min(c->xmin, p[0]);
	c->xmax = max(c->xmax, p[0] + 1);
	c->ymin = min(c->ymin, p[1]);
	c->ymax = max(c->ymax, p[1] + 1);
}
