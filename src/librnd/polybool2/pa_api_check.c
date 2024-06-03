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

/* Check validity/integrity of polyareas and polylines */

/* Consider the previous and next edge around pn; consider a vector from pn
   to p2 called cdir. Return true if cdir is between the two edge vectors,
   inside the poligon. In other words, return if the direction from pn to
   p2 is going inside the polygon from pn. */
static rnd_bool pa_vect_inside_sect(rnd_vnode_t *pn, rnd_vector_t p2)
{
	rnd_vector_t cdir, ndir, pdir;
	int cdir_above_prev, cdir_above_next, poly_edge_pos;

	TODO("arc: extend this to work with arcs");

	assert(pn != NULL);

	Vsub2(cdir, p2,              pn->point);            /* p2 to pn */
	Vsub2(pdir, pn->point,       pn->prev->point);      /* pn to pn prev */
	Vsub2(ndir, pn->next->point, pn->point);            /* pn next to pn */

	/* Whether target vector (cdir) is "above" previous and next edge vectors */
	cdir_above_prev = rnd_vect_det2(pdir, cdir) >= 0;
	cdir_above_next = rnd_vect_det2(ndir, cdir) >= 0;

	/* Whetner next is "above" previous on the poly edge at pn */
	poly_edge_pos = rnd_vect_det2(pdir, ndir) >= 0;

	/* See doc/developer/polybool/pa_vect_inside_sect.svg; the code below
	   checks if cdir is on the right combination of the thin arcs on the
	   bottom row drawings; the right combination is the one that overlaps
	   the in==true thick arc. */
	if (poly_edge_pos)
		return (cdir_above_prev && cdir_above_next);
	else
		return (cdir_above_prev || cdir_above_next);

	return rnd_false; /* can't get here */
}

/*** pa_chk: remember coords where the contour broke to ease debugging ***/
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


/* If an intersection happens close to endpoints of a pline edge, return
   the node involved, else return NULL */
rnd_vnode_t *pa_check_find_close_node(rnd_vector_t intersection, rnd_vnode_t *pn)
{
	double d, dn;

	/* cheap and precise: direct match */
	if (Vequ2(pn->point, intersection))
		return pn;
	if (Vequ2(pn->next->point, intersection))
		return pn->next;

	/* no direct hit due to precision problems - calculate distance instead */
	d = rnd_vect_dist2(intersection, pn->point);
	dn = rnd_vect_dist2(intersection, pn->next->point);

	if ((d < RND_POLY_ENDP_EPSILON) && (dn < RND_POLY_ENDP_EPSILON)) {
		/* line segment too short both ends are close - return the closer one */
		if (d < dn)
			return pn;
		return pn->next;
	}

	if (d < RND_POLY_ENDP_EPSILON)
		return pn;
	if (dn < RND_POLY_ENDP_EPSILON)
		return pn->next;
	return NULL;
}

/* There's an intersection at isc, caused by pt or pt->next or pt->prev.
   Returns 1 if either outgoing edge of isc overlaps with
   either outgoing edge of other. Such cases are accepted as non-error
   becausew of test case fixedy3 that would be very expensive to detect */
RND_INLINE int pa_chk_ll_olap(rnd_vnode_t *isc, rnd_vnode_t *pt, rnd_vnode_t *other)
{
	rnd_vector_t tmp1, tmp2;

/*
	rnd_trace("ll olap: %ld;%ld other: %ld;%ld - %ld;%ld; - %ld;%ld\n",
		isc->point[0], isc->point[1],
		other->prev->point[0], other->prev->point[1],
		other->point[0], other->point[1],
		other->next->point[0], other->next->point[1]
		);
*/

	if (pa_isc_edge_edge(other->prev, other, isc, isc->next, &tmp1, &tmp2) == 2)
		return 1;

	if (pa_isc_edge_edge(other->prev, other, isc, isc->prev, &tmp1, &tmp2) == 2)
		return 1;

	if (pa_isc_edge_edge(other->next, other, isc, isc->next, &tmp1, &tmp2) == 2)
		return 1;

	if (pa_isc_edge_edge(other->next, other, isc, isc->prev, &tmp1, &tmp2) == 2)
		return 1;

	return 0;
}

