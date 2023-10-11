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

/* rnd_polyarea_t internal/low level */

/* Return whether a and b has any chance to intersect comparing their bboxes */
RND_INLINE int pa_polyarea_box_overlap(rnd_polyarea_t *a, rnd_polyarea_t *b)
{
	if (a->contours->xmax < b->contours->xmin) return 0;
	if (a->contours->ymax < b->contours->ymin) return 0;
	if (a->contours->xmin > b->contours->xmax) return 0;
	if (a->contours->ymin > b->contours->ymax) return 0;

	return 1;
}

/* Internal callback for counting countours the pline cl is inside */
static rnd_r_dir_t pa_pline_count_inside_cb(const rnd_box_t *b, void *cl)
{
	rnd_pline_t *me = (rnd_pline_t *)cl;
	rnd_pline_t *check = (rnd_pline_t *)b;

	if (pa_pline_inside_pline(check, me))
		return RND_R_DIR_FOUND_CONTINUE;

	return RND_R_DIR_NOT_FOUND;
}

/* Returns whether pl is inside pa; if first_only is set, only the first island
   of pa is considered */
static int pa_is_pline_in_polyarea(rnd_pline_t *pl, rnd_polyarea_t *pa, rnd_bool first_only)
{
	rnd_polyarea_t *outer = pa;
	rnd_heap_t *heap = rnd_heap_create();

	assert(pl != NULL);
	assert(outer != NULL);

	do {
		if (pa_pline_box_inside(pl, outer->contours))
			rnd_heap_insert(heap, outer->contours->area, (void *)outer);
	}	while(!first_only && (outer = outer->f) != pa);

	/* start checking from smallest to largest island whether pl is within
	   that island; if it's within 2 contours that means it's in a hole */
	while(!rnd_heap_is_empty(heap)) {
		int cnt;

		/* count how many positive and negative contours are around pl in the smallest island */
		outer = (rnd_polyarea_t *)rnd_heap_remove_smallest(heap);
		rnd_r_search(outer->contour_tree, (rnd_box_t *)pl, NULL, pa_pline_count_inside_cb, pl, &cnt);

		switch (cnt) {
			case 0: break;            /* pl is outside of any contour of this island */
			case 1: goto inside;     /* pl is inside an island (+1), and not any of its holes */
			case 2: goto not_inside; /* pl is inside a hole (+1) in the smallest island (+1) so far. No need to check the other polygons */
			default:
				fprintf(stderr, "pa_is_pline_in_polyarea() internal error - please report this bug\n");
				assert(0);
				break;
		}
	}

	not_inside:;
	rnd_heap_destroy(&heap);
	return rnd_false;

	inside:;
	rnd_heap_destroy(&heap);
	return rnd_true;
}

void pa_polyarea_del_pline(rnd_polyarea_t *pa, rnd_pline_t *pl)
{
	rnd_pline_t *n, *prev;

	/* unlink */
	for(n = pa->contours, prev = NULL; n != NULL; n = n->next)
		if (n == pl)
			prev->next = pl->next;

	rnd_r_delete_entry(pa->contour_tree, (rnd_box_t *)pl);
}

rnd_bool pa_polyarea_copy_plines(rnd_polyarea_t *dst, const rnd_polyarea_t *src)
{
	rnd_pline_t *n, *last, *newpl;

	dst->f = dst->b = dst;

	for(n = src->contours, last = NULL; n != NULL; n = n->next) {
		newpl = pa_pline_dup(n);
		if (newpl == NULL)
			return rnd_false; /* allocation failure */

		/* link newpl in */
		if (last == NULL)
			dst->contours = newpl;
		else
			last->next = newpl;
		last = newpl;

		rnd_r_insert_entry(dst->contour_tree, (rnd_box_t *)newpl);
	}

	return rnd_true;
}

rnd_polyarea_t *rnd_polyarea_dup(const rnd_polyarea_t *src)
{
	rnd_polyarea_t *res = NULL;
	if (src != NULL)
		res = (rnd_polyarea_t *) calloc(1, sizeof(rnd_polyarea_t));

	if (res == NULL)
		return NULL;

	res->contour_tree = rnd_r_create_tree();
	pa_polyarea_copy_plines(res, src);
	return res;
}

rnd_bool pa_polyarea_alloc_copy(rnd_polyarea_t **dst, const rnd_polyarea_t *src)
{
	*dst = rnd_polyarea_dup(src);
	return (*dst != NULL);
}

