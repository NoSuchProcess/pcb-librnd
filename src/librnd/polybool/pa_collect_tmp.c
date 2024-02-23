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

/* helper routines for managing polylines while they are being collected */

RND_INLINE void insert_pline(jmp_buf *e, rnd_polyarea_t **dst, rnd_pline_t *pl)
{
	rnd_polyarea_t *newp = malloc(sizeof(rnd_polyarea_t));

	if (RND_UNLIKELY(newp == NULL))
		pa_error(pa_err_no_memory);

	if (*dst == NULL) {
		*dst = newp;
		(*dst)->f = (*dst)->b = *dst;
	}
	else {
		newp->f = *dst;
		newp->b = (*dst)->b;
		newp->f->b = newp->b->f = newp;
	}

	newp->contours = pl;
	newp->contour_tree = rnd_r_create_tree();
	newp->from_selfisc = 0;
	rnd_r_insert_entry(newp->contour_tree, (rnd_box_t *)pl);
	pl->next = NULL;
}

/* Put pl on a list:
    - if it's an outer contour, on contours;
    - if it's a hole with a known new_parent, on the pline list of the new parent
    - if it's a hole with a unknown new_parent, on the temporary holes list
   Arguments:
    - old_parent is the current parent of pl (or NULL).
    - new_parent is where pl is inserted (or NULL)
    - new_parent_contour is the outer contour of new_parent (or NULL)
*/
static void put_contour(jmp_buf *e, rnd_pline_t *pl, rnd_polyarea_t **contours, rnd_pline_t **holes, rnd_polyarea_t *old_parent, rnd_polyarea_t *new_parent, rnd_pline_t *new_parent_contour)
{
	assert((pl != NULL) && (pl->Count > 2));

	pl->next = NULL; /* unlink from its original list of plines */

	if (pl->flg.orient == RND_PLF_DIR) { /* outer contour */
		if (old_parent != NULL)
			rnd_r_delete_entry(old_parent->contour_tree, (rnd_box_t *)pl);
		insert_pline(e, contours, pl);
	}
	else { /* inner: hole */
		if (new_parent_contour != NULL) { /* known parent */
			pl->next = new_parent_contour->next;
			new_parent_contour->next = pl;
			if (old_parent != new_parent) { /* parent changed, move from one rtree to another */
				if (old_parent != NULL)
					rnd_r_delete_entry(old_parent->contour_tree, (rnd_box_t *)pl);
				rnd_r_insert_entry(new_parent->contour_tree, (rnd_box_t *)pl);
			}
		}
		else { /* no known parent - put hole into temporary list */
			/* prepend pl in holes */
			pl->next = *holes;
			*holes = pl;
			/* don't insert temporary holes into an r-tree, just on a linked list */
			if (old_parent != NULL)
				rnd_r_delete_entry(old_parent->contour_tree, (rnd_box_t *)pl);
		}
	}
}

RND_INLINE void remove_contour(rnd_polyarea_t *pa, rnd_pline_t *prev_contour, rnd_pline_t *contour, int remove_from_rtree)
{
	if (pa->contours == contour) {
		/* remove from the front of the list */
		pa->contours = contour->next;
	}
	else if (prev_contour != NULL) {
		/* remove from the middle of the list */
		assert(prev_contour->next == contour);
		prev_contour->next = contour->next;
	}

	contour->next = NULL;

	if (remove_from_rtree)
		rnd_r_delete_entry(pa->contour_tree, (rnd_box_t *)contour);
}

void pa_remove_contour(rnd_polyarea_t *pa, rnd_pline_t *prev_contour, rnd_pline_t *contour, int remove_from_rtree)
{
	remove_contour(pa, prev_contour, contour, remove_from_rtree);
}


typedef struct pa_insert_holes_s {
	rnd_box_t bbox;
	rnd_polyarea_t *pa;
} pa_insert_holes_t;

/* insert (pa_insert_holes_t *) in the heap passed in ctx */
static rnd_r_dir_t pa_inshole_heap_it_cb(const rnd_box_t *b, void *ctx)
{
	rnd_heap_t *heap = (rnd_heap_t *)ctx;
	pa_insert_holes_t *insh_ctx = (pa_insert_holes_t *)b;
	rnd_pline_t *p = insh_ctx->pa->contours;

	if (p->Count == 0)
		return RND_R_DIR_NOT_FOUND; /* shouldn't happen */

	rnd_heap_insert(heap, p->area, insh_ctx);
	return RND_R_DIR_FOUND_CONTINUE;
}

typedef struct pa_inshole_find_inside_s {
	jmp_buf jb;
	rnd_pline_t *want_inside;
	rnd_pline_t *result;
} pa_inshole_find_inside_t;

static rnd_r_dir_t pa_inshole_find_inside_cb(const rnd_box_t *b, void *cl)
{
	pa_inshole_find_inside_t *info = (pa_inshole_find_inside_t *)cl;
	rnd_pline_t *check = (rnd_pline_t *)b;

	if (check->flg.orient == RND_PLF_DIR)
		return RND_R_DIR_NOT_FOUND;

	if (pa_pline_inside_pline(info->want_inside, check)) {
		info->result = check;
		longjmp(info->jb, 1);
	}

	return RND_R_DIR_NOT_FOUND;
}

