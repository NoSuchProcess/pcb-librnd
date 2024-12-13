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

#ifdef RND_API_VER
#include <librnd/core/safe_fs.h>
#endif

#ifndef NDEBUG

/* Auto determine thickness and annotation scale; if dimensions of the seg_tree
   is large enough, assume an export from pcb-rnd */
#define ANNOT_SCALE() \
do { \
	if (ctx->annot_scale <= 0) { \
		rnd_coord_t __size__ = (ctx->seg_tree.bbox.x2 - ctx->seg_tree.bbox.x1) + (ctx->seg_tree.bbox.y2 - ctx->seg_tree.bbox.y1); \
		ctx->annot_scale = (__size__ > 30000) ? 100 : 1; \
	} \
} while(0)

#define ANNOT(x)  ((double)x * ctx->annot_scale)

static void pb2_annot_seg_midpt(pb2_seg_t *seg, double *labx, double *laby)
{
	switch(seg->shape_type) {
		case RND_VNODE_LINE:
			*labx = (double)(seg->start[0] + seg->end[0])/2.0;
			*laby = (double)(seg->start[1] + seg->end[1])/2.0;
			break;

		case RND_VNODE_ARC:
			pb2_arc_get_midpoint_dbl(seg, labx, laby);
			break;
	}
}

static void pb2_draw_segs(pb2_ctx_t *ctx, FILE *F)
{
	rnd_rtree_it_t it;
	pb2_seg_t *seg;

	ANNOT_SCALE();

	fprintf(F, "\n<!-- segments -->\n");
	for(seg = rnd_rtree_all_first(&it, &ctx->seg_tree); seg != NULL; seg = rnd_rtree_all_next(&it)) {
		double labx, laby;
		int large, sweep;
		const char *dash = (seg->discarded ? "stroke-dasharray=\"0.25\"" : "");

		switch(seg->shape_type) {
			case RND_VNODE_LINE:
				fprintf(F, " <line x1=\"%ld\" y1=\"%ld\" x2=\"%ld\" y2=\"%ld\" stroke-width=\"%.3f\" stroke=\"grey\" %s/>\n",
					(long)seg->start[0], (long)seg->start[1], (long)seg->end[0], (long)seg->end[1], ANNOT(0.02), dash);
				break;

			case RND_VNODE_ARC:
				large = fabs(seg->shape.arc.delta) > 180.0;
				sweep = seg->shape.arc.adir;
				fprintf(F, " <path d=\"M %ld %ld A %f,%f 0 %d %d %ld,%ld\" fill=\"none\" stroke-width=\"%.3f\" stroke=\"grey\" %s/>",
					(long)seg->start[0], (long)seg->start[1],
					seg->shape.arc.r, seg->shape.arc.r, large, sweep,
					(long)seg->end[0], (long)seg->end[1],
					ANNOT(0.02), dash);
				break;
			default: abort();
		}

		pb2_annot_seg_midpt(seg, &labx, &laby);
		fprintf(F, " <text x=\"%.2f\" y=\"%.2f\" stroke=\"none\" fill=\"grey\" font-size=\"0.5px\">S%ld:%d:%d</text>\n", labx, laby, PB2_UID_GET(seg), seg->cntA, seg->cntB);
	}
}


RND_INLINE void draw_curve_line(pb2_ctx_t *ctx, pb2_seg_t *seg, FILE *F, pb2_seg_t *fs, pb2_seg_t *ls, const char *dash)
{
		double x1 = seg->start[0], y1 = seg->start[1], x2 = seg->end[0], y2 = seg->end[1];
		double vx, vy, len;

		if ((seg == fs) || (seg == ls)) {
			vx = x2 - x1;
			vy = y2 - y1;
			len = sqrt(vx*vx + vy*vy);
			if (len != 0) {
				vx /= len;
				vy /= len;
			}
			vx *= 0.7;
			vy *= 0.7;
		}

		if (seg == fs) {
			x1 += vx;
			y1 += vy;
		}
		if (seg == ls) {
			x2 -= vx;
			y2 -= vy;
		}

		fprintf(F, " <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke-width=\"%.3f\" stroke=\"purple\" %s/>\n",
			x1, y1, x2, y2, ANNOT(0.05), dash);
}