RND_INLINE int chk_on_same_pt(rnd_vnode_t *va, rnd_vnode_t *vb)
{
	if (va == vb) return 1;
	return (va->point[0] == vb->point[0]) && (va->point[1] == vb->point[1]);
}

/* Walk the polyline from start, either jumping on ->next (dir>0) or on ->prev
   (dir<0) and compute the polarity of that loop. Returns +1 for outline, -1
   for hole. */
RND_INLINE int pa_chk_loop_polarity(rnd_vnode_t *start, int dir)
{
	double area = 0;
	rnd_vnode_t *c, *p;

	p = start;
	c = (dir > 0) ? p->next : p->prev;
/*	rnd_trace("LOOP POLARITY: %d;%d dir=%d ", c->point[0], c->point[1], dir);*/
	for(; !chk_on_same_pt(c, start); c = (dir > 0) ? c->next : c->prev) {
		area += (double)(p->point[0] - c->point[0]) * (double)(p->point[1] + c->point[1]);
		p = c;
	}

	/* close the loop */
	c = start;
	area += (double)(p->point[0] - c->point[0]) * (double)(p->point[1] + c->point[1]);


/*	rnd_trace(" area=%f -> ", area);
	if (dir > 0)
		rnd_trace(" %d\n", area > 0 ? +1 : -1);
	else
		rnd_trace(" %d\n", area > 0 ? -1 : +1);
*/

	if (dir > 0)
		return area > 0 ? +1 : -1;

	return area > 0 ? -1 : +1;
}

/*** Contour check ***/

/* returns rnd_true if contour is invalid: a self-touching contour is valid, but
   a self-intersecting contour is not. */
