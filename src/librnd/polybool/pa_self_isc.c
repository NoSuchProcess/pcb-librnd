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
	unsigned restart:1;      /* restart the rtree search on the current line */
	rnd_vnode_t *skip_to;    /* skip to this node while walking the outline, skipping sections of hidden inner islands */

	rnd_vnode_t *search_seg_v;
	pa_seg_t *search_seg;

	pa_conn_desc_t *cdl;

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
	assert(pl->next == NULL);
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

/* Insert a new node and a cvc at an intersection point as the next node of vn */
RND_INLINE rnd_vnode_t *pa_selfisc_ins_pt(pa_selfisc_t *ctx, rnd_vnode_t *vn, pa_big_vector_t pt)
{
	rnd_vnode_t *new_node;
	pa_seg_t *sg;
	
	new_node = pa_ensure_point_and_prealloc_cvc(vn, pt);
	if (new_node == NULL)
		return NULL;
	new_node->next = vn->next;
	new_node->prev = vn;
	vn->next->prev = new_node;
	vn->next = new_node;
	ctx->pl->Count++;
	sg = pa_selfisc_find_seg(ctx, vn);
	if (pa_adjust_tree(ctx->pl->tree, sg) != 0)
		assert(0); /* failed memory allocation */

	return new_node;
}

/* Class 5: line-line overlap: split up both lines then cut the path in three:
    - a loop before isc1
    - a loop after isc2
    - segments in between isc1 and isc2 (these are dropped)
   Parameters:
    - ctx->v is the first point of the line segment we are searching for
    - sv is the first point of the other line segment participating in the
      intersection; this 
    - isc1 and isc2 are the intersection points
   Notes:
    - ctx->v (and ctx->v->prev) are guaranteed to be on the outer loop
    - ctx->v->next is going to be in the island
    - it is enough to mark the lines on the overlapping section and the
      collect() calls will ignore them
*/
static rnd_r_dir_t pa_selfisc_line_line_overlap(pa_selfisc_t *ctx, rnd_vnode_t *sv, pa_big_vector_t isco, pa_big_vector_t isci)
{
	pa_big2_coord_t disto, disti;
	pa_big_vector_t ctxv1, ctxv2, sv1, sv2; /* line endpoints: ctx->v/ctx->v->next and sv;sv->next */
	rnd_vnode_t *ctxn1, *ctxn2, *sn1, *sn2; /* final intersection points */
	int need_swap;

	rnd_trace("line-line overlap: %ld;%ld %ld;%ld vs %ld;%ld %ld;%ld\n",
		ctx->v->point[0], ctx->v->point[1], ctx->v->next->point[0], ctx->v->next->point[1],
		sv->point[0], sv->point[1], sv->next->point[0], sv->next->point[1]
		);


	rnd_pa_big_load_cvc(&ctxv1, ctx->v);
	rnd_pa_big_load_cvc(&ctxv2, ctx->v->next);
	rnd_pa_big_load_cvc(&sv1, sv);
	rnd_pa_big_load_cvc(&sv2, sv->next);

	/* order isco and isci so that isco is on the outer loop and isci is the island
	   loop. This can be done by looking at the distance from ctx->v, which is
	   guaranteed to be on the outer loop */
	rnd_vect_u_dist2_big(disto, ctxv1, isco);
	rnd_vect_u_dist2_big(disti, ctxv1, isci);

	need_swap = (pa_big2_coord_cmp(disti, disto) < 0);
rnd_trace("## I1 isco=%f;%f isci=%f;%f   ctxv1=%f;%f\n### disti=%f disto=%f swap=%d for %d\n",
	pa_big_double(isco.x), pa_big_double(isco.y),
	pa_big_double(isci.x), pa_big_double(isci.y),
	pa_big_double(ctxv1.x), pa_big_double(ctxv1.y),
	pa_big2_double(disti), pa_big2_double(disto),
	need_swap, pa_big2_coord_cmp(disti, disto)
	);
	if (need_swap)
		SWAP(pa_big_vector_t, isco, isci);

	/* add the intersection points; when pa_selfisc_ins_pt() returns NULL, we are at an endpoint */
	ctxn2 = pa_selfisc_ins_pt(ctx, ctx->v, isci);
	if (ctxn2 == NULL) ctxn2 = ctx->v->next;
	else ctxn2->flg.mark = 1;
	ctxn1 = pa_selfisc_ins_pt(ctx, ctx->v, isco);
	if (ctxn1 == NULL) ctxn1 = ctx->v;

	sn2 = pa_selfisc_ins_pt(ctx, sv, isco);
rnd_trace(" sn2=%p @ isco=%f;%f isci=%f;%f\n", sn2, pa_big_double(isco.x), pa_big_double(isco.y), pa_big_double(isci.x), pa_big_double(isci.y));
	if (sn2 == NULL) sn2 = sv->next;
	else sn2->flg.mark = 1;
	sn1 = pa_selfisc_ins_pt(ctx, sv, isci);
	if (sn1 == NULL) sn1 = sv;

	/* block the overlapping part from collect*() */
	ctxn1->flg.mark = ctxn1->flg.blocked = 1;
	sn1->flg.mark = sn1->flg.blocked = 1;

	/* the resulting island has no access from the outer contour because of
	   the blocking so we need to remember them separately */
	rnd_trace(" hidden island:  %ld;%ld\n", ctxn2->point[0], ctxn2->point[1]);
	vtp0_append(&ctx->hidden_islands, ctxn2);


	rnd_trace(" blocking enter: %ld;%ld %ld;%ld {%p}\n",
		ctxn1->point[0], ctxn1->point[1], ctxn1->next->point[0], ctxn1->next->point[1],
		ctxn1);
	rnd_trace(" blocking leave: %ld;%ld %ld;%ld {%p}\n",
		sn1->point[0], sn1->point[1], sn1->next->point[0], sn1->next->point[1],
		sn1);

	ctx->skip_to = sn2;
	rnd_trace(" skipping to:    %ld;%ld (to: %ld;%ld)\n", sn2->point[0], sn2->point[1], sn2->next->point[0], sn2->next->point[1]);

	ctx->num_isc++;

	return RND_R_DIR_NOT_FOUND;
}