RND_INLINE void draw_curve_arc(pb2_ctx_t *ctx, pb2_seg_t *seg, FILE *F, pb2_seg_t *fs, pb2_seg_t *ls, const char *dash)
{
	double x1 = seg->start[0], y1 = seg->start[1], x2 = seg->end[0], y2 = seg->end[1];
	int large, sweep;

	if (seg == fs)
		pb2_arc_shift_end_dbl(seg, 1, 0.7, &x1, &y1);
	if (seg == ls)
		pb2_arc_shift_end_dbl(seg, 2, 0.7, &x2, &y2);

	large = fabs(seg->shape.arc.delta) > 180.0;
	sweep = seg->shape.arc.adir;
	fprintf(F, " <path d=\"M %.2f %.2f A %f,%f 0 %d %d %.2f,%.2f\" fill=\"none\" stroke-width=\"%.3f\" stroke=\"purple\" %s/>",
		x1, y1, seg->shape.arc.r, seg->shape.arc.r, large, sweep, x2, y2,
		ANNOT(0.05), dash);
}

RND_INLINE void draw_curve(pb2_ctx_t *ctx, pb2_curve_t *curve, FILE *F)
{
	pb2_seg_t *seg, *fs, *ls;
	double labx, laby;
	const char *dash = (curve->pruned ? "stroke-dasharray=\"0.35\"" : "");


	fs = gdl_first(&curve->segs);
	ls = gdl_last(&curve->segs);

	for(seg = fs; seg != NULL; seg = gdl_next(&curve->segs, seg)) {
		switch(seg->shape_type) {
			case RND_VNODE_LINE: draw_curve_line(ctx, seg, F, fs, ls, dash); break;
			case RND_VNODE_ARC: draw_curve_arc(ctx, seg, F, fs, ls, dash); break;
		}
	}

	pb2_annot_seg_midpt(fs, &labx, &laby);
	laby -= 0.4;
	fprintf(F, " <text x=\"%.2f\" y=\"%.2f\" stroke=\"none\" fill=\"purple\" font-size=\"0.5px\">C%ld</text>\n",
		labx, laby, curve->uid);

	if (fs != ls) {
		pb2_annot_seg_midpt(ls, &labx, &laby);
		laby -= 0.4;
		fprintf(F, " <text x=\"%.2f\" y=\"%.2f\" stroke=\"none\" fill=\"purple\" font-size=\"0.5px\">C%ld</text>\n",
			labx, laby, curve->uid);
	}
}

static void pb2_draw_curves(pb2_ctx_t *ctx, FILE *F)
{
	pb2_curve_t *c;

	fprintf(F, "\n<!-- curves -->\n");
	for(c = gdl_first(&ctx->curves); c != NULL; c = gdl_next(&ctx->curves, c))
		draw_curve(ctx, c, F);
}

