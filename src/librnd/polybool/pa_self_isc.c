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
 */

#include <librnd/core/error.h>

RND_INLINE double NODE_CRD_X(rnd_vnode_t *nd)
{
	if (nd->cvclst_prev == NULL) return nd->point[0];
	return pa_big_double(nd->cvclst_prev->isc.x);
}

RND_INLINE double NODE_CRD_Y(rnd_vnode_t *nd)
{
	if (nd->cvclst_prev == NULL) return nd->point[1];
	return pa_big_double(nd->cvclst_prev->isc.y);
}

#define NODE_CRDS(node) NODE_CRD_X(node), NODE_CRD_Y(node)

/* Return the "top-left" vnode of pl (the node that has the smallest x and y) */
static rnd_vnode_t *pa_find_minnode(rnd_pline_t *pl)
{
	rnd_vnode_t *n, *min;
	for(min = pl->head, n = min->next; n != pl->head; n = n->next) {
		if (n->point[0] < min->point[0])
			min = n;
		else if ((n->point[0] == min->point[0]) && (n->point[1] < min->point[1]))
			min = n;
	}
	return min;
}

/*** map intersections ***/
typedef struct {
	rnd_vnode_t *v;
	rnd_pline_t *pl;
	long num_isc;
/*	unsigned go_back:1;      /* go back one segment in the rtree search on the current line (it got split) */
	unsigned cut_line_line_overlap:1; /*  config: when enabled, cut class5 line-line overlap cases (bones) */

	rnd_vnode_t *search_seg_v;
	pa_seg_t *search_seg;

	rnd_vnode_t *tee;

	vtp0_t hidden_islands;
} pa_selfisc_t;

/* A collection of positive and negative polylines */
typedef struct {
	rnd_pline_t *first_pos; /* optimization: don't start a vtp0 if there's only one (most common case) */
	vtp0_t subseq_pos;      /* if there's more than one positive, store them here; can't use pos->next because ->next is for holes */

	/* negative islands are always collected in a list; tail pointer is kept
	   for quick append */
	rnd_pline_t *neg_head, *neg_tail;
} pa_posneg_t;

static void posneg_append_pline(pa_posneg_t *posneg, int polarity, rnd_pline_t *pl)
{
	assert(pl != NULL);
	assert(polarity != 0);

	if (polarity > 0) {
		if (posneg->first_pos != NULL) {
			if (posneg->subseq_pos.used == 0)
				vtp0_append(&posneg->subseq_pos, posneg->first_pos); /* the vector should include first_pos too so it's not a special case */
			vtp0_append(&posneg->subseq_pos, pl);
		}
		else
			posneg->first_pos = pl;

	}
	else {
		if (posneg->neg_head == NULL)
			posneg->neg_head = pl; /* first entry */
		else
			posneg->neg_tail->next = pl; /* append to tail */

		/* set new tail */
		posneg->neg_tail = pl;

		/* make sure tail points to the last entry */
		while(posneg->neg_tail->next != NULL)
			posneg->neg_tail = posneg->neg_tail->next;
	}
}


static rnd_r_dir_t pa_selfisc_find_seg_cb(const rnd_box_t *b, void *ctx_)
{
	pa_selfisc_t *ctx = (pa_selfisc_t *)ctx_;
	pa_seg_t *s = (pa_seg_t *)b;

	if (ctx->search_seg_v == s->v) {
		ctx->search_seg = s;
		return RND_R_DIR_CANCEL; /* found */
	}
	return RND_R_DIR_NOT_FOUND;
}

/* Find the segment for pt in tree */
RND_INLINE pa_seg_t *pa_selfisc_find_seg(pa_selfisc_t *ctx, rnd_vnode_t *pt)
{
	rnd_r_dir_t rres;
	rnd_box_t box;

	ctx->search_seg_v = pt;
	ctx->search_seg = NULL;

	box.X1 = pt->point[0];    box.Y1 = pt->point[1];
	box.X2 = box.X1 + 1;      box.Y2 = box.Y1 + 1;
	rres = rnd_r_search(ctx->pl->tree, &box, NULL, pa_selfisc_find_seg_cb, ctx, NULL);
	assert(rres == RND_R_DIR_CANCEL);

	return ctx->search_seg;
}

