/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2024)
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

#define PB2_7_OUT_SHAPE(nd, seg, positive) \
do { \
	switch(seg->shape_type) { \
		case RND_VNODE_LINE: break; \
		case RND_VNODE_ARC: \
			nd->flg.curve_type = seg->shape_type; \
			nd->curve.arc.center[0] = seg->shape.arc.center[0]; \
			nd->curve.arc.center[1] = seg->shape.arc.center[1]; \
			nd->curve.arc.adir = seg->shape.arc.adir; \
			if (!Vequ2(seg->start, nd->point)) nd->curve.arc.adir = !nd->curve.arc.adir; \
			break; \
	} \
} while(0)

RND_INLINE rnd_pline_t *pb2_7_mkpline(pb2_ctx_t *ctx, pb2_face_t *f, int positive)
{
	pb2_face_it_t it;
	rnd_pline_t *pl;
	rnd_vector_t pt;
	long count;
	pb2_seg_t *seg;
	rnd_vnode_t *nd;

	if ((seg = pb2_face_it_first(&it, f, NULL, pt, !positive)) == NULL)
		return NULL;

	pl = pa_pline_new(pt);
	count = 1;

	nd = pl->head;
	PB2_7_OUT_SHAPE(nd, seg, positive);


	while((seg = pb2_face_it_next(&it, NULL, pt)) != NULL) {
TODO("arc: optimize: merge adjacent arcs that are on the same circle, but avoid single-arc full-circle");
		if (rnd_poly_vertex_include(pl->head->prev, (nd = rnd_poly_node_create(pt))) == 0)
			count++;
		PB2_7_OUT_SHAPE(nd, seg, positive);
	}

	/* Cheaper version of pa_pline_update(pl, 1), reusing fields of face: */
	pl->xmin = f->bbox.x1; pl->ymin = f->bbox.y1;
	pl->xmax = f->bbox.x2; pl->ymax = f->bbox.y2;
	pl->area = f->area;
	pl->Count = count;
	pl->flg.orient = positive ? RND_PLF_DIR : RND_PLF_INV;
	if (!ctx->inhibit_edge_tree)
		pl->tree = rnd_poly_make_edge_tree(pl);

	return pl;
}

RND_INLINE void pb2_7_mkpline_and_insert(pb2_ctx_t *ctx, rnd_polyarea_t **res, rnd_polyarea_t **parent_pa, pb2_face_t *f, int positive)
{
	rnd_pline_t *pl = pb2_7_mkpline(ctx, f, positive);

	assert(pl != NULL);
	if (pl == NULL)
		return;

	if (positive) {
		rnd_polyarea_t *pa = pa_polyarea_alloc();
		pa->contours = pl;
		*parent_pa = pa;
		rnd_polyarea_m_include(res, pa);
	}
	else {
		/* insert hole in parent positive */
		if (!pa_polyarea_insert_pline(*parent_pa, pl))
			pa_pline_free(&pl);
	}
}


RND_INLINE int pb2_7_face_is_sorrunded_by_holes(pb2_face_t *f)
{
	long n;

	for(n = 0; n < f->num_curves; n++) {
		pb2_cgout_t *o = f->outs[n];
		pb2_curve_t *c = o->curve;
		pb2_face_t *other = NULL;

		if (c->face[0] == f)
			other = c->face[1];
		else if (c->face[1] == f)
			other = c->face[0];

		if ((other != NULL) && (other->out))
			return 0;

	}
	return 1;
}


RND_INLINE void pb2_7_output_subtree(pb2_ctx_t *ctx, rnd_polyarea_t **res, pb2_face_t *parent, rnd_polyarea_t *parent_pa)
{
	pb2_face_t *f;

	for(f = parent->children; f != NULL; f = f->next) {
		if (f->out && parent->out && pb2_7_face_is_sorrunded_by_holes(f)) {
			/* corner case: in self-intersection resolvemenent of test case si_class2a
			   a central positive area is formed that's not part of any larger hole but
			   is sorrunded by smaller holes. It appears as a positive island but
			   does not need to be exported. */
			/* if we wanted to output it as a new island: rnd_polyarea_t *parent_pa = NULL; pb2_7_output_subtree(ctx, res, f, new_parent_pa); */
			if_trace("omitting F%ld: positive-in-positive, sorrunded by islands\n", PB2_UID_GET(f));
		}
		else {
			assert(f->out != parent->out);
			pb2_7_mkpline_and_insert(ctx, res, &parent_pa, f, f->out);
			pb2_7_output_subtree(ctx, res, f, parent_pa);
		}
	}
}


RND_INLINE void pb2_7_output(pb2_ctx_t *ctx, rnd_polyarea_t **res)
{
	rnd_polyarea_t *parent_pa = NULL;

	*res = NULL;
	pb2_7_output_subtree(ctx, res, &ctx->root, parent_pa);
}
