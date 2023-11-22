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

/* rnd_pline_t low level and internal */

/* Allocate a new node for a given location. Returns NULL if allocationf fails. */
rnd_vnode_t *rnd_poly_node_create(rnd_vector_t v)
{
	rnd_vnode_t *res = calloc(sizeof(rnd_vnode_t), 1);

	assert(v != NULL);
	if (res == NULL)
		return NULL;

	Vcpy2(res->point, v);
	return res;
}

/* Return node for a point next to dst. Returns existing node on coord
   match else allocates a new node. Returns NULL if new node can't be
   allocated. */
rnd_vnode_t *rnd_poly_node_add_single(rnd_vnode_t *dst, rnd_vector_t ptv)
{
	rnd_vnode_t *newnd;

  /* no new allocation for redundant node around dst */
	if (Vequ2(ptv, dst->point))        return dst;
	if (Vequ2(ptv, dst->next->point))  return dst->next;

	/* have to allocate a new node */
	newnd = rnd_poly_node_create(ptv);
	if (newnd != NULL)
		newnd->flg.plabel = PA_PTL_UNKNWN;

	return newnd;
}

static rnd_vnode_t *pa_node_add_single(rnd_vnode_t *dst, pa_big_vector_t pt)
{
#ifdef PA_BIGCOORD_ISC
	return pa_big_node_add_single(dst, pt);
#else
	return rnd_poly_node_add_single(dst, pt);
#endif
}


static pa_conn_desc_t *pa_prealloc_conn_desc(pa_big_vector_t isc);

/* Return a new node for a point next to dst, or NULL if pt falls on an existing
   node. Crashes/asserts if allocation fails. Preallocates cross vertex
   descriptor */
static rnd_vnode_t *pa_ensure_point_and_prealloc_cvc(rnd_vnode_t *dst, pa_big_vector_t pt)
{
	rnd_vnode_t *dnext = dst->next, *newnd;

	newnd = pa_node_add_single(dst, pt);
	assert(newnd != NULL);

	/* we may already have a pre-allocation to this point */
	if (newnd->cvclst_prev == NULL)
		newnd->cvclst_prev = pa_prealloc_conn_desc(pt);
	if (newnd->cvclst_next == NULL)
		newnd->cvclst_next = pa_prealloc_conn_desc(pt);

	if ((newnd == dst) || (newnd == dnext))
		return NULL; /* no new allocation */

	return newnd;
}

/* Returns whether pl1's bbox can overlap/intersect pl2's bbox */
RND_INLINE rnd_bool pa_pline_box_inside(rnd_pline_t *pl1, rnd_pline_t *pl2)
{
	assert((pl1 != NULL) && (pl2 != NULL));

	if (pl1->xmin < pl2->xmin) return 0;
	if (pl1->ymin < pl2->ymin) return 0;
	if (pl1->xmax > pl2->xmax) return 0;
	if (pl1->ymax > pl2->ymax) return 0;

	return 1;
}

/* Returns whether pt is within the bbox of pl */
RND_INLINE rnd_bool pa_is_point_in_pline_box(const rnd_pline_t *pl, const rnd_vector_t pt)
{
	if (pt[0] < pl->xmin) return 0;
	if (pt[1] < pl->ymin) return 0;
	if (pt[0] > pl->xmax) return 0;
	if (pt[1] > pl->ymax) return 0;

	return 1;
}

/* Returns true if node a and b are neighbours or are the same node */
RND_INLINE rnd_bool pa_are_nodes_neighbours(rnd_vnode_t *a, rnd_vnode_t *b)
{
	if (a == b) return 1;
	if ((a->next == b) || (b->next == a)) return 1;
	if (a->next == b->next) return 1; /* when could this even happen?! */

	return 0;
}

void pa_pline_init(rnd_pline_t *pl)
{
	if (pl == NULL)
		return;

	if (pl->head == NULL)
		pl->head = calloc(sizeof(rnd_vnode_t), 1);

	pl->head->next = pl->head->prev = pl->head;
	pl->xmin = pl->ymin = RND_COORD_MAX;
	pl->xmax = pl->ymax = -RND_COORD_MAX-1;
}

rnd_pline_t *pa_pline_new(const rnd_vector_t pt)
{
	rnd_pline_t *res = calloc(sizeof(rnd_pline_t), 1);
	if (res == NULL)
		return NULL;

	pa_pline_init(res);

	if (pt != NULL) {
		res->head->next = res->head->prev = res->head;
		Vcpy2(res->head->point, pt);
		pa_pline_box_bump(res, pt);
	}

	return res;
}

RND_INLINE void pa_pline_free_fields(rnd_pline_t *pl)
{
	rnd_vnode_t *n, *next;

	/* free cross-vertex-connectin list for all nodes in pl; free all nodes too */
	n = pl->head;
	do {
		next = n->next;
		if (n->cvclst_next != NULL) {
			free(n->cvclst_next);
			free(n->cvclst_prev);
		}
		free(n);
		n = next;
	} while(n != pl->head);

	if (pl->tree) {
		rnd_rtree_t *r = pl->tree;
		rnd_r_free_tree_data(r, free);
		rnd_r_destroy_tree(&r);
	}
}

void pa_pline_free(rnd_pline_t **pl)
{
	if (*pl == NULL)
		return;

	pa_pline_free_fields(*pl);

	free(*pl);
	*pl = NULL;
}

