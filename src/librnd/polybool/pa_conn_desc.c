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

/* Connection (intersection) descriptors making up a connectivity list.
   Low level helper functions. (It was "cvc" in the original code.) */

/*
new_descriptor
  (C) 2006 harry eaton
*/
static pa_conn_desc_t *new_descriptor(rnd_vnode_t * a, char poly, char side)
{
	pa_conn_desc_t *l = (pa_conn_desc_t *) malloc(sizeof(pa_conn_desc_t));
	rnd_vector_t v;
	register double ang, dx, dy;

	if (!l)
		return NULL;
	l->head = NULL;
	l->parent = a;
	l->poly = poly;
	l->side = side;
	l->next = l->prev = l;
	if (side == 'P')							/* previous */
		vect_sub(v, a->prev->point, a->point);
	else													/* next */
		vect_sub(v, a->next->point, a->point);
	/* Uses slope/(slope+1) in quadrant 1 as a proxy for the angle.
	 * It still has the same monotonic sort result
	 * and is far less expensive to compute than the real angle.
	 */
	if (vect_equal(v, rnd_vect_zero)) {
		if (side == 'P') {
			if (a->prev->cvc_prev == (pa_conn_desc_t *) - 1)
				a->prev->cvc_prev = a->prev->cvc_next = NULL;
			rnd_poly_vertex_exclude(NULL, a->prev);
			vect_sub(v, a->prev->point, a->point);
		}
		else {
			if (a->next->cvc_prev == (pa_conn_desc_t *) - 1)
				a->next->cvc_prev = a->next->cvc_next = NULL;
			rnd_poly_vertex_exclude(NULL, a->next);
			vect_sub(v, a->next->point, a->point);
		}
	}
	assert(!vect_equal(v, rnd_vect_zero));
	dx = fabs((double) v[0]);
	dy = fabs((double) v[1]);
	ang = dy / (dy + dx);
	/* now move to the actual quadrant */
	if (v[0] < 0 && v[1] >= 0)
		ang = 2.0 - ang;						/* 2nd quadrant */
	else if (v[0] < 0 && v[1] < 0)
		ang += 2.0;									/* 3rd quadrant */
	else if (v[0] >= 0 && v[1] < 0)
		ang = 4.0 - ang;						/* 4th quadrant */
	l->angle = ang;
	assert(ang >= 0.0 && ang <= 4.0);
#ifdef DEBUG_ANGLE
	DEBUGP("node on %c at %#mD assigned angle %g on side %c\n", poly, a->point[0], a->point[1], ang, side);
#endif
	return l;
}

/*
insert_descriptor
  (C) 2006 harry eaton

   argument a is a cross-vertex node.
   argument poly is the polygon it comes from ('A' or 'B')
   argument side is the side this descriptor goes on ('P' for previous
   'N' for next.
   argument start is the head of the list of cvclists
*/
static pa_conn_desc_t *insert_descriptor(rnd_vnode_t * a, char poly, char side, pa_conn_desc_t * start)
{
	pa_conn_desc_t *l, *newone, *big, *small;

	if (!(newone = new_descriptor(a, poly, side)))
		return NULL;
	/* search for the pa_conn_desc_t for this point */
	if (!start) {
		start = newone;							/* return is also new, so we know where start is */
		start->head = newone;				/* circular list */
		return newone;
	}
	else {
		l = start;
		do {
			assert(l->head);
			if (l->parent->point[0] == a->point[0]
					&& l->parent->point[1] == a->point[1]) {	/* this pa_conn_desc_t is at our point */
				start = l;
				newone->head = l->head;
				break;
			}
			if (l->head->parent->point[0] == start->parent->point[0]
					&& l->head->parent->point[1] == start->parent->point[1]) {
				/* this seems to be a new point */
				/* link this cvclist to the list of all cvclists */
				for (; l->head != newone; l = l->next)
					l->head = newone;
				newone->head = start;
				return newone;
			}
			l = l->head;
		}
		while (1);
	}
	assert(start);
	l = big = small = start;
	do {
		if (l->next->angle < l->angle) {	/* find start/end of list */
			small = l->next;
			big = l;
		}
		else if (newone->angle >= l->angle && newone->angle <= l->next->angle) {
			/* insert new cvc if it lies between existing points */
			newone->prev = l;
			newone->next = l->next;
			l->next = l->next->prev = newone;
			return newone;
		}
	}
	while ((l = l->next) != start);
	/* didn't find it between points, it must go on an end */
	if (big->angle <= newone->angle) {
		newone->prev = big;
		newone->next = big->next;
		big->next = big->next->prev = newone;
		return newone;
	}
	assert(small->angle >= newone->angle);
	newone->next = small;
	newone->prev = small->prev;
	small->prev = small->prev->next = newone;
	return newone;
}

/*
 add_descriptors
 (C) 2006 harry eaton
*/
static pa_conn_desc_t *add_descriptors(rnd_pline_t * pl, char poly, pa_conn_desc_t * list)
{
	rnd_vnode_t *node = pl->head;

	do {
		if (node->cvc_prev) {
			assert(node->cvc_prev == (pa_conn_desc_t *) - 1 && node->cvc_next == (pa_conn_desc_t *) - 1);
			list = node->cvc_prev = insert_descriptor(node, poly, 'P', list);
			if (!node->cvc_prev)
				return NULL;
			list = node->cvc_next = insert_descriptor(node, poly, 'N', list);
			if (!node->cvc_next)
				return NULL;
		}
	}
	while ((node = node->next) != pl->head);
	return list;
}
