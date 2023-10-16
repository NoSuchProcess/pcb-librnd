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

/* Handle line-line overlap: split up both lines then cut the path in three:
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
	rnd_vect_m_dist2_big(disto, ctxv1, isco);
	rnd_vect_m_dist2_big(disti, ctxv1, isci);
	if (disti < disto)
		SWAP(pa_big_vector_t, isco, isci);

	/* add the intersection points; when pa_selfisc_ins_pt() returns NULL, we are at an endpoint */
	ctxn2 = pa_selfisc_ins_pt(ctx, ctx->v, isci);
	if (ctxn2 == NULL) ctxn2 = ctx->v->next;
	ctxn1 = pa_selfisc_ins_pt(ctx, ctx->v, isco);
	if (ctxn1 == NULL) ctxn1 = ctx->v;

	sn2 = pa_selfisc_ins_pt(ctx, sv, isco);
	if (sn2 == NULL) sn2 = sv->next;
	sn1 = pa_selfisc_ins_pt(ctx, sv, isci);
	if (sn1 == NULL) sn1 = sv;

	/* block the overlapping part from collect*() */
	ctxn1->flg.mark = 1;
	sn1->flg.mark = 1;

	/* the resulting island has no access from the outer contour because of
	   the blocking so we need to remember them separately */
	rnd_trace(" hidden island:  %ld;%ld\n", ctxn2->point[0], ctxn2->point[1]);
	vtp0_append(&ctx->hidden_islands, ctxn2);


	rnd_trace(" blocking enter: %ld;%ld %ld;%ld\n",
		ctxn1->point[0], ctxn1->point[1], ctxn1->next->point[0], ctxn1->next->point[1]);
	rnd_trace(" blocking leave: %ld;%ld %ld;%ld\n",
		sn1->point[0], sn1->point[1], sn1->next->point[0], sn1->next->point[1]);

	ctx->skip_to = sn2;
	rnd_trace(" skipping to:    %ld;%ld\n", sn2->point[0], sn2->point[1]);

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

	/* Having two intersections means line-line overlap */
	if (num_isc == 2)
		return pa_selfisc_line_line_overlap(ctx, s->v, isc1, isc2);

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

/* Step from n to the next node according to dir, walking the outline */
RND_INLINE rnd_vnode_t *pa_selfisc_next_o(rnd_vnode_t *n, char *dir)
{
	pa_conn_desc_t *c, *start;
	rnd_vnode_t *onto;

	rnd_trace(" next: ");
	if (n->cvclst_prev == NULL) {
		rnd_trace("straight to %d %d\n", n->next->point[0], n->next->point[1]);
		if (*dir == 'N') return n->next;
		return n->prev;
	}

	rnd_trace("CVC\n");
	start = c = n->cvclst_prev->next;
	do {
		if (c->side == 'N') onto = c->parent->next;
		else onto = c->parent->prev;
		rnd_trace("  %d %d '%c'", onto->point[0], onto->point[1], c->side);
		if (!onto->flg.mark) {
			*dir = c->side;
			rnd_trace(" accept, dir '%c'!\n", *dir);
			return onto;
		}
		rnd_trace(" refuse (marked)\n");
	} while((c = c->prev) != start);

	assert(!"nowhere to go from CVC");
	return NULL;
}

RND_INLINE void pa_selfisc_collect_outline(rnd_pline_t **dst_, rnd_pline_t *src, rnd_vnode_t *start)
{
	rnd_vnode_t *n, *last, *newn;
	rnd_pline_t *dst;
	char dir = 'N';

	assert(!start->flg.mark); /* should face marked nodes only as outgoing edges of intersections */
	start->flg.mark = 1;
	dst = pa_pline_new(start->point);

	rnd_trace("selfi collect outline from %d %d\n", start->point[0], start->point[1]);

	/* append dst to the list of plines */
	if (*dst_ != NULL) {
		rnd_pline_t *last;
		for(last = *dst_; last->next != NULL; last = last->next) ;
		last->next = dst;
	}
	else
		*dst_ = dst;

	/* collect a closed loop */
	last = dst->head;
	for(n = pa_selfisc_next_o(start, &dir); n != start; n = pa_selfisc_next_o(n, &dir)) {
		rnd_trace(" at %d %d", n->point[0], n->point[1]);
		/* Can't assert for this: in the bowtie case the same crossing point has two roles
			assert(!n->flg.mark); (should face marked nodes only as outgoing edges of intersections)
		*/
		n->flg.mark = 1;
		newn = calloc(sizeof(rnd_vnode_t), 1);
		newn->point[0] = n->point[0];
		newn->point[1] = n->point[1];
		rnd_poly_vertex_include(last, newn);
		last = newn;
	}
}

/* Step from n to the next node according to dir, walking an island */
RND_INLINE rnd_vnode_t *pa_selfisc_next_i(rnd_vnode_t *n, char *dir)
{
	pa_conn_desc_t *c, *start;
	rnd_vnode_t *onto;

	rnd_trace("    next: ");
	if (n->cvclst_prev == NULL) {
		rnd_trace("straight to %d %d\n", n->next->point[0], n->next->point[1]);
		if (*dir == 'N') return n->prev;
		return n->next;
	}

	rnd_trace("CVC\n");
	start = c = n->cvclst_prev->next;
	do {
		if (c->side == 'N') onto = c->parent->prev;
		else onto = c->parent->next;
		rnd_trace("     %d %d '%c'", onto->point[0], onto->point[1], c->side);
		if (!onto->flg.mark) {
			*dir = c->side;
			rnd_trace("    accept, dir '%c'!\n", *dir);
			return onto;
		}
		rnd_trace("    refuse (marked)\n");
	} while((c = c->prev) != start);

	return NULL;
}

