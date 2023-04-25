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
	unsigned restart:1;

	rnd_vnode_t *search_seg_v;
	pa_seg_t *search_seg;
} pa_selfi_t;

static rnd_r_dir_t pa_selfi_find_seg_cb(const rnd_box_t *b, void *ctx_)
{
	pa_selfi_t *ctx = (pa_selfi_t *)ctx_;
	pa_seg_t *s = (pa_seg_t *)b;

	if (ctx->search_seg_v == s->v) {
		ctx->search_seg = s;
		return RND_R_DIR_CANCEL; /* found */
	}
	return RND_R_DIR_NOT_FOUND;
}

/* Find the segment for pt in tree */
RND_INLINE pa_seg_t *pa_selfi_find_seg(pa_selfi_t *ctx, rnd_vnode_t *pt)
{
	rnd_r_dir_t rres;
	rnd_box_t box;

	ctx->search_seg_v = pt;
	ctx->search_seg = NULL;

	box.X1 = pt->point[0];    box.Y1 = pt->point[1];
	box.X2 = box.X1 + 1;      box.Y2 = box.Y1 + 1;
	rres = rnd_r_search(ctx->pl->tree, &box, NULL, pa_selfi_find_seg_cb, ctx, NULL);
	assert(rres == RND_R_DIR_CANCEL);

	return ctx->search_seg;
}

/* Insert a new node and a cvc at an intersection point as the next node of vn */
RND_INLINE rnd_vnode_t *pa_selfi_ins_pt(pa_selfi_t *ctx, rnd_vnode_t *vn, pa_big_vector_t pt)
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
	sg = pa_selfi_find_seg(ctx, vn);
	if (pa_adjust_tree(ctx->pl->tree, sg) != 0)
		assert(0); /* failed memory allocation */

	return new_node;
}

/* Called back from an rtree query to figure if two edges intersect */
static rnd_r_dir_t pa_selfi_cross_cb(const rnd_box_t *b, void *cl)
{
	pa_selfi_t *ctx = (pa_selfi_t *)cl;
	pa_seg_t *s = (pa_seg_t *)b;
	pa_big_vector_t isc1, isc2;
	int num_isc, got_isc = 0;
	rnd_vnode_t *new_node;

	if ((s->v == ctx->v) || (s->v == ctx->v->next) || (s->v == ctx->v->prev))
		return RND_R_DIR_NOT_FOUND;

	num_isc = pa_isc_edge_edge_(s->v, s->v->next, ctx->v, ctx->v->next, &isc1, &isc2);
	if (num_isc == 0)
		return RND_R_DIR_NOT_FOUND;

	new_node = pa_selfi_ins_pt(ctx, ctx->v, isc1);
	if (new_node != NULL) got_isc = 1;

	new_node = pa_selfi_ins_pt(ctx, s->v, isc1);
	if (new_node != NULL) got_isc = 1;

	if (new_node != NULL)
		rnd_trace("isc1 %d %d | %d %d %d %d\n", new_node->point[0], new_node->point[1], ctx->v->point[0], ctx->v->point[1], ctx->v->next->point[0], ctx->v->next->point[1]);

TODO("overlap always means break and remove shared section");
	assert(num_isc < 2);

	if (got_isc) {
		ctx->num_isc++;
		ctx->restart = 1; /* because the rtree changed */
		return RND_R_DIR_CANCEL;
	}
	return RND_R_DIR_NOT_FOUND;
}

RND_INLINE rnd_vnode_t *pa_selfi_next(rnd_vnode_t *n)
{
	pa_conn_desc_t *c, *start;

	rnd_trace(" next: ");
	if (n->cvclst_prev == NULL) {
		rnd_trace("straight to %d %d\n", n->next->point[0], n->next->point[1]);
		return n->next;
	}

	rnd_trace("CVC\n");
	c = n->cvclst_next;
	start = c->prev;
	for(;;c = c->next) {
		assert(c != start); /* we came back to where we started without finding anything */
		rnd_trace("  %d %d ", c->parent->point[0], c->parent->point[1]);
		if (!c->parent->flg.mark) {
			rnd_trace(" accept!\n");
			return c->parent;
		}
		rnd_trace(" refuse (marked)\n");
	}
	
}

RND_INLINE void pa_selfi_collect(rnd_pline_t **dst_, rnd_pline_t *src, rnd_vnode_t *start)
{
	rnd_vnode_t *n, *last, *newn;
	rnd_pline_t *dst;

	assert(!start->flg.mark); /* should face marked nodes only as outgoing edges of intersections */
	start->flg.mark = 1;
	dst = pa_pline_new(start->point);

	rnd_trace("selfi collect from %d %d\n", start->point[0], start->point[1]);

	/* append dst to the list of plines */
	if (*dst_ != NULL) {
		rnd_pline_t *last;
		for(last = *dst_; last->next != NULL; last = last->next) ;
		last->next = dst;
	}
	else
		*dst_ = dst;

	/* collect a closed loop */
	last = start;
	for(n = pa_selfi_next(start); n != start; n = pa_selfi_next(n)) {
		rnd_trace(" at %d %d", n->point[0], n->point[1]);
		assert(!n->flg.mark); /* should face marked nodes only as outgoing edges of intersections */
		n->flg.mark = 1;
		newn = calloc(sizeof(rnd_vnode_t), 1);
		newn->point[0] = n->point[0];
		newn->point[1] = n->point[1];
		rnd_poly_vertex_include(last, newn);
		last = n;
	}
}

rnd_pline_t *rnd_pline_split_selfi(rnd_pline_t *pl)
{
	rnd_vnode_t *n, *start = pa_find_minnode(pl);
	pa_selfi_t ctx = {0};
	rnd_pline_t *res = NULL;

	ctx.pl = pl;

	n = start;
	do {
		rnd_box_t box;

		n->flg.mark = 0;
		ctx.v = n;
		box.X1 = pa_min(n->point[0], n->next->point[0]); box.Y1 = pa_min(n->point[1], n->next->point[1]);
		box.X2 = pa_max(n->point[0], n->next->point[0]); box.Y2 = pa_max(n->point[1], n->next->point[1]);
		do {
			ctx.restart = 0;
			rnd_r_search(pl->tree, &box, NULL, pa_selfi_cross_cb, &ctx, NULL);
		} while(ctx.restart);

	} while((n = n->next) != start);

	if (ctx.num_isc == 0)
		return pl;

	/* collect outer line */
	pa_selfi_collect(&res, pl, start);

	pa_pline_free(&pl);

	return res;
}
