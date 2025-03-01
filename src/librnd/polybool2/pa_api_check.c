/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2023, 2024 Tibor 'Igor2' Palinkas
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

/* Check validity/integrity of polyareas and polylines */

/* Consider the previous and next edge around pn; consider a vector from pn
   to p2 called cdir. Return true if cdir is between the two edge vectors,
   inside the poligon. In other words, return if the direction from pn to
   p2 is going inside the polygon from pn. */
static rnd_bool pa_vect_inside_sect(rnd_vnode_t *pn, rnd_vector_t p2)
{
	rnd_vector_t cdir, ndir, pdir;
	int cdir_above_prev, cdir_above_next, poly_edge_pos;

	assert(pn != NULL);

	Vsub2(cdir, p2,              pn->point);            /* p2 to pn */
	pa_curve_tangent(pdir, pn, -1);
	pa_curve_tangent(ndir, pn, +1);

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
	int marks, lines, arcs;
#ifndef NDEBUG
	rnd_coord_t x[8], y[8]; /* marks */
	rnd_coord_t x1[8], y1[8], x2[8], y2[8]; /* lines */
	rnd_coord_t cx[9], cy[8]; double r[8]; /* arcs */
	char msg[256];
#endif
} pa_chk_res_t;


#ifndef NDEBUG
#define CHK_RES_INIT(chk) chk.msg[0] = chk.marks = chk.lines = chk.arcs = 0
#else
#define CHK_RES_INIT(chk) chk.marks = chk.lines = chk.arcs = 0
#endif

#ifndef NDEBUG
/* error reporting: remember offending objects */
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
#define PA_CHK_ARC(arc_) \
do { \
	if (res->arcs < sizeof(res->cx) / sizeof(res->cx[0])) { \
		res->cx[res->arcs] = arc_->curve.arc.center[0]; \
		res->cy[res->arcs] = arc_->curve.arc.center[1]; \
		res->r[res->arcs] = rnd_vect_dist2(arc_->curve.arc.center, arc_->point); \
		if (res->r[res->arcs] != 0) res->r[res->arcs] = sqrt(res->r[res->arcs]); \
		res->arcs++; \
	} \
} while(0)
#else
#define PA_CHK_MARK(x, y)
#define PA_CHK_LINE(x1, y1, x2, y2)
#define PA_CHK_ARC(arc)
#endif


RND_INLINE rnd_bool PA_CHK_ERROR(pa_chk_res_t *res, const char *first, ...)
{
#ifndef NDEBUG
	va_list ap;
	va_start(ap, first);
	vsnprints(res->msg, sizeof(res->msg), first, ap);
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

	if ((d < RND_POLY_VALID_ENDP_EPSILON) && (dn < RND_POLY_VALID_ENDP_EPSILON)) {
		/* line segment too short both ends are close - return the closer one */
		if (d < dn)
			return pn;
		return pn->next;
	}

	if (d < RND_POLY_VALID_ENDP_EPSILON)
		return pn;
	if (dn < RND_POLY_VALID_ENDP_EPSILON)
		return pn->next;
	return NULL;
}

/* There's an intersection at isc, caused by pt or pt->next or pt->prev.
   Returns 1 if either outgoing edge of isc overlaps with
   either outgoing edge of other. Such cases are accepted as non-error
   because of test case fixedy3 that would be very expensive to detect */
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

	if (pa_seg_seg_olap_(other->prev, isc)) return 1;
	if (pa_seg_seg_olap_(other->prev, isc->prev)) return 1;
	if (pa_seg_seg_olap_(other, isc)) return 1;
	if (pa_seg_seg_olap_(other, isc->prev)) return 1;

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

