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

/* Check validity/integrity */

static rnd_bool inside_sector(rnd_vnode_t * pn, rnd_vector_t p2)
{
	rnd_vector_t cdir, ndir, pdir;
	int p_c, n_c, p_n;

	assert(pn != NULL);
	vect_sub(cdir, p2, pn->point);
	vect_sub(pdir, pn->point, pn->prev->point);
	vect_sub(ndir, pn->next->point, pn->point);

	p_c = rnd_vect_det2(pdir, cdir) >= 0;
	n_c = rnd_vect_det2(ndir, cdir) >= 0;
	p_n = rnd_vect_det2(pdir, ndir) >= 0;

	if ((p_n && p_c && n_c) || ((!p_n) && (p_c || n_c)))
		return rnd_true;
	else
		return rnd_false;
}																/* inside_sector */

/* returns rnd_true if bad contour */
typedef struct {
	int marks, lines;
#ifndef NDEBUG
	rnd_coord_t x[8], y[8];
	rnd_coord_t x1[8], y1[8], x2[8], y2[8];
	char msg[256];
#endif
} pa_chk_res_t;


#ifndef NDEBUG
#define PA_CHK_MARK(x_, y_) \
do { \
	if (res->marks < sizeof(res->x) / sizeof(res->x[0])) { \
		res->x[res->marks] = x_; \
		res->y[res->marks] = y_; \
		res->marks++; \
	} \
} while(0)
#define PA_CHK_LINE(x1_, y1_, x2_, y2_) \
do { \
	if (res->lines < sizeof(res->x1) / sizeof(res->x1[0])) { \
		res->x1[res->lines] = x1_; \
		res->y1[res->lines] = y1_; \
		res->x2[res->lines] = x2_; \
		res->y2[res->lines] = y2_; \
		res->lines++; \
	} \
} while(0)
#else
#define PA_CHK_MARK(x, y)
#define PA_CHK_LINE(x1, y1, x2, y2)
#endif


RND_INLINE rnd_bool PA_CHK_ERROR(pa_chk_res_t *res, const char *fmt, ...)
{
#ifndef NDEBUG
	va_list ap;
	va_start(ap, fmt);
	rnd_vsnprintf(res->msg, sizeof(res->msg), fmt, ap);
	va_end(ap);
#endif
	return rnd_true;
}

rnd_bool rnd_polyarea_contour_check_(rnd_pline_t *a, pa_chk_res_t *res)
{
	rnd_vnode_t *a1, *a2, *hit1, *hit2;
	rnd_vector_t i1, i2;
	int icnt;

#ifndef NDEBUG
	*res->msg = '\0';
#endif
	res->marks = res->lines = 0;

	assert(a != NULL);
	a1 = a->head;
	do {
		a2 = a1;
		do {
			if (!node_neighbours(a1, a2) && (icnt = rnd_vect_inters2(a1->point, a1->next->point, a2->point, a2->next->point, i1, i2)) > 0) {
				if (icnt > 1) {
					PA_CHK_MARK(a1->point[0], a1->point[1]);
					PA_CHK_MARK(a2->point[0], a2->point[1]);
					return PA_CHK_ERROR(res, "icnt > 1 (%d) at %mm;%mm or  %mm;%mm", icnt, a1->point[0], a1->point[1], a2->point[0], a2->point[1]);
				}

TODO(": ugly workaround: test where exactly the intersection happens and tune the endpoint of the line")
				if (rnd_vect_dist2(i1, a1->point) < RND_POLY_ENDP_EPSILON)
					hit1 = a1;
				else if (rnd_vect_dist2(i1, a1->next->point) < RND_POLY_ENDP_EPSILON)
					hit1 = a1->next;
				else
					hit1 = NULL;

				if (rnd_vect_dist2(i1, a2->point) < RND_POLY_ENDP_EPSILON)
					hit2 = a2;
				else if (rnd_vect_dist2(i1, a2->next->point) < RND_POLY_ENDP_EPSILON)
					hit2 = a2->next;
				else
					hit2 = NULL;

				if ((hit1 == NULL) && (hit2 == NULL)) {
					/* If the intersection didn't land on an end-point of either
					 * line, we know the lines cross and we return rnd_true.
					 */
					PA_CHK_LINE(a1->point[0], a1->point[1], a1->next->point[0], a1->next->point[1]);
					PA_CHK_LINE(a2->point[0], a2->point[1], a2->next->point[0], a2->next->point[1]);
					return PA_CHK_ERROR(res, "lines cross between %mm;%mm and %mm;%mm", a1->point[0], a1->point[1], a2->point[0], a2->point[1]);
				}
				else if (hit1 == NULL) {
					/* An end-point of the second line touched somewhere along the
					   length of the first line. Check where the second line leads. */
					if (inside_sector(hit2, a1->point) != inside_sector(hit2, a1->next->point)) {
						PA_CHK_MARK(a1->point[0], a1->point[1]);
						PA_CHK_MARK(hit2->point[0], hit2->point[1]);
						return PA_CHK_ERROR(res, "lines is inside sector (1) at %mm;%mm", a1->point[0], a1->point[1]);
					}
				}
				else if (hit2 == NULL) {
					/* An end-point of the first line touched somewhere along the
					   length of the second line. Check where the first line leads. */
					if (inside_sector(hit1, a2->point) != inside_sector(hit1, a2->next->point)) {
						PA_CHK_MARK(a2->point[0], a2->point[1]);
						PA_CHK_MARK(hit1->point[0], hit1->point[1]);
						return PA_CHK_ERROR(res, "lines is inside sector (2) at %mm;%mm", a2->point[0], a2->point[1]);
					}
				}
				else {
					/* Both lines intersect at an end-point. Check where they lead. */
					if (inside_sector(hit1, hit2->prev->point) != inside_sector(hit1, hit2->next->point)) {
						PA_CHK_MARK(hit1->point[0], hit2->point[1]);
						PA_CHK_MARK(hit2->point[0], hit2->point[1]);
						return PA_CHK_ERROR(res, "lines is inside sector (3) at %mm;%mm or %mm;%mm", hit1->point[0], hit1->point[1], hit2->point[0], hit2->point[1]);
					}
				}
			}
		}
		while ((a2 = a2->next) != a->head);
	}
	while ((a1 = a1->next) != a->head);
	return rnd_false;
}

