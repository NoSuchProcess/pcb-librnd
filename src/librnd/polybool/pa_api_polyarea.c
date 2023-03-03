/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2017, 2023 Tibor 'Igor2' Palinkas
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
rnd_bool rnd_polyarea_touching(rnd_polyarea_t *a, rnd_polyarea_t *b)
{
	jmp_buf e;
	int code;

	code = setjmp(e);
	if (code == 0) {
#ifdef DEBUG
		if (!rnd_poly_valid(a) || !rnd_poly_valid(b)) return -1;
#endif
		pa_polyarea_intersect(&e, a, b, rnd_false);

		if (pa_polyarea_label(a, b, rnd_true)) return rnd_true;
		if (pa_polyarea_label(b, a, rnd_true)) return rnd_true;
	}
	else if (code == PA_ISC_TOUCHES)
		return rnd_true;
	return rnd_false;
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

