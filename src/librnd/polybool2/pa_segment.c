/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2023, 2024 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2023,2024)
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

/* some structures for handling segment intersections using the rtrees */

/* used by pcb-rnd */
rnd_vnode_t *rnd_pline_seg2vnode(void *box)
{
	pa_seg_t *seg = box;
	return seg->v;
}

/* Recalculate bbox of the segment from current coords */
RND_INLINE void pa_seg_update_bbox(pa_seg_t *s)
{
	switch(s->v->flg.curve_type) {
		case RND_VNODE_LINE:
			s->box.X1 = pa_min(s->v->point[0], s->v->next->point[0]);
			s->box.X2 = pa_max(s->v->point[0], s->v->next->point[0]) + 1;
			s->box.Y1 = pa_min(s->v->point[1], s->v->next->point[1]);
			s->box.Y2 = pa_max(s->v->point[1], s->v->next->point[1]) + 1;
			break;
		case RND_VNODE_ARC:
			{
				rnd_rtree_box_t bb;
				pb2_raw_arc_bbox(&bb, s->v->point, s->v->next->point, s->v->curve.arc.center, s->v->curve.arc.adir);
				s->box.X1 = bb.x1; s->box.Y1 = bb.y1;
				s->box.X2 = bb.x2; s->box.Y2 = bb.y2;
			}
			break;
	}
}

void *rnd_poly_make_edge_tree(rnd_pline_t *pl)
{
	rnd_vnode_t *bv = pl->head;
	rnd_rtree_t *res = rnd_r_create_tree();

	do {
		pa_seg_t *s = malloc(sizeof(pa_seg_t));
		s->v = bv;
		s->p = pl;
		pa_seg_update_bbox(s);
		rnd_r_insert_entry(res, (const rnd_box_t *)s);
	} while((bv = bv->next) != pl->head);

	return res;
}

/* Copy the structure of an rtree, without dummy leaf nodes that have the
   bbox and non-pointer content filled in but pointers left empty */
static void rnd_poly_copy_edge_tree_(rnd_rtree_t *dst, rnd_rtree_t *parent, const rnd_rtree_t *src, rnd_pline_t *dst_pline)
{
	int n;

	dst->bbox = src->bbox;
	dst->parent = parent;
	dst->size = src->size;
	dst->is_leaf = src->is_leaf;
	dst->used = src->used;

	if (src->is_leaf) {
		for(n = 0; n < src->used; n++) {
			pa_seg_t *ds = malloc(sizeof(pa_seg_t)), *ss = src->child.obj[n].obj;
			rnd_vnode_t *src_node = ss->v;
			rnd_vnode_t *dst_node = src_node->shared;

			memcpy(ds, ss, sizeof(pa_seg_t));

			ds->box.X1 = src->child.obj[n].box->x1;
			ds->box.Y1 = src->child.obj[n].box->y1;
			ds->box.X2 = src->child.obj[n].box->x2;
			ds->box.Y2 = src->child.obj[n].box->y2;
			ds->p = dst_pline;
			ds->v = dst_node;
			dst->child.obj[n].box = (rnd_rtree_box_t *)ds;
			dst->child.obj[n].obj = ds;

			/* undo the temporary link hack done in pa_pline_dup() */
			src_node->shared = dst_node->shared;
			dst_node->shared = NULL;
/*printf("put    %d %d %d %d\n", dst->child.obj[n].box->x1, dst->child.obj[n].box->y1, dst->child.obj[n].box->x2, dst->child.obj[n].box->y2);*/
		}
	}
	else {
		for(n = 0; n < src->used; n++) {
			dst->child.node[n] = malloc(sizeof(rnd_rtree_t));
			rnd_poly_copy_edge_tree_(dst->child.node[n], dst, src->child.node[n], dst_pline);
		}
	}
}

void rnd_poly_copy_edge_tree(rnd_pline_t *dst, const rnd_pline_t *src)
{
	if (src->tree == NULL)
		return;
	/* copy the tree recursively */
	dst->tree = rnd_r_create_tree();
	rnd_poly_copy_edge_tree_(dst->tree, NULL, src->tree, dst);
}


/*** seg-in-seg search helpers */

typedef struct pa_seg_seg_s {
	double m, b;
	rnd_vnode_t *v;
	pa_seg_t *s;
} pa_seg_seg_t;

/* manhattan distance^2 */
RND_INLINE int man_dist_2(rnd_vector_t a, rnd_vector_t b)
{
	rnd_coord_t dx = a[0] - b[0], dy = a[1] - b[1];
	return dx*dx + dy*dy;
}

/* return if seg1 and seg2 overlap */
RND_INLINE int pa_seg_seg_olap_(rnd_vnode_t *seg1, rnd_vnode_t *seg2)
{
	rnd_vector_t tmp1, tmp2;

	if (seg1->flg.curve_type != seg2->flg.curve_type)
		return 0; /* different curve shapes can not overlap */

	switch(seg1->flg.curve_type) {
		case RND_VNODE_LINE:
			return pa_vect_inters2(seg1->point, seg1->next->point, seg2->point, seg2->next->point, tmp1, tmp2, 1) == 2;
		case RND_VNODE_ARC:
			if (!Vequ2(seg1->curve.arc.center, seg2->curve.arc.center))
				return 0;
			return pb2_raw_isc_arc_arc(
				seg1->point, seg1->next->point, seg1->curve.arc.center, seg1->curve.arc.adir,
				seg2->point, seg2->next->point, seg2->curve.arc.center, seg2->curve.arc.adir,
				tmp1, tmp2) == 2;
	}
	return 0;
}


/* Cancels the search and sets ctx->s if b is the same as ctx->v. Usefule
   for finding the segment for a node */
static rnd_r_dir_t pa_get_seg_cb(const rnd_box_t *b, void *ctx_)
{
	pa_seg_seg_t *ctx = (pa_seg_seg_t *)ctx_;
	pa_seg_t *s = (pa_seg_t *)b;

	if (ctx->v == s->v) {
		ctx->s = s;
		return RND_R_DIR_CANCEL; /* found */
	}
	return RND_R_DIR_NOT_FOUND;
}

/* Find the segment for pt in tree; result is filled in segctx. Caller should
   initialize the following fields of segctx: env, touch, need_restart,
   node_insert_list */
RND_INLINE void pa_find_seg_for_pt(rnd_rtree_t *tree, rnd_vnode_t *pt, pa_seg_seg_t *segctx)
{
	rnd_r_dir_t rres;
	rnd_box_t box;
	double dx = pt->next->point[0] - pt->point[0]; /* compute the slant for region trimming */

	segctx->v = pt;
	if (dx != 0) {
		segctx->m = (pt->next->point[1] - pt->point[1]) / dx;
		segctx->b = pt->point[1] - segctx->m * pt->point[0];
	}
	else
		segctx->m = 0;

	box.X1 = pt->point[0];    box.Y1 = pt->point[1];
	box.X2 = box.X1 + 1;      box.Y2 = box.Y1 + 1;
	rres = rnd_r_search(tree, &box, NULL, pa_get_seg_cb, segctx, NULL);
	assert(rres == RND_R_DIR_CANCEL);
}
