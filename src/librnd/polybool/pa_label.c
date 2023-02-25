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

#ifdef DEBUG

static char *theState(rnd_vnode_t * v)
{
	static char u[] = "UNKNOWN";
	static char i[] = "INSIDE";
	static char o[] = "OUTSIDE";
	static char s[] = "SHARED";
	static char s2[] = "SHARED2";

	switch (v->Flags.pstatus) {
	case INSIDE:
		return i;
	case OUTSIDE:
		return o;
	case SHARED:
		return s;
	case SHARED2:
		return s2;
	default:
		return u;
	}
}

#ifdef DEBUG_ALL_LABELS
static void print_labels(rnd_pline_t * a)
{
	rnd_vnode_t *c = a->head;

	do {
		DEBUGP("%#mD->%#mD labeled %s\n", c->point[0], c->point[1], c->next->point[0], c->next->point[1], theState(c));
	}
	while ((c = c->next) != a->head);
}
#endif
#endif

/*
node_label
 (C) 2006 harry eaton
*/
static pa_plinept_status_t node_label(rnd_vnode_t * pn)
{
	rnd_cvc_list_t *first_l, *l;
	char this_poly;
	pa_plinept_status_t region = PA_PTS_UNKNWN;

	assert(pn);
	assert(pn->cvc_next);
	this_poly = pn->cvc_next->poly;
	/* search counter-clockwise in the cross vertex connectivity (CVC) list
	 *
	 * check for shared edges (that could be prev or next in the list since the angles are equal)
	 * and check if this edge (pn -> pn->next) is found between the other poly's entry and exit
	 */
	if (pn->cvc_next->angle == pn->cvc_next->prev->angle)
		l = pn->cvc_next->prev;
	else
		l = pn->cvc_next->next;

	first_l = l;
	while ((l->poly == this_poly) && (l != first_l->prev))
		l = l->next;
	assert(l->poly != this_poly);

	assert(l && l->angle >= 0 && l->angle <= 4.0);
	if (l->poly != this_poly) {
		if (l->side == 'P') {
			if (l->parent->prev->point[0] == pn->next->point[0] && l->parent->prev->point[1] == pn->next->point[1]) {
				region = PA_PTS_SHARED2;
				pn->shared = l->parent->prev;
			}
			else
				region = PA_PTS_INSIDE;
		}
		else {
			if (l->angle == pn->cvc_next->angle) {
				assert(l->parent->next->point[0] == pn->next->point[0] && l->parent->next->point[1] == pn->next->point[1]);
				region = PA_PTS_SHARED;
				pn->shared = l->parent;
			}
			else
				region = PA_PTS_OUTSIDE;
		}
	}
	if (region == PA_PTS_UNKNWN) {
		for (l = l->next; l != pn->cvc_next; l = l->next) {
			if (l->poly != this_poly) {
				if (l->side == 'P')
					region = PA_PTS_INSIDE;
				else
					region = PA_PTS_OUTSIDE;
				break;
			}
		}
	}
	assert(region != PA_PTS_UNKNWN);
	assert(pn->Flags.pstatus == PA_PTS_UNKNWN || pn->Flags.pstatus == region);
	pn->Flags.pstatus = region;
	if (region == PA_PTS_SHARED || region == PA_PTS_SHARED2)
		return PA_PTS_UNKNWN;
	return region;
}																/* node_label */


/*
label_contour
 (C) 2006 harry eaton
 (C) 1993 Klamer Schutte
 (C) 1997 Alexey Nikitin, Michael Leonov
*/

static rnd_bool label_contour(rnd_pline_t * a)
{
	rnd_vnode_t *cur = a->head;
	rnd_vnode_t *first_labelled = NULL;
	pa_plinept_status_t label = PA_PTS_UNKNWN;

	do {
		if (cur->cvc_next) {				/* examine cross vertex */
			label = node_label(cur);
			if (first_labelled == NULL)
				first_labelled = cur;
			continue;
		}

		if (first_labelled == NULL)
			continue;

		/* This labels nodes which aren't cross-connected */
		assert(label == PA_PTS_INSIDE || label == PA_PTS_OUTSIDE);
		cur->Flags.pstatus = label;
	}
	while ((cur = cur->next) != first_labelled);
#ifdef DEBUG_ALL_LABELS
	print_labels(a);
	DEBUGP("\n\n");
#endif
	return rnd_false;
}																/* label_contour */

static rnd_bool cntr_label_rnd_polyarea_t(rnd_pline_t * poly, rnd_polyarea_t * ppl, rnd_bool test)
{
	assert(ppl != NULL && ppl->contours != NULL);
	if (poly->Flags.lstatus == PA_PLS_ISECTED) {
		label_contour(poly);				/* should never get here when rnd_bool is rnd_true */
	}
	else if (cntr_in_M_rnd_polyarea_t(poly, ppl, test)) {
		if (test)
			return rnd_true;
		poly->Flags.lstatus = PA_PLS_INSIDE;
	}
	else {
		if (test)
			return rnd_false;
		poly->Flags.lstatus = PA_PLS_OUTSIDE;
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
