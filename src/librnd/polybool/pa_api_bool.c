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

int rnd_polybool_disable_autocheck = 0;

/* If vertices have high precision coords, the actual output will differ
   slightly after rounding the coords. In other words rounding moves vertices.
   In some cases this may change the topology and cause self-intersecting
   polygons or zero-length edges. Remove them here. */
RND_INLINE void pa_bool_postproc(rnd_polyarea_t **pa, int from_selfisc, int papa_touch_risk)
{
#ifdef PA_BIGCOORD_ISC
	pa_big_bool_postproc(pa, from_selfisc, papa_touch_risk);
#endif
	/* else don't do naything: this happens only with big coords */
}


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

int rnd_polyarea_boolean_free_nochk(rnd_polyarea_t *a_, rnd_polyarea_t *b_, rnd_polyarea_t **res, int op)
{
	rnd_polyarea_t *a = a_, *b = b_;
	rnd_pline_t *a_isected = NULL, *holes = NULL;
	jmp_buf e;
	int code, from_selfisc = a_->from_selfisc && b_->from_selfisc;
	int papa_touch_risk = 0;

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
		pa_polyarea_intersect(&e, a, b, op, rnd_true, &papa_touch_risk);

		/* We could speed things up a lot here if we only processed the relevant contours */
		/* (Relevant parts of a are labeled below) */
		pa_polyarea_label(b, a, rnd_false);

		*res = a;
		pa_polyarea_update_primary(&e, res, &holes, op, b);
		pa_polyarea_separate_intersected(&e, res, &holes, &a_isected);
		pa_polyarea_label_pline(a_isected, b, rnd_false);
		pa_polyarea_collect_separated(&e, a_isected, res, &holes, op, rnd_false, b);
		pa_collect_b_area(&e, b, res, &holes, op);

		pa_polyarea_free_all(&b);
		rnd_poly_plines_free(&a_isected);

		rnd_poly_insert_holes(&e, *res, &holes, op);
	}

	rnd_poly_plines_free(&holes); /* delete holes if any left (they are already inserted) */

	if (code != 0) {
		pa_polyarea_free_all(res);
		return code;
	}

	if (*res != NULL)
		pa_bool_postproc(res, from_selfisc, papa_touch_risk);

	return code;
}

int rnd_polyarea_boolean_free(rnd_polyarea_t *a_, rnd_polyarea_t *b_, rnd_polyarea_t **res, int op)
{
	int code = rnd_polyarea_boolean_free_nochk(a_, b_, res, op);

	if ((code == 0) && (*res != NULL) && !rnd_polybool_disable_autocheck) {
		assert(rnd_poly_valid(*res));
	}

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

int rnd_polyarea_and_subtract_free(rnd_polyarea_t *a, rnd_polyarea_t *b, rnd_polyarea_t **aandb, rnd_polyarea_t **aminusb)
{
	rnd_pline_t *holes = NULL;
	jmp_buf e;
	int code;

	*aandb = *aminusb = NULL;

#ifdef DEBUG
		if (!rnd_poly_valid(a) || !rnd_poly_valid(b))
			return -1;
#endif


	code = setjmp(e);
	if (code == 0) {
		int papa_touch_risk = 0;

		/* map and label */
		pa_polyarea_intersect(&e, a, b, RND_PBO_ISECT, rnd_true, &papa_touch_risk);
		pa_polyarea_label(a, b, rnd_false);
		pa_polyarea_label(b, a, rnd_false);

		/* calculate aandb */
		pa_polyarea_collect(&e, a, aandb, &holes, RND_PBO_ISECT, rnd_false);
		rnd_poly_insert_holes(&e, *aandb, &holes, RND_PBO_ISECT);

#if 0
		/* breaks test case work/test_poly/dicer1.lht; this function is used only
		   in the dicer with vertical cutting edges, it is safe to skip this */
		if (*aandb != NULL)
			pa_bool_postproc(*aandb, 0, papa_touch_risk);
#endif

		assert(rnd_poly_valid(*aandb));

		/* clean up temporary marks and hole list */
		rnd_poly_plines_free(&holes); /* delete holes if any left (not inserted) */
		pa_polyarea_clear_marks(a);
		pa_polyarea_clear_marks(b);

		/* calculate aminusb */
		pa_polyarea_collect(&e, a, aminusb, &holes, RND_PBO_SUB, rnd_false);
		rnd_poly_insert_holes(&e, *aminusb, &holes, RND_PBO_SUB);

#if 0
		/* breaks test case work/test_poly/dicer1.lht; this function is used only
		   in the dicer with vertical cutting edges, it is safe to skip this */
		if (*aminusb != NULL)
			pa_bool_postproc(*aminusb, 0, papa_touch_risk);
#endif

		assert(rnd_poly_valid(*aminusb));

		pa_polyarea_free_all(&a);
		pa_polyarea_free_all(&b);

		TODO("postproc with papa_touch_risk");
	}

	rnd_poly_plines_free(&holes); /* delete holes if any left (not inserted) */

	if (code != 0) {
		pa_polyarea_free_all(aandb);
		pa_polyarea_free_all(aminusb);
		return code;
	}

	return code;
}