/* Builds an rtree and an insert-holes array of all polyeares of src */
RND_INLINE void pa_inshole_build_rtree(rnd_polyarea_t *src, pa_insert_holes_t *all_insh_ctx, rnd_rtree_t *tree)
{
	int i = 0;
	rnd_polyarea_t *pa = src;

	do {
		all_insh_ctx[i].bbox.X1 = pa->contours->xmin; all_insh_ctx[i].bbox.Y1 = pa->contours->ymin;
		all_insh_ctx[i].bbox.X2 = pa->contours->xmax; all_insh_ctx[i].bbox.Y2 = pa->contours->ymax;
		all_insh_ctx[i].pa = pa;
		rnd_r_insert_entry(tree, &all_insh_ctx[i].bbox);
		i++;
	} while((pa = pa->f) != src);
}


static rnd_pline_t orp_cont;

/* Search for the container of pl. Also loads insh_ctx as a side effect */
RND_INLINE rnd_pline_t *pa_inshole_find_container(jmp_buf *e, rnd_polyarea_t *dst, rnd_rtree_t *tree, rnd_pline_t *pl, pa_insert_holes_t **insh_ctx, int *risky)
{
	rnd_heap_t *heap;
	rnd_pline_t *container = NULL;

	/* build a heap of all of the polys that the hole is inside its bounding box */
	heap = rnd_heap_create();
	rnd_r_search(tree, (rnd_box_t *)pl, NULL, pa_inshole_heap_it_cb, heap, NULL);
	if (rnd_heap_is_empty(heap)) {
		int orp = pl->flg.orphaned;
#ifndef NDEBUG
#ifdef DEBUG
		pa_poly_dump(dst);
#endif
#endif
		pa_pline_free(&pl);
		rnd_heap_destroy(&heap);

		/* do not panic if a hole of an removed-island (oprhaned hole) didn't
		   find a container. Test case: gixedy */
		if (orp)
			return &orp_cont;

		pa_error(pa_err_bad_parm);
	}

	pl->flg.orphaned = 0;

	/* Search the heap for the container. */
	*insh_ctx = (pa_insert_holes_t *)rnd_heap_remove_smallest(heap);

	if (rnd_heap_is_empty(heap)) {
		/* only one possibility it must be the right one */
		if (!pa_pline_inside_pline((*insh_ctx)->pa->contours, pl))
			*risky = 1;
		container = (*insh_ctx)->pa->contours;
	}
	else {
		for(;;) {
			if (pa_pline_inside_pline((*insh_ctx)->pa->contours, pl)) {
				container = (*insh_ctx)->pa->contours;
				break;
			}
			if (rnd_heap_is_empty(heap))
				break;
			*insh_ctx = (pa_insert_holes_t *)rnd_heap_remove_smallest(heap);
		}
	}
	rnd_heap_destroy(&heap);
	return container;
}

/* Src is a list of holes; try inserting them in dst; in *src return holes that
   are not inserted  */
void rnd_poly_insert_holes(jmp_buf *e, rnd_polyarea_t *dst, rnd_pline_t **src)
{
	rnd_pline_t *pl, *container;
	rnd_rtree_t *tree;
	int num_polyareas = 0;
	pa_insert_holes_t *all_insh_ctx, *insh_ctx;

	if (*src == NULL) return; /* empty hole list */
	if (dst == NULL) pa_error(pa_err_bad_parm); /* empty contour list */

	num_polyareas = pa_polyarea_count(dst);

	/* build an rtree of all the contours in dst and remember them in an array as well */
	all_insh_ctx = malloc(sizeof(pa_insert_holes_t) * num_polyareas);
	tree = rnd_r_create_tree();
	pa_inshole_build_rtree(dst, all_insh_ctx, tree);

	/* loop through the holes and put them where they belong */
	while((pl = *src) != NULL) {
		int risky = 0;

		*src = pl->next;

		container = pa_inshole_find_container(e, dst, tree, pl, &insh_ctx, &risky);
		if (container == &orp_cont)
			continue;

		if (container == NULL) {
#ifndef NDEBUG
#ifdef DEBUG
			pa_poly_dump(dst);
#endif
#endif
			pl->next = NULL;
			pa_pline_free(&pl);
			pa_error(pa_err_bad_parm);
		}
		else {
			if (risky)
				container->flg.risky = 1; /* see also: test case fixedv */

			/* New hole may trigger reprocessing on some existing holes (so those
			   are removed from the pa and put on the result list and returned in *src) */
			for(;;) {
				pa_inshole_find_inside_t info;
				rnd_pline_t *prev;

				info.want_inside = pl;
				info.result = NULL;

				if (!setjmp(info.jb)) {
					/* this shouldn't find the outer contour */
					rnd_r_search(insh_ctx->pa->contour_tree, (rnd_box_t *)pl, NULL, pa_inshole_find_inside_cb, &info, NULL);
					break; /* if there was no longjump, there was nothing found, quit from the for() */
				}

				/* long jump lands here when info.result is found */
				assert(info.result != NULL);

				/* find the contour that precedes the result */
				prev = container;
				while(prev->next != info.result)
					prev = prev->next;

				/* Remove hole from the contour and put it on the to-be-processed list */
				remove_contour(insh_ctx->pa, prev, info.result, rnd_true);
				info.result->next = *src;
				*src = info.result;
			}

			/* link at front of hole list */
			pl->next = container->next;
			container->next = pl;
			rnd_r_insert_entry(insh_ctx->pa->contour_tree, (rnd_box_t *)pl);
		}
	}

	rnd_r_destroy_tree(&tree);
	free(all_insh_ctx);
}
