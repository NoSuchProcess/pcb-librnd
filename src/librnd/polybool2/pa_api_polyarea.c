/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2017, 2023, 2024 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2023,2024)
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
 */

/* High level polyarea API functions (called mostly or only from outside) */

void rnd_polyarea_bbox(rnd_polyarea_t *pa, rnd_box_t *b)
{
	rnd_pline_t *n = pa->contours;

	b->X1 = b->X2 = n->xmin;
	b->Y1 = b->Y2 = n->ymin;

	for(;n != NULL; n = n->next) {
		if (n->xmin < b->X1) b->X1 = n->xmin;
		if (n->ymin < b->Y1) b->Y1 = n->ymin;
		if (n->xmax > b->X2) b->X2 = n->xmax;
		if (n->ymax > b->Y2) b->Y2 = n->ymax;
	}
}

/* Checks only the first island */
rnd_bool rnd_polyarea_contour_inside(rnd_polyarea_t *pa, rnd_vector_t pt)
{
	rnd_pline_t *n;

	if ((pa == NULL) || (pt == NULL) || (pa->contours == NULL))
		return rnd_false;

	n = pa->contours;

	/* if pt is outside the outer contour of pa */
	if (!pa_pline_is_point_inside(n, pt))
		return rnd_false;

	/* if pt is in any of the holes */
	for(n = n->next; n != NULL; n = n->next)
		if (pa_pline_is_point_inside(n, pt))
			return rnd_false;

	return rnd_true;
}

/* determine if two polygons touch or overlap; used in pcb-rnd */
rnd_bool rnd_polyarea_touching(rnd_polyarea_t *A, rnd_polyarea_t *B)
{
	rnd_polyarea_t *a, *b;

	a = A;
	do {
		b = B;
		do {
			if (rnd_polyarea_island_isc(a, b)) /* cheap bbox check is done in this call */
				return 1;
		} while((b = b->f) != B);
	} while((a = a->f) != A);

	return 0;
}

typedef enum pa_island_isc_e {
	PA_ISLAND_ISC_OUT,   /* island is fully outside of a hole */
	PA_ISLAND_ISC_IN,    /* island is fully within a hole */
	PA_ISLAND_ISC_CROSS  /* island has an intersection with the hole */
} pa_island_isc_t;


/* Iterates through the nodes of plines figuring if small is fully within big. */
static pa_island_isc_t pa_pline_pline_isc(const rnd_pline_t *big, const rnd_pline_t *small, int point_on_edge_is_in)
{
	int got_in = 0, got_out = 0;
	rnd_vnode_t *n;

	/* cheap bbox checks: if bboxes don't intersect, small is outside of big */
	if ((small->xmax < big->xmin) || (small->ymax < big->ymin))
		return PA_ISLAND_ISC_OUT;
	if ((small->xmin > big->xmax) || (small->ymin > big->ymax))
		return PA_ISLAND_ISC_OUT;

	n = small->head;
	do {
		if (pa_pline_is_vnode_inside(big, n, point_on_edge_is_in))
			got_in = 1;
		else
			got_out = 1;

		if (got_in && got_out) /* we are both inside and outside -> that's a crossing */
			return PA_ISLAND_ISC_CROSS;
	} while((n = n->next) != small->head);

	/* Corner case: the big one can have a node within the small without the
	   small having a node within the big. Test case: chk_hole_cntr */
	n = big->head;
	do {
		if (pa_pline_is_vnode_inside(small, n, 0))
			return PA_ISLAND_ISC_CROSS;
	} while((n = n->next) != big->head);

	/* if we got here, small is either full inside or fully outside */
	return got_in ? PA_ISLAND_ISC_IN : PA_ISLAND_ISC_OUT;
}

/* Returns whether a polyline is fully inside, fully outside or half-in-half-out
   of pa's holes. Checks only the first island of pa. If pl is fully within
   any island, PA_ISLAND_ISC_IN is returned. */
static pa_island_isc_t pa_polyarea_pline_island_isc(const rnd_polyarea_t *pa, const rnd_pline_t *pl)
{
	rnd_pline_t *hole;

	for(hole = pa->contours->next; hole != NULL; hole = hole->next) {
		pa_island_isc_t r = pa_pline_pline_isc(hole, pl, 1);
		if (r != PA_ISLAND_ISC_OUT)
			return r;
	}

	return PA_ISLAND_ISC_OUT;
}

