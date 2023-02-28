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

int rnd_polyarea_boolean(const rnd_polyarea_t *a_, const rnd_polyarea_t *b_, rnd_polyarea_t **res, int op)
{
	rnd_polyarea_t *a = NULL, *b = NULL;

	if (!rnd_polyarea_alloc_copy_all(&a, a_))
		return pa_err_no_memory;

	if (!rnd_polyarea_alloc_copy_all(&b, b_)) {
		pa_polyarea_free_all(&a);
		return pa_err_no_memory;
	}

	return rnd_polyarea_boolean_free(a, b, res, op);
}

int rnd_polyarea_boolean_free(rnd_polyarea_t *a_, rnd_polyarea_t *b_, rnd_polyarea_t **res, int op)
{
	rnd_polyarea_t *a = a_, *b = b_;
	rnd_pline_t *a_isected = NULL, *holes = NULL;
	jmp_buf e;
	int code;

	*res = NULL;

	/* handle the case when either input is empty */
	if (a == NULL) {
		switch (op) {
			case RND_PBO_XOR:
			case RND_PBO_UNITE:
				*res = b_;
				return pa_err_ok;
			case RND_PBO_SUB:
			case RND_PBO_ISECT:
				if (b != NULL)
					pa_polyarea_free_all(&b);
				return pa_err_ok;
		}
	}
	if (b == NULL) {
		switch (op) {
			case RND_PBO_SUB:
			case RND_PBO_XOR:
			case RND_PBO_UNITE:
				*res = a_;
				return pa_err_ok;
			case RND_PBO_ISECT:
				if (a != NULL)
					pa_polyarea_free_all(&a);
				return pa_err_ok;
		}
	}

	/* Need to calculate intersections, label contours, etc. */

	code = setjmp(e);
	if (code == 0) {
#ifdef DEBUG
		assert(rnd_poly_valid(a) && rnd_poly_valid(b));
#endif

		/* intersect needs to make a list of the contours in a and b which are intersected */
		pa_polyarea_intersect(&e, a, b, rnd_true);

		/* We could speed things up a lot here if we only processed the relevant contours */
		/* (Relevant parts of a are labeled below) */
		pa_polyarea_label(b, a, rnd_false);

		*res = a;
		M_rnd_polyarea_t_update_primary(&e, res, &holes, op, b);
		M_rnd_polyarea_separate_isected(&e, res, &holes, &a_isected);
		pa_polyarea_label_pline(a_isected, b, rnd_false);
		M_rnd_polyarea_t_Collect_separated(&e, a_isected, res, &holes, op, rnd_false);
		M_B_AREA_Collect(&e, b, res, &holes, op);

		pa_polyarea_free_all(&b);
		rnd_poly_plines_free(&a_isected);

		rnd_poly_insert_holes(&e, *res, &holes);
	}

	rnd_poly_plines_free(&holes); /* delete holes if any left (they are already inserted) */

	if (code != 0) {
		pa_polyarea_free_all(res);
		return code;
	}

	assert(!*res || rnd_poly_valid(*res));
	return code;
}

RND_INLINE void pa_polyarea_clear_marks(rnd_polyarea_t *pa)
{
	rnd_polyarea_t *pan = pa;

	do {
		rnd_pline_t *pln;
		for(pln = pan->contours; pln != NULL; pln = pln->next) {
			rnd_vnode_t *vn = pln->head;
			do {
				vn->flg.mark = 0;
			} while((vn = vn->next) != pln->head);
		}
	} while((pan = pan->f) != pa);
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

		pa_polyarea_label(a, b, rnd_false);
		pa_polyarea_label(b, a, rnd_false);

		M_rnd_polyarea_t_Collect(&e, a, aandb, &holes, RND_PBO_ISECT, rnd_false);
		rnd_poly_insert_holes(&e, *aandb, &holes);
		assert(rnd_poly_valid(*aandb));
		/* delete holes if any left */
		while ((p = holes) != NULL) {
			holes = p->next;
			pa_pline_free(&p);
		}
		holes = NULL;
		pa_polyarea_clear_marks(a);
		pa_polyarea_clear_marks(b);
		M_rnd_polyarea_t_Collect(&e, a, aminusb, &holes, RND_PBO_SUB, rnd_false);
		rnd_poly_insert_holes(&e, *aminusb, &holes);
		pa_polyarea_free_all(&a);
		pa_polyarea_free_all(&b);
		assert(rnd_poly_valid(*aminusb));
	}
	/* delete holes if any left */
	while ((p = holes) != NULL) {
		holes = p->next;
		pa_pline_free(&p);
	}


	if (code) {
		pa_polyarea_free_all(aandb);
		pa_polyarea_free_all(aminusb);
		return code;
	}
	assert(!*aandb || rnd_poly_valid(*aandb));
	assert(!*aminusb || rnd_poly_valid(*aminusb));
	return code;
} /* poly_AndSubtract_free */
