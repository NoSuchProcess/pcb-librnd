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

/* Connection (intersection) descriptors making up a connectivity list.
   Low level helper functions. (It was "cvc" in the original code.) */


/* preallocate a new, preliminary conn desc, to store precise intersection
   coords (isc) */
pa_conn_desc_t *pa_prealloc_conn_desc(pa_big_vector_t isc)
{
	pa_conn_desc_t *cd = calloc(sizeof(pa_conn_desc_t), 1);
	if (cd == NULL)
		return NULL;

	cd->prelim = 1;
	pa_big_copy(cd->isc, isc);

	return cd;
}

RND_INLINE double pa_vect_to_angle_small(rnd_vector_t v)
{
	double ang, dx, dy;

	dx = fabs((double)v[0]);
	dy = fabs((double)v[1]);
	ang = dy / (dy + dx);

	/* now move to the actual quadrant */
	if ((v[0] < 0) && (v[1] >= 0))         ang = 2.0 - ang; /* 2nd quadrant */
	else if ((v[0] < 0) && (v[1] < 0))     ang = 2.0 + ang; /* 3rd quadrant */
	else if ((v[0] >= 0) && (v[1] < 0))    ang = 4.0 - ang; /* 4th quadrant */
	else                                   { /* ang */ }    /* 1st quadrant */

	assert((ang >= 0.0) && (ang <= 4.0));

	return ang;
}


#ifndef PA_BIGCOORD_ISC
RND_INLINE void pa_conn_calc_angle_small(pa_conn_desc_t *cd, rnd_vnode_t *pt, char poly, char side)
{
	rnd_vector_t v;

	if (side == 'P') /* previous */
		Vsub2(v, pt->prev->point, pt->point);
	else /* next */
		Vsub2(v, pt->next->point, pt->point);

	if (Vequ2(v, rnd_vect_zero)) {
		if (side == 'P') {
			if (pt->prev->cvclst_prev->prelim) {
				free(pt->prev->cvclst_prev);
				free(pt->prev->cvclst_next);
				pt->prev->cvclst_prev = pt->prev->cvclst_next = NULL;
			}
			rnd_poly_vertex_exclude(NULL, pt->prev);
			Vsub2(v, pt->prev->point, pt->point);
		}
		else {
			if (pt->next->cvclst_prev->prelim) {
				free(pt->prev->cvclst_prev);
				free(pt->prev->cvclst_next);
				pt->next->cvclst_prev = pt->next->cvclst_next = NULL;
			}
			rnd_poly_vertex_exclude(NULL, pt->next);
			Vsub2(v, pt->next->point, pt->point);
		}
	}

	assert(!Vequ2(v, rnd_vect_zero));

	cd->angle = pa_dxdy_to_angle_small(v);

	DEBUG_ANGLE("point on %c at %$mD assigned angle %.08f on side %c\n", poly, pt->point[0], pt->point[1], cd->angle, side);
}
#endif

RND_INLINE void pa_conn_calc_angle(pa_conn_desc_t *cd, rnd_vnode_t *pt, char poly, char side)
{
#ifdef PA_BIGCOORD_ISC
	pa_big_calc_angle(cd, pt, poly, side);
#else
	pa_conn_calc_angle_small(cd, pt, poly, side);
#endif
}

static pa_conn_desc_t *pa_new_conn_desc(pa_conn_desc_t *cd, rnd_vnode_t *pt, char poly, char side)
{
	cd->head = NULL;
	cd->parent = pt;
	cd->poly = poly;
	cd->side = side;
	cd->next = cd->prev = cd;
	cd->prelim = 0;

	/* Uses slope/(slope+1) in quadrant 1 as a proxy for the angle. It has the same
	   monotonic sort result and is less expensive to compute than the real angle. */
	pa_conn_calc_angle(cd, pt, poly, side);

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

RND_INLINE int pa_desc_node_incident(pa_conn_desc_t *d, rnd_vnode_t *n)
{
#ifdef PA_BIGCOORD_ISC
	return pa_big_desc_node_incident(d, n);
#else
	return Vequ2(d->parent->point, n->point);
#endif
}

RND_INLINE int pa_desc_desc_incident(pa_conn_desc_t *a, pa_conn_desc_t *b)
{
#ifdef PA_BIGCOORD_ISC
	return pa_big_desc_desc_incident(a, b);
#else
	return Vequ2(a->parent->point, b->parent->point);
#endif
}

/* Finalize preliminary conn_desc cd for an intersection point and insert it in the
   right lists.
   Arguments:
    - a is an intersection node.
    - poly is the polygon it comes from ('A' or 'B')
    - side is the side this descriptor goes on ('P'=previous 'N'=next)
    - start is the head of the list of cnlst
    */
static pa_conn_desc_t *pa_insert_conn_desc(pa_conn_desc_t *cd, rnd_vnode_t *a, char poly, char side, pa_conn_desc_t *start)
{
	pa_conn_desc_t *l, *newd, *big, *small;

	newd = pa_new_conn_desc(cd, a, poly, side);
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

		if (pa_desc_node_incident(l, a)) {
			/* this conn_desc is at our point */
			start = l;
			newd->head = l->head;
			break;
		}

		if (pa_desc_desc_incident(l->head, start)) {
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
		if (pa_angle_lt(l->next->angle, l->angle)) {
			/* found start/end of list */
			small = l->next;
			big = l;
		}
		else if (pa_angle_gte(newd->angle, l->angle) && pa_angle_lte(newd->angle, l->next->angle))
			return pa_link_after_conn_desc(newd, l); /* insert new conn desc if it lies between existing points */
	} while((l = l->next) != start);

	/* didn't find it between points, it must go on an end, depending on the angle */
	if (pa_angle_lte(big->angle, newd->angle))
		return pa_link_after_conn_desc(newd, big);

	assert(pa_angle_gte(small->angle, newd->angle));

	/* link in at small end */
	return pa_link_before_conn_desc(newd, small);
}

RND_INLINE pa_conn_desc_t *pa_add_conn_desc_at_(rnd_vnode_t *node, char poly, pa_conn_desc_t *list)
{
	pa_conn_desc_t *p = node->cvclst_prev, *n = node->cvclst_next;

	/* must be preliminary allocation that we are finalizing here */
	assert((p->prelim) && (n->prelim));

	list = node->cvclst_prev = pa_insert_conn_desc(p, node, poly, 'P', list);
	if (node->cvclst_prev == NULL)
		return NULL;

	list = node->cvclst_next = pa_insert_conn_desc(n, node, poly, 'N', list);
	if (node->cvclst_next == NULL)
		return NULL;

	return list;
}

pa_conn_desc_t *pa_add_conn_desc_at(rnd_vnode_t *node, char poly, pa_conn_desc_t *list)
{
	return pa_add_conn_desc_at_(node, poly, list);
}

/* Add all intersected nodes of pl into list */
pa_conn_desc_t *pa_add_conn_desc(rnd_pline_t *pl, char poly, pa_conn_desc_t *list)
{
	rnd_vnode_t *node = pl->head;

	do {
		if (node->cvclst_prev != NULL) { /* node had an intersection if has a preliminary desc */
			list = pa_add_conn_desc_at_(node, poly, list);
			if (list == NULL)
				return NULL;
		}
	} while((node = node->next) != pl->head);

	return list;
}


