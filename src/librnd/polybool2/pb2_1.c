
typedef struct {
	pb2_seg_t *seg;
	int num_isc;
	rnd_vector_t isc[2];
} pb2_isc_t;

#define GVT(x) vtisc_ ## x
#define GVT_ELEM_TYPE pb2_isc_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 1024
#define GVT_START_SIZE 32
#define GVT_FUNC RND_INLINE
#define GVT_SET_NEW_BYTES_TO 0

#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
#include <genvector/genvector_impl.c>
#include <genvector/genvector_undef.h>

#include "htvlist.h"
#include "htvlist.c"

#define ISCS ((vtisc_t *)(&ctx->iscs))

#include "pa_vect_inline.c"

typedef struct {
	double offs; /* distance^2 from the search object's starting point (for sorting) */
	rnd_vector_t isc;
} pb2_split_at_t;


#define GVT(x) vtsplit_ ## x
#define GVT_ELEM_TYPE pb2_split_at_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 1024
#define GVT_START_SIZE 32
#define GVT_FUNC RND_INLINE
#define GVT_SET_NEW_BYTES_TO 0

#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
#include <genvector/genvector_impl.c>
#include <genvector/genvector_undef.h>

#define SPLITS ((vtsplit_t *)(&ctx->splits))

RND_INLINE void p2b_split_at_new(pb2_ctx_t *ctx, double offs, rnd_vector_t pt)
{
	pb2_split_at_t *split;

	split = vtsplit_alloc_append(SPLITS, 1);
	split->offs = offs;
	split->isc[0] = pt[0];
	split->isc[1] = pt[1];
}


RND_INLINE void seg_inc_poly(pb2_seg_t *seg, char poly_id)
{
	if (poly_id == 'A') seg->cntA++;
	else if (poly_id == 'B') seg->cntB++;
}

RND_INLINE void seg_merge_into(pb2_seg_t *dst, pb2_seg_t *discard)
{
	dst->cntA += discard->cntA;
	dst->cntB += discard->cntB;
	discard->discarded = 1;
}


/* Return 1 if line a1:a2 is the same as isc->isc[] */
RND_INLINE int pb2_1_ends_match(const pb2_isc_t *isc, const rnd_vector_t a1, const rnd_vector_t a2)
{
	if (isc->num_isc != 2) return 0;

	if (Vequ2(a1, isc->isc[0]) && Vequ2(a2, isc->isc[1])) return 1;
	if (Vequ2(a1, isc->isc[1]) && Vequ2(a2, isc->isc[0])) return 1;
	return 0;
}

typedef struct {
	pb2_ctx_t *ctx;
	rnd_vector_t p1, p2;
} isc_ctx_t;

static rnd_rtree_dir_t pb2_1_isc_line_cb(void *udata, void *obj, const rnd_rtree_box_t *box)
{
	isc_ctx_t *ictx = udata;
	pb2_seg_t *seg = obj;
	pb2_ctx_t *ctx = ictx->ctx;
	int num_isc;
	rnd_vector_t iscpt[2];
	pb2_isc_t *isc;

	if (seg->discarded)
		return 0;

	TODO("arc: this assumes seg is a line; move most of this into pb2_geo.c");
	num_isc = pa_vect_inters2(seg->start, seg->end, ictx->p1, ictx->p2, iscpt[0], iscpt[1], 0);
	if (num_isc == 0)
		return 0;

	isc = vtisc_alloc_append(ISCS, 1);
	isc->seg = seg;
	isc->num_isc = num_isc;
	memcpy(isc->isc, iscpt, sizeof(iscpt));

	p2b_split_at_new(ctx, rnd_vect_dist2(ictx->p1, iscpt[0]), iscpt[0]);
	if (num_isc > 1) {
		assert(!Vequ2(iscpt[0], iscpt[1])); /* can not be the same coord */
		p2b_split_at_new(ctx, rnd_vect_dist2(ictx->p1, iscpt[1]), iscpt[1]);
	}

	return rnd_RTREE_DIR_FOUND;
}