RND_INLINE rnd_bool pa_pline_check_(rnd_pline_t *a, pa_chk_res_t *res)
{
	rnd_vnode_t *a1, *a2, *hit1, *hit2;
	rnd_vector_t i1, i2;
	int icnt;

#ifndef NDEBUG
	*res->msg = '\0';
#endif
	res->marks = res->lines = 0;

	assert(a != NULL);

	if (a->Count < 3)
		return PA_CHK_ERROR(res, "pline with points too few at %ld;%ld", a->head->point[0], a->head->point[1]);

	a1 = a->head;
	do {
		a2 = a1;

		/* Stubs are invalid; see test case gixedm*; first the cheap test: full overlap */
		if ((a1->prev->point[0] == a1->next->point[0]) && (a1->prev->point[1] == a1->next->point[1])) {
			PA_CHK_LINE(a1->point[0], a1->point[1], a1->next->point[0], a1->next->point[1]);
			PA_CHK_LINE(a1->point[0], a1->point[1], a1->prev->point[0], a1->prev->point[1]);
			return PA_CHK_ERROR(res, "lines overlap at stub %mm;%mm (full)", a1->point[0], a1->point[1]);
		}

		/* More expensive test: partly overlapping adjacent lines */
		if (pa_is_node_on_line(a1->next, a1, a1->prev) || pa_is_node_on_line(a1->prev, a1, a1->next)) {
			PA_CHK_LINE(a1->point[0], a1->point[1], a1->next->point[0], a1->next->point[1]);
			PA_CHK_LINE(a1->point[0], a1->point[1], a1->prev->point[0], a1->prev->point[1]);
			return PA_CHK_ERROR(res, "lines overlap at stub %mm;%mm (partial)", a1->point[0], a1->point[1]);
		}

		do {

			/* count invalid intersections */
			if (pa_are_nodes_neighbours(a1, a2)) continue; /* neighbors are okay to intersect */
			icnt = rnd_vect_inters2(a1->point, a1->next->point, a2->point, a2->next->point, i1, i2);
			if (icnt == 0) continue;

			/* two intersections; must be overlapping lines - we have to accept that, see test case fixedy3 */
#if 0
			if (icnt > 1) { 
				PA_CHK_MARK(a1->point[0], a1->point[1]);
				PA_CHK_MARK(a2->point[0], a2->point[1]);
				return PA_CHK_ERROR(res, "icnt > 1 (%d) at %ld;%ld or  %ld;%ld", icnt, a1->point[0], a1->point[1], a2->point[0], a2->point[1]);
			}
#endif

			/* we have one intersection; figure if it happens on a node next to a1
			   or a2 and store them in hit1 and hit2 */
			hit1 = pa_check_find_close_node(i1, a1);
			hit2 = pa_check_find_close_node(i1, a2);

			if ((hit1 == NULL) && (hit2 == NULL)) {
				/* intersection in the middle of two lines */
				PA_CHK_LINE(a1->point[0], a1->point[1], a1->next->point[0], a1->next->point[1]);
				PA_CHK_LINE(a2->point[0], a2->point[1], a2->next->point[0], a2->next->point[1]);
				return PA_CHK_ERROR(res, "lines cross between %mm;%mm and %mm;%mm", a1->point[0], a1->point[1], a2->point[0], a2->point[1]);
			}
			else if (hit1 == NULL) {
				/* An end-point of a2 touched somewhere along the length of a1. Check
				   two edges of a1, one before and one after the intersection; if one
				   is inside and one is outside, that's a1 crossing a2 (else it only
				   touches and bounces back) */
				if ((pa_vect_inside_sect(hit2, a1->point) != pa_vect_inside_sect(hit2, a1->next->point)) && !pa_chk_ll_olap(hit2, a2, a1)) {
					PA_CHK_MARK(a1->point[0], a1->point[1]);
					PA_CHK_MARK(hit2->point[0], hit2->point[1]);
					return PA_CHK_ERROR(res, "plines crossing (1) at %mm;%mm (%ld;%ld)", a1->point[0], a1->point[1], a1->point[0], a1->point[1]);
				}
			}
			else if (hit2 == NULL) {
				/* An end-point of a1 touched somewhere along the length of a2. Check
				   two edges of a2, one before and one after the intersection; if one
				   is inside and one is outside, that's a2 crossing a1 (else it only
				   touches and bounces back) */
				if ((pa_vect_inside_sect(hit1, a2->point) != pa_vect_inside_sect(hit1, a2->next->point)) && !pa_chk_ll_olap(hit1, a1, a2)) {
					PA_CHK_MARK(a2->point[0], a2->point[1]);
					PA_CHK_MARK(hit1->point[0], hit1->point[1]);
					return PA_CHK_ERROR(res, "plines crossing (2) at %mm;%mm (%ld;%ld)", a2->point[0], a2->point[1], a2->point[0], a2->point[1]);
				}
			}
			else {
				/* Same as the above two cases, but compare hit1 to hit2 to find if it's a crossing */
				if (pa_vect_inside_sect(hit1, hit2->prev->point) != pa_vect_inside_sect(hit1, hit2->next->point)) {
					int polarity[4];

					/* This might be the polyline crossing itself. But there is a corner
					   case, gixedz2, that technically does cross itself but really yields
					   3 (or generally an odd number of) valid polygon fans. The simplest
					   invalid case is bowtie. Check all fan blade areas if: blades have
					   the same polarity, this is a valid crossing; any flip/twist makes
					   the crossing invalid. */
					polarity[0] = pa_chk_loop_polarity(hit1, -1);
					polarity[1] = pa_chk_loop_polarity(hit1, +1);
					polarity[2] = pa_chk_loop_polarity(hit2, -1);
					polarity[3] = pa_chk_loop_polarity(hit2, +1);

					if ((polarity[1] != polarity[0]) || (polarity[2] != polarity[0]) || (polarity[3] != polarity[0])) {
						PA_CHK_MARK(hit1->point[0], hit2->point[1]);
						PA_CHK_MARK(hit2->point[0], hit2->point[1]);
						return PA_CHK_ERROR(res, "plines crossing/twisting (3) at %mm;%mm (%ld;%ld) or %mm;%mm (%ld;%ld)", hit1->point[0], hit1->point[1], hit1->point[0], hit1->point[1], hit2->point[0], hit2->point[1], hit2->point[0], hit2->point[1]);
					}
				}
			}
		} while((a2 = a2->next) != a->head);
	} while((a1 = a1->next) != a->head);

	return rnd_false;
}

