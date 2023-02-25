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

#define rnd_vertex_equ(a,b) (memcmp((a),(b),sizeof(rnd_vector_t))==0)
#define rnd_vertex_cpy(a,b) memcpy((a),(b),sizeof(rnd_vector_t))

void rnd_poly_vertex_exclude(rnd_pline_t *parent, rnd_vnode_t * node)
{
	assert(node != NULL);
	if (parent != NULL) {
		if (parent->head == node) /* if node is excluded from a pline, it can not remain the head */
			parent->head = node->next;
	}
	if (node->cvc_next) {
		free(node->cvc_next);
		free(node->cvc_prev);
	}
	node->prev->next = node->next;
	node->next->prev = node->prev;

	if (parent != NULL) {
		if (parent->head == node) /* special case: removing head which was the last node in pline  */
			parent->head = NULL;
	}
}

RND_INLINE void rnd_poly_vertex_include_force_(rnd_vnode_t *after, rnd_vnode_t *node)
{
	assert(after != NULL);
	assert(node != NULL);

	node->prev = after;
	node->next = after->next;
	after->next = after->next->prev = node;
}

void rnd_poly_vertex_include_force(rnd_vnode_t *after, rnd_vnode_t *node)
{
	rnd_poly_vertex_include_force_(after, node);
}

void rnd_poly_vertex_include(rnd_vnode_t *after, rnd_vnode_t *node)
{
	double a, b;

	rnd_poly_vertex_include_force_(after, node);

	/* remove points on same line */
	if (node->prev->prev == node)
		return;											/* we don't have 3 points in the poly yet */
	a = (node->point[1] - node->prev->prev->point[1]);
	a *= (node->prev->point[0] - node->prev->prev->point[0]);
	b = (node->point[0] - node->prev->prev->point[0]);
	b *= (node->prev->point[1] - node->prev->prev->point[1]);
	if (fabs(a - b) < EPSILON) {
		rnd_vnode_t *t = node->prev;
		t->prev->next = node;
		node->prev = t->prev;
		free(t);
	}
}