/* Called back from an rtree query to figure if two edges intersect */
static rnd_r_dir_t pa_selfisc_cross_cb(const rnd_box_t *b, void *cl)
{
	pa_selfisc_t *ctx = (pa_selfisc_t *)cl;
	pa_seg_t *s = (pa_seg_t *)b;
	pa_big_vector_t isc1, isc2;
	int num_isc, got_isc = 0;
	rnd_vnode_t *new_node;

	if ((s->v == ctx->v) || (s->v == ctx->v->next) || (s->v == ctx->v->prev))
		return RND_R_DIR_NOT_FOUND;

	num_isc = pa_isc_edge_edge_(s->v, s->v->next, ctx->v, ctx->v->next, &isc1, &isc2);
	if (num_isc == 0)
		return RND_R_DIR_NOT_FOUND;

	/* Having two intersections means line-line overlap: class 5 */
	if (num_isc == 2) {
		if (s->v->flg.mark)
			return RND_R_DIR_NOT_FOUND; /* already handled */
		s->v->flg.mark = 1;
		return pa_selfisc_line_line_overlap(ctx, s->v, isc1, isc2);
	}

	/* singe intersection between two lines; new_node is NULL if isc1 is at
	   one end of the line */
	new_node = pa_selfisc_ins_pt(ctx, ctx->v, isc1);
	if (new_node != NULL) got_isc = 1;

	new_node = pa_selfisc_ins_pt(ctx, s->v, isc1);
	if (new_node != NULL) got_isc = 1;

	if (new_node != NULL)
		rnd_trace("isc1 %d %d | %d %d %d %d\n", new_node->point[0], new_node->point[1], ctx->v->point[0], ctx->v->point[1], ctx->v->next->point[0], ctx->v->next->point[1]);


	if (got_isc) {
		ctx->num_isc++;
		ctx->restart = 1; /* because the rtree changed */
		return RND_R_DIR_CANCEL;
	}
	return RND_R_DIR_NOT_FOUND;
}

RND_INLINE int pa_eff_dir_forward(char dir1, char dir2)
{
	int eff_dir = 1;

	if (dir1 != 'N') eff_dir = !eff_dir;
	if (dir2 != 'N') eff_dir = !eff_dir;

	return eff_dir;
}

/* Step from n to the next node according to dir, walking the outline (trying
   to achieve largest area) */