RND_INLINE void pb2_1_split_seg_at_iscs(pb2_ctx_t *ctx, const pb2_isc_t *isc)
{
	rnd_vector_t orig_end;
	int isc1_bad = 0, isc2_bad = 0, num_isc;
	rnd_vector_t ip0, ip1;
	pb2_seg_t *news;

	num_isc = isc->num_isc;
	Vcpy2(orig_end, isc->seg->end);
	Vcpy2(ip0, isc->isc[0]);
	Vcpy2(ip1, isc->isc[1]);


	if (Vequ2(ip0, isc->seg->start)) isc1_bad = 1;
	else if (Vequ2(ip0, orig_end)) isc1_bad = 1;

	/* don't split at endpoints: remove 1 or 2 bad intersections
	   from num_isc:ip0:ip1 */
	if (num_isc == 2) {
		if (Vequ2(ip1, isc->seg->start)) isc2_bad = 1;
		else if (Vequ2(ip1, orig_end)) isc2_bad = 1;

		if (isc2_bad && isc1_bad)
			return; /* 2 intersections at exactly the endpoints */

		if (isc1_bad) {
			/* ip0 is a bad intersection, drop it but preserve ip1 as that's still valid */
			Vcpy2(ip0, ip1);
			num_isc--;
		}
		else if (isc2_bad)
			num_isc--;
	}
	else /* (num_isc == 1) */ {
		if (isc1_bad)
			return; /* 1 intersection at exactly one of the endpoints */
	}


	TODO("arc: implement this for arcs; below is the line-only implementation; move most of this in pb2_geo.c");

	rnd_rtree_delete(&ctx->seg_tree, isc->seg, &isc->seg->bbox);
	if (num_isc == 2) {
		double d1, d2;
		rnd_vector_t *i1, *i2;
		
		/* order iscs to this: seg->start .. isc[0] .. isc[1] .. orig_end */
		d1 = rnd_vect_dist2(isc->seg->start, ip0);
		d2 = rnd_vect_dist2(isc->seg->start, ip1);
		if (d2 > d1) {
			/* normal order */
			i1 = &ip0;
			i2 = &ip1;
		}
		else {
			/* reverse order */
			i1 = &ip1;
			i2 = &ip0;
		}

		/* reuse the original segment for the first part */
		Vcpy2(isc->seg->end, *i1);
		pb2_1_seg_bbox(isc->seg);
		if (!pb2_1_ends_match(isc, isc->seg->start, *i1)) {
			rnd_rtree_insert(&ctx->seg_tree, isc->seg, &isc->seg->bbox);
			isc->seg->risky = 1;
		}
		else
			isc->seg->discarded = 1;

		/* two more segments after the two intersections */
		if (!pb2_1_ends_match(isc, *i1, *i2)) {
			news = pb2_seg_new_alike(ctx, *i1, *i2, isc->seg);
			news->risky = 1;
		}
		if (!pb2_1_ends_match(isc, *i2, orig_end)) {
			news = pb2_seg_new_alike(ctx, *i2, orig_end, isc->seg);
			news->risky = 1;
		}
	}
	else if (num_isc == 1) {
		/* reuse the original segment for the first part */
		Vcpy2(isc->seg->end, ip0);
		pb2_1_seg_bbox(isc->seg);
		if (!pb2_1_ends_match(isc, isc->seg->start, ip0)) {
			rnd_rtree_insert(&ctx->seg_tree, isc->seg, &isc->seg->bbox);
			isc->seg->risky = 1;
		}
		else
			isc->seg->discarded = 1;


		/* plus add the extra segment */
		if (!pb2_1_ends_match(isc, ip0, orig_end)) {
			news = pb2_seg_new_alike(ctx, ip0, orig_end, isc->seg);
			news->risky = 1;
		}
	}
	else {
		assert(!"invalid isc->num_isc");
		abort();
	}
}

static int cmp_split_cb(const void *A, const void *B)
{
	const pb2_split_at_t *a = A, *b = B;
	return (a->offs < b->offs) ? -1 : +1;
}

