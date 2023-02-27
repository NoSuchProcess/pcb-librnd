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

rnd_bool rnd_polyarea_copy0(rnd_polyarea_t ** dst, const rnd_polyarea_t * src)
{
	*dst = NULL;
	if (src != NULL)
		*dst = (rnd_polyarea_t *) calloc(1, sizeof(rnd_polyarea_t));
	if (*dst == NULL)
		return rnd_false;
	(*dst)->contour_tree = rnd_r_create_tree();

	return pa_polyarea_copy_plines(*dst, src);
}


void rnd_polyarea_m_include(rnd_polyarea_t ** list, rnd_polyarea_t * a)
{
	if (*list == NULL)
		a->f = a->b = a, *list = a;
	else {
		a->f = *list;
		a->b = (*list)->b;
		(*list)->b = (*list)->b->f = a;
	}
}

rnd_bool rnd_polyarea_m_copy0(rnd_polyarea_t ** dst, const rnd_polyarea_t * srcfst)
{
	const rnd_polyarea_t *src = srcfst;
	rnd_polyarea_t *di;

	*dst = NULL;
	if (src == NULL)
		return rnd_false;
	do {
		if ((di = rnd_polyarea_create()) == NULL || !pa_polyarea_copy_plines(di, src))
			return rnd_false;
		rnd_polyarea_m_include(dst, di);
	}
	while ((src = src->f) != srcfst);
	return rnd_true;
}

rnd_bool rnd_polyarea_contour_include(rnd_polyarea_t * p, rnd_pline_t * c)
{
	rnd_pline_t *tmp;

	if ((c == NULL) || (p == NULL))
		return rnd_false;
	if (c->flg.orient == RND_PLF_DIR) {
		if (p->contours != NULL)
			return rnd_false;
		p->contours = c;
	}
	else {
		if (p->contours == NULL)
			return rnd_false;
		/* link at front of hole list */
		tmp = p->contours->next;
		p->contours->next = c;
		c->next = tmp;
	}
	rnd_r_insert_entry(p->contour_tree, (rnd_box_t *) c);
	return rnd_true;
}

void rnd_polyarea_init(rnd_polyarea_t * p)
{
	p->f = p->b = p;
	p->contours = NULL;
	p->contour_tree = rnd_r_create_tree();
}

rnd_polyarea_t *rnd_polyarea_create(void)
{
	rnd_polyarea_t *res;

	if ((res = (rnd_polyarea_t *) malloc(sizeof(rnd_polyarea_t))) != NULL)
		rnd_polyarea_init(res);
	return res;
}


void rnd_polyarea_free(rnd_polyarea_t ** p)
{
	rnd_polyarea_t *cur;

	if (*p == NULL)
		return;
	for (cur = (*p)->f; cur != *p; cur = (*p)->f) {
		rnd_poly_plines_free(&cur->contours);
		rnd_r_destroy_tree(&cur->contour_tree);
		cur->f->b = cur->b;
		cur->b->f = cur->f;
		free(cur);
	}
	rnd_poly_plines_free(&cur->contours);
	rnd_r_destroy_tree(&cur->contour_tree);
	free(*p), *p = NULL;
}

void rnd_polyarea_bbox(rnd_polyarea_t * p, rnd_box_t * b)
{
	rnd_pline_t *n;
	/*int cnt;*/

	n = p->contours;
	b->X1 = b->X2 = n->xmin;
	b->Y1 = b->Y2 = n->ymin;

	for (/*cnt = 0*/; /*cnt < 2 */ n != NULL; n = n->next) {
		if (n->xmin < b->X1)
			b->X1 = n->xmin;
		if (n->ymin < b->Y1)
			b->Y1 = n->ymin;
		if (n->xmax > b->X2)
			b->X2 = n->xmax;
		if (n->ymax > b->Y2)
			b->Y2 = n->ymax;
/*		if (n == p->contours)
			cnt++;*/
	}
}

rnd_bool rnd_polyarea_contour_inside(rnd_polyarea_t * p, rnd_vector_t v0)
{
	rnd_pline_t *cur;

	if ((p == NULL) || (v0 == NULL) || (p->contours == NULL))
		return rnd_false;
	cur = p->contours;
	if (pa_pline_is_point_inside(cur, v0)) {
		for (cur = cur->next; cur != NULL; cur = cur->next)
			if (pa_pline_is_point_inside(cur, v0))
				return rnd_false;
		return rnd_true;
	}
	return rnd_false;
}

/* determine if two polygons touch or overlap; used in pcb-rnd */
rnd_bool rnd_polyarea_touching(rnd_polyarea_t * a, rnd_polyarea_t * b)
{
	jmp_buf e;
	int code;

	if ((code = setjmp(e)) == 0) {
#ifdef DEBUG
		if (!rnd_poly_valid(a))
			return -1;
		if (!rnd_poly_valid(b))
			return -1;
#endif
		pa_polyarea_intersect(&e, a, b, rnd_false);

		if (pa_polyarea_label(a, b, rnd_true))
			return rnd_true;
		if (pa_polyarea_label(b, a, rnd_true))
			return rnd_true;
	}
	else if (code == PA_ISC_TOUCHES)
		return rnd_true;
	return rnd_false;
}


/*
 * rnd_polyarea_move()
 * (C) 2017 Tibor 'Igor2' Palinkas
*/
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
			pl->tree = (rnd_rtree_t *)rnd_poly_make_edge_tree(pl);

			rnd_r_insert_entry(pa->contour_tree, (rnd_box_t *)pl);
		}
	}
}