#include "pa_self_isc_pl.c"

/*** class 2 ***/

/* Called back from an rtree query to figure if two edges intersect */
static rnd_r_dir_t pa_pline_isc_pline_cb(const rnd_box_t *b, void *cl)
{
	rnd_vnode_t *va1 = (rnd_vnode_t *)cl;
	pa_seg_t *s = (pa_seg_t *)b;
	int num_isc;
	pa_big_vector_t isc1, isc2;

	num_isc = pa_isc_edge_edge_(s->v, s->v->next, va1, va1->next, &isc1, &isc2);
	if (num_isc > 0)
		return RND_R_DIR_CANCEL;

	return RND_R_DIR_NOT_FOUND;
}


/* Returns whether pl1 intersects with pl2 */
int rnd_pline_isc_pline(rnd_pline_t *pl1, rnd_pline_t *pl2)
{
	rnd_vnode_t *n, *start;
	rnd_r_dir_t res;

	/* if bboxes don't overlap there's no need to run anything epensive */
	if ((pl1->xmax < pl2->xmin) || (pl2->xmax < pl1->xmin)) return 0;
	if ((pl1->ymax < pl2->ymin) || (pl2->ymax < pl1->ymin)) return 0;


	/* do the linear iteration on the smaller one (pl1) */
	if (pl1->Count > pl2->Count)
		SWAP(rnd_pline_t *, pl1, pl2);

	n = start = pl1->head;
	do {
		rnd_box_t box;

		box.X1 = pa_min(n->point[0], n->next->point[0]); box.Y1 = pa_min(n->point[1], n->next->point[1]);
		box.X2 = pa_max(n->point[0], n->next->point[0]); box.Y2 = pa_max(n->point[1], n->next->point[1]);
		res = rnd_r_search(pl2->tree, &box, NULL, pa_pline_isc_pline_cb, n, NULL);
		if (res == RND_R_DIR_CANCEL)
			return 1;
	} while((n = n->next) != start);

	return 0;
}

/* The pline self-isc code may introduce CVCs - we need to get rid of them
   before we do poly bool on the pa */
RND_INLINE void remove_all_cvc(rnd_polyarea_t **pa1)
{
	rnd_polyarea_t *pa = *pa1, *next_pa;

	do {
		rnd_pline_t *pl, *prev_pl = NULL, *next_pl;
		next_pa = pa->f;
		for(pl = pa->contours; pl != NULL; prev_pl = pl, pl = next_pl) {
			rnd_vnode_t *p, *nxt, *n = pl->head;
			int redundant, rebuild_tree = 0;

			next_pl = pl->next;

			do {
				if (n->cvclst_prev != NULL) {
					free(n->cvclst_prev);
					free(n->cvclst_next);
					n->cvclst_prev = n->cvclst_next = NULL;
					/* no need to unlink, all cvcs are free'd here */
				}

				if (pl->Count < 3)
					break;

				nxt = n->next;
				p = n->prev;
				redundant = (p->point[0] == n->point[0]) && (p->point[1] == n->point[1]);
				if (!redundant)
					redundant = (nxt->point[0] == n->point[0]) && (nxt->point[1] == n->point[1]);
				if (redundant) {
					rnd_poly_vertex_exclude(pl, n);
					free(n);
					rebuild_tree = 1;
					pl->Count--;
				}
			} while((n = nxt) != pl->head);

			/* invalid hole (reduced into a single line); test case: gixedc */
			if (pl->Count < 3) {
				if (prev_pl != NULL) {
					/* hole: remove it */
					prev_pl->next = pl->next;
					pa_pline_free(&pl);
				}
				else {
					/* contour: remove the pa; test case: gixedc2 */
					assert(pa->contours == pl);
					if (pa == *pa1)
						*pa1 = (*pa1)->f;
					if (pa == *pa1) {
						/* this was the last island in pa1 */
						*pa1 = NULL;
						return;
					}
					pa->b->f = pa->f;
					pa->f->b = pa->b;
					pa->f = pa->b = NULL;
					pa_polyarea_free(pa);
					goto skip_this_pa;
				}
			}

			if (rebuild_tree && (pl != NULL) && (pl->tree != NULL)) {
				rnd_r_free_tree_data(pl->tree, free);
				rnd_r_destroy_tree(&pl->tree);
				pl->tree = rnd_poly_make_edge_tree(pl);
			}
		}
		skip_this_pa:;
	} while((pa = next_pa) != *pa1);
}