/* Collect all unmarked islands starting from a cvc node */
RND_INLINE void pa_selfisc_collect_island(rnd_pline_t *outline, rnd_vnode_t *start)
{
	int accept;
	char dir = 'N';
	rnd_vnode_t *n, *newn, *last;
	rnd_pline_t *dst;

	dst = pa_pline_new(start->point);
	last = dst->head;

	rnd_trace("  island {:\n");
	rnd_trace("   IS1 %d %d\n", start->point[0], start->point[1]);
	for(n = pa_selfisc_next_i(start, &dir); (n != start) && (n != NULL); n = pa_selfisc_next_i(n, &dir)) {
		rnd_trace("   IS2 %d %d\n", n->point[0], n->point[1]);
		n->flg.mark = 1;

		newn = calloc(sizeof(rnd_vnode_t), 1);
		newn->point[0] = n->point[0];
		newn->point[1] = n->point[1];
		rnd_poly_vertex_include(last, newn);
		last = newn;
	}
	start->flg.mark = 1;
	pa_pline_update(dst, 1);
	accept = (dst->flg.orient != RND_PLF_DIR) && (dst->Count >= 3); /* only negative orientation should be treated as cutout */
	rnd_trace("  } (end island: len=%d accept=%d)\n", dst->Count, accept);


	if (accept) {
		dst->next = outline->next;
		outline->next = dst;
	}
	else
		pa_pline_free(&dst); /* drop overlapping positive */
}

RND_INLINE void pa_selfisc_collect_islands(rnd_pline_t *outline, rnd_pline_t *src, rnd_vnode_t *start)
{
	rnd_vnode_t *n;

	rnd_trace("selfi collect islands from %d %d\n", start->point[0], start->point[1]);

	/* detect uncollected loops */
	n = start;
	do {
		pa_conn_desc_t *c, *cstart = n->cvclst_prev;
		
		if (cstart == NULL)
			continue;
		
		rnd_trace(" at %d %d\n", n->point[0], n->point[1]);
		c = cstart;
		do {
			if (!c->parent->flg.mark)
				pa_selfisc_collect_island(outline, c->parent);
		} while((c = c->next) != cstart);

	} while((n = n->next) != start);
}

/* Build the outer contour of self-intersecting pl and return it. If there is
   no self intersection, return NULL. */
rnd_pline_t *rnd_pline_split_selfisc(rnd_pline_t *pl)
{
	rnd_vnode_t *n, *start;
	pa_selfisc_t ctx = {0};
	rnd_pline_t *res = NULL;
	long i;

	ctx.pl = pl;

	n = start = pa_find_minnode(pl);
	rnd_trace("loop start (outer @ rnd_pline_split_selfisc)\n");
	do {
		rnd_box_t box;

		rnd_trace(" loop %ld;%ld (outer)\n", n->point[0], n->point[1]);

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
		return NULL;

	ctx.cdl = pa_add_conn_desc(pl, 'A', NULL);

	/* collect the outline first; anything that remains is an island,
	   add them as a hole depending on their loop orientation */
	pa_selfisc_collect_outline(&res, pl, start);
	pa_selfisc_collect_islands(res, pl, start);

	/* collect hiddel islands - they are not directly reachable from the
	   outline */
	for(i = 0; i < ctx.hidden_islands.used; i++)
		pa_selfisc_collect_island(res, ctx.hidden_islands.array[i]);

	vtp0_uninit(&ctx.hidden_islands);

	return res;
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

rnd_cardinal_t rnd_polyarea_split_selfisc(rnd_polyarea_t **pa)
{
	rnd_polyarea_t *paa, *pab;
	rnd_pline_t *pl, *next, *pl2, *next2, *newpl;
	rnd_cardinal_t cnt = 0;

	/* pline intersects itself */
	for(pl = (*pa)->contours; pl != NULL; pl = next) {
		next = pl->next;
		newpl = rnd_pline_split_selfisc(pl);
		if (newpl != NULL) {
			/* replace pl with newpl in pa; really just swap the vertex list... */
			SWAP(rnd_vnode_t *, pl->head, newpl->head);
			SWAP(rnd_pline_t *, pl->next, newpl->next);
			pa_pline_update(pl, 0);

			/* ... so newpl holds the old list now and can be freed */
			pa_pline_free(&newpl);
		}
	}

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

restart_2b:;
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
			rnd_polyarea_boolean_free(*pa, tmpa, &tmpc, RND_PBO_SUB);
			*pa = tmpc;

			goto restart_2b; /* now we have a new hole with a different geo and changed the list... */
		}
	}

restart3:;

	/* pa-pa intersections: different islands of the same polygon object intersect */
	paa = *pa;
	do {
		for(pab = paa->f; pab != *pa; pab = pab->f) {
			rnd_polyarea_t *pfa, *pfb;

			int touching;
			rnd_trace("pa-pa %p %p\n", paa, pab);

			/* remove ->f for this test to make sure only that oen island is checked */
			pfa = paa->f;
			pfb = pab->f;
			paa->f = paa;
			pab->f = pab;
			touching = rnd_polyarea_touching(paa, pab);
			paa->f = pfa;
			pab->f = pfb;

			if (touching) {
				int res;
				rnd_polyarea_t *tmp = NULL;

				pa_polyarea_unlink(pa, pab);
				res = rnd_polyarea_boolean_free(paa, pab, &tmp, RND_PBO_UNITE);
				*pa = tmp;

				rnd_trace("pa-pa isc! -> resolving with an union: %d -> %p\n", res, *pa);
				cnt++;
				goto restart3;
			}
		}
	} while((paa = paa->f) != *pa);

	return cnt;
}
