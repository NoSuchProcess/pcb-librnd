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

int rnd_polyarea_boolean(const rnd_polyarea_t * a_org, const rnd_polyarea_t * b_org, rnd_polyarea_t ** res, int action)
{
	rnd_polyarea_t *a = NULL, *b = NULL;

	if (!rnd_polyarea_m_copy0(&a, a_org) || !rnd_polyarea_m_copy0(&b, b_org))
		return rnd_err_no_memory;

	return rnd_polyarea_boolean_free(a, b, res, action);
}																/* poly_Boolean */

/* just like poly_Boolean but frees the input polys */
int rnd_polyarea_boolean_free(rnd_polyarea_t * ai, rnd_polyarea_t * bi, rnd_polyarea_t ** res, int action)
{
	rnd_polyarea_t *a = ai, *b = bi;
	rnd_pline_t *a_isected = NULL;
	rnd_pline_t *p, *holes = NULL;
	jmp_buf e;
	int code;

	*res = NULL;

	if (!a) {
		switch (action) {
		case RND_PBO_XOR:
		case RND_PBO_UNITE:
			*res = bi;
			return rnd_err_ok;
		case RND_PBO_SUB:
		case RND_PBO_ISECT:
			if (b != NULL)
				rnd_polyarea_free(&b);
			return rnd_err_ok;
		}
	}
	if (!b) {
		switch (action) {
		case RND_PBO_SUB:
		case RND_PBO_XOR:
		case RND_PBO_UNITE:
			*res = ai;
			return rnd_err_ok;
		case RND_PBO_ISECT:
			if (a != NULL)
				rnd_polyarea_free(&a);
			return rnd_err_ok;
		}
	}

	if ((code = setjmp(e)) == 0) {
#ifdef DEBUG
		assert(rnd_poly_valid(a));
		assert(rnd_poly_valid(b));
#endif

		/* intersect needs to make a list of the contours in a and b which are intersected */
		pa_polyarea_intersect(&e, a, b, rnd_true);

		/* We could speed things up a lot here if we only processed the relevant contours */
		/* NB: Relevant parts of a are labeled below */
		M_rnd_polyarea_t_label(b, a, rnd_false);

		*res = a;
		M_rnd_polyarea_t_update_primary(&e, res, &holes, action, b);
		M_rnd_polyarea_separate_isected(&e, res, &holes, &a_isected);
		M_rnd_polyarea_t_label_separated(a_isected, b, rnd_false);
		M_rnd_polyarea_t_Collect_separated(&e, a_isected, res, &holes, action, rnd_false);
		M_B_AREA_Collect(&e, b, res, &holes, action);
		rnd_polyarea_free(&b);

		/* free a_isected */
		while ((p = a_isected) != NULL) {
			a_isected = p->next;
			rnd_poly_contour_del(&p);
		}

		rnd_poly_insert_holes(&e, *res, &holes);
	}
	/* delete holes if any left */
	while ((p = holes) != NULL) {
		holes = p->next;
		rnd_poly_contour_del(&p);
	}

	if (code) {
		rnd_polyarea_free(res);
		return code;
	}
	assert(!*res || rnd_poly_valid(*res));
	return code;
}																/* poly_Boolean_free */

static void clear_marks(rnd_polyarea_t * p)
{
	rnd_polyarea_t *n = p;
	rnd_pline_t *c;
	rnd_vnode_t *v;

	do {
		for (c = n->contours; c; c = c->next) {
			v = c->head;
			do {
				v->flg.mark = 0;
			}
			while ((v = v->next) != c->head);
		}
	}
	while ((n = n->f) != p);
}

/* compute the intersection and subtraction (divides "a" into two pieces)
 * and frees the input polys. This assumes that bi is a single simple polygon.
 */
int rnd_polyarea_and_subtract_free(rnd_polyarea_t * ai, rnd_polyarea_t * bi, rnd_polyarea_t ** aandb, rnd_polyarea_t ** aminusb)
{
	rnd_polyarea_t *a = ai, *b = bi;
	rnd_pline_t *p, *holes = NULL;
	jmp_buf e;
	int code;

	*aandb = NULL;
	*aminusb = NULL;

	if ((code = setjmp(e)) == 0) {

#ifdef DEBUG
		if (!rnd_poly_valid(a))
			return -1;
		if (!rnd_poly_valid(b))
			return -1;
#endif
		pa_polyarea_intersect(&e, a, b, rnd_true);

		M_rnd_polyarea_t_label(a, b, rnd_false);
		M_rnd_polyarea_t_label(b, a, rnd_false);

		M_rnd_polyarea_t_Collect(&e, a, aandb, &holes, RND_PBO_ISECT, rnd_false);
		rnd_poly_insert_holes(&e, *aandb, &holes);
		assert(rnd_poly_valid(*aandb));
		/* delete holes if any left */
		while ((p = holes) != NULL) {
			holes = p->next;
			rnd_poly_contour_del(&p);
		}
		holes = NULL;
		clear_marks(a);
		clear_marks(b);
		M_rnd_polyarea_t_Collect(&e, a, aminusb, &holes, RND_PBO_SUB, rnd_false);
		rnd_poly_insert_holes(&e, *aminusb, &holes);
		rnd_polyarea_free(&a);
		rnd_polyarea_free(&b);
		assert(rnd_poly_valid(*aminusb));
	}
	/* delete holes if any left */
	while ((p = holes) != NULL) {
		holes = p->next;
		rnd_poly_contour_del(&p);
	}


	if (code) {
		rnd_polyarea_free(aandb);
		rnd_polyarea_free(aminusb);
		return code;
	}
	assert(!*aandb || rnd_poly_valid(*aandb));
	assert(!*aminusb || rnd_poly_valid(*aminusb));
	return code;
} /* poly_AndSubtract_free */