void pb2_1_map_seg_line(pb2_ctx_t *ctx, const rnd_vector_t p1, const rnd_vector_t p2, char poly_id, int isected)
{
	rnd_rtree_box_t bbox;
	isc_ctx_t ictx;
	long n;
	pb2_split_at_t *ss, *se;

	ictx.ctx = ctx;
	ictx.p1[0] = p1[0]; ictx.p1[1] = p1[1];
	ictx.p2[0] = p2[0]; ictx.p2[1] = p2[1];

	bbox.x1 = pa_min(p1[0], p2[0]); bbox.y1 = pa_min(p1[1], p2[1]);
	bbox.x2 = pa_max(p1[0], p2[0]); bbox.y2 = pa_max(p1[1], p2[1]);

	ISCS->used = 0;
	SPLITS->used = 0;
	ss = vtsplit_alloc_append(SPLITS, 1);
	ss->offs = 0;
	Vcpy2(ss->isc, p1);

	rnd_rtree_search_obj(&ctx->seg_tree, &bbox, pb2_1_isc_line_cb, &ictx);

	if (SPLITS->used > 1) { /* intersected */
		int found = 0;
		pb2_seg_t *seg;

		/* split up existing segments */
		for(n = 0; n < ISCS->used; n++)
			pb2_1_split_seg_at_iscs(ctx, &ISCS->array[n]);

		qsort(SPLITS->array, SPLITS->used, sizeof(pb2_split_at_t), cmp_split_cb);

		/* create a new segment for each part of the input segment that has
		   intersections thus has at least 2 parts */

		se = vtsplit_alloc_append(SPLITS, 1);
		se->offs = RND_COORD_MAX;
		Vcpy2(se->isc, p2);

		for(n = 0; n < SPLITS->used-1; n++) {
			if (!Vequ2(SPLITS->array[n].isc, SPLITS->array[n+1].isc)) {
				seg = pb2_seg_new(ctx, SPLITS->array[n].isc, SPLITS->array[n+1].isc);
				seg->risky = 1;
				seg_inc_poly(seg, poly_id);
				found++;
			}
		}
		if (found < 2) {
			/* if we ended up creating only one segment and that is the original
			   segment, that means it did not intersect with rounding so no further
			   checking is needed */
			if (Vequ2(p1, seg->start) && Vequ2(p2, seg->end))
				seg->risky = 0;
		}
	}
	else  {
		pb2_seg_t *seg = pb2_seg_new(ctx, p1, p2);
		seg_inc_poly(seg, poly_id);
	}
}

RND_INLINE int pb2_new_split(pb2_ctx_t *ctx, pb2_seg_t *seg, rnd_vector_t ip0)
{
	pb2_seg_t *news;
	rnd_vector_t orig_end;

	orig_end[0] = seg->end[0];
	orig_end[1] = seg->end[1];

	/* reuse the original segment for the first part */
	rnd_rtree_delete(&ctx->seg_tree, seg, &seg->bbox);
	Vcpy2(seg->end, ip0);
	pb2_1_seg_bbox(seg);
	rnd_rtree_insert(&ctx->seg_tree, seg, &seg->bbox);
	seg->risky = 1;

	/* plus add the extra segment */
	news = pb2_seg_new_alike(ctx, ip0, orig_end, seg);
	news->risky = 1;

	return 1;
}