void rnd_polyarea_m_include(rnd_polyarea_t **list, rnd_polyarea_t *a)
{
	if (*list != NULL) {
		/* insert at *list */
		a->f = *list;
		a->b = (*list)->b;
		(*list)->b = (*list)->b->f = a;
	}
	else {
		/* create new list */
		a->f = a->b = a;
		*list = a;
	}
}

rnd_polyarea_t *pa_polyarea_dup_all(const rnd_polyarea_t *src)
{
	rnd_polyarea_t *dst = NULL;
	const rnd_polyarea_t *s = src;

	do {
		rnd_polyarea_t *dpa = rnd_polyarea_dup(s);
		if (dpa == NULL)
			return NULL;

		rnd_polyarea_m_include(&dst, dpa);
	} while((s = s->f) != src);

	return dst;
}

rnd_bool rnd_polyarea_alloc_copy_all(rnd_polyarea_t **dst, const rnd_polyarea_t *src)
{
	*dst = NULL;
	if (src == NULL)
		return rnd_false;

	*dst = pa_polyarea_dup_all(src);
	return (dst != NULL);
}

rnd_bool pa_polyarea_insert_pline(rnd_polyarea_t *pa, rnd_pline_t *pl)
{
	if ((pa == NULL) || (pl == NULL))
		return rnd_false;

	if (pl->flg.orient == RND_PLF_DIR) {
		/* outer (positive) contour */
		if (pa->contours != NULL)
			return rnd_false;

		pa->contours = pl;
	}
	else {
		rnd_pline_t *tmp;

		/* inner (nagative, hole) contour */
		if (pa->contours == NULL)
			return rnd_false;

		/* link at front of hole list */
		tmp = pa->contours->next;
		pa->contours->next = pl;
		pl->next = tmp;
	}

	rnd_r_insert_entry(pa->contour_tree, (rnd_box_t *)pl);
	return rnd_true;
}

void pa_polyarea_init(rnd_polyarea_t *pa)
{
	pa->f = pa->b = pa; /* single island */
	pa->contours = NULL;
	pa->contour_tree = rnd_r_create_tree();
}

rnd_polyarea_t *pa_polyarea_alloc(void)
{
	rnd_polyarea_t *res =malloc(sizeof(rnd_polyarea_t));

	if (res != NULL)
		pa_polyarea_init(res);

	return res;
}

/* free a single polyarea, a single island (does NOT unlink it from the
   island list) */
RND_INLINE void pa_polyarea_free(rnd_polyarea_t *pa)
{
	rnd_poly_plines_free(&pa->contours);
	rnd_r_destroy_tree(&pa->contour_tree);
	free(pa);
}

void pa_polyarea_free_all(rnd_polyarea_t **pa)
{
	rnd_polyarea_t *n, *head = *pa;

	if (*pa == NULL)
		return;

	for(n = head->f; n != head; n = head->f) {
		n->f->b = n->b;
		n->b->f = n->f;
		pa_polyarea_free(n);
	}

	/* the above loop did not run on head because we started on head->f */
	assert(n == head);
	pa_polyarea_free(n);

	*pa = NULL;
}

/* Returns the number of polyareas on the list of src */
RND_INLINE long pa_polyarea_count(rnd_polyarea_t *src)
{
	rnd_polyarea_t *n = src;
	long cnt = 0;

	do { cnt++; } while((n = n->f) != src);

	return cnt;
}

/* Unlink polyarea from list; *list may be modified or even set to NULL */
void pa_polyarea_unlink(rnd_polyarea_t **list, rnd_polyarea_t *island)
{
	/* avoid "island being the head of list" corner case by bumping the list */
	if (*list == island)
		*list = (*list)->f;

	/* corner case: island is still the head so island is the only element of the list */
	if (*list == island)
		*list = NULL;

	/* unlink */
	island->b->f = island->f;
	island->f->b = island->b;
	island->f = island->b = island;
}

/* Calculate the bbox of a polyarea merging all islands' outer polyline bboxes */
RND_INLINE void pa_polyarea_bbox(rnd_box_t *dst, const rnd_polyarea_t *src)
{
	const rnd_polyarea_t *pa;
	rnd_box_t box = *((rnd_box_t *)src->contours);

	for(pa = src; (pa = pa->f) != src; ) {
		rnd_box_t *b_box = (rnd_box_t *)pa->contours;
		RND_MAKE_MIN(box.X1, b_box->X1);
		RND_MAKE_MIN(box.Y1, b_box->Y1);
		RND_MAKE_MAX(box.X2, b_box->X2);
		RND_MAKE_MAX(box.Y2, b_box->Y2);
	}

	*dst = box;
}