RND_INLINE rnd_bool pa_pline_check_stub(rnd_vnode_t *a1, pa_chk_res_t *res)
{
	/* we are always checking the overlap on a1->prev ~ a1 ~ a1->next; so the
	   curves are a1->prev and a1; if they don't have the same shape they
	   can not overlap */
	if (a1->flg.curve_type != a1->prev->flg.curve_type)
		return rnd_false;

	/* Stubs are invalid; see test case gixedm*; first the cheap test: full overlap;
	   this checks the two segs going out from a1 (a1 and a1->prev) */
	if ((a1->prev->point[0] == a1->next->point[0]) && (a1->prev->point[1] == a1->next->point[1])) {
		int bad = 0;
		switch(a1->flg.curve_type) {
			case RND_VNODE_LINE:
				PA_CHK_LINE(a1->point[0], a1->point[1], a1->next->point[0], a1->next->point[1]);
				PA_CHK_LINE(a1->point[0], a1->point[1], a1->prev->point[0], a1->prev->point[1]);
				bad = 1;
				break;
			case RND_VNODE_ARC:
				if ((a1->curve.arc.center[0] == a1->prev->curve.arc.center[0]) && (a1->curve.arc.center[1] == a1->prev->curve.arc.center[1]) && (a1->curve.arc.adir == !a1->prev->curve.arc.adir)) {
					PA_CHK_ARC(a1->prev);
					PA_CHK_ARC(a1);
					bad = 1;
				}
				break;
		}
		if (bad)
			return PA_CHK_ERROR(res, "lines overlap at stub ", Pvnodep(a1), " (full)", 0);
	}

	/* More expensive test: partly overlapping adjacent lines; checks if a1->next
	   or a1->prev is on the other seg */
	if (pa_is_node_on_curve(a1->next, a1->prev) || pa_is_node_on_curve(a1->prev, a1)) {
		switch(a1->flg.curve_type) {
			case RND_VNODE_LINE:
				PA_CHK_LINE(a1->point[0], a1->point[1], a1->next->point[0], a1->next->point[1]);
				PA_CHK_LINE(a1->point[0], a1->point[1], a1->prev->point[0], a1->prev->point[1]);
				break;
			case RND_VNODE_ARC:
				PA_CHK_ARC(a1->prev);
				PA_CHK_ARC(a1);
				break;
		}
		return PA_CHK_ERROR(res, "lines overlap at stub ", Pvnodep(a1), " (partial)", 0);
	}

	return rnd_false;
}