RND_INLINE void pb2_1_handle_new_iscs(pb2_ctx_t *ctx)
{
	rnd_rtree_it_t its;
	pb2_seg_t *s1, *s2;
	rnd_vector_t isc1, isc2;
	rnd_vector_t *s1_ins1, *s1_ins2, *s2_ins1, *s2_ins2; /* points to insert in s1 */

	restart:;
	for(s1 = ctx->all_segs; s1 != NULL; s1 = s1->next_all) {
		if (!s1->risky) continue;
		if (s1->discarded) continue;

		s1->risky = 0;

		for(s2 = rnd_rtree_first(&its, &ctx->seg_tree, &s1->bbox); s2 != NULL; s2 = rnd_rtree_next(&its)) {
			int num_isc, changed;

			if ((s1 == s2 || s2->discarded)) continue;

			num_isc = pa_vect_inters2(s1->start, s1->end, s2->start, s2->end, isc1, isc2, 0);
			if (num_isc == 0) continue;

			/* ignore simple endpoint-endpoint isc */
			if (num_isc == 1) {
				if (Vequ2(s1->start, s2->start)) continue;
				if (Vequ2(s1->start, s2->end)) continue;
				if (Vequ2(s1->end, s2->start)) continue;
				if (Vequ2(s1->end, s2->end)) continue;
			}

			/* remove one of two fully overlapping segments (merge them) */
			if (num_isc == 2) {
				int same = 0;
				if (Vequ2(s1->start, s2->start) && Vequ2(s1->end, s2->end)) same = 1;
				else if (Vequ2(s1->end, s2->start) && Vequ2(s1->start, s2->end)) same = 1;

				if (same) {
					/*rnd_trace("  NEW ISC full overlap S%ld S%ld, discard S%ld\n", s1->uid, s2->uid, s2->uid);*/
					seg_merge_into(s1, s2);
					goto restart;
				}
			}

			changed = 0;

			s1_ins1 = s1_ins2 = s2_ins1 = s2_ins2 = NULL;

			/* collect intersection points */
			if (!Vequ2(isc1, s1->start) && !Vequ2(isc1, s1->end))
				s1_ins1 = &isc1;
			if (!Vequ2(isc1, s2->start) && !Vequ2(isc1, s2->end))
				s2_ins1 = &isc1;

			if (num_isc == 2) {
				if (!Vequ2(isc2, s1->start) && !Vequ2(isc2, s1->end)) {
					if (s1_ins1 == NULL) s1_ins1 = &isc2;
					else s1_ins2 = &isc2;
				}
				if (!Vequ2(isc2, s2->start) && !Vequ2(isc2, s2->end)) {
					if (s2_ins1 == NULL) s2_ins1 = &isc2;
					else s2_ins2 = &isc2;
				}
			}

			/* if there are two intersections, order them from start to end */
			if (s1_ins2 != NULL)
				if (rnd_vect_dist2(s1->start, *s1_ins2) < rnd_vect_dist2(s1->start, *s1_ins1))
					rnd_swap(rnd_vector_t *, s1_ins1, s1_ins2);

			if (s2_ins2 != NULL)
				if (rnd_vect_dist2(s2->start, *s2_ins2) < rnd_vect_dist2(s2->start, *s2_ins1))
					rnd_swap(rnd_vector_t *, s2_ins1, s2_ins2);

			/* insert points in the right order: first the one that's farhter from
			   start (*ins2) so that the closer one (ins1) still falls on the
			   original/remaining seg */
			if (s1_ins2 != NULL)
				changed |= pb2_new_split(ctx, s1, *s1_ins2);
			if (s1_ins1 != NULL)
				changed |= pb2_new_split(ctx, s1, *s1_ins1);
			if (s2_ins2 != NULL)
				changed |= pb2_new_split(ctx, s2, *s2_ins2);
			if (s2_ins1 != NULL)
				changed |= pb2_new_split(ctx, s2, *s2_ins1);

			if (changed)
				goto restart;
		}
	}
}

/*** curve graph ***/

RND_INLINE void curve_glue_segs(pb2_ctx_t *ctx, pb2_seg_t *sa, pb2_seg_t *sb);

RND_INLINE int seg_common_pt_is_node(pb2_seg_t *sa, pb2_seg_t *sb)
{
	if (sa->in_graph && sb->in_graph) {
		if (sa->gr_start && sb->gr_start && Vequ2(sa->start, sb->start)) return 1;
		if (sa->gr_start && sb->gr_end && Vequ2(sa->start, sb->end)) return 1;
		if (sa->gr_end && sb->gr_end && Vequ2(sa->end, sb->end)) return 1;
		if (sa->gr_end && sb->gr_start && Vequ2(sa->end, sb->start)) return 1;
	}
	return 0;
}


/* glue s to ca auto-detecting which end to use */
RND_INLINE void curve_glue_seg_to_curve(pb2_ctx_t *ctx, pb2_curve_t *c, pb2_seg_t *s)
{
	pb2_seg_t *f = gdl_first(&c->segs), *l = gdl_last(&c->segs);

	if (Vequ2(s->end, l->end) && !seg_common_pt_is_node(l, s)) curve_glue_segs(ctx, l, s);
	else if (Vequ2(s->start, l->end) && !seg_common_pt_is_node(l, s)) curve_glue_segs(ctx, l, s);
	else if (Vequ2(s->end, f->start) && !seg_common_pt_is_node(f, s)) curve_glue_segs(ctx, f, s);
	else if (Vequ2(s->start, f->start) && !seg_common_pt_is_node(f, s)) curve_glue_segs(ctx, f, s);
	else {
		assert(!"trying to glue in a seg where it doesn't connect (curve)");
		abort();
	}
}

/* Glue sa and sb together in the same curve; if there was no curve, create it;
   if there were two curves, merge them */