RND_INLINE rnd_cardinal_t split_selfisc_pline_pline(rnd_polyarea_t **pa)
{
	rnd_polyarea_t *pa_start;
	rnd_pline_t *pl, *next, *pl2, *next2;

	pa_start = *pa;
	do {
		/* pline intersects other plines within a pa island; since the first pline
		   is the outer contour, this really means a hole-contour or a hole-hole
		   intersection within a single island. Start with merging hole-hole
		   intersections.  */
		restart_2a:;
		for(pl = (*pa)->contours->next; pl != NULL; pl = next) {
			next = pl->next;
			for(pl2 = next; pl2 != NULL; pl2 = next2) {
				next2 = pl2->next;
				if (rnd_pline_isc_pline(pl, pl2)) {
					rnd_polyarea_t *tmpa, *tmpb, *tmpc = NULL, *na;

					rnd_trace("selfisc class 2a hole-hole\n");
					pa_polyarea_del_pline(*pa, pl);
					pa_polyarea_del_pline(*pa, pl2);

					/* hole-to-contour for unite */
					pa_pline_invert(pl); assert(pl->flg.orient == RND_PLF_DIR);
					pa_pline_invert(pl2); assert(pl2->flg.orient == RND_PLF_DIR);

					tmpa = pa_polyarea_alloc();
					tmpb = pa_polyarea_alloc();

					pa_polyarea_insert_pline(tmpa, pl);
					pa_polyarea_insert_pline(tmpb, pl2);

					tmpa->from_selfisc = 1;
					tmpb->from_selfisc = 1;
					rnd_polyarea_boolean_free_nochk(tmpa, tmpb, &tmpc, RND_PBO_UNITE);

					/* unlink all contours from tmpc and free up temps. Need to do all
					   islands in a loop; related test case: fixedr (W shaped cutout) */
					na = tmpc;
					do {
						pl = na->contours;
						pa_polyarea_del_pline(na, pl);
						pa_pline_invert(pl); /* contour to hole for insertion */
						pa_polyarea_insert_pline(*pa, pl);
					} while((na = na->f) != tmpc);

					rnd_polyarea_free(&tmpc);


					goto restart_2a; /* now we have a new hole with a different geo and changed the list... */
				}
			}
		}
	} while((*pa = (*pa)->f) != pa_start);

	return 0;
}