RND_INLINE rnd_vnode_t *pa_selfisc_next_o(rnd_vnode_t *n, char *dir)
{
	pa_conn_desc_t *c, *start;
	rnd_vnode_t *onto;

	rnd_trace(" next: ");
	if (n->cvclst_prev == NULL) {
		if (*dir == 'N') {
			onto = n->next;
			n->flg.mark = 1;
		}
		else {
			onto = n->prev;
			onto->flg.mark = 1;
		}
		rnd_trace("straight to %d %d\n", onto->point[0], onto->point[1]);

		return onto;
	}

	start = c = n->cvclst_prev->next;
	rnd_trace("CVC (c->side=%c *dir=%c)\n", c->side, *dir);
	do {
		int eff_dir = pa_eff_dir_forward(c->side, *dir);

		if (eff_dir) onto = c->parent->next;
		else onto = c->parent->prev;

		rnd_trace("  %d %d '%c'", onto->point[0], onto->point[1], c->side);
		if (c->parent->flg.blocked)
			continue;
		if (!onto->flg.mark) {
			*dir = eff_dir ? 'N' : 'P';
			if (eff_dir)
				c->parent->flg.mark = 1;
			else
				onto->flg.mark = 1;
			
			rnd_trace(" accept, dir '%c' (onto) {%p}!\n", *dir, onto);
			return onto;
		}
		rnd_trace(" refuse (marked) {%p}\n", onto);
	} while((c = c->prev) != start);

	/* corner case: bowtie-kind of self-isc with the original 'start' node
	   being the next node from the last cvc junction. Since the 'start' node
	   is already marked, the above loop will not pick it up and we'd panic.
	   As a special exception in this case search the outgoing edges for
	   the original 'start' node and if found, and there were no better way
	   to continue, just close the poly. Test case: class1c */
	start = c = n->cvclst_prev->next;
	do {
		int eff_dir = pa_eff_dir_forward(c->side, *dir);

		if (eff_dir) onto = c->parent->next;
		else onto = c->parent->prev;

		if (onto->flg.start) {
			rnd_trace(" accept, dir '%c' (start %ld;%ld)!\n", *dir, onto->point[0], onto->point[1]);
			if (eff_dir)
				c->parent->flg.mark = 1;
			else
				onto->flg.mark = 1;
			return onto;
		}
	} while((c = c->prev) != start);

	/* didn't find a way out of CVC that's still available or is closing the loop */
	assert(!"nowhere to go from CVC (outline)");
	return NULL;
}

/* Collect the outline, largest area possible; remember islands cut off */
RND_INLINE void pa_selfisc_collect_outline(pa_posneg_t *posneg, rnd_pline_t *src, rnd_vnode_t *start)
{
	rnd_vnode_t *n, *last, *newn;
	rnd_pline_t *dst;
	char dir = 'N';

	assert(!start->flg.mark); /* should face marked nodes only as outgoing edges of intersections */
	start->flg.mark = 1;
	start->flg.start = 1;
	dst = pa_pline_new(start->point);

	rnd_trace("selfi collect outline from %d %d\n", start->point[0], start->point[1]);

	/* append dst to the list of plines */
	posneg_append_pline(posneg, +1, dst);

	/* collect a closed loop */
	last = dst->head;
	for(n = pa_selfisc_next_o(start, &dir); n != start; n = pa_selfisc_next_o(n, &dir)) {
		rnd_trace(" at out %d %d {%p}", n->point[0], n->point[1], n);
		/* Can't assert for this: in the bowtie case the same crossing point has two roles
			assert(!n->flg.mark); (should face marked nodes only as outgoing edges of intersections)
		*/
		newn = calloc(sizeof(rnd_vnode_t), 1);
		newn->point[0] = n->point[0];
		newn->point[1] = n->point[1];
		rnd_poly_vertex_include(last, newn);
		last = newn;
	}
	start->flg.start = 0;
}

/* Step from n to the next node according to dir, walking an island (trying to
   get minimal area portions) */
