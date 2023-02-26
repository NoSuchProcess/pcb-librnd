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

/* Contour labelling */

/* label an intersecting, node as per the paper */
static pa_plinept_label_t pa_node_label(rnd_vnode_t *pn)
{
	pa_conn_desc_t *l1, *l;
	char this_poly;
	pa_plinept_label_t region = PA_PTL_UNKNWN;

	assert(pn != NULL);
	assert(pn->cvclst_next != NULL);

	this_poly = pn->cvclst_next->poly;

	/* search counter-clockwise in the cross-vertex connectivity list, check
	   for shared edges (that could be prev or next in the list since the
	   angles are equal) and check if this edge (pn -> pn->next) is found
	   between the other poly's entry and exit */
	if (pn->cvclst_next->angle == pn->cvclst_next->prev->angle)
		l = pn->cvclst_next->prev;
	else
		l = pn->cvclst_next->next;

	for(l1 = l; (l->poly == this_poly) && (l != l1->prev); l = l->next) ;

	assert(l != NULL);
	assert(l->poly != this_poly);
	assert((l->angle >= 0.0) && (l->angle <= 4.0));

	if (l->poly != this_poly) {
		if (l->side == 'P') {
			if (Vequ2(l->parent->prev->point, pn->next->point)) {
				region = PA_PTL_SHARED2;
				pn->shared = l->parent->prev;
			}
			else
				region = PA_PTL_INSIDE;
		}
		else {
			if (l->angle == pn->cvclst_next->angle) {
				assert(Vequ2(l->parent->next->point, pn->next->point));
				region = PA_PTL_SHARED;
				pn->shared = l->parent;
			}
			else
				region = PA_PTL_OUTSIDE;
		}
	}

	if (region == PA_PTL_UNKNWN) {
		for (l = l->next; l != pn->cvclst_next; l = l->next) {
			if (l->poly != this_poly) {
				region = (l->side == 'P') ? PA_PTL_INSIDE : PA_PTL_OUTSIDE;
				break;
			}
		}
	}

	assert(region != PA_PTL_UNKNWN);
	assert((pn->flg.plabel == PA_PTL_UNKNWN) || (pn->flg.plabel == region));
	pn->flg.plabel = region;

	if ((region == PA_PTL_SHARED) || (region == PA_PTL_SHARED2))
		return PA_PTL_UNKNWN;
	return region;
}


/* Label all nodes on a contour */
static void pa_label_contour(rnd_pline_t *pl)
{
	rnd_vnode_t *nd, *first_isected = NULL;
	pa_plinept_label_t label = PA_PTL_UNKNWN;

	/* loop from head until the first intersection node; label that node and
	   carry the label to mark all non-intersecting nodes until the next
	   intersecting node. Repeat until we are back at the first intersected
	   node.
	   This means initially, before the first intersection, we can't label
	   non-intersected nodes because we don't yet have a label; but the loop
	   terminates only after reaching the first intersection point again,
	   giving a second visit to those pre-first-intersection nodes. */
	nd = pl->head;
	do {
		if (nd->cvclst_next != NULL) {
			/* nd is intersected, need to label it */
			label = pa_node_label(nd);
			if (first_isected == NULL)
				first_isected = nd;
			continue;
		}

		if (first_isected != NULL) {
			/* nd is between two intersected nodes, on a non-intersected node */
			assert((label == PA_PTL_INSIDE) || (label == PA_PTL_OUTSIDE));
			nd->flg.plabel = label;
		}
	} while((nd = nd->next) != first_isected);

#ifdef DEBUG_ALL_LABELS
	pa_print_pline_labels(a);
	DEBUGP("\n\n");
#endif
}

static rnd_bool cntr_label_rnd_polyarea_t(rnd_pline_t * poly, rnd_polyarea_t * ppl, rnd_bool test)
{
	assert(ppl != NULL && ppl->contours != NULL);
	if (poly->flg.llabel == PA_PLL_ISECTED) {
		pa_label_contour(poly);				/* should never get here when rnd_bool is rnd_true */
	}
	else if (cntr_in_M_rnd_polyarea_t(poly, ppl, test)) {
		if (test)
			return rnd_true;
		poly->flg.llabel = PA_PLL_INSIDE;
	}
	else {
		if (test)
			return rnd_false;
		poly->flg.llabel = PA_PLL_OUTSIDE;
	}
	return rnd_false;
}																/* cntr_label_rnd_polyarea_t */

static rnd_bool M_rnd_polyarea_t_label_separated(rnd_pline_t * afst, rnd_polyarea_t * b, rnd_bool touch)
{
	rnd_pline_t *curc = afst;

	for (curc = afst; curc != NULL; curc = curc->next) {
		if (cntr_label_rnd_polyarea_t(curc, b, touch) && touch)
			return rnd_true;
	}
	return rnd_false;
}

static rnd_bool M_rnd_polyarea_t_label(rnd_polyarea_t * afst, rnd_polyarea_t * b, rnd_bool touch)
{
	rnd_polyarea_t *a = afst;
	rnd_pline_t *curc;

	assert(a != NULL);
	do {
		for (curc = a->contours; curc != NULL; curc = curc->next)
			if (cntr_label_rnd_polyarea_t(curc, b, touch)) {
				if (touch)
					return rnd_true;
			}
	}
	while (!touch && (a = a->f) != afst);
	return rnd_false;
}