RND_INLINE void curve_glue_segs(pb2_ctx_t *ctx, pb2_seg_t *sa, pb2_seg_t *sb)
{
	pb2_curve_t *ca, *cb;
	pb2_seg_t *s;

	ca = pb2_seg_parent_curve(sa);
	cb = pb2_seg_parent_curve(sb);

	if ((ca != NULL) && (ca == cb))
		return; /* they are already on the same curve */

	/* do not glue segments together at their end that's already in a node */
	if (seg_common_pt_is_node(sa, sb))
		return;

	if ((ca != NULL) && (cb != NULL)) {
		/* merge sb's curve into sa's curve; sb must be the first or last in cb */
		if (gdl_first(&cb->segs) == sb) {
			while((s = gdl_first(&cb->segs)) != NULL) {
				gdl_remove(&cb->segs, s, link);
				curve_glue_seg_to_curve(ctx, ca, s);
			}
			pb2_curve_free(ctx, cb);
		}
		else if (gdl_last(&cb->segs) == sb) {
			while((s = gdl_last(&cb->segs)) != NULL) {
				gdl_remove(&cb->segs, s, link);
				curve_glue_seg_to_curve(ctx, ca, s);
			}
			pb2_curve_free(ctx, cb);
		}
		else {
			fprintf(stderr, "Internal error: curve_glue_segs(): sb is not the first or last in its curve\n");
			abort();
		}
		return;
	}

	/* make sure sa has a curve and sb doesn't */
	if ((ca == NULL) && (cb == NULL)) {
		ca = pb2_curve_new_for_seg(ctx, sa);
	}
	else if ((cb != NULL) && (ca == NULL)) {
		/* swap them */
		rnd_swap(pb2_seg_t *, sa, sb);
		rnd_swap(pb2_curve_t *, ca, cb);
	}

	/* glue sb to sa; sa has a curve, sb doesn't */
	if (Vequ2(sa->end, sb->start)) {
		gdl_append(&ca->segs, sb, link);
	}
	else if (Vequ2(sa->end, sb->end)) {
		seg_reverse(sb);
		gdl_append(&ca->segs, sb, link);
	}
	else if (Vequ2(sa->start, sb->end)) {
		gdl_insert(&ca->segs, sb, link);
	}
	else if (Vequ2(sa->start, sb->start)) {
		seg_reverse(sb);
		gdl_insert(&ca->segs, sb, link);
	}
	else {
		assert(!"trying to glue in a seg where it doesn't connect (seg)");
		abort();
	}
}

static int cmp_cgout_angle(const void *a_, const void *b_)
{
	const pb2_cgout_t *a = a_, *b = b_;

	/* no overlapping curves: overlapping segs are all removed already */
	assert(a->angle != b->angle);

	return (a->angle < b->angle) ? -1 : +1;
}

/* Create a new node from a list of segments starting or ending at that node.
   The new node's outgoing edges ordered by angles. */
RND_INLINE pb2_cgnode_t *cg_create_node_from_segs(pb2_ctx_t *ctx, rnd_vector_t at, vtp0_t *segs)
{
	pb2_seg_t *seg;
	pb2_cgnode_t *node;
	int n;

	/* if a node already exists we have already fully mapped it - the first
	   segment reveals it */
	seg = segs->array[0];
	if (seg->in_graph) {
		for(node = gdl_first(&ctx->cgnodes); node != NULL; node = gdl_next(&ctx->cgnodes, node))
			if ((node->bbox.x1 == at[0]) && (node->bbox.y1 == at[1]))
				return node;
	}

	node = calloc(sizeof(pb2_cgnode_t) + sizeof(pb2_cgout_t) * (segs->used-1), 1);
	gdl_append(&ctx->cgnodes, node, link);
	PB2_UID_SET(node);
	node->bbox.x1 = at[0];   node->bbox.y1 = at[1];
	node->bbox.x2 = at[0]+1; node->bbox.y2 = at[1]+1;
	node->num_edges = segs->used;

	/* num_edges is int and segs->used is long so the above could overflow;
	   but even on a 16 bit int system, who the heck would have 32k edges from
	   a single node in a planar graph?! */
	assert(node->num_edges == segs->used);

	for(n = 0; n < segs->used; n++) {
		pb2_seg_t *seg = segs->array[n];

		seg->in_graph = 1;
		if (Vequ2(seg->start, at)) seg->gr_start = 1;
		if (Vequ2(seg->end, at)) seg->gr_end = 1;
		assert(seg->gr_start || seg->gr_end);

		node->edges[n].node = (pb2_cgnode_t *)seg; /* temporarily store the segment for postprocessing */
		PB2_UID_SET(node->edges+n);

		seg_angle_from(&node->edges[n].angle, seg, at);
	}

	qsort(node->edges, node->num_edges, sizeof(pb2_cgout_t), cmp_cgout_angle);

	return node;
}

