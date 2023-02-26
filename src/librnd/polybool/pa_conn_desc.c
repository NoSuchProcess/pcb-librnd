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

static pa_conn_desc_t *pa_new_conn_desc(rnd_vnode_t *pt, char poly, char side)
{
	rnd_vector_t v;
	double ang, dx, dy;
	pa_conn_desc_t *cd;

	cd = malloc(sizeof(pa_conn_desc_t));
	if (cd == NULL)
		return NULL;

	cd->head = NULL;
	cd->parent = pt;
	cd->poly = poly;
	cd->side = side;
	cd->next = cd->prev = cd;

	if (side == 'P') /* previous */
		Vsub2(v, pt->prev->point, pt->point);
	else /* next */
		Vsub2(v, pt->next->point, pt->point);


	/* Uses slope/(slope+1) in quadrant 1 as a proxy for the angle. It has the same
	   monotonic sort result and is less expensive to compute than the real angle. */
	if (Vequ2(v, rnd_vect_zero)) {
		if (side == 'P') {
			if (pt->prev->cnlst_prev == PA_CONN_DESC_INVALID)
				pt->prev->cnlst_prev = pt->prev->cnlst_next = NULL;
			rnd_poly_vertex_exclude(NULL, pt->prev);
			Vsub2(v, pt->prev->point, pt->point);
		}
		else {
			if (pt->next->cnlst_prev == PA_CONN_DESC_INVALID)
				pt->next->cnlst_prev = pt->next->cnlst_next = NULL;
			rnd_poly_vertex_exclude(NULL, pt->next);
			Vsub2(v, pt->next->point, pt->point);
		}
	}

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
#ifdef DEBUG_ANGLE
	DEBUGP("point on %c at %#mD assigned angle %g on side %c\n", poly, pt->point[0], pt->point[1], ang, side);
#endif

	return cd;
}

/* Internal call for linking in a newd before or after pos */
static pa_conn_desc_t *pa_link_before_conn_desc(pa_conn_desc_t *newd, pa_conn_desc_t *pos)
{
	newd->next = pos;
	newd->prev = pos->prev;
	pos->prev = pos->prev->next = newd;
	return newd;
}
static pa_conn_desc_t *pa_link_after_conn_desc(pa_conn_desc_t *newd, pa_conn_desc_t *pos)
{
	newd->prev = pos;
	newd->next = pos->next;
	pos->next = pos->next->prev = newd;
	return newd;
}

/* Allocate a new conn_desc fot an intersection point and insert it in the
   right lists.
   Arguments:
    - a is an intersection node.
    - poly is the polygon it comes from ('A' or 'B')
    - side is the side this descriptor goes on ('P'=previous 'N'=next.
    - start is the head of the list of cnlst */
static pa_conn_desc_t *pa_insert_conn_desc(rnd_vnode_t *a, char poly, char side, pa_conn_desc_t *start)
{
	pa_conn_desc_t *l, *newd, *big, *small;

	newd = pa_new_conn_desc(a, poly, side);
	if (newd == NULL)
		return NULL; /* failed to allocate */

	/* search for the pa_conn_desc_t for this point */
	if (start == NULL) {
		/* start was empty, create a new list */
		start = newd;
		start->head = newd; /* circular list */
		return newd;
	}

	l = start;
	for(;;) {
		assert(l->head != NULL);

		if (Vequ2(l->parent->point, a->point)) {
			/* this conn_desc is at our point */
			start = l;
			newd->head = l->head;
			break;
		}

		if (Vequ2(l->head->parent->point, start->parent->point)) {
			/* we are back at start, so our input is a new point;
		     link this connlist to the list of all connlists */
			for(; l->head != newd; l = l->next)
				l->head = newd;
			newd->head = start;
			return newd;
		}

		l = l->head;
	}

	/* got here by finding a conn desc (start) on the same geometrical point;
	   insert new node keeping the list sorted by angle */

	/* find two existing conn descs to link in between (by angle) */
	l = big = small = start;
	do {
		if (l->next->angle < l->angle) {
			/* found start/end of list */
			small = l->next;
			big = l;
		}
		else if ((newd->angle >= l->angle) && (newd->angle <= l->next->angle))
			return pa_link_after_conn_desc(newd, l); /* insert new conn desc if it lies between existing points */
	} while((l = l->next) != start);

	/* didn't find it between points, it must go on an end, depending on the angle */
	if (big->angle <= newd->angle)
		return pa_link_after_conn_desc(newd, big);

	assert(small->angle >= newd->angle);

	/* link in at small end */
	return pa_link_before_conn_desc(newd, small);
}

/*
 (C) 2006 harry eaton
*/
static pa_conn_desc_t *pa_add_conn_desc(rnd_pline_t * pl, char poly, pa_conn_desc_t * list)
{
	rnd_vnode_t *node = pl->head;

	do {
		if (node->cnlst_prev) {
			assert(node->cnlst_prev == PA_CONN_DESC_INVALID && node->cnlst_next == PA_CONN_DESC_INVALID);
			list = node->cnlst_prev = pa_insert_conn_desc(node, poly, 'P', list);
			if (!node->cnlst_prev)
				return NULL;
			list = node->cnlst_next = pa_insert_conn_desc(node, poly, 'N', list);
			if (!node->cnlst_next)
				return NULL;
		}
	}
	while ((node = node->next) != pl->head);
	return list;
}