rnd_bool rnd_polyarea_island_isc(const rnd_polyarea_t *a, const rnd_polyarea_t *b)
{
	rnd_pline_t *pla, *plb;
	rnd_vnode_t *n;

	/* it is enough to check outline only, no holes, assuming holes are within
	   the outline*/
	pla = a->contours;
	plb = b->contours;

	/* cheap bbox checks: if bboxes don't intersect, pla is outside of plb */
	if ((pla->xmax < plb->xmin) || (pla->ymax < plb->ymin))
		return 0;
	if ((pla->xmin > plb->xmax) || (pla->ymin > plb->ymax))
		return 0;

	/* order them so pla is the smaller (matters for speed) */
	if (pla->Count > plb->Count) {
		SWAP(rnd_pline_t *, pla, plb);
		SWAP(const rnd_polyarea_t *, a, b);
	}

	/* Corner case: if plb is fully within pla, there is no crossing so the loop
	   below doesn't trigger. In this case any node of plb is in pla so it is
	   enough to verify a single point of plb. */
	if (pa_pline_is_vnode_inside(pla, plb->head, 0)) {
		/* ... except if plb is in an island */
		int r = pa_polyarea_pline_island_isc(a, plb);
		switch(r) {
			case PA_ISLAND_ISC_IN:    return 0; /* plb island is fully in an island of pa -> no intersection pssible */
			case PA_ISLAND_ISC_OUT:   return 1; /* plb island is fully outside of any island of pa -> it is in the solid area */
			case PA_ISLAND_ISC_CROSS: return 1; /* crossing an island boundary means some points are inside the solid */
		}
		assert("!invalid return by pa_polyarea_is_vnode_island_isc()");
		return 1;
	}

	/* check each outline vertex of pla whether it is inside of plb; if any is
	   inside, pla is either inside plb or intersects plb */
	n = pla->head;
	do {
		if (pa_pline_is_vnode_inside(plb, n, 0)) {
			int r = pa_polyarea_pline_island_isc(b, pla);
			switch(r) {
				case PA_ISLAND_ISC_IN:    return 0; /* pla island is fully in an island of pb -> no intersection possible */
				case PA_ISLAND_ISC_OUT:   return 1; /* pla island is fully outside of any island of pb -> it is in the solid area */
				case PA_ISLAND_ISC_CROSS: return 1; /* crossing an island boundary means some points are inside the solid */
			}
			assert("!invalid return by pa_polyarea_is_vnode_island_isc()");
			return 1;
		}
	} while((n = n->next) != pla->head);

	/* check each outline vertex of plb whether it is inside of pla; if any is
	   inside, plb is either inside pla or intersects pla */
	n = plb->head;
	do {
		if (pa_pline_is_vnode_inside(pla, n, 0)) {
			int r = pa_polyarea_pline_island_isc(b, plb);
			switch(r) {
				case PA_ISLAND_ISC_IN:    return 0; /* plb island is fully in an island of pla -> no intersection possible */
				case PA_ISLAND_ISC_OUT:   return 1; /* plb island is fully outside of any island of pla -> it is in the solid area */
				case PA_ISLAND_ISC_CROSS: return 1; /* crossing an island boundary means some points are inside the solid */
			}
			assert("!invalid return by pa_polyarea_is_vnode_island_isc()");
			return 1;
		}
	} while((n = n->next) != plb->head);

	return 0; /* no intersections */
}


void rnd_polyarea_move(rnd_polyarea_t *pa1, rnd_coord_t dx, rnd_coord_t dy)
{
	int cnt;
	rnd_polyarea_t *pa;

	for (pa = pa1, cnt = 0; pa != NULL; pa = pa->f) {
		rnd_pline_t *pl;
		if (pa == pa1) {
			cnt++;
			if (cnt > 1)
				break;
		}
		if (pa->contour_tree != NULL)
			rnd_r_destroy_tree(&pa->contour_tree);
		pa->contour_tree = rnd_r_create_tree();
		for(pl = pa->contours; pl != NULL; pl = pl->next) {
			rnd_vnode_t *v;
			int cnt2 = 0;
			for(v = pl->head; v != NULL; v = v->next) {
				if (v == pl->head) {
					cnt2++;
					if (cnt2 > 1)
						break;
				}
				v->point[0] += dx;
				v->point[1] += dy;
			}
			pl->xmin += dx;
			pl->ymin += dy;
			pl->xmax += dx;
			pl->ymax += dy;
			if (pl->tree != NULL) {
				rnd_r_free_tree_data(pl->tree, free);
				rnd_r_destroy_tree(&pl->tree);
			}
			pl->tree = rnd_poly_make_edge_tree(pl);

			rnd_r_insert_entry(pa->contour_tree, (rnd_box_t *)pl);
		}
	}
}

