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

/* A collection of positive and negative polylines */
typedef struct {
	rnd_pline_t *first_pos; /* optimization: don't start a vtp0 if there's only one (most common case) */
	vtp0_t subseq_pos;      /* if there's more than one positive, store them here; can't use pos->next because ->next is for holes */

	/* negative islands are always collected in a list; tail pointer is kept
	   for quick append */
	rnd_pline_t *neg_head, *neg_tail;
} pa_posneg_t;

/* Reset a polyline back to pre-mapping state; enable tree rebuild if
   nodes have been added or removed. */
static void pl_remove_cvcs(rnd_pline_t *pl, rnd_bool rebuild_tree)
{
	rnd_vnode_t *n = pl->head;

	do {
		if (n->cvclst_prev != NULL) {
			free(n->cvclst_prev);
			free(n->cvclst_next);
			n->cvclst_prev = n->cvclst_next = NULL;
			/* no need to unlink, all cvcs are free'd here */
		}

		n->shared = NULL;
		n->flg.mark = n->flg.start = 0;
	} while((n = n->next) != pl->head);

	if (rebuild_tree) {
		rnd_r_free_tree_data(pl->tree, free);
		rnd_r_destroy_tree(&pl->tree);
		pl->tree = rnd_poly_make_edge_tree(pl);
	}
}

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

RND_INLINE int on_same_pt(rnd_vnode_t *va, rnd_vnode_t *vb)
{
	if (va == vb) return 1;
	return (va->point[0] == vb->point[0]) && (va->point[1] == vb->point[1]);
}


RND_INLINE rnd_vnode_t *pa_selfisc_ins_pt_(pa_selfisc_t *ctx, rnd_vnode_t *vn, pa_big_vector_t pt, int retnull, int *alloced)
{
	rnd_vnode_t *new_node;
	pa_seg_t *sg;

	new_node = pa_node_add_single(vn, pt);
	if ((new_node == vn) || (new_node == vn->next)) {
		/* redundant */
		if (retnull)
			return NULL;
		return new_node;
	}

	if (alloced != NULL)
		*alloced = 1;

	new_node->next = vn->next;
	new_node->prev = vn;
	vn->next->prev = new_node;
	vn->next = new_node;
	new_node->shared = vn->shared;
	ctx->pl->Count++;

	sg = pa_selfisc_find_seg(ctx, vn);
	if (pa_adjust_tree(ctx->pl->tree, sg) != 0)
		assert(0); /* failed memory allocation */

	return new_node;
}

/* Insert a new node at an intersection point as the next node of vn; return
   NULL if no new node got inserted (redundancy) */
RND_INLINE rnd_vnode_t *pa_selfisc_ins_pt(pa_selfisc_t *ctx, rnd_vnode_t *vn, pa_big_vector_t pt)
{
	return pa_selfisc_ins_pt_(ctx, vn, pt, 1, NULL);
}

/* Insert a new node at an intersection point as the next node of vn; return
   an existing node if no new node got inserted (redundancy) */
RND_INLINE rnd_vnode_t *pa_selfisc_ins_pt2(pa_selfisc_t *ctx, rnd_vnode_t *vn, pa_big_vector_t pt, int *alloced)
{
	return pa_selfisc_ins_pt_(ctx, vn, pt, 0, alloced);
}



/* Return 1 if va and vb have matching endpoints (shared line seg) plus
   mark them shared */
RND_INLINE int on_same_seg_mark(rnd_vnode_t *va, rnd_vnode_t *vb)
{
	if (on_same_pt(va, vb) && on_same_pt(va->next, vb->next)) {
		va->shared = vb;
		vb->shared = va;
		rnd_trace("     shared+={%p} {%p}", va, vb);
		return 1;
	}
	if (on_same_pt(va->next, vb) && on_same_pt(va, vb->next)) {
		va->shared = vb;
		vb->shared = va;
		rnd_trace("     shared+={%p} {%p}", va, vb);
		return 1;
	}
	return 0;
}



/* Called back from an rtree query to figure if two edges intersect; inserts
   points (but no cvc); after looping with this the pline is guaranteed
   to have an integer point at each and every self-isc and self-touch */