RND_INLINE rnd_vnode_t *pa_selfisc_next_i(rnd_vnode_t *n, char *dir, int *rev, rnd_vnode_t **first)
{
	pa_conn_desc_t *c, *start;
	rnd_vnode_t *onto;

	rnd_trace("    next (first:%d): ", first);
	if (n->cvclst_prev == NULL) {
		if (*dir == 'N') {
			onto = n->prev;
			onto->flg.mark = 1;
rnd_trace("[mark %ld;%ld] ", onto->point[0], onto->point[1]);
			if (first) {
				*first = onto;
				onto->flg.start = 1;
			}
		}
		else {
			onto = n->next;
			n->flg.mark = 1;
rnd_trace("[mark %ld;%ld] ", n->point[0], n->point[1]);
			if (first) {
				*first = n;
				n->flg.start = 1;
			}
		}
		rnd_trace("straight to %d %d\n", onto->point[0], onto->point[1]);
		return onto;
	}

	rnd_trace("CVC\n");
	start = c = n->cvclst_prev->next;
	do {
		int eff_dir = pa_eff_dir_forward(c->side, *dir);

		if (c->side == *dir)
			*rev = 1;

		if (eff_dir) onto = c->parent->prev;
		else onto = c->parent->next;

		rnd_trace("     %d %d '%c' m%d s%d %d onto={%p} parent={%p}", onto->point[0], onto->point[1], c->side, onto->flg.mark, onto->flg.start, c->parent->flg.start, onto, c->parent);
		if (c->parent->flg.start) {
			if (first) continue; /* tried to start onto an already mapped path; try the next */
			return NULL; /* arrived back to the starting point - finish normally */
		}

		if (!onto->flg.mark || onto->flg.start) { /* also accept flg.start here: greedy algorithm to find shortest loops */
			*dir = c->side;
			if (eff_dir) {
				onto->flg.mark = 1;
				if (first) {
					*first = onto;
					onto->flg.start = 1;
				}
				rnd_trace(" mark %d %d M1a s%d {%p}", onto->point[0], onto->point[1], first, onto);
			}
			else {
				c->parent->flg.mark = 1;
				if (first) {
					*first = c->parent;
					c->parent->flg.start = 1;
				}
				rnd_trace(" mark %d %d M1b s%d {%p}", c->parent->point[0], c->parent->point[1], first, c->parent);
			}

			rnd_trace("    accept, dir '%c' (onto)!\n", *dir);
			return onto;
		}
		rnd_trace("    refuse (marked)\n");
	} while((c = c->prev) != start);

	if (!first) {
		/* didn't find a way out of CVC that's still available or is closing the
		   loop; it's really an error */
		assert(!"nowhere to go from CVC (island)");
	}
	else {
		/* can't start in any direction: all islands are mapped, return empty */
	}
	return NULL;
}

/* Collect all unmarked islands starting from a cvc node */
RND_INLINE void pa_selfisc_collect_island(pa_posneg_t *posneg, rnd_vnode_t *start)
{
	int accept_pol = 0;
	char dir = 'N';
	rnd_vnode_t *n, *newn, *last, *started;
	rnd_pline_t *dst;
	int rev = 0;

	dst = pa_pline_new(start->point);
	last = dst->head;

	rnd_trace("  island {:\n");
	rnd_trace("   IS1 %d %d\n", start->point[0], start->point[1]);
	for(n = pa_selfisc_next_i(start, &dir, &rev, &started); (n != start) && (n != NULL); n = pa_selfisc_next_i(n, &dir, &rev, 0)) {
		rnd_trace("   IS2 %d %d\n", n->point[0], n->point[1]);

		newn = calloc(sizeof(rnd_vnode_t), 1);
		newn->point[0] = n->point[0];
		newn->point[1] = n->point[1];
		rnd_poly_vertex_include(last, newn);
		last = newn;
	}
	pa_pline_update(dst, 1);

	if (dst->Count >= 3) {
		/* if there are enough points for a polygon, figure its polarity */
		if ((dir == 'N') && (dst->flg.orient == RND_PLF_DIR))
			accept_pol = -1;
		else if ((dir == 'P') && (dst->flg.orient != RND_PLF_DIR))
			accept_pol = -1;
		else if ((dir == 'N') && (dst->flg.orient != RND_PLF_DIR)) /* ? */
			accept_pol = +1;
		else if ((dir == 'P') && (dst->flg.orient == RND_PLF_DIR))
			accept_pol = +1;
	}
	rnd_trace("  } (end island: len=%d dir=%c PLF=%d rev=%d accept=%d)\n", dst->Count, dir, dst->flg.orient == RND_PLF_DIR, rev, accept_pol);
	if (started != NULL)
		started->flg.start = 0;

	if (accept_pol != 0) {
		if ((accept_pol > 0) && (dst->flg.orient != RND_PLF_DIR))
			pa_pline_invert(dst);
		posneg_append_pline(posneg, accept_pol, dst);
	}
	else
		pa_pline_free(&dst); /* drop overlapping positive */
}

