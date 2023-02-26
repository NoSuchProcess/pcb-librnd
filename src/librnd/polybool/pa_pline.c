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

/* Return a new node for a point next to dst, or NULL if pt falls on an existing
   node. Crashes/asserts if allocation fails.
   Side effect: cvclst of the node at pt is reset, even for existing nodes. */
static rnd_vnode_t *pa_ensure_point_and_reset_cvc(rnd_vnode_t *dst, rnd_vector_t pt)
{
	rnd_vnode_t *dnext = dst->next, *newnd;

	newnd = rnd_poly_node_add_single(dst, pt);
	assert(newnd != NULL);

	newnd->cvclst_prev = newnd->cvclst_next = PA_CONN_DESC_INVALID;

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

/* Update the bbox of pl using pt's coords */
RND_INLINE void pa_pline_box_bump(rnd_pline_t *pl, const rnd_vector_t pt)
{
	if (pt[0]     < pl->xmin) pl->xmin = pt[0];
	if ((pt[0]+1) > pl->xmax) pl->xmax = pt[0]+1;
	if (pt[1]     < pl->ymin) pl->ymin = pt[1];
	if ((pt[1]+1) > pl->ymax) pl->ymax = pt[1]+1;
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
	pl->is_round = rnd_false;
	pl->cx = pl->cy = 0;
	pl->radius = 0;
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

void pa_pline_free_fields(rnd_pline_t *pl)
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