void pa_pline_optimize(rnd_pline_t *pl)
{
	rnd_vnode_t *p = pl->head, *c; /* previous and current node in the iteration */

	for(c = p->next; c != pl->head; c = (p = c)->next) {
		rnd_vector_t p1, p2;

		Vsub2(p1, c->point, p->point);
		Vsub2(p2, c->next->point, c->point);

		/* If det2 is zero then the points on either side of c are on the same line! */
		if (rnd_vect_det2(p1, p2) == 0) {
			rnd_poly_vertex_exclude(pl, c);
			free(c);
			c = p;
		}
	}
}

void pa_pline_update(rnd_pline_t *pl, rnd_bool optimize)
{
	double area = 0;
	rnd_vnode_t *p, *c; /* previous and current node in the iteration */

	assert(pl != NULL);

	if (optimize)
		pa_pline_optimize(pl);

	/* Update count and bbox and calculate area */
	pl->Count = 0;
	pl->xmin = pl->xmax = pl->head->point[0];
	pl->ymin = pl->ymax = pl->head->point[1];
	p = (c = pl->head)->prev;
	if (c != p) {
		do {
			/* calculate area for orientation */
			area += (double)(p->point[0] - c->point[0]) * (double)(p->point[1] + c->point[1]);
			pa_pline_box_bump(pl, c->point);
			pl->Count++;
		}
		while ((c = (p = c)->next) != pl->head);
	}
	pl->area = RND_ABS(area);

	/* inverse orientation results in negative area */
	if (pl->Count > 2)
		pl->flg.orient = ((area < 0) ? RND_PLF_INV : RND_PLF_DIR);

	pl->tree = rnd_poly_make_edge_tree(pl);
}

void rnd_poly_plines_free(rnd_pline_t **pline)
{
	while(*pline != NULL) {
		rnd_pline_t *pl = *pline;
		*pline = pl->next;
		pa_pline_free(&pl);
	}
}

/* src can't be const because of PB_OPTIMIZE_TREE_COPY */
rnd_pline_t *pa_pline_dup(rnd_pline_t *src)
{
	rnd_pline_t *dst;
	rnd_vnode_t *n;

	assert(src != NULL);

	dst = pa_pline_new(src->head->point);
	if (dst == NULL)
		return NULL;

	dst->Count = src->Count;
	dst->flg.orient = src->flg.orient;
	dst->xmin = src->xmin; dst->xmax = src->xmax;
	dst->ymin = src->ymin; dst->ymax = src->ymax;
	dst->area = src->area;

#ifdef PB_OPTIMIZE_TREE_COPY
	/* ugly hack:see below where newnd->shared is loaded */
	dst->head->shared = src->head->shared;
	src->head->shared = dst->head;
#endif


	for(n = src->head->next; n != src->head; n = n->next) {
		rnd_vnode_t *newnd = rnd_poly_node_create(n->point);
		if (newnd == NULL) {
			pa_pline_free(&dst);
			return NULL;
		}

#ifdef PB_OPTIMIZE_TREE_COPY
		/* ugly hack: remember which node is copied from src and save n->shared
		   in newnd->shared; this is for rnd_poly_copy_edge_tree() */
		newnd->shared = n->shared;
		n->shared = newnd;
#endif

		newnd->flg.risk = n->flg.risk;
		rnd_poly_vertex_include(dst->head->prev, newnd);
	}

#ifdef PB_OPTIMIZE_TREE_COPY
	rnd_poly_copy_edge_tree(dst, src);
#else
	dst->tree = rnd_poly_make_edge_tree(dst);
#endif
	return dst;
}

rnd_bool pa_pline_alloc_copy(rnd_pline_t **dst, rnd_pline_t *src)
{
	*dst = pa_pline_dup(src);
	return dst != NULL;
}

static rnd_r_dir_t pa_invert_cb(const rnd_box_t *b, void *cl)
{
	pa_seg_t *s = (pa_seg_t *)b;
	s->v = s->v->prev;
	return RND_R_DIR_FOUND_CONTINUE;
}

void pa_pline_invert(rnd_pline_t *pl)
{
	rnd_vnode_t *n, *next;

	assert(pl != NULL);

	/* flip order on the node list */
	n = pl->head;
	do {
		next = n->next;
		n->next = n->prev;
		n->prev = next;
		n = next;
	} while(n != pl->head);

	/* flip segment ->prev references in the tree */
	if (pl->tree != NULL) {
		int cnt;
		rnd_r_search(pl->tree, NULL, NULL, pa_invert_cb, NULL, &cnt);
		assert(cnt == pl->Count);
	}

	pl->flg.orient ^= 1;
}

RND_INLINE int pa_pline_is_first(rnd_polyarea_t *pa, rnd_pline_t *pl)
{
	return (pa->contours == pl);
}


RND_INLINE int pa_pline_is_last(rnd_pline_t *pl)
{
	return (pl->next == NULL);
}

RND_INLINE void pa_pline_unlink(rnd_polyarea_t *pa, rnd_pline_t *prev, rnd_pline_t *pl)
{
	if (pa->contours == pl)
		pa->contours = pl->next;
	else
		prev->next = pl->next;

	pl->next = NULL;
	rnd_r_delete_entry(pa->contour_tree, (rnd_box_t *)pl);
}