static rnd_r_dir_t pa_selfisc_map_cross_cb(const rnd_box_t *b, void *cl)
{
	pa_selfisc_t *ctx = (pa_selfisc_t *)cl;
	pa_seg_t *s = (pa_seg_t *)b;
	pa_big_vector_t isc1, isc2, start;
	rnd_vector_t isc1_small,isc2_small;
	int num_isc, num_isc_small, got_isc = 0;
	rnd_vnode_t *ip1, *ip2;

	if ((s->v == ctx->v) || (s->v == ctx->v->next) || (s->v == ctx->v->prev))
		return RND_R_DIR_NOT_FOUND;

rnd_trace("  ISC vs %.2f;%.2f  %.2f;%.2f:\n", NODE_CRDS(s->v), NODE_CRDS(s->v->next));

	if (on_same_seg_mark(ctx->v, s->v)) {
		/* avoid infinite loop of re-discovering the same shared segment, plus
		   optimization for trivial shared lines */
		rnd_trace("   shared seg marked (quick)\n");
		ctx->num_isc++;
		return RND_R_DIR_NOT_FOUND;
	}

	TODO("arc: check these for overlap");

	TODO("if we settle with rounded coords, probably call the old, cheaper edge-edge isc func");
	num_isc = pa_isc_edge_edge_(s->v, s->v->next, ctx->v, ctx->v->next, &isc1, &isc2);

	if (num_isc > 0) {
		pa_big_round(isc1.x);
		pa_big_round(isc1.y);
	}
	if (num_isc > 1) {
		pa_big_round(isc2.x);
		pa_big_round(isc2.y);
	}


	if (num_isc == 0)
		return RND_R_DIR_NOT_FOUND;


rnd_trace("   [%d] %.2f;%.2f", num_isc, pa_big_double(isc1.x), pa_big_double(isc1.y));
if (num_isc > 1)
	rnd_trace("  and   %.2f;%.2f\n", pa_big_double(isc2.x), pa_big_double(isc2.y));
else
	rnd_trace("\n");

	/* Having two intersections means line-line overlap: class 5 */
	if (num_isc == 2) {
		rnd_vnode_t *ipa1, *ipa2, *ipb1, *ipb2, *sha, *shb;
		rnd_vnode_t *cv = ctx->v, *sv = s->v; /* have to make copies because inserting points may change s and thus s->v */
		pa_big2_coord_t d1, d2;
		int alloced = 0;

		/* create the two nodes on cv and sv (first the farther one) */
		rnd_pa_big_load_cvc(&start, cv);
		rnd_vect_u_dist2_big(d1, start, isc1);
		rnd_vect_u_dist2_big(d2, start, isc2);
		if (pa_big2_coord_cmp(d1, d2) > 0) {
			ipa1 = pa_selfisc_ins_pt2(ctx, cv, isc1, &alloced);
			ipa2 = pa_selfisc_ins_pt2(ctx, cv, isc2, &alloced);
			sha = ipa2;
		}
		else {
			ipa2 = pa_selfisc_ins_pt2(ctx, cv, isc2, &alloced);
			ipa1 = pa_selfisc_ins_pt2(ctx, cv, isc1, &alloced);
			sha = ipa1;
		}

		rnd_pa_big_load_cvc(&start, sv);
		rnd_vect_u_dist2_big(d1, start, isc1);
		rnd_vect_u_dist2_big(d2, start, isc2);

		if (pa_big2_coord_cmp(d1, d2) > 0) {
			ipb1 = pa_selfisc_ins_pt2(ctx, sv, isc1, &alloced);
			ipb2 = pa_selfisc_ins_pt2(ctx, sv, isc2, &alloced);
			shb = ipb2;
		}
		else {
			ipb2 = pa_selfisc_ins_pt2(ctx, sv, isc2, &alloced);
			ipb1 = pa_selfisc_ins_pt2(ctx, sv, isc1, &alloced);
			shb = ipb1;
		}

		/* mark the shared section */
		shb->shared = sha;
		sha->shared = shb;

		rnd_trace("   shared seg marked (alloced=%d)\n", alloced);
		ctx->num_isc++;
		if (alloced)
			return RND_R_DIR_CANCEL; /* modified the tree, need to restart the search at this node */
		return RND_R_DIR_NOT_FOUND;
	}

	/* single intersection between two lines */
	ip1 = pa_selfisc_ins_pt(ctx, ctx->v, isc1);
	if (ip1 != NULL) got_isc = 1;

	ip2 = pa_selfisc_ins_pt(ctx, s->v, isc1);
	if (ip2 != NULL) got_isc = 1;

	if (got_isc) {
		ctx->num_isc++;
		return RND_R_DIR_CANCEL; /* modified the tree, need to restart the search at this node */
	}

	return RND_R_DIR_NOT_FOUND;
}