RND_INLINE rnd_cardinal_t split_selfisc_hole_outline(rnd_polyarea_t **pa)
{
	rnd_polyarea_t *pa_start;
	rnd_pline_t *pl, *next;

	restart_2b:; /* need to restart the loop because *pa changes */
	pa_start = *pa;
	do {
		/* hole vs. contour intersection */
		for(pl = (*pa)->contours->next; pl != NULL; pl = next) {
			next = pl->next;
			if (rnd_pline_isc_pline(pl, (*pa)->contours)) {
				rnd_polyarea_t *tmpa, *tmpc = NULL, *tmpd = NULL;
				rnd_polyarea_t *pa_remain;

				rnd_trace("selfisc class 2b hole-contour\n");

				pa_polyarea_del_pline(*pa, pl);

				/* hole-to-contour for sub */
				pa_pline_invert(pl); assert(pl->flg.orient == RND_PLF_DIR);

				tmpa = pa_polyarea_alloc();
				pa_polyarea_insert_pline(tmpa, pl);

				TODO("optimize: it'd be better simply not to add the cvcs; test case : fixed8");
				remove_all_cvc(pa);

				/* unlink *pa so other islands don't interfere; the remaining
				   polyarea is pa_remain or NULL if *pa was single-island;
				   test case: si_class2bc, gixede */
				if ((*pa)->b != *pa) {
					pa_remain = (*pa)->b;
					pa_polyarea_unlink(&pa_remain, *pa);
				}
				else
					pa_remain = NULL;

				/* pl is the hole sitting in tmpa; *pa is now the only island pl was in.
				   Sub tmpa from *pa and put the result in tmpc */
				(*pa)->from_selfisc = 1;
				tmpa->from_selfisc = 1;
				rnd_polyarea_boolean_free_nochk(*pa, tmpa, &tmpc, RND_PBO_SUB);

				if (pa_remain != NULL) {
					/* put back tmpc into pa_remain and make the combined result *pa again */
					*pa = pa_remain;
					(*pa)->from_selfisc = 1;
					tmpc->from_selfisc = 1;
					rnd_polyarea_boolean_free_nochk(*pa, tmpc, &tmpd, RND_PBO_UNITE);
					*pa = tmpd;
				}
				else
					*pa = tmpc;

				goto restart_2b; /* now we have a new hole with a different geo and changed the list... */
			}
			else if (!pa_pline_inside_pline((*pa)->contours, pl)) {
				/* the hole is fully outside of the island, unlink and discard it */
				pa_polyarea_del_pline(*pa, pl);
				pa_pline_free(&pl);
			}
		}
	} while((*pa = (*pa)->f) != pa_start);

	return 0;
}

RND_INLINE rnd_cardinal_t split_selfisc_pa_pa(rnd_polyarea_t **pa)
{
	rnd_polyarea_t *paa, *pab, *pab_next;
	rnd_cardinal_t cnt = 0;
	vtp0_t floating = {0};
	long n;

	/* class 3: pa-pa intersections: different islands of the same polygon object intersect */
	paa = *pa;
	do {
		for(pab = paa->f; pab != *pa; pab = pab_next) {
			int touching;

			pab_next = pab->f;
			rnd_trace("pa-pa %p %p\n", paa, pab);

			touching = rnd_polyarea_island_isc(paa, pab); /* this call doesn't add cvcs */
			if (touching) {
				/* unlink and collect now floating pab on a list for postponed merging */
				pa_polyarea_unlink(pa, pab);
				vtp0_append(&floating, pab);

				rnd_trace("pa-pa isc! -> resolving with an union (later)\n");
			}
		}
	} while((paa = paa->f) != *pa);

	/* class 3: merge floating pab's */
	for(n = 0; n < floating.used; n++) {
		int res;
		rnd_polyarea_t *tmp = NULL, *fl = floating.array[n];

		rnd_trace("pa-pa isc union:\n");
		(*pa)->from_selfisc = 1;
		fl->from_selfisc = 1;
		res = rnd_polyarea_boolean_free(*pa, fl, &tmp, RND_PBO_UNITE);
		*pa = tmp;

		rnd_trace("  pa-pa isc union result: %d -> %p\n", res, *pa);
		cnt++;
	}
	vtp0_uninit(&floating);

	return cnt;
}

rnd_cardinal_t rnd_polyarea_split_selfisc(rnd_polyarea_t **pa)
{
	rnd_cardinal_t cnt;

	cnt = split_selfisc_pline(pa);

	/* clean up pa so it doesn't have cvc (confuses the poly_bool algo) */
	TODO("optimize: run this only if there's any cvc in there? see: fixedr");
	remove_all_cvc(pa);

	cnt += split_selfisc_pline_pline(pa);
	cnt += split_selfisc_hole_outline(pa);

	/* clean up pa so it doesn't have cvc (confuses the poly_bool algo) */
	TODO("optimize: run this only if there's any cvc in there? see: fixed8");
	remove_all_cvc(pa);

	cnt += split_selfisc_pa_pa(pa);

	return cnt;
}