RND_INLINE void draw_cgnode(pb2_cgnode_t *cgn, FILE *F)
{
	int n;

	fprintf(F, " <circle cx=\"%ld\" cy=\"%ld\" r=\"0.2\" stroke=\"none\" fill=\"purple\" />\n",
		(long)cgn->bbox.x1, (long)cgn->bbox.y1);

	fprintf(F, " <text x=\"%ld\" y=\"%.2f\" stroke=\"none\" fill=\"purple\" font-size=\"0.25px\">N%ld</text>\n",
		(long)cgn->bbox.x1, (double)(cgn->bbox.y1)+0.4, cgn->uid);

	for(n = 0; n < cgn->num_edges; n++) {
		pb2_seg_t *labseg;
		double vx, vy, len, labx, laby;
	
		if (cgn->edges[n].reverse)
			labseg = gdl_last(&cgn->edges[n].curve->segs);
		else
			labseg = gdl_first(&cgn->edges[n].curve->segs);

		vx = (labseg->end[0] - labseg->start[0]);
		vy = (labseg->end[1] - labseg->start[1]);
		len = sqrt(vx*vx + vy*vy);
		if (len != 0) {
			vx /= len;
			vy /= len;
		}

		if (cgn->edges[n].reverse) {
			labx = labseg->end[0] - vx * 0.5;
			laby = labseg->end[1] - vy * 0.5;
		}
		else {
			labx = labseg->start[0] + vx * 0.5;
			laby = labseg->start[1] + vy * 0.5;
		}

		fprintf(F, " <text x=\"%.2f\" y=\"%.2f\" stroke=\"none\" fill=\"black\" font-size=\"0.25px\">O%ld%s%s</text>\n",
			labx, laby, cgn->edges[n].uid, cgn->edges[n].reverse ? "r" : "", cgn->edges[n].corner_mark ? "*" : "");
	}
}

static void pb2_draw_curve_graph(pb2_ctx_t *ctx, FILE *F)
{
	pb2_cgnode_t *n;

	fprintf(F, "\n<!-- cgnodes -->\n");
	for(n = gdl_first(&ctx->cgnodes); n != NULL; n = gdl_next(&ctx->cgnodes, n))
		draw_cgnode(n, F);
}

RND_INLINE void draw_face(pb2_ctx_t *ctx, pb2_face_t *f, FILE *F)
{
	double dx, dy, len;

	dx = f->polarity_dir[0];
	dy = f->polarity_dir[1];
	len = sqrt(dx*dx + dy*dy);
	if (len != 0) {
		dx /= len;
		dy /= len;
	}
	dx *= ANNOT(0.75);
	dy *= ANNOT(0.75);
	dx += f->polarity_pt[0];
	dy += f->polarity_pt[1];

	fprintf(F, " <line x1=\"%ld\" y1=\"%ld\" x2=\"%.2f\" y2=\"%.2f\" stroke-width=\"%.3f\" stroke=\"red\"/>\n",
		(long)f->polarity_pt[0], (long)f->polarity_pt[1], dx, dy, ANNOT(0.05));

		fprintf(F, " <text x=\"%.2f\" y=\"%.2f\" stroke=\"none\" fill=\"red\" font-size=\"%.3fpx\">F%ld%s%s%s</text>\n",
			dx, dy, ANNOT(0.35), f->uid, f->inA ? "A" : "", f->inB ? "B" : "", f->out ? "o" : "");
}

static void pb2_draw_faces(pb2_ctx_t *ctx, FILE *F)
{
	pb2_face_t *f;

	fprintf(F, "\n<!-- faces -->\n");
	for(f = gdl_first(&ctx->faces); f != NULL; f = gdl_next(&ctx->faces, f))
		draw_face(ctx, f, F);
}

static void svg_print_poly_contour_node(FILE *F, const rnd_vnode_t *v)
{
	int large, sweep;

	switch(v->prev->flg.curve_type) {
		case RND_VNODE_LINE:
			fprintf(F, "    L %ld,%ld", (long)v->point[0], (long)v->point[1]);
			break;
		case RND_VNODE_ARC:
			{
				rnd_vnode_t *vp = v->prev;
				double r, dx, dy, sx, sy, ex, ey, cx, cy, cp;
				int large, sweep = vp->curve.arc.adir;

				dx = vp->curve.arc.center[0] - v->point[0];
				dy = vp->curve.arc.center[1] - v->point[1];
				r = dx*dx + dy*dy;
				if (r == 0)
					break;
				r = sqrt(r);

				/* decide if sweep is larger than 180 deg: e.g. if center is
				   right-of start-end and it's CCW it is large */
				sx = vp->point[0]; sy = vp->point[1];
				ex = v->point[0]; ey = v->point[1];
				cx = vp->curve.arc.center[0]; cy = vp->curve.arc.center[1];

				cp = (ex - sx)*(cy - sy) - (ey - sy)*(cx - sx);
				large = ((cp > 0) != sweep);

				fprintf(F, " A %.1f,%.1f 0 %d %d %ld,%ld", r, r, large, sweep, (long)v->point[0], (long)v->point[1]);
			}
			break;
	}
}