static rnd_r_dir_t pa_selfisc_install_cvc_cb(const rnd_box_t *b, void *cl)
{
	pa_selfisc_t *ctx = (pa_selfisc_t *)cl;
	pa_seg_t *s = (pa_seg_t *)b;
	rnd_vnode_t *offender;
	pa_big_vector_t pt;
	pa_conn_desc_t *list;


	/* ignore previous/next edge so a plain edge-edge chain won't cause cvcs */
	if ((s->v == ctx->v) || (s->v == ctx->v->next) || (s->v == ctx->v->prev))
		return RND_R_DIR_NOT_FOUND;

	/* Our target point is ctx->v; there is a third party offender in s->v
	   potentially ending on ctx->v, find which one is that */
	if (on_same_pt(ctx->v, s->v))
		offender = s->v;
	else if (on_same_pt(ctx->v, s->v->next))
		offender = s->v->next;
	else
		return RND_R_DIR_NOT_FOUND;


	if ((offender->cvclst_prev != NULL) && (ctx->v->cvclst_prev != NULL))
		return RND_R_DIR_NOT_FOUND;

	if (ctx->v->cvclst_prev == NULL) {
		TODO("optimize: if we are all integers, avoid high res coords here");
		rnd_pa_big_load_cvc(&pt, ctx->v);

		/* first offender: need to create ctx->v's cvc */
		ctx->v->cvclst_prev = pa_prealloc_conn_desc(pt);
		ctx->v->cvclst_next = pa_prealloc_conn_desc(pt);
		list = pa_add_conn_desc_at(ctx->v, 's', NULL);

		rnd_trace("  include initial:  %ld;%ld - %ld;%ld\n",
			(long)ctx->v->point[0], (long)ctx->v->point[1],
			(long)ctx->v->next->point[0], (long)ctx->v->next->point[1]);
	}
	else {
		list = ctx->v->cvclst_prev;
		memcpy(&pt, &ctx->v->cvclst_prev->isc, sizeof(pt));
	}

	rnd_trace("  include offender: %ld;%ld - %ld;%ld\n",
		(long)offender->point[0], (long)offender->point[1],
		(long)offender->next->point[0], (long)offender->next->point[1]);

	/* add the offender */
	offender->cvclst_prev = pa_prealloc_conn_desc(pt);
	offender->cvclst_next = pa_prealloc_conn_desc(pt);
	list = pa_add_conn_desc_at(offender, 's', list);

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
	rnd_vnode_t *onto, *shtest;

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
		rnd_trace("straight to %.2f %.2f\n", NODE_CRDS(onto));

		return onto;
	}

	start = c = n->cvclst_prev->next;
	rnd_trace("CVC (c->side=%c *dir=%c)\n", c->side, *dir);
	do {
		int eff_dir = pa_eff_dir_forward(c->side, *dir);

		if (eff_dir) {
			onto = c->parent->next;
			shtest = c->parent;
		}
		else shtest = onto = c->parent->prev;

		rnd_trace("  %.2f %.2f '%c' eff_dir=%d sh?={%p}", NODE_CRDS(onto), c->side, eff_dir, c->parent);
		if (shtest->shared) {
			rnd_trace(" refuse (shared)\n");
			continue;
		}
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
			rnd_trace(" accept, dir '%c' (start %.2f;%.2f)!\n", *dir, NODE_CRDS(onto));
			if (eff_dir)
				c->parent->flg.mark = 1;
			else
				onto->flg.mark = 1;
			return onto;
		}
	} while((c = c->prev) != start);

	fprintf(stderr, "pa_self_isc: nowhere to go in next_o\n");
	abort();
}


/* Stub detection: next node has a cvc and the previous and next nodes are at
	   the same place, so there are two overlapping lines from and to 'start'.
	   In this case start from the next point and when we reach prev simply
	   stop so that this stub is excluded.
	
	   This needs to be done in a loop because there may be a chain of stubs
	   (and we are starting from the far end), see gixedm5:
	
	   *  556;421 <- starting point
	   |
	   *  556;422
	   |
	   *  556;423 <- node pair; new start is the right point, stop_at is the left
	   |'\
	   |  '\
	   *----'* 558;425
	   557;
	   425
	
	   Other stubs elsewhere on the loop are safely discarded by
	   pa_selfisc_next_o(), this affects the starting point only.
*/
RND_INLINE int is_node_in_stub(rnd_vnode_t **start, rnd_vnode_t **sprev, rnd_vnode_t **stop_at)
{
	if (((*start)->next->cvclst_next != NULL) && pa_vnode_equ(*sprev, (*start)->next)) {
		*stop_at = *sprev;
		(*start)->flg.mark = 1; /* don't start at this node */
		*start = (*start)->next;
		*sprev = (*sprev)->prev;
		return 1;
	}

	/* Special case: gixedm6: if there's a longer chain the last-minus-1st node
	   won't look like a stub because the last node of the stub has empty cvc */
	if (((*start)->cvclst_next != NULL) && ((*start)->next->cvclst_next == NULL) && pa_vnode_equ(*start, (*start)->next->next)) {
		/* jump onto the next node, assuming it's the end of the stub */
		(*start)->flg.mark = 1; /* don't start at this node */
		*start = (*start)->next;
		*sprev = (*start)->prev;
		*stop_at = *sprev;
		return 1;
	}

	return 0;
}