rnd_bool rnd_polyarea_contour_check(rnd_pline_t *a)
{
	pa_chk_res_t res;
	return pa_pline_check_(a, &res);
}

#ifndef NDEBUG
static void rnd_poly_valid_report(rnd_pline_t *c, rnd_vnode_t *pl, pa_chk_res_t *chk)
{
	rnd_vnode_t *v, *n;
	rnd_coord_t minx = RND_COORD_MAX, miny = RND_COORD_MAX, maxx = -RND_COORD_MAX, maxy = -RND_COORD_MAX;
	int small;

#define is_small(v)  (((v) > -100000) && ((v) < 100000))
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
	} while ((v = v->next) != pl);

	small = is_small(minx) && is_small(miny) && is_small(maxx) && is_small(maxy);

	rnd_fprintf(stderr, "scale 1 -1\n");
	if (small)
		rnd_fprintf(stderr, "viewport %ld %ld - %ld %ld\n", (long)minx, (long)miny, (long)maxx, (long)maxy);
	else
		rnd_fprintf(stderr, "viewport %mm %mm - %mm %mm\n", minx, miny, maxx, maxy);
	rnd_fprintf(stderr, "frame\n");
	v = pl;
	do {
		n = v->next;
		if (small)
			rnd_fprintf(stderr, "line %ld %ld %ld %ld\n", (long)v->point[0], (long)v->point[1], (long)n->point[0], (long)n->point[1]);
		else
			rnd_fprintf(stderr, "line %#mm %#mm %#mm %#mm\n", v->point[0], v->point[1], n->point[0], n->point[1]);
	}
	while ((v = v->next) != pl);

	if ((chk != NULL) && (chk->marks > 0)) {
		int n, MR = small ? 100 : RND_MM_TO_COORD(0.05);
		fprintf(stderr, "color #770000\n");
		for(n = 0; n < chk->marks; n++) {
			if (small) {
				rnd_fprintf(stderr, "! X at %ld %ld\n", (long)chk->x[n], (long)chk->y[n]);
				rnd_fprintf(stderr, "line %ld %ld %ld %ld\n", (long)chk->x[n]-MR, (long)chk->y[n]-MR, (long)chk->x[n]+MR, (long)chk->y[n]+MR);
				rnd_fprintf(stderr, "line %ld %ld %ld %ld\n", (long)chk->x[n]-MR, (long)chk->y[n]+MR, (long)chk->x[n]+MR, (long)chk->y[n]-MR);
			}
			else {
				rnd_fprintf(stderr, "! X at %#mm %#mm\n", (long)chk->x[n], (long)chk->y[n]);
				rnd_fprintf(stderr, "line %#mm %#mm %#mm %#mm\n", chk->x[n]-MR, chk->y[n]-MR, chk->x[n]+MR, chk->y[n]+MR);
				rnd_fprintf(stderr, "line %#mm %#mm %#mm %#mm\n", chk->x[n]-MR, chk->y[n]+MR, chk->x[n]+MR, chk->y[n]-MR);
			}
		}
	}

	if ((chk != NULL) && (chk->lines > 0)) {
		int n;
		fprintf(stderr, "color #990000\n");
		for(n = 0; n < chk->lines; n++)
			if (small)
				fprintf(stderr, "line %ld %ld %ld %ld\n", (long)chk->x1[n], (long)chk->y1[n], (long)chk->x2[n], (long)chk->y2[n]);
			else
				rnd_fprintf(stderr, "line %#mm %#mm %#mm %#mm\n", chk->x1[n], chk->y1[n], chk->x2[n], chk->y2[n]);
	}

	fprintf(stderr, "flush\n");
	fprintf(stderr, "!!!animator end\n");