/* Cross-link cruve ends with nodes */
RND_INLINE void cg_postproc_node(pb2_ctx_t *ctx, pb2_cgnode_t *node)
{
	long n;

	/* finalzie node out edges (now sorted) */
	for(n = 0; n < node->num_edges; n++) {
		pb2_seg_t *seg = (pb2_seg_t *)node->edges[n].node, *first, *last;
		pb2_curve_t *curve = pb2_seg_parent_curve(seg);
		int found = 0;

		if (curve == NULL)
			curve = pb2_curve_new_for_seg(ctx, seg);

		node->edges[n].node = node;
		node->edges[n].curve = curve;
		node->edges[n].nd_idx = n;

		first = gdl_first(&curve->segs); last = gdl_last(&curve->segs);

		/* corner case: we may have a loop with a T junction that falls not at
		   the end but one seg after the start or before the end. This is because
		   the curve builder has no idea where the end of loops should be. Detect
		   this and fix it up by "rotating" the loop
		   Test case: fixedx */
		if ((last != first) && (first->start[0] == last->end[0]) && (first->start[1] == last->end[1])) {
			if ((seg == first) && (seg->end[0] == node->bbox.x1) && (seg->end[1] == node->bbox.y1)) {
				/* move from first to last */
				gdl_remove(&curve->segs, seg, link);
				gdl_append(&curve->segs, seg, link);
				first = gdl_first(&curve->segs); last = gdl_last(&curve->segs);
			}
			else if ((seg == last) && (seg->start[0] == node->bbox.x1) && (seg->start[1] == node->bbox.y1)) {
				/* move from last to first */
				gdl_remove(&curve->segs, seg, link);
				gdl_insert(&curve->segs, seg, link);
				first = gdl_first(&curve->segs); last = gdl_last(&curve->segs);
			}
		}

		/* figure if curve start/end is connected to this out by the seg that was temporarily bound to the out */
		if ((seg == first) && (seg->start[0] == node->bbox.x1) && (seg->start[1] == node->bbox.y1)) {
			curve->out_start = &node->edges[n];
			node->edges[n].reverse = 0;
			found = 1;
		}
		if ((seg == last) && (seg->end[0] == node->bbox.x1) && (seg->end[1] == node->bbox.y1)) {
			curve->out_end = &node->edges[n];
			node->edges[n].reverse = 1;
			found = 1;
		}

		assert(found);
	}
}

/* Link in segments into endpoint lists in an endpoint hash table;
   discard it if it is in overlap with any other non-discarded segment */
RND_INLINE void pb2_1_build_cg_seg(pb2_ctx_t *ctx, pb2_seg_t *seg, htvlist_t *htl)
{
	htvlist_key_t k;
	htvlist_entry_t *e;
	pb2_seg_t *s;

	if (seg->discarded)
		return;

	k.x = seg->start[0]; k.y = seg->start[1];
	e = htvlist_getentry(htl, k);
	if (e == NULL) {
		htvlist_value_t v = {0};
		htvlist_set(htl, k, v);
		e = htvlist_getentry(htl, k);
	}
	else {
		for(s = e->value.heads; s != NULL; s = s->nexts) {
			if (seg_seg_olap(seg, s)) {
				seg_merge_into(s, seg);
				return;
			}
		}
		for(s = e->value.heade; s != NULL; s = s->nexte) {
			if (seg_seg_olap(seg, s)) {
				seg_merge_into(s, seg);
				return;
			}
		}
	}


	seg->nexts = e->value.heads;
	e->value.heads = seg;

	k.x = seg->end[0]; k.y = seg->end[1];
	e = htvlist_getentry(htl, k);
	if (e == NULL) {
		htvlist_value_t v = {0};
		htvlist_set(htl, k, v);
		e = htvlist_getentry(htl, k);
	}
	else {
			for(s = e->value.heads; s != NULL; s = s->nexts) {
			if (seg_seg_olap(seg, s)) {
				seg_merge_into(s, seg);
				return;
			}
		}
			for(s = e->value.heade; s != NULL; s = s->nexte) {
			if (seg_seg_olap(seg, s)) {
				seg_merge_into(s, seg);
				return;
			}
		}
	}

	seg->nexte = e->value.heade;
	e->value.heade = seg;
}