RND_INLINE void pa_selfisc_collect_islands(pa_posneg_t *posneg, rnd_vnode_t *start)
{
	rnd_vnode_t *n;

	rnd_trace("selfi collect islands from %d %d\n", start->point[0], start->point[1]);

	/* detect uncollected loops */
	n = start;
	do {
		pa_conn_desc_t *c, *cstart = n->cvclst_prev;
		
		if ((cstart == NULL) || n->flg.mark)
			continue;
		
		rnd_trace(" at isl %d %d\n", n->point[0], n->point[1]);
		c = cstart;
		do {
			if (!c->parent->flg.mark)
				pa_selfisc_collect_island(posneg, c->parent);
		} while((c = c->next) != cstart);

	} while((n = n->next) != start);
}

/* Build the outer contour of self-intersecting pl and return it. Return
   whether there was a self intersection (and posneg got loaded). */
static rnd_bool rnd_pline_split_selfisc(pa_posneg_t *posneg, rnd_pline_t *pl)
{
	rnd_vnode_t *n, *start;
	pa_selfisc_t ctx = {0};
	long i;

	ctx.pl = pl;

	n = start = pa_find_minnode(pl);
	rnd_trace("loop start (outer @ rnd_pline_split_selfisc)\n");
	do {
		rnd_box_t box;

		rnd_trace(" loop %ld;%ld (outer) {%p}\n", n->point[0], n->point[1], n);

		n->flg.mark = 0;
		ctx.v = n;
		box.X1 = pa_min(n->point[0], n->next->point[0]); box.Y1 = pa_min(n->point[1], n->next->point[1]);
		box.X2 = pa_max(n->point[0], n->next->point[0]); box.Y2 = pa_max(n->point[1], n->next->point[1]);
		do {
			ctx.restart = 0;
			rnd_r_search(pl->tree, &box, NULL, pa_selfisc_cross_cb, &ctx, NULL);
		} while(ctx.restart);

		if (ctx.skip_to != NULL) {
			n = ctx.skip_to;
			ctx.skip_to = NULL;
		}
		else
			n = n->next;
	} while(n != start);

	if (ctx.num_isc == 0)
		return 0; /* no self intersection */

	/* preserve original holes as negatives */
	if (pl->next != NULL) {
		posneg_append_pline(posneg, -1, pl->next);
		pl->next = NULL;
	}

	ctx.cdl = pa_add_conn_desc(pl, 'A', NULL);

	/* collect the outline first; anything that remains is an island,
	   add them as a hole depending on their loop orientation */
	pa_selfisc_collect_outline(posneg, pl, start);

	pa_selfisc_collect_islands(posneg, start);

	/* collect hiddel islands - they are not directly reachable from the
	   outline */
	for(i = 0; i < ctx.hidden_islands.used; i++)
		pa_selfisc_collect_island(posneg, ctx.hidden_islands.array[i]);

	vtp0_uninit(&ctx.hidden_islands);

	return 1;
}

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
RND_INLINE void remove_all_cvc(rnd_polyarea_t *pa1)
{
	rnd_polyarea_t *pa = pa1;
	do {
		rnd_pline_t *pl;
		for(pl = pa->contours; pl != NULL; pl = pl->next) {
			rnd_vnode_t *n = pl->head;
			do {
				if (n->cvclst_prev != NULL) {
					free(n->cvclst_prev);
					n->cvclst_prev = n->cvclst_next = NULL;
					/* no need to unlink, all cvcs are free'd here */
				}
			} while((n = n->next) != pl->head);
		}
	} while((pa = pa->f) != pa1);
}

