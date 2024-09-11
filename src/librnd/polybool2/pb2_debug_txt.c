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

#include <librnd/core/error.h>

typedef enum {
	PB2_DUMP_SEGS = 1,
	PB2_DUMP_CURVES = 2,
	PB2_DUMP_CURVE_GRAPH = 4,
	PB2_DUMP_FACES = 8,
	PB2_DUMP_FACE_TREE = 16,

	/* only in drawings */
	PB2_DRAW_INPUT_POLY = 32
} pb2_dump_what_t;

#ifndef NDEBUG
static void pb2_dump_segs(pb2_ctx_t *ctx)
{
	rnd_rtree_it_t it;
	pb2_seg_t *seg;

	pa_trace(" Segments:\n", 0);
	for(seg = rnd_rtree_all_first(&it, &ctx->seg_tree); seg != NULL; seg = rnd_rtree_all_next(&it)) {
		pa_trace("  S", Plong(PB2_UID_GET(seg)), " ", Pvect(seg->start), " -> ", Pvect(seg->end), (seg->discarded ? " DISCARDED:" : ":"), " A=", Pint(seg->cntA), " B=", Pint(seg->cntB), 0);
		switch(seg->shape_type) {
			case RND_VNODE_LINE: pa_trace(" line\n", 0); break;
			case RND_VNODE_ARC:  pa_trace(" arc\n", 0); break;
			default: pa_trace("??? unknown type ", Pint(seg->shape_type), "\n", 0);
		}
	}
}

/* gdb callable */
static void dump_curve(pb2_curve_t *curve)
{
	pb2_seg_t *seg, *fs, *ls;

	fs = gdl_first(&curve->segs);
	ls = gdl_last(&curve->segs);

	pa_trace("  C", Plong(curve->uid), ": ", Pvect(fs->start), " -> ", Pvect(ls->end), ":", 0);

	if (curve->out_start != NULL) {
		pa_trace(" (O", Plong(curve->out_start->uid), ")", 0);
		assert(curve->out_start->curve == curve);
	}

	for(seg = fs; seg != NULL; seg = gdl_next(&curve->segs, seg)) {
		pa_trace(" S", Plong(seg->uid), 0);
		assert(pb2_seg_parent_curve(seg) == curve);
	}

	if (curve->out_end != NULL) {
		pa_trace(" (O", Plong(curve->out_end->uid), ")", 0);
		assert(curve->out_end->curve == curve);
	}

	if (curve->pruned)
		pa_trace(" {pruned}", 0);
	if (curve->face[0] != NULL)
		pa_trace("    #  F", Plong(curve->face[0]->uid), 0);
	if (curve->face[1] != NULL)
		pa_trace(" F", Plong(curve->face[1]->uid), 0);
	if (curve->face_1_implicit)
		pa_trace("i", 0);

	pa_trace("\n", 0);
}

static void pb2_dump_curves(pb2_ctx_t *ctx)
{
	pb2_curve_t *c;

	pa_trace(" Curves:\n", 0);
	for(c = gdl_first(&ctx->curves); c != NULL; c = gdl_next(&ctx->curves, c))
		dump_curve(c);
}

/* gdb callable */
static void dump_cgnode(pb2_cgnode_t *cgn)
{
	int n;

	pa_trace("  N", Plong(cgn->uid), ": ", Plong(cgn->bbox.x1), ";", Plong(cgn->bbox.y1), "\n", 0);

	for(n = 0; n < cgn->num_edges; n++)
		pa_trace("   Edge: O", Plong(cgn->edges[n].uid), " C", Plong(cgn->edges[n].curve->uid), " rev=", Pint(cgn->edges[n].reverse), " mark=", Pint(cgn->edges[n].corner_mark), " angle=", PdblF(cgn->edges[n].angle, 0,3, 0), "\n", 0);
}

static void pb2_dump_curve_graph(pb2_ctx_t *ctx)
{
	pb2_cgnode_t *n;

	pa_trace(" Curve graph:\n", 0);
	for(n = gdl_first(&ctx->cgnodes); n != NULL; n = gdl_next(&ctx->cgnodes, n))
		dump_cgnode(n);

}

/* gdb callable */
static void dump_face(pb2_face_t *f)
{
	long n;

	pa_trace("  F", Plong(f->uid), ": pp=", Pvect(f->polarity_pt), " dir=", Pvect(f->polarity_dir),
		" inA=", Pint(f->inA), " inB=", Pint(f->inB)," out=", Pint(f->out),
		" area=", PdblF(f->area, 0, 2, 0), (f->destroy ? " {destroy}\n" : "\n"), 0);
	for(n = 0; n < f->num_curves; n++)
		pa_trace("   O", Plong(f->outs[n]->uid), " C", Plong(f->outs[n]->curve->uid), (f->outs[n]->reverse ? " rev" : " fwd"), " cmark=", Pint(f->outs[n]->corner_mark), "\n", 0);
}

static void pb2_dump_faces(pb2_ctx_t *ctx)
{
	pb2_face_t *f;

	pa_trace(" Faces:\n", 0);
	for(f = gdl_first(&ctx->faces); f != NULL; f = gdl_next(&ctx->faces, f))
		dump_face(f);
}

RND_INLINE void pb2_dump_face_tree_(pb2_face_t *parent, int ind)
{
	pb2_face_t *f;
	int n;

	for(n = 0; n < ind; n++)
		pa_trace(" ", 0);

	pa_trace("F", Plong(parent->uid), " out=", Pint(parent->out), " area=", PdblF(parent->area,0,0,0), "\n", 0);

	for(f = parent->children; f != NULL; f = f->next)
		pb2_dump_face_tree_(f, ind+1);
}

static void pb2_dump_face_tree(pb2_ctx_t *ctx)
{
	pa_trace(" Face tree:\n", 0);
	pb2_dump_face_tree_(&ctx->root, 2);
}


static void pb2_dump(pb2_ctx_t *ctx, pb2_dump_what_t what)
{
	if (what & PB2_DUMP_SEGS) pb2_dump_segs(ctx);
	if (what & PB2_DUMP_CURVES) pb2_dump_curves(ctx);
	if (what & PB2_DUMP_CURVE_GRAPH) pb2_dump_curve_graph(ctx);
	if (what & PB2_DUMP_FACES) pb2_dump_faces(ctx);
	if (what & PB2_DUMP_FACE_TREE) pb2_dump_face_tree(ctx);
}
#else
static void pb2_dump(pb2_ctx_t *ctx, pb2_dump_what_t what) {}
#endif