/* Stubs have to be removed before outline collection because a stub can
   overlap a legit edge that needs to be kept. This is done in this
   preprocessing step. The function returns start or NULL. When NULL is
   returned, the polyline has been modified and needs to be re-mapped.
   Related test case: gixedp; legit edge is:
     (39;292) 48;289 49;289 50;289 (52;289)
   offending overlapping stub is:
     (52;288) 50;289 49;289 48;289 49;289 50;289 (52;287)

   A simpler test case is si_class6

   This also fixed fixedm4.
*/
static rnd_vnode_t *stub_remover(pa_selfisc_t *ctx, rnd_vnode_t *start)
{
	rnd_vnode_t *n, *loop_start, *sprev, *stop_at, *next;

/*	rnd_trace("STUB removal\n");*/

	/* don't start from a stub */
	loop_start = start;
	sprev = start->prev;
	while(is_node_in_stub(&loop_start, &sprev, &stop_at)) ;

	n = loop_start;
	do {
		bypass_restart:;
		next = n->next;
		if ((n->next->cvclst_next != NULL) && (n->prev->cvclst_next != NULL) && pa_vnode_equ(n->prev, n->next)) {
			pa_conn_desc_t *c1, *c2;

			/* n is the endpoint of a stub */
/*			rnd_trace("STUB  found endpoint at: %ld;%ld prev=%ld;%ld next=%ld;%ld\n", n->point[0], n->point[1], n->prev->point[0], n->prev->point[1], n->next->point[0], n->next->point[1]);*/

			if (n == start)
				start = n->next;

/*rnd_trace("    stub removal1: %ld;%ld -> %ld;%ld\n", n->point[0], n->point[1], n->next->point[0], n->next->point[1]);*/
			rnd_poly_vertex_exclude(ctx->pl, n);

			/* now we have two points at the new stub endpoint: next and next->prev;
			   Remove one of them. */
			n = next;
			next = n->prev;
/*rnd_trace("    stub removal2: %ld;%ld -> %ld;%ld\n", n->point[0], n->point[1], n->next->point[0], n->next->point[1]);*/
			rnd_poly_vertex_exclude(ctx->pl, n);

			start = NULL;

			if (n == loop_start) {
				loop_start = next;
				n = next;
				goto bypass_restart;
			}
		}
	} while((n = next) != loop_start);

	return start;
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

	rnd_trace("selfi collect outline from %.2f %.2f\n", NODE_CRDS(start));

	/* append dst to the list of plines */
	posneg_append_pline(posneg, +1, dst);

	/* collect a closed loop */
	last = dst->head;

	n = pa_selfisc_next_o(start, &dir);

	for(; (n != start) && (n != NULL); n = pa_selfisc_next_o(n, &dir)) {
		rnd_trace(" at out %.2f %.2f {%p}", NODE_CRDS(n), n);
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

/* Step from n to the next node according to dir, walking a hole island (trying to
   get minimal area portions) */
RND_INLINE rnd_vnode_t *pa_selfisc_next_i(rnd_vnode_t *n, char *dir, rnd_vnode_t **first, rnd_vnode_t *real_start)
{
	pa_conn_desc_t *c, *start;
	rnd_vnode_t *onto, *shtest;

	rnd_trace("    next (first:%p dir='%c'): ", first, *dir);
	if (n->cvclst_prev == NULL) {
		if (*dir == 'N') {
			onto = n->prev;
			onto->flg.mark = 1;
rnd_trace("[mark %.2f;%.2f] ", NODE_CRDS(onto));
			if (first) {
				*first = onto;
				onto->flg.start = 1;
			}
		}
		else {
			onto = n->next;
			n->flg.mark = 1;
rnd_trace("[mark %.2f;%.2f] ", NODE_CRDS(n));
			if (first) {
				*first = n;
				n->flg.start = 1;
			}
		}
		rnd_trace("straight to %.2f %.2f\n", NODE_CRDS(onto));
		return onto;
	}

	rnd_trace("CVC\n");

	/* quick return: if we can get back to the starting point from this CVC, do
	   that; other loops will be picked up separately. Test case: gixedj */
	if (real_start != NULL) {
		start = c = n->cvclst_prev->next;
		do {
			int eff_dir = pa_eff_dir_forward(c->side, *dir);

			if (eff_dir) onto = c->parent->prev;
			else onto = c->parent->next;

			if (onto == real_start)
				return NULL; /* no need to return the start point, it's already added */
		} while((c = c->prev) != start);
	}

	/* normal CVC next: pick shorter loop (by angle) */
	start = c = n->cvclst_prev->next;
	do {
		int eff_dir = pa_eff_dir_forward(c->side, *dir);

		if (eff_dir) {
			onto = c->parent->prev;
			shtest = onto;
		}
		else {
			onto = c->parent->next;
			shtest = c->parent;
		}

		rnd_trace("     %.2f %.2f '%c' m%d s%d %d onto={%p} parent={%p}", NODE_CRDS(onto), c->side, onto->flg.mark, onto->flg.start, c->parent->flg.start, onto, c->parent);
		if (c->parent->flg.start) {
			if (first) continue; /* tried to start onto an already mapped path; try the next */
			return NULL; /* arrived back to the starting point - finish normally */
		}

		if (shtest->shared) {
			rnd_trace(" refuse (shared)\n");
			continue;
		}

		if (!onto->flg.mark || onto->flg.start) { /* also accept flg.start here: greedy algorithm to find shortest loops */
			if (!onto->flg.start) /* don't remember new direction going out from the endpoint; test case: gixedbo2 */
				*dir = eff_dir ? 'N' : 'P';
			if (eff_dir) {
				onto->flg.mark = 1;
				if (first) {
					*first = onto;
					onto->flg.start = 1;
				}
				rnd_trace(" mark %.2f %.2f M1a s%d {%p}", NODE_CRDS(onto), first, onto);
			}
			else {
				c->parent->flg.mark = 1;
				if (first) {
					*first = c->parent;
					c->parent->flg.start = 1;
				}
				rnd_trace(" mark %.2f %.2f M1b s%d {%p}", NODE_CRDS(c->parent), first, c->parent);
			}

			rnd_trace("    accept, dir '%c' (onto)!\n", *dir);
			return onto;
		}
		rnd_trace("    refuse (marked)\n");
	} while((c = c->prev) != start);

	if (!first) {
		/* didn't find a way out of CVC that's still available or is closing the
		   loop; this happens with stubs left over by multi line-line overlap;
		   test cases: si_class5c, fixedp */
	}
	else {
		/* can't start in any direction: all islands are mapped, return empty */
	}
	return NULL;
}

/* Collect all unmarked hole islands starting from a cvc node */
RND_INLINE void pa_selfisc_collect_island(pa_posneg_t *posneg, rnd_vnode_t *start)
{
	int accept_pol = 0, has_selfisc = 0;
	char dir = 'N';
	rnd_vnode_t *n, *newn, *last, *started = NULL;
	rnd_pline_t *dst;

	dst = pa_pline_new(start->point);
	last = dst->head;

	rnd_trace("  island {:\n");
	rnd_trace("   IS1 %.2f %.2f\n", NODE_CRDS(start));
	n = pa_selfisc_next_i(start, &dir, &started, NULL);
	for(; (n != start) && (n != NULL); n = pa_selfisc_next_i(n, &dir, 0, started)) {
		rnd_trace("   IS2 %.2f %.2f\n", NODE_CRDS(n));

		/* This is rounding n->cvc into newn->point */
		newn = calloc(sizeof(rnd_vnode_t), 1);
		newn->point[0] = n->point[0];
		newn->point[1] = n->point[1];
		rnd_trace("      appn: %ld;%ld %p\n", newn->point[0], newn->point[1], newn);
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
	rnd_trace("\n  } (end island: len=%d dir=%c PLF=%d accept=%d)\n", dst->Count, dir, dst->flg.orient == RND_PLF_DIR, accept_pol);
	if (started != NULL)
		started->flg.start = 0;

	if (accept_pol != 0) {
		if ((accept_pol > 0) && (dst->flg.orient != RND_PLF_DIR))
			pa_pline_invert(dst);
		else if ((accept_pol < 0) && (dst->flg.orient == RND_PLF_DIR))
			pa_pline_invert(dst);

		if (has_selfisc) {
			/* need to rebuild the tree because of node deletion */
			rnd_r_free_tree_data(dst->tree, free);
			rnd_r_destroy_tree(&dst->tree);
			pa_pline_update(dst, 1);
		}
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
		
		rnd_trace(" at isl %.2f %.2f\n", NODE_CRDS(n));
		c = cstart;
		do {
			if (!c->parent->flg.mark)
				pa_selfisc_collect_island(posneg, c->parent);
		} while((c = c->next) != cstart);

	} while((n = n->next) != start);
}

/* Figure if there's a stub (overlapping pair of lines sticking out from nd) */
int stub_from(rnd_vnode_t *nd)
{
	pa_conn_desc_t *c = nd->cvclst_prev;
/*	rnd_trace("STUB at %ld %ld:\n", nd->point[0], nd->point[1]);*/
	do {
#if 0
		rnd_trace(" %f %d ", pa_big_double(c->angle), (int)c->prelim);
		rnd_trace("P %p (%ld %ld) ", c->parent->prev, (long)c->parent->prev->point[0], (long)c->parent->prev->point[1]);
		rnd_trace("N %p (%ld %ld)\n", c->parent->next, (long)c->parent->next->point[0], (long)c->parent->next->point[1]);
#endif
		if (pa_big_coord_cmp(c->angle, c->prev->angle) == 0) {
			/* test case: gixedd at 836;559 <-> 835;559 */
TODO("arc: simply having the same angle works for lines but won't work for arcs; check if the outgoing edges fully overlap");
			rnd_trace("(STUB detected at %ld %ld)\n", nd->point[0], nd->point[1]);
			return 1;
		}
	} while((c = c->next) != nd->cvclst_prev);
	return 0;
}

/* Special case: self touching means we have a point that's visited twice
   on the outline. Normally this is a >< topology, but due to rouding
   there may be a triangle flip and the topology may change into a X.
   Test case: fixedo at 225;215. Detect the X case and mark ctx self-isc */
static void selfisc_detect_cvc_crossing(pa_selfisc_t *ctx, rnd_vnode_t *nd)
{
	rnd_trace(" CVC cross detect: %ld;%ld\n", nd->point[0], nd->point[1]);

#if 0
	pa_conn_desc_t *c;

	c = nd->cvclst_prev;
	do {
		rnd_vnode_t *cn = c->parent;
		rnd_trace("  LS:   %p == %p\n", c->parent, nd);
		rnd_trace("  ls:   %ld;%ld {%ld;%ld} %ld;%ld (%d) %c\n",
			cn->prev->point[0], cn->prev->point[1],
			cn->point[0], cn->point[1],
			cn->next->point[0], cn->next->point[1],
			c->prelim, c->side
		);
	} while((c = c->next) != nd->cvclst_prev);
#endif

	if (pa_cvc_crossing_at_node(nd)) {
		ctx->num_isc++;
		rnd_trace("  -> X crossing\n");
	}
	else if (stub_from(nd)) {
		rnd_trace("  -> no crossing, >< topology but found stub\n");
		ctx->num_isc++;
	}
	else
		rnd_trace("  -> no crossing, >< topology\n");
}


static rnd_vnode_t *split_selfisc_map(pa_selfisc_t *ctx)
{
	rnd_vnode_t *n, *start, *next;
	int has_cvc = 0;

	/* the outline mapper needs minnode anyway, best if we figure that early
	   so all output is deterministic */
	n = start = pa_find_minnode(ctx->pl);
	rnd_trace("self-isc: map_cross start\n");
	do {
		rnd_box_t box;
		int rr;

		rnd_trace(" map cross: %.2f;%.2f - %.2f;%.2f\n", NODE_CRDS(n), NODE_CRDS(n->next));

		next = n->next;
		ctx->v = n;

		box.X1 = pa_min(n->point[0], n->next->point[0]); box.Y1 = pa_min(n->point[1], n->next->point[1]);
		box.X2 = pa_max(n->point[0], n->next->point[0]); box.Y2 = pa_max(n->point[1], n->next->point[1]);
		do {
			rr = rnd_r_search(ctx->pl->tree, &box, NULL, pa_selfisc_map_cross_cb, ctx, NULL);
		} while(rr == RND_R_DIR_CANCEL);
	} while((n = next) != start);

	/* There are only integer coords and point-point intersections now; there
	   can be T or X junctions in the input, so it is not enough to check the
	   newly added points. It's (probably) the cheapest to just do a search from
	   each point and add CVCs to any that ahs more than 2 edges coming in . */
	n = start;
	rnd_trace("self-isc: install cvc start\n");
	do {
		rnd_box_t box;

		rnd_trace(" inst cvc: %.2f;%.2f\n", NODE_CRDS(n));

		n->flg.mark = 0;
		ctx->v = n;

		box.X1 = n->point[0];   box.Y1 = n->point[1];
		box.X2 = n->point[0]+1; box.Y2 = n->point[1]+1;
		rnd_r_search(ctx->pl->tree, &box, NULL, pa_selfisc_install_cvc_cb, ctx, NULL);

		if (n->cvclst_prev != NULL)
			has_cvc = 1;
	} while((n = n->next) != start);


	/* crossing happening in point-point on the input side - these
	   are not caught above; have to check only if we haven't already
	   decided we have intersections */
	if (has_cvc && (ctx->num_isc == 0)) {
		n = start;
		do {
			if (n->cvclst_prev != NULL)
				selfisc_detect_cvc_crossing(ctx, n);
			n = n->next;
		} while((ctx->num_isc == 0) && (n != start));
	}

	return start;
}


/* Build the outer contour of self-intersecting pl. Return
   whether there was a self intersection (and posneg got loaded). */
static rnd_bool rnd_pline_split_selfisc_o(pa_posneg_t *posneg, rnd_pline_t *pl)
{
	rnd_vnode_t *start;
	pa_selfisc_t ctx = {0};
	long i;

	ctx.pl = pl;
	start = split_selfisc_map(&ctx);
	if (ctx.num_isc == 0)
		return 0; /* no self intersection */

	start = stub_remover(&ctx, start);
	if (start == NULL) {
		pl_remove_cvcs(pl, 1);
		start = split_selfisc_map(&ctx);
		if (ctx.num_isc == 0)
			return 0; /* no self intersection */
	}

	/* preserve original holes as negatives */
	if (pl->next != NULL) {
		posneg_append_pline(posneg, -1, pl->next);
		pl->next = NULL;
	}

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

/* Build the contour of self-intersecting hole pl. Return
   whether there was a self intersection (and posneg got loaded). */
static rnd_bool rnd_pline_split_selfisc_i(pa_posneg_t *posneg, rnd_polyarea_t **pa, rnd_pline_t *pl, rnd_pline_t *pl_prev, rnd_pline_t **prev_out)
{
	rnd_vnode_t *start;
	pa_selfisc_t ctx = {0};
	int first_pos_null;

	/* cut_line_line_overlap is disbaled, see test case fixedu: when collecting
	   islands we are going by picking smallest loops; in this case a "bone"
	   construct causes no problem as it's simply a two-vertex polygon that
	   is eliminated. */
	ctx.pl = pl;
	start = split_selfisc_map(&ctx);
	if (ctx.num_isc == 0)
		return 0; /* no self intersection */

	start = stub_remover(&ctx, start);
	if (start == NULL) {
		pl_remove_cvcs(pl, 1);
		start = split_selfisc_map(&ctx);
		if (ctx.num_isc == 0)
			return 0; /* no self intersection */
	}

	first_pos_null = (posneg->first_pos == NULL);

	pa_selfisc_collect_islands(posneg, start);

	/* special case optimization: self intersecting cutout without a
	   self intersecting outline; just put back all parts into pl without
	   having to do expensive poly bools */
	if (first_pos_null) {
		long n;


		if (posneg->neg_head != NULL) { /* add cutouts */
			rnd_pline_t *ng;

			/* link in new islands after the original cutout */
			posneg->neg_tail->next = pl->next;
			pl->next = posneg->neg_head;

			/* remove the original (self-intersecting) cutout */
			pa_pline_unlink(*pa, pl_prev, pl);
			pa_pline_free(&pl);

			*prev_out = posneg->neg_tail;
		}

		if ((posneg->first_pos != NULL) || (posneg->subseq_pos.used != 0)) { /* add positives - they are all within the negative */
			if ((posneg->first_pos != NULL) && (posneg->subseq_pos.used == 0))
				vtp0_append(&posneg->subseq_pos, posneg->first_pos);
			for(n = 0; n < posneg->subseq_pos.used; n++) {
				rnd_polyarea_t *pa_new;
				rnd_pline_t *island = posneg->subseq_pos.array[n];
				
				assert(island->flg.orient == RND_PLF_DIR);
				pa_new = pa_polyarea_alloc();
				rnd_polyarea_contour_include(pa_new, island);
				rnd_polyarea_m_include(pa, pa_new);
			}
		}

		/* reset posneg */
		vtp0_uninit(&posneg->subseq_pos);
		posneg->first_pos = NULL;
		posneg->neg_head = posneg->neg_tail = NULL;

		return 0;
	}

	return 1;
}

static int cmp_pline_area(const void *Pl1, const void *Pl2)
{
	const rnd_pline_t *pl1 = Pl1, *pl2 = Pl2;
	return (pl1->area < pl2->area) ? +1 : -1;
}

/* Add plines of posneg in pa, replacing the original pl (freeing it) */
RND_INLINE void split_pline_add_islands(rnd_polyarea_t **pa, rnd_pline_t *pl, pa_posneg_t *posneg, rnd_pline_t *firstpos)
{
	long n;

	/* first positive: replace existing pl in pa; really just swap the
	   vertex list and islands... This is an optimization that saves
	   on memory allocations plus keeps the polyarea as intact as
	   possible for the simple/common cases. Note: firstpos is NULL if
	   outline did not self-intersect. */
	if (firstpos != NULL) {
		SWAP(rnd_vnode_t *, pl->head, firstpos->head);
		SWAP(rnd_pline_t *, pl->next, firstpos->next);
		SWAP(rnd_rtree_t *, pl->tree, firstpos->tree);

		if (pl->tree != NULL) {
			rnd_rtree_t *r = pl->tree;
			rnd_r_free_tree_data(r, free);
			rnd_r_destroy_tree(&r);
			pl->tree = NULL;
		}
	
		pa_pline_update(pl, 0);

		if (pl->flg.orient != RND_PLF_DIR)
			pa_pline_invert(pl);

		/* ... so newpl holds the old list now and can be freed */
		pa_pline_free(&firstpos);
	}

	/* insert subsequent islands; skip the 0th one, it's already in */
	for(n = 1; n < posneg->subseq_pos.used; n++) {
		rnd_polyarea_t *pa_new;
		rnd_pline_t *island = posneg->subseq_pos.array[n];
		
		assert(island->flg.orient == RND_PLF_DIR);
		pa_new = pa_polyarea_alloc();
		rnd_polyarea_contour_include(pa_new, island);
		rnd_polyarea_m_include(pa, pa_new);
	}

	vtp0_uninit(&posneg->subseq_pos);
}

/* We have a fixed up a self-intersecting pline; need to re-insert it in pa,
   replacing pl */
RND_INLINE void split_selfisc_pline_resolved(rnd_polyarea_t **pa, rnd_pline_t *pl, pa_posneg_t *posneg)
{
	rnd_pline_t *hole, *hole_next, *last, *firstpos = posneg->first_pos;
	long n;

	/* install holes (neg) in islands (pos) */
	if (posneg->subseq_pos.used == 0) {
		if ((firstpos != NULL) && (firstpos->tree == NULL)) /* only islands self-intersected, not the outline */
			pa_pline_update(firstpos, 0);
		only_one_island:;
		/* special case optimization: if there's only one positive island,
		   all holes go in there - this is the common case, only the "bone"
		   cases will result in multiple positive islands */
		if (firstpos == NULL) {
			if (posneg->neg_head != NULL) {
				/* append islands collected in the hole resolver loop above */
				for(last = pl; last->next != NULL; last = last->next) ;
				last->next = posneg->neg_head;
				posneg->neg_head = NULL;
			}
		}
		else
			firstpos->next = posneg->neg_head;
		
		posneg->neg_head = posneg->neg_tail = NULL;
	}
	else {
		/* make sure all islands are updated so they have an area */
		for(n = 0; n < posneg->subseq_pos.used; n++) {
			rnd_pline_t *island = posneg->subseq_pos.array[n];
			if (island->area <= 0)
				pa_pline_update(island, 0);
		}

		/* sort islands so that the largest is first; there are only
		   a few islands expected so this shouldn't be too slow */
		qsort(posneg->subseq_pos.array, posneg->subseq_pos.used, sizeof(void *), cmp_pline_area);

		/* if a positive island is within a larger positive island, remove it */
		for(n = 1; n < posneg->subseq_pos.used; n++) {
			long m;
			int del = 0;
			rnd_pline_t *small = posneg->subseq_pos.array[n];
			
			for(m = 0; m < n; m++) {
				rnd_pline_t *big = posneg->subseq_pos.array[m];
				if (pa_pline_inside_pline(big, small)) {
					del = 1;
					break;
				}
			}

			if (del) {
				pa_pline_free(&small);
				vtp0_remove(&posneg->subseq_pos, n, 1);
				n--;
			}
		}

		if (posneg->neg_head != NULL) {
			if (posneg->subseq_pos.used < 2)
				goto only_one_island; /* the above deletions of redundant positives may have lead to this */

			/* find out which hole goes in which island; after the pline self isc
			   resolve function there's no intersection. Put every hole in the
			   smallest island it is inside */
			for(hole = posneg->neg_head; hole != NULL; hole = hole_next) {
				int found = 0;

				hole_next = hole->next;
				hole->next = NULL;

				for(n = posneg->subseq_pos.used-1; n >= 0; n--) {
					rnd_pline_t *island = posneg->subseq_pos.array[n];
					if (pa_pline_inside_pline(island, hole)) {
						hole->next = island->next;
						island->next = hole;
						found = 1;
						break;
					}
				}
				if (!found) /* Hole not in any island; forget it; test case: gixedg */
					pa_pline_free(&hole);
			}
		}

		firstpos = posneg->subseq_pos.array[0];
	}

	split_pline_add_islands(pa, pl, posneg, firstpos);
}

/* Handle self-interesecting plines (outlines or holes), e.g. bowties */
RND_INLINE rnd_cardinal_t split_selfisc_pline(rnd_polyarea_t **pa)
{
	rnd_polyarea_t *pa_start, *paf;
	rnd_pline_t *pl, *next, *prev;

	pa_start = *pa;
	do {
		pa_posneg_t posneg = {0};
		int has_selfisc;

		rnd_trace("^ pa %p (f=%p in=%p)\n", *pa, (*pa)->f, pa_start);

		/* remember pa->f so that new positive islands we are inserting after (*pa)
		   are not affected */
		paf = (*pa)->f;

		/* pline intersects itself: holes first; any self-intersecting hole is
		   removed, cleaned up (sometimes split) and as a result new loops are
		   added to posneg for later processing */
		has_selfisc = 0;
		prev = (*pa)->contours;
		for(pl = (*pa)->contours->next; pl != NULL; prev = pl, pl = next) {
			int si;
			next = pl->next;
			si = rnd_pline_split_selfisc_i(&posneg, pa, pl, prev, &pl);
			if (si) {
				/* remove pl, the resolved variant is in posneg already */
				pa_pline_unlink(*pa, prev, pl);
				pa_pline_free(&pl);
			}
			has_selfisc += si;
		}

		/* pline intersects itself: outline */
		pl = (*pa)->contours;
		has_selfisc += rnd_pline_split_selfisc_o(&posneg, pl);

		if (has_selfisc != 0)
			split_selfisc_pline_resolved(pa, pl, &posneg);

	} while((*pa = paf) != pa_start);

	return 0;
}
