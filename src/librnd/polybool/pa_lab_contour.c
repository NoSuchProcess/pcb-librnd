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

	switch (NODE_LABEL(v)) {
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
label_contour
 (C) 2006 harry eaton
 (C) 1993 Klamer Schutte
 (C) 1997 Alexey Nikitin, Michael Leonov
*/

static rnd_bool label_contour(rnd_pline_t * a)
{
	rnd_vnode_t *cur = a->head;
	rnd_vnode_t *first_labelled = NULL;
	int label = UNKNWN;

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
		assert(label == INSIDE || label == OUTSIDE);
		LABEL_NODE(cur, label);
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
	if (poly->Flags.status == ISECTED) {
		label_contour(poly);				/* should never get here when rnd_bool is rnd_true */
	}
	else if (cntr_in_M_rnd_polyarea_t(poly, ppl, test)) {
		if (test)
			return rnd_true;
		poly->Flags.status = INSIDE;
	}
	else {
		if (test)
			return rnd_false;
		poly->Flags.status = OUTSIDE;
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