#undef update_minmax
#undef is_small
}
#endif


rnd_bool rnd_poly_valid_island(rnd_polyarea_t *p)
{
	rnd_pline_t *n;
	pa_chk_res_t chk;

	/* Broken cyclic list: if p's prev or next is itself, then the other neighbour
	   ptr needs to be itself too */
	if ((p->b == p) && (p->f != p)) {
#ifndef NDEBUG
		rnd_fprintf(stderr, "Invalid polyarea ->f\n");
#endif
		return rnd_false;
	}
	if ((p->f == p) && (p->b != p)) {
#ifndef NDEBUG
		rnd_fprintf(stderr, "Invalid polyarea ->f\n");
#endif
		return rnd_false;
	}

	if ((p == NULL) || (p->contours == NULL)) {
		/* technically an empty polyarea should be valid, tho */
		return rnd_false;
	}

	if (p->contours->flg.orient == RND_PLF_INV) {
#ifndef NDEBUG
		rnd_fprintf(stderr, "Invalid Outer pline: wrong orientation (shall be positive)\n");
		rnd_poly_valid_report(p->contours, p->contours->head, NULL);
#endif
		return rnd_false;
	}

	if ((p->contours->tree != NULL) && (p->contours->Count != p->contours->tree->size)) {
#ifndef NDEBUG
		rnd_fprintf(stderr, "Invalid Outer pline: rtree size mismatch %ld != %ld\n", (long)p->contours->Count, (long)p->contours->tree->size);
#endif
		return rnd_false;
	}

	if (pa_pline_check_(p->contours, &chk)) {
#ifndef NDEBUG
		rnd_fprintf(stderr, "Invalid Outer pline: self-intersection\n");
		rnd_poly_valid_report(p->contours, p->contours->head, &chk);
#endif
		return rnd_false;
	}

	/* check all holes */
	for(n = p->contours->next; n != NULL; n = n->next) {
		if (n->flg.orient == RND_PLF_DIR) {
#ifndef NDEBUG
			rnd_fprintf(stderr, "Invalid Inner (hole): pline orient (shall be negative)\n");
			rnd_poly_valid_report(n, n->head, NULL);
#endif
			return rnd_false;
		}
		if ((p->contours->tree != NULL) && (p->contours->Count != p->contours->tree->size)) {
#ifndef NDEBUG
			rnd_fprintf(stderr, "Invalid Inner (hole): pline rtree size mismatch %ld != %ld\n", (long)p->contours->Count, (long)p->contours->tree->size);
#endif
			return rnd_false;
		}
		if (pa_pline_check_(n, &chk)) {
#ifndef NDEBUG
			rnd_fprintf(stderr, "Invalid Inner (hole): self-intersection\n");
			rnd_poly_valid_report(n, n->head, &chk);
#endif
			return rnd_false;
		}
		if ((p->contours->tree != NULL) && (!pa_pline_inside_pline(p->contours, n))) {
#ifndef NDEBUG
			rnd_fprintf(stderr, "Invalid Inner (hole): overlap with outer\n");
			rnd_poly_valid_report(n, n->head, NULL);
#endif
			return rnd_false;
		}
	}
	return rnd_true;
}

rnd_bool rnd_poly_valid(rnd_polyarea_t *pa)
{
	rnd_polyarea_t *p, *q;

	/* verify all polylines */
	p = pa;
	do {
		if (!rnd_poly_valid_island(p))
			return rnd_false;
	} while((p = p->f) != pa);

	/* verify that no two island intersect (if there are more than one islands) */
	if (pa->f != pa) {
		p = pa;
		do {
			q = p->f;
			while(q != pa) {
				if (rnd_polyarea_island_isc(p, q)) {
#ifndef NDEBUG
					rnd_fprintf(stderr, "island-island intersect\n");
#endif
					return rnd_false;
				}
				q = q->f;
			}
		} while((p = p->f) != pa);
	}

	return rnd_true;
}