RND_INLINE rnd_bool both_endpoints_match(rnd_vnode_t *a1, rnd_vnode_t *a2)
{
	if (Vequ2(a1->point, a2->point) && Vequ2(a1->next->point, a2->next->point)) return 1;
	if (Vequ2(a1->point, a2->next->point) && Vequ2(a1->next->point, a2->point)) return 1;
	return 0;
}

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

	if (a->Count < 3) {
		int has_arc = 0;

		/* with arcs only 2 object is required */
		if (a->Count == 2) {
			a1 = a->head;
			do {
				if (a1->flg.curve_type == RND_VNODE_ARC) {
					has_arc = 1;
					break;
				}
			} while((a1 = a1->next) != a->head);
		}

		if (!has_arc)
			return PA_CHK_ERROR(res, "pline with points too few at ", Pvnodep(a->head), 0);
	}

	a1 = a->head;
	do {
		a2 = a1;

		if (pa_pline_check_stub(a1, res))
			return rnd_true;

		/* check for invalid intersections */
		do {
			int neigh;

			if (a1 == a2) continue;

			neigh = pa_are_nodes_neighbours(a1, a2);
			if (neigh && ((a1->flg.curve_type == RND_VNODE_LINE) && (a2->flg.curve_type == RND_VNODE_LINE)))
				continue; /* neighboring lines are okay to intersect once or twice (overlap) so don't run expensive tests */

			icnt = pa_curves_isc(a1, a2, i1, i2);
/*rnd_trace("@@@ icnt=%d types=%d %d\n", icnt, a1->flg.curve_type, a2->flg.curve_type);*/

			if (icnt == 0) continue;
			if ((icnt == 1) && neigh) continue; /* normal endpoint-endpoint isc */

			if (icnt > 1) {
				int bad = 0;

				if ((a1->flg.curve_type == RND_VNODE_LINE) && (a2->flg.curve_type == RND_VNODE_LINE)) {
					/* two intersections: (partially) overlapping lines - we have to accept that, see test case fixedy3 */
				}
				else if ((a1->flg.curve_type == RND_VNODE_ARC) && (a2->flg.curve_type == RND_VNODE_ARC)) {
					/* arc-arc: */
					if (!Vequ2(a1->curve.arc.center, a2->curve.arc.center)) {
						/* two arcs intersecting in two points */
						if (!both_endpoints_match(a1, a2)) /* O shaped case; test case: arc16 */
							bad = 1;
					}
					else {
						/* arcs on the same circle -> overlap, have to accept */
					}
				}
				else {
					/* line-arc */
					if (!both_endpoints_match(a1, a2)) /* D shaped case; test case: arc16 */
						bad = 1;
				}

				if (bad) {
					if (a1->flg.curve_type == RND_VNODE_LINE)
						PA_CHK_LINE(a1->point[0], a1->point[1], a1->next->point[0], a1->next->point[1]);
					else
						PA_CHK_ARC(a1);

					if (a2->flg.curve_type == RND_VNODE_LINE)
						PA_CHK_LINE(a2->point[0], a2->point[1], a2->next->point[0], a2->next->point[1]);
					else
						PA_CHK_ARC(a2);

					return PA_CHK_ERROR(res, "neighbor curves intersect ", Pvnodep(a1), " and ", Pvnodep(a2), 0);
				}
			}
#if 0
			if (icnt > 1) {
				PA_CHK_MARK(a1->point[0], a1->point[1]);
				PA_CHK_MARK(a2->point[0], a2->point[1]);
				return PA_CHK_ERROR(res, "icnt > 1 (", Pint(icnt) ,") at ", Pnodep(a1), " or ",  Pnodep(a2), 0);
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
				return PA_CHK_ERROR(res, "lines cross between ", Pvnodep(a1), " and ", Pvnodep(a2), 0);
			}
			else if (hit1 == NULL) {
				/* An end-point of a2 touched somewhere along the length of a1. Check
				   two edges of a1, one before and one after the intersection; if one
				   is inside and one is outside, that's a1 crossing a2 (else it only
				   touches and bounces back) */
				if ((pa_vect_inside_sect(hit2, a1->point) != pa_vect_inside_sect(hit2, a1->next->point)) && !pa_chk_ll_olap(hit2, a2, a1)) {
					PA_CHK_MARK(a1->point[0], a1->point[1]);
					PA_CHK_MARK(hit2->point[0], hit2->point[1]);
					return PA_CHK_ERROR(res, "plines crossing (1) at ", Pvnodep(a1), 0);
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
					return PA_CHK_ERROR(res, "plines crossing (2) at ", Pvnodep(a2), 0);
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
						return PA_CHK_ERROR(res, "plines crossing/twisting (3) at ", Pvnodep(hit1), " or ", Pvnodep(hit2), 0);
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
	CHK_RES_INIT(res);
	return pa_pline_check_(a, &res);
}

#ifndef NDEBUG
static void rnd_poly_valid_report(rnd_pline_t *c, pa_chk_res_t *chk)
{
	rnd_vnode_t *vn;
	rnd_coord_t minx = RND_COORD_MAX, miny = RND_COORD_MAX, maxx = -RND_COORD_MAX, maxy = -RND_COORD_MAX;
	int small, MR;
	FILE *F;
	double fill_opacity=0.4, poly_bloat = 1, SW;
	const char *clr = "green";


	F = stderr;

#define is_small(v)  (((v) > -100000) && ((v) < 100000))

#define update_minmax(min, max, val) \
	if (val < min) min = val; \
	if (val > max) max = val;

	vn = c->head;
	do {
		update_minmax(minx, maxx, vn->point[0]);
		update_minmax(miny, maxy, vn->point[1]);
	} while((vn = vn->next) != c->head);

#undef update_minmax


	fprintf(F, "<?xml version=\"1.0\"?>\n");
	fprintf(F, "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.0\" width=\"1000\" height=\"1000\" viewBox=\"0 0 1000 1000\">\n");

	if (chk != NULL)
		fprintf(stderr, "<!-- Details: %s -->\n", chk->msg);

	small = is_small(minx) && is_small(miny) && is_small(maxx) && is_small(maxy);

	fprintf(F, "\n<g fill-rule=\"even-odd\" stroke-width=\"%.3f\" stroke=\"%s\" fill=\"%s\" fill-opacity=\"%.3f\" stroke-opacity=\"%.3f\">\n<path d=\"", poly_bloat, clr, clr, fill_opacity, fill_opacity*2);
	pb2_svg_print_pline(F, c);
	fprintf(F, "\n\"/></g>\n");

#ifdef RND_API_VER
		MR = small ? 100 : RND_MM_TO_COORD(0.05);
		SW = small ? 0.1 : RND_MM_TO_COORD(0.005);
#else
		MR = small ? 100 : 1000;
		SW = small ? 0.1 : 100;
#endif

	if ((chk != NULL) && (chk->marks > 0)) {
		int n;

		for(n = 0; n < chk->marks; n++) {
			rnd_fprintf(F, "<!-- X at %#mm %#mm-->\n", (long)chk->x[n], (long)chk->y[n]);
			fprintf(F, " <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\" stroke-width=\"%.3f\" stroke=\"red\"/>\n",
				(long)chk->x[n]-MR, (long)chk->y[n]-MR, (long)chk->x[n]+MR, (long)chk->y[n]+MR, SW);
			fprintf(F, " <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\" stroke-width=\"%.3f\" stroke=\"red\"/>\n",
				(long)chk->x[n]-MR, (long)chk->y[n]+MR, (long)chk->x[n]+MR, (long)chk->y[n]-MR, SW);
		}
	}

	if ((chk != NULL) && (chk->lines > 0)) {
		int n;
		for(n = 0; n < chk->lines; n++)
			fprintf(F, " <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\" stroke-width=\"%.3f\" stroke=\"purple\"/>\n",
				(long)chk->x1[n], (long)chk->y1[n], (long)chk->x2[n], (long)chk->y2[n], SW);
	}

	if ((chk != NULL) && (chk->arcs > 0)) {
		int n;
		for(n = 0; n < chk->arcs; n++)
			fprintf(F, " <circle cx=\"%ld\" cy=\"%ld\" r=\"%ld\" stroke=\"none\" fill=\"purple\" />\n",
				(long)chk->cx[n], (long)chk->cy[n], (long)chk->r[n]);
	}

	fprintf(F, "</svg>\n");

#undef is_small
}
#endif


rnd_bool rnd_poly_valid_island(rnd_polyarea_t *p)
{
	rnd_pline_t *n, *h;
	rnd_rtree_it_t it;
	rnd_rtree_box_t bbox;
	pa_chk_res_t chk;

	CHK_RES_INIT(chk);

	/* Broken cyclic list: if p's prev or next is itself, then the other neighbour
	   ptr needs to be itself too */
	if ((p->b == p) && (p->f != p)) {
#ifndef NDEBUG
		fprintf(stderr, "Invalid polyarea ->f\n");
#endif
		return rnd_false;
	}
	if ((p->f == p) && (p->b != p)) {
#ifndef NDEBUG
		fprintf(stderr, "Invalid polyarea ->f\n");
#endif
		return rnd_false;
	}

	if ((p == NULL) || (p->contours == NULL)) {
		/* technically an empty polyarea should be valid, tho */
		return rnd_false;
	}

	if (p->contours->flg.orient == RND_PLF_INV) {
#ifndef NDEBUG
		fprintf(stderr, "Invalid Outer pline: wrong orientation (shall be positive)\n");
		rnd_poly_valid_report(p->contours, NULL);
#endif
		return rnd_false;
	}

	if ((p->contours->tree != NULL) && (p->contours->Count != p->contours->tree->size)) {
#ifndef NDEBUG
		fprintf(stderr, "Invalid Outer pline: rtree size mismatch %ld != %ld\n", (long)p->contours->Count, (long)p->contours->tree->size);
#endif
		return rnd_false;
	}

	if (pa_pline_check_(p->contours, &chk)) {
#ifndef NDEBUG
		fprintf(stderr, "Invalid Outer pline: self-intersection\n");
		rnd_poly_valid_report(p->contours, &chk);
#endif
		return rnd_false;
	}

	/* check all holes */
	for(n = p->contours->next; n != NULL; n = n->next) {
		if (n->flg.orient == RND_PLF_DIR) {
#ifndef NDEBUG
			fprintf(stderr, "Invalid Inner (hole): pline orient (shall be negative)\n");
			rnd_poly_valid_report(n, NULL);
#endif
			return rnd_false;
		}
		if ((p->contours->tree != NULL) && (p->contours->Count != p->contours->tree->size)) {
#ifndef NDEBUG
			fprintf(stderr, "Invalid Inner (hole): pline rtree size mismatch %ld != %ld\n", (long)p->contours->Count, (long)p->contours->tree->size);
#endif
			return rnd_false;
		}
		if (pa_pline_check_(n, &chk)) {
#ifndef NDEBUG
			fprintf(stderr, "Invalid Inner (hole): self-intersection\n");
			rnd_poly_valid_report(n, &chk);
#endif
			return rnd_false;
		}
		if ((p->contours->tree != NULL) && (!pa_pline_inside_pline(p->contours, n))) {
#ifndef NDEBUG
			fprintf(stderr, "Invalid Inner (hole): overlap with outer\n");
			rnd_poly_valid_report(n, NULL);
#endif
			return rnd_false;
		}

		/* verify that hole is not intersecting with other holes */
		bbox.x1 = n->xmin; bbox.y1 = n->ymin;
		bbox.x2 = n->xmax; bbox.y2 = n->ymax;
		for(h = rnd_rtree_first(&it, p->contour_tree, &bbox); h != NULL; h = rnd_rtree_next(&it)) {
			pa_island_isc_t ires;

			if (h >= n) continue; /* check each pair only once; don't check hole against itself */
			if (h->flg.orient != RND_PLF_INV) continue; /* check hole-hole only */

			ires = pa_pline_pline_isc(h, n, 0);
			if ((ires == PA_ISLAND_ISC_CROSS) || (ires == PA_ISLAND_ISC_IN)) {
#ifndef NDEBUG
				fprintf(stderr, "Invalid Inner (hole): overlaps with another hole\n");
				rnd_poly_valid_report(n, NULL);
#endif
				return rnd_false;
			}
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
					fprintf(stderr, "island-island intersect\n");
#endif
					return rnd_false;
				}
				q = q->f;
			}
		} while((p = p->f) != pa);
	}

	return rnd_true;
}

