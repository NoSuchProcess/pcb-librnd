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

static void pb2_dump_segs(pb2_ctx_t *ctx)
{
	rnd_rtree_it_t it;
	pb2_seg_t *seg;

	rnd_trace(" Segments:\n");
	for(seg = rnd_rtree_all_first(&it, &ctx->seg_tree); seg != NULL; seg = rnd_rtree_all_next(&it)) {
		rnd_trace("  S%ld %ld;%ld -> %ld;%ld%s A=%d B=%d", PB2_UID_GET(seg), seg->start[0], seg->start[1], seg->end[0], seg->end[1], (seg->discarded ? " DISCARDED:" : ":"), seg->cntA, seg->cntB);
		switch(seg->shape_type) {
			case RND_VNODE_LINE: rnd_trace(" line\n"); break;
			case RND_VNODE_ARC:  rnd_trace(" arc\n"); break;
			default: rnd_trace("??? unknown type %d\n", seg->shape_type);
		}
	}
}

/* gdb callable */
static void dump_curve(pb2_curve_t *curve)
{
	pb2_seg_t *seg, *fs, *ls;

	fs = gdl_first(&curve->segs);
	ls = gdl_last(&curve->segs);

	rnd_trace("  C%ld: %ld;%ld -> %ld;%ld:", curve->uid, fs->start[0], fs->start[1], ls->end[0], ls->end[1]);

	if (curve->out_start != NULL) {
		rnd_trace(" (O%ld)", curve->out_start->uid);
		assert(curve->out_start->curve == curve);
	}

	for(seg = fs; seg != NULL; seg = gdl_next(&curve->segs, seg)) {
		rnd_trace(" S%ld", seg->uid);
		assert(pb2_seg_parent_curve(seg) == curve);
	}

	if (curve->out_end != NULL) {
		rnd_trace(" (O%ld)", curve->out_end->uid);
		assert(curve->out_end->curve == curve);
	}

	if (curve->pruned)
		rnd_trace(" {pruned}");
	if (curve->face[0] != NULL)
		rnd_trace("    #  F%ld", curve->face[0]->uid);
	if (curve->face[1] != NULL)
		rnd_trace(" F%ld", curve->face[1]->uid);
	if (curve->face_1_implicit)
		rnd_trace("i");

	rnd_trace("\n");
}

static void pb2_dump_curves(pb2_ctx_t *ctx)
{
	pb2_curve_t *c;

	rnd_trace(" Curves:\n");
	for(c = gdl_first(&ctx->curves); c != NULL; c = gdl_next(&ctx->curves, c))
		dump_curve(c);
}

/* gdb callable */
static void dump_cgnode(pb2_cgnode_t *cgn)
{
	int n;

	rnd_trace("  N%ld: %ld;%ld\n", cgn->uid, cgn->bbox.x1, cgn->bbox.y1);

	for(n = 0; n < cgn->num_edges; n++)
		rnd_trace("   Edge: O%ld C%ld rev=%d mark=%d angle=%.3f\n", cgn->edges[n].uid, cgn->edges[n].curve->uid, cgn->edges[n].reverse, cgn->edges[n].corner_mark, cgn->edges[n].angle);
}

static void pb2_dump_curve_graph(pb2_ctx_t *ctx)
{
	pb2_cgnode_t *n;

	rnd_trace(" Curve graph:\n");
	for(n = gdl_first(&ctx->cgnodes); n != NULL; n = gdl_next(&ctx->cgnodes, n))
		dump_cgnode(n);

}

/* gdb callable */
static void dump_face(pb2_face_t *f)
{
	long n;

	rnd_trace("  F%ld: pp=%ld;%ld inA=%d inB=%d out=%d area=%.2f%s\n",
		f->uid, f->polarity_pt[0], f->polarity_pt[1], f->inA, f->inB, f->out, f->area,
		(f->destroy ? " {destroy}" : ""));
	for(n = 0; n < f->num_curves; n++)
		rnd_trace("   O%ld C%ld %s cmark=%d\n", f->outs[n]->uid, f->outs[n]->curve->uid,
			f->outs[n]->reverse ? "rev" : "fwd", f->outs[n]->corner_mark);
}

static void pb2_dump_faces(pb2_ctx_t *ctx)
{
	pb2_face_t *f;

	rnd_trace(" Faces:\n");
	for(f = gdl_first(&ctx->faces); f != NULL; f = gdl_next(&ctx->faces, f))
		dump_face(f);
}

RND_INLINE void pb2_dump_face_tree_(pb2_face_t *parent, int ind)
{
	pb2_face_t *f;
	int n;

	for(n = 0; n < ind; n++)
		rnd_trace(" ");

	rnd_trace("F%ld out=%d area=%.0f\n", parent->uid, parent->out, parent->area);

	for(f = parent->children; f != NULL; f = f->next)
		pb2_dump_face_tree_(f, ind+1);
}

static void pb2_dump_face_tree(pb2_ctx_t *ctx)
{
	rnd_trace(" Face tree:\n");
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