rnd_cardinal_t rnd_polyarea_split_selfisc(rnd_polyarea_t **pa)
{
	rnd_polyarea_t *paa, *pab, *pan, *pa_start, *pab_next, *paf;
	rnd_pline_t *pl, *next, *pl2, *next2, *firstpos;
	rnd_cardinal_t cnt = 0;
	vtp0_t floating = {0};
	long n;

	pa_start = *pa;
	do {
		rnd_trace("^ pa %p (f=%p in=%p)\n", *pa, (*pa)->f, pa_start);

		/* remember pa->f so that new positive islands we are inserting after (*pa)
		   are not affected */
		paf = (*pa)->f;

		/* pline intersects itself */
		for(pl = (*pa)->contours; pl != NULL; pl = next) {
			pa_posneg_t posneg = {0};
			next = pl->next;

			if (rnd_pline_split_selfisc(&posneg, pl) == 0)
				continue;

			firstpos = posneg.first_pos;

			/* install holes (neg) in islands (pos) */
			if (posneg.subseq_pos.used == 0) {
				/* special case optimization: if there's only one positive island,
				   all holes go in there - this is the common case, only the "bone"
				   cases will result in multiple positive islands */
				firstpos->next = posneg.neg_head;
				posneg.neg_head = posneg.neg_tail = NULL;
			}
			else {
TODO("sort out which island goes where");
				rnd_trace("TODO#751\n");
				if (posneg.neg_head != NULL)
					abort();

				firstpos = posneg.subseq_pos.array[0];
			}

			/* first positive: replace existing pl in pa; really just swap the
			   vertex list and islands... This is an optimization that saves
			   on memory allocations plus keeps the polyarea as intact as
			   possible for the simple/common cases. */
			if (firstpos != NULL) {
				SWAP(rnd_vnode_t *, pl->head, firstpos->head);
				SWAP(rnd_pline_t *, pl->next, firstpos->next);
				pa_pline_update(pl, 0);

				/* ... so newpl holds the old list now and can be freed */
				pa_pline_free(&firstpos);
			}

			/* insert subsequent islands; skip the 0th one, it's already in */
			for(n = 1; n < posneg.subseq_pos.used; n++) {
				rnd_polyarea_t *pa_new;
				rnd_pline_t *island = posneg.subseq_pos.array[n];
				TODO("insert island as a new pa");
				pa_new = pa_polyarea_alloc();
				rnd_polyarea_contour_include(pa_new, island);
				rnd_polyarea_m_include(pa, pa_new);
			}

			vtp0_uninit(&posneg.subseq_pos);
		}
	} while((*pa = paf) != pa_start);

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
					rnd_polyarea_t *tmpa, *tmpb, *tmpc = NULL;

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

					rnd_polyarea_boolean_free(tmpa, tmpb, &tmpc, RND_PBO_UNITE);

					/* unlunk from tmpc and free up temps */
					pl = tmpc->contours;
					pa_polyarea_del_pline(tmpc, pl);
					rnd_polyarea_free(&tmpc);

					pa_pline_invert(pl); /* contour to hole for insertion */
					pa_polyarea_insert_pline(*pa, pl);

					goto restart_2a; /* now we have a new hole with a different geo and changed the list... */
				}
			}
		}
	} while((*pa = (*pa)->f) != pa_start);

	restart_2b:; /* need to restart the loop because *pa changes */
	pa_start = *pa;
	do {
		/* hole vs. contour intersection */
		for(pl = (*pa)->contours->next; pl != NULL; pl = next) {
			if (rnd_pline_isc_pline(pl, (*pa)->contours)) {
				rnd_polyarea_t *tmpa, *tmpc = NULL;

				rnd_trace("selfisc class 2b hole-contour\n");

				pa_polyarea_del_pline(*pa, pl);

				/* hole-to-contour for unite */
				pa_pline_invert(pl); assert(pl->flg.orient == RND_PLF_DIR);

				tmpa = pa_polyarea_alloc();
				pa_polyarea_insert_pline(tmpa, pl);

				TODO("optimize: it'd be better simply not to add the cvcs; test case : fixed8");
				remove_all_cvc(*pa);

				rnd_polyarea_boolean_free(*pa, tmpa, &tmpc, RND_PBO_SUB);
				*pa = tmpc;

				goto restart_2b; /* now we have a new hole with a different geo and changed the list... */
			}
		}
	} while((*pa = (*pa)->f) != pa_start);

	/* clean up pa so it doesn't have cvc (confuses the poly_bool algo) */
	TODO("optimize: it'd be better simply not to add the cvcs; test case : fixed8");
	remove_all_cvc(*pa);

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
		res = rnd_polyarea_boolean_free(*pa, fl, &tmp, RND_PBO_UNITE);
		*pa = tmp;

		rnd_trace("  pa-pa isc union result: %d -> %p\n", res, *pa);
		cnt++;
	}
	vtp0_uninit(&floating);

	return cnt;
}