void pb2_svg_print_pline(FILE *F, const rnd_pline_t *pl)
{
	const rnd_vnode_t *v;

	v = pl->head;
	fprintf(F, "\n	M %ld,%ld", (long)v->point[0], (long)v->point[1]);
	v = v->next;
	do {
		svg_print_poly_contour_node(F, v);
	} while((v = v->next) != pl->head);

	/* draw last segment returning to first point, don't let 'z' do that, because
	   it may be an arc */
	svg_print_poly_contour_node(F, v);

	fprintf(F, "z");
}

void pb2_draw_polyarea(pb2_ctx_t *ctx, FILE *F, const rnd_polyarea_t *pa, const char *clr, double fill_opacity)
{
	const rnd_pline_t *pl;
	const rnd_polyarea_t *pn = pa;
	static const double poly_bloat = 0.02;

	if (pa == NULL)
		return;

	fprintf(F, "\n<g fill-rule=\"even-odd\" stroke-width=\"%.3f\" stroke=\"%s\" fill=\"%s\" fill-opacity=\"%.3f\" stroke-opacity=\"%.3f\">\n<path d=\"", poly_bloat, clr, clr, fill_opacity, fill_opacity*2);
	do {
		/* check if we have a contour for the given island */
		pl = pn->contours;
		if (pl != NULL) {
			/* iterate over the vectors of the contour */
			pb2_svg_print_pline(F, pl);

			/* iterate over all holes within this island */
			for(pl = pl->next; pl != NULL; pl = pl->next)
				pb2_svg_print_pline(F, pl);
		}
	} while ((pn = pn->f) != pa);
	fprintf(F, "\n\"/></g>\n");
}

RND_INLINE void pb2_draw_input_poly(pb2_ctx_t *ctx, FILE *F)
{
	fprintf(F, "\n<!-- input poly A -->\n");
	pb2_draw_polyarea(ctx, F, ctx->input_A, "#FF0000", 0.1);
	if (ctx->has_B) {
		fprintf(F, "\n<!-- input poly B -->\n");
		pb2_draw_polyarea(ctx, F, ctx->input_B, "#00FF00", 0.1);
	}
}

static void pb2_draw(pb2_ctx_t *ctx, const char *fn, pb2_dump_what_t what)
{
	FILE *F = rnd_fopen(NULL, fn, "w");

	if (F == NULL) {
		fprintf(stderr, "pb2_draw(): failed to open '%s' for write\n", fn);
		return;
	}


	fprintf(F, "<?xml version=\"1.0\"?>\n");
	fprintf(F, "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.0\" width=\"1000\" height=\"1000\" viewBox=\"0 0 1000 1000\">\n");

	if (what & PB2_DRAW_INPUT_POLY) pb2_draw_input_poly(ctx, F);
	if (what & PB2_DUMP_CURVES) pb2_draw_curves(ctx, F);
	if (what & PB2_DUMP_CURVE_GRAPH) pb2_draw_curve_graph(ctx, F);
	if (what & PB2_DUMP_SEGS) pb2_draw_segs(ctx, F);
	if (what & PB2_DUMP_FACES) pb2_draw_faces(ctx, F);

#if 0
	if (what & PB2_DUMP_FACE_TREE) pb2_draw_face_tree(ctx, F);
#endif

	fprintf(F, "</svg>\n");

	fclose(F);
}
#else
static void pb2_draw(pb2_ctx_t *ctx, const char *fn, pb2_dump_what_t what) {}
#endif