rnd_bool rnd_polyarea_contour_check(rnd_pline_t *a)
{
	pa_chk_res_t res;
	return rnd_polyarea_contour_check_(a, &res);
}

#ifndef NDEBUG
static void rnd_poly_valid_report(rnd_pline_t *c, rnd_vnode_t *pl, pa_chk_res_t *chk)
{
	rnd_vnode_t *v, *n;
	rnd_coord_t minx = RND_COORD_MAX, miny = RND_COORD_MAX, maxx = -RND_COORD_MAX, maxy = -RND_COORD_MAX;

#define update_minmax(min, max, val) \
	if (val < min) min = val; \
	if (val > max) max = val;
	if (chk != NULL)
		rnd_fprintf(stderr, "Details: %s\n", chk->msg);
	rnd_fprintf(stderr, "!!!animator start\n");
	v = pl;
	do {
		n = v->next;
		update_minmax(minx, maxx, v->point[0]);
		update_minmax(miny, maxy, v->point[1]);
		update_minmax(minx, maxx, n->point[0]);
		update_minmax(miny, maxy, n->point[1]);
	}
	while ((v = v->next) != pl);
	rnd_fprintf(stderr, "scale 1 -1\n");
	rnd_fprintf(stderr, "viewport %mm %mm - %mm %mm\n", minx, miny, maxx, maxy);
	rnd_fprintf(stderr, "frame\n");
	v = pl;
	do {
		n = v->next;
		rnd_fprintf(stderr, "line %#mm %#mm %#mm %#mm\n", v->point[0], v->point[1], n->point[0], n->point[1]);
	}
	while ((v = v->next) != pl);

	if ((chk != NULL) && (chk->marks > 0)) {
		int n, MR=RND_MM_TO_COORD(0.05);
		fprintf(stderr, "color #770000\n");
		for(n = 0; n < chk->marks; n++) {
			rnd_fprintf(stderr, "line %#mm %#mm %#mm %#mm\n", chk->x[n]-MR, chk->y[n]-MR, chk->x[n]+MR, chk->y[n]+MR);
			rnd_fprintf(stderr, "line %#mm %#mm %#mm %#mm\n", chk->x[n]-MR, chk->y[n]+MR, chk->x[n]+MR, chk->y[n]-MR);
		}
	}

	if ((chk != NULL) && (chk->lines > 0)) {
		int n;
		fprintf(stderr, "color #990000\n");
		for(n = 0; n < chk->lines; n++)
			rnd_fprintf(stderr, "line %#mm %#mm %#mm %#mm\n", chk->x1[n], chk->y1[n], chk->x2[n], chk->y2[n]);
	}

	fprintf(stderr, "flush\n");
	fprintf(stderr, "!!!animator end\n");

#undef update_minmax
}
#endif


rnd_bool rnd_poly_valid(rnd_polyarea_t * p)
{
	rnd_pline_t *c;
	pa_chk_res_t chk;

	if ((p == NULL) || (p->contours == NULL)) {
#if 0
(disabled for too many false positive)
#ifndef NDEBUG
		rnd_fprintf(stderr, "Invalid polyarea: no contours\n");
#endif
#endif
		return rnd_false;
	}

	if (p->contours->flg.orient == RND_PLF_INV) {
#ifndef NDEBUG
		rnd_fprintf(stderr, "Invalid Outer rnd_pline_t: failed orient\n");
		rnd_poly_valid_report(p->contours, p->contours->head, NULL);
#endif
		return rnd_false;
	}

	if (rnd_polyarea_contour_check_(p->contours, &chk)) {
#ifndef NDEBUG
		rnd_fprintf(stderr, "Invalid Outer rnd_pline_t: failed contour check\n");
		rnd_poly_valid_report(p->contours, p->contours->head, &chk);
#endif
		return rnd_false;
	}

	for (c = p->contours->next; c != NULL; c = c->next) {
		if (c->flg.orient == RND_PLF_DIR) {
#ifndef NDEBUG
			rnd_fprintf(stderr, "Invalid Inner: rnd_pline_t orient = %d\n", c->flg.orient);
			rnd_poly_valid_report(c, c->head, NULL);
#endif
			return rnd_false;
		}
		if (rnd_polyarea_contour_check_(c, &chk)) {
#ifndef NDEBUG
			rnd_fprintf(stderr, "Invalid Inner: failed contour check\n");
			rnd_poly_valid_report(c, c->head, &chk);
#endif
			return rnd_false;
		}
		if (!rnd_poly_contour_in_contour(p->contours, c)) {
#ifndef NDEBUG
			rnd_fprintf(stderr, "Invalid Inner: overlap with outer\n");
			rnd_poly_valid_report(c, c->head, NULL);
#endif
			return rnd_false;
		}
	}
	return rnd_true;
}
