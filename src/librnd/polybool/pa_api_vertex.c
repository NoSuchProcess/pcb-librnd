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
 *
 *  This is a full rewrite of pcb-rnd's (and PCB's) polygon lib originally
 *  written by Harry Eaton in 2006, in turn building on "poly_Boolean: a
 *  polygon clip library" by Alexey Nikitin, Michael Leonov from 1997 and
 *  "nclip: a polygon clip library" Klamer Schutte from 1993.
 *
 *  English translation of the original paper the lib is largely based on:
 *  https://web.archive.org/web/20160418014630/http://www.complex-a5.ru/polyboolean/downloads/polybool_eng.pdf
 *
 */

void rnd_poly_vertex_exclude(rnd_pline_t *parent, rnd_vnode_t *node)
{
	assert(node != NULL);

	/* remove point from the parent pline  */
	if ((parent != NULL) && (parent->head == node)) {
		/* if node is excluded from a pline, it can not remain the head */
		parent->head = node->next;
	}
	node->prev->next = node->next;
	node->next->prev = node->prev;

	/* remove from connectivity list */
	if (node->cvc_next != NULL) {
		free(node->cvc_next);
		free(node->cvc_prev);
	}

	if ((parent != NULL) && (parent->head == node)) {
		/* when removing head which was the last node in pline */
		parent->head = NULL;
	}
}

/* Add node without side effects/optimization (add even if it's colinear with
   existing points) */
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

/* Returns whether node lies on the same line as its 2 previous nodes */
RND_INLINE int pa_vertices_are_coaxial(rnd_vnode_t *node)
{
	double dx, dy, a, b;

	if ((node->prev->prev == node) || (node->prev == node))
		return 0; /* less than 3 points in the polyline, can't be coaxial */

	dy = node->point[1] - node->prev->prev->point[1];
	dx = node->prev->point[0] - node->prev->prev->point[0];
	a = dx * dy;

	dx = node->point[0] - node->prev->prev->point[0];
	dy = node->prev->point[1] - node->prev->prev->point[1];
	b = dx * dy;

	return fabs(a - b) < EPSILON;
}

TODO("rename: it's a pline function");
void rnd_poly_vertex_include(rnd_vnode_t *after, rnd_vnode_t *node)
{
	rnd_poly_vertex_include_force_(after, node);

	/* If node is coaxial with the previous two, remove the previous node
	   (which is the middle node of 3 nodes on the same line) */
	if (pa_vertices_are_coaxial(node)) {
		rnd_vnode_t *t = node->prev;
		t->prev->next = node;
		node->prev = t->prev;
		free(t);
	}
}