RND_INLINE void pb2_1_dummy_node_for_loop(pb2_ctx_t *ctx, pb2_curve_t *c, vtp0_t *tmp)
{
	pb2_seg_t *s_start, *s_end;
	rnd_vector_t pt;

	s_start = gdl_first(&c->segs);
	s_end = gdl_last(&c->segs);
	if ((s_start->start[0] == s_end->end[0]) && (s_start->start[1] == s_end->end[1])) {
		Vcpy2(pt, s_start->start);
	}
	else if ((s_start->end[0] == s_end->start[0]) && (s_start->end[1] == s_end->start[1])) {
		Vcpy2(pt, s_start->end);
	}
	else {
		TODO("stub: should enable (stub removal)");
		c->pruned = 1;
		assert(!"loop not closed");
		abort();
		return;
	}

	tmp->used = 0;
	vtp0_append(tmp, s_start);
	vtp0_append(tmp, s_end);
	cg_create_node_from_segs(ctx, pt, tmp);
}


/* Iterate over each segment in the unordered haystack of segments and place
   them in curves and the curve graph (or discard them if they are overlapping) */
RND_INLINE void pb2_1_build_cg(pb2_ctx_t *ctx)
{
	pb2_seg_t *seg;
	pb2_curve_t *c;
	pb2_cgnode_t *node;
	htvlist_t htl;
	htvlist_entry_t *e;
	vtp0_t vtmp = {0};

	htvlist_init(&htl, htvlist_keyhash, htvlist_keyeq);

	/* build the endpoint hash list (htl) */
	for(seg = ctx->all_segs; seg != NULL; seg = seg->next_all)
		pb2_1_build_cg_seg(ctx, seg, &htl);

	/* build the actual cg from the endpoint hash list; look at each endpoint:
	   if there are more than 2 segments occupy an endpoint,
	   make that a cg node, otherwise glue segs together into a curve */
	for(e = htvlist_first(&htl); e != NULL; e = htvlist_next(&htl, e)) {
		vtmp.used = 0;

		for(seg = e->value.heads; seg != NULL; seg = seg->nexts)
			vtp0_append(&vtmp, seg);
		for(seg = e->value.heade; seg != NULL; seg = seg->nexte)
			vtp0_append(&vtmp, seg);

		if (vtmp.used == 2) {
			curve_glue_segs(ctx, vtmp.array[0], vtmp.array[1]);
		}
		else if (vtmp.used > 2) {
			rnd_vector_t v;
			v[0] = e->key.x;
			v[1] = e->key.y;
			cg_create_node_from_segs(ctx, v, &vtmp);
		}
	}

	/* handle curves that are single loop stand-alone so they don't have a node */
	for(c = gdl_first(&ctx->curves); c != NULL; c = gdl_next(&ctx->curves, c)) {
		pb2_seg_t *first, *last;

		first = gdl_first(&c->segs);
		last  = gdl_last(&c->segs);

		if (!first->in_graph && !last->in_graph)
			pb2_1_dummy_node_for_loop(ctx, c, &vtmp);
	}

	for(node = gdl_first(&ctx->cgnodes); node != NULL; node = gdl_next(&ctx->cgnodes, node))
		cg_postproc_node(ctx, node);


	/* prune dangling stubs */
	for(c = gdl_first(&ctx->curves); c != NULL; c = gdl_next(&ctx->curves, c))
		if ((c->out_start == NULL) || (c->out_end == NULL))
			c->pruned = 1;

	htvlist_uninit(&htl);
	vtp0_uninit(&vtmp);
}

/*** Switching to topology: execute section 1 of the algo ***/
RND_INLINE void pb2_1_to_topo(pb2_ctx_t *ctx)
{
	/* step 1.1. is done by the caller before calling the operator function */

	/* step 1.2. */
	pb2_1_handle_new_iscs(ctx);

	vtisc_uninit(ISCS);
	vtsplit_uninit(SPLITS);

	if (pb2_debug_draw_steps)
		pb2_draw(ctx, "step0.svg", PB2_DRAW_INPUT_POLY | PB2_DUMP_SEGS | PB2_DUMP_CURVES | PB2_DUMP_CURVE_GRAPH);

	/* step 1.3: build the curve graph */
	pb2_1_build_cg(ctx);
}

