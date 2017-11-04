/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include "thermal.h"

#include "compat_misc.h"
#include "data.h"
#include "obj_pstk.h"
#include "obj_pstk_inlines.h"
#include "obj_pinvia_therm.h"
#include "polygon.h"

pcb_cardinal_t pcb_themal_style_new2old(unsigned char t)
{
	switch(t) {
		case 0: return 0;
		case PCB_THERMAL_ON | PCB_THERMAL_SHARP | PCB_THERMAL_DIAGONAL: return 1;
		case PCB_THERMAL_ON | PCB_THERMAL_SHARP: return 2;
		case PCB_THERMAL_ON | PCB_THERMAL_SOLID: return 3;
		case PCB_THERMAL_ON | PCB_THERMAL_ROUND | PCB_THERMAL_DIAGONAL: return 4;
		case PCB_THERMAL_ON | PCB_THERMAL_ROUND: return 5;
	}
	return 0;
}

unsigned char pcb_themal_style_old2new(pcb_cardinal_t t)
{
	switch(t) {
		case 0: return 0;
		case 1: return PCB_THERMAL_ON | PCB_THERMAL_SHARP | PCB_THERMAL_DIAGONAL;
		case 2: return PCB_THERMAL_ON | PCB_THERMAL_SHARP;
		case 3: return PCB_THERMAL_ON | PCB_THERMAL_SOLID;
		case 4: return PCB_THERMAL_ON | PCB_THERMAL_ROUND | PCB_THERMAL_DIAGONAL;
		case 5: return PCB_THERMAL_ON | PCB_THERMAL_ROUND;
	}
	return 0;
}

pcb_thermal_t pcb_thermal_str2bits(const char *str)
{
	/* shape */
	if (strcmp(str, "noshape") == 0) return PCB_THERMAL_NOSHAPE;
	if (strcmp(str, "round") == 0)   return PCB_THERMAL_ROUND;
	if (strcmp(str, "sharp") == 0)   return PCB_THERMAL_SHARP;
	if (strcmp(str, "solid") == 0)   return PCB_THERMAL_SOLID;

	/* orientation */
	if (strcmp(str, "diag") == 0)   return PCB_THERMAL_DIAGONAL;

	if (strcmp(str, "on") == 0)     return PCB_THERMAL_ON;

	return 0;
}

const char *pcb_thermal_bits2str(pcb_thermal_t *bit)
{
	if ((*bit) & PCB_THERMAL_ON) {
		*bit &= ~PCB_THERMAL_ON;
		return "on";
	}
	if ((*bit) & PCB_THERMAL_DIAGONAL) {
		*bit &= ~PCB_THERMAL_ON;
		return "diag";
	}
	if ((*bit) & 3) {
		switch((*bit) & 3) {
			*bit &= ~3;
			case PCB_THERMAL_NOSHAPE: return "noshape";
			case PCB_THERMAL_ROUND: return "round";
			case PCB_THERMAL_SHARP: return "sharp";
			case PCB_THERMAL_SOLID: return "solid";
			default: return NULL;
		}
	}
	return NULL;
}


pcb_polyarea_t *pcb_thermal_area_pin(pcb_board_t *pcb, pcb_pin_t *pin, pcb_layer_id_t lid)
{
	return ThermPoly(pcb, pin, lid);
}

/* generate a round-cap line polygon */
static pcb_polyarea_t *pa_line_at(double x1, double y1, double x2, double y2, pcb_coord_t clr, pcb_bool square)
{
	pcb_line_t ltmp;

	if (square)
		ltmp.Flags = pcb_flag_make(PCB_FLAG_SQUARE);
	else
		ltmp.Flags = pcb_no_flags();
	ltmp.Point1.X = pcb_round(x1); ltmp.Point1.Y = pcb_round(y1);
	ltmp.Point2.X = pcb_round(x2); ltmp.Point2.Y = pcb_round(y2);
	return pcb_poly_from_line(&ltmp, clr);
}

/* generate a round-cap arc polygon knowing the center and endpoints */
static pcb_polyarea_t *pa_arc_at(double cx, double cy, double r, double e1x, double e1y, double e2x, double e2y, pcb_coord_t clr, double max_span_angle)
{
	double sa, ea, da;
	pcb_arc_t atmp;

	sa = atan2(-(e1y - cy), e1x - cx) * PCB_RAD_TO_DEG + 180.0;
	ea = atan2(-(e2y - cy), e2x - cx) * PCB_RAD_TO_DEG + 180.0;

/*	pcb_trace("sa=%f ea=%f diff=%f\n", sa, ea, ea-sa);*/

	atmp.Flags = pcb_no_flags();
	atmp.X = pcb_round(cx);
	atmp.Y = pcb_round(cy);

	da = ea-sa;
	if ((da < max_span_angle) && (da > -max_span_angle)) {
		atmp.StartAngle = sa;
		atmp.Delta = ea-sa;
	}
	else {
		atmp.StartAngle = ea;
		atmp.Delta = 360-ea+sa;
	}
	atmp.Width = atmp.Height = r;
	return pcb_poly_from_arc(&atmp, clr);
}

pcb_polyarea_t *pcb_thermal_area_line(pcb_board_t *pcb, pcb_line_t *line, pcb_layer_id_t lid)
{
	pcb_polyarea_t *pa, *pb, *pc;
	double dx, dy, len, vx, vy, nx, ny, clr, clrth, x1, y1, x2, y2, mx, my;
	pcb_coord_t th;

	if ((line->Point1.X == line->Point2.X) && (line->Point1.Y == line->Point2.Y)) {
		/* conrer case zero-long line is a circle: do the same as for vias */
#warning thermal TODO
		abort();
	}

	x1 = line->Point1.X;
	y1 = line->Point1.Y;
	x2 = line->Point2.X;
	y2 = line->Point2.Y;
	mx = (x1+x2)/2.0;
	my = (y1+y2)/2.0;
	dx = x1 - x2;
	dy = y1 - y2;

	len = sqrt(dx*dx + dy*dy);
	vx = dx / len;
	vy = dy / len;
	nx = -vy;
	ny = vx;

	clr = line->Clearance;
	clrth = (line->Clearance + line->Thickness) / 2;

	assert(line->thermal & PCB_THERMAL_ON); /* caller should have checked this */
	switch(line->thermal & 3) {
		case PCB_THERMAL_NOSHAPE:
		case PCB_THERMAL_SOLID: return 0;

		case PCB_THERMAL_ROUND:
			if (line->thermal & PCB_THERMAL_DIAGONAL) {
				/* side clear lines */
				pa = pa_line_at(
					x1 - clrth * nx - clr * vx * 0.75, y1 - clrth * ny - clr * vy * 0.75,
					x2 - clrth * nx + clr * vx * 0.75, y2 - clrth * ny + clr * vy * 0.75,
					clr, pcb_false);
				pb = pa_line_at(
					x1 + clrth * nx - clr * vx * 0.75, y1 + clrth * ny - clr * vy * 0.75,
					x2 + clrth * nx + clr * vx * 0.75, y2 + clrth * ny + clr * vy * 0.75,
					clr, pcb_false);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_UNITE);

				/* x1;y1 cap arc */
				pa = pc; pc = NULL;
				pb = pa_arc_at(x1, y1, clrth,
					x1 - clrth * nx + clr * vx * 2.0, y1 - clrth * ny + clr * vy * 2.0,
					x1 + clrth * nx + clr * vx * 2.0, y1 + clrth * ny + clr * vy * 2.0,
					clr, 180.0);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_UNITE);

				/* x2;y2 cap arc */
				pa = pc; pc = NULL;
				pb = pa_arc_at(x2, y2, clrth,
					x2 - clrth * nx - clr * vx * 2.0, y2 - clrth * ny - clr * vy * 2.0,
					x2 + clrth * nx - clr * vx * 2.0, y2 + clrth * ny - clr * vy * 2.0,
					clr, 180.0);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_UNITE);
			}
			else { /* non-diagonal */
				/* split side lines */
				pa = pa_line_at(
					x1 - clrth * nx - clr * vx * 0.00, y1 - clrth * ny - clr * vy * 0.00,
					mx - clrth * nx + clr * vx * 1.00, my - clrth * ny + clr * vy * 1.00,
					clr, pcb_false);
				pb = pa_line_at(
					x1 + clrth * nx - clr * vx * 0.00, y1 + clrth * ny - clr * vy * 0.00,
					mx + clrth * nx + clr * vx * 1.00, my + clrth * ny + clr * vy * 1.00,
					clr, pcb_false);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_UNITE);

				pa = pc; pc = NULL;
				pb = pa_line_at(
					mx - clrth * nx - clr * vx * 1.00, my - clrth * ny - clr * vy * 1.00,
					x2 - clrth * nx + clr * vx * 0.00, y2 - clrth * ny + clr * vy * 0.00,
					clr, pcb_false);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_UNITE);

				pa = pc; pc = NULL;
				pb = pa_line_at(
					mx + clrth * nx - clr * vx * 1.00, my + clrth * ny - clr * vy * 1.00,
					x2 + clrth * nx + clr * vx * 0.00, y2 + clrth * ny + clr * vy * 0.00,
					clr, pcb_false);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_UNITE);

				/* split round cap, x1;y1 */
				pa = pc; pc = NULL;
				pb = pa_arc_at(x1, y1, clrth,
					x1 - clrth * nx, y1 - clrth * ny,
					x1 + clrth * vx - clr * nx, y1 + clrth * vy - clr * ny,
					clr, 180.0);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_UNITE);

				pa = pc; pc = NULL;
				pb = pa_arc_at(x1, y1, clrth,
					x1 + clrth * nx, y1 + clrth * ny,
					x1 + clrth * vx + clr * nx, y1 + clrth * vy + clr * ny,
					clr, 180.0);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_UNITE);

				/* split round cap, x2;y2 */
				pa = pc; pc = NULL;
				pb = pa_arc_at(x2, y2, clrth,
					x2 - clrth * nx, y2 - clrth * ny,
					x2 - clrth * vx - clr * nx, y2 - clrth * vy - clr * ny,
					clr, 180.0);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_UNITE);

				pa = pc; pc = NULL;
				pb = pa_arc_at(x2, y2, clrth,
					x2 + clrth * nx, y2 + clrth * ny,
					x2 - clrth * vx + clr * nx, y2 - clrth * vy + clr * ny,
					clr, 180.0);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_UNITE);
			}
			return pc;
		case PCB_THERMAL_SHARP:
			pa = pcb_poly_from_line(line, line->Thickness + line->Clearance*2);
			th = line->Thickness/2 < clr ? line->Thickness/2 : clr;
			clrth *= 2;
			if (line->thermal & PCB_THERMAL_DIAGONAL) {
				/* x1;y1 V-shape */
				pb = pa_line_at(x1, y1, x1-nx*clrth+vx*clrth, y1-ny*clrth+vy*clrth, th, pcb_false);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_SUB);

				pa = pc; pc = NULL;
				pb = pa_line_at(x1, y1, x1+nx*clrth+vx*clrth, y1+ny*clrth+vy*clrth, th, pcb_false);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_SUB);

				/* x2;y2 V-shape */
				pa = pc; pc = NULL;
				pb = pa_line_at(x2, y2, x2-nx*clrth-vx*clrth, y2-ny*clrth-vy*clrth, th, pcb_false);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_SUB);

				pa = pc; pc = NULL;
				pb = pa_line_at(x2, y2, x2+nx*clrth-vx*clrth, y2+ny*clrth-vy*clrth, th, pcb_false);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_SUB);
			}
			else {
				/* perpendicular */
				pb = pa_line_at(mx-nx*clrth, my-ny*clrth, mx+nx*clrth, my+ny*clrth, th, pcb_true);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_SUB);

				/* straight */
				pa = pc; pc = NULL;
				pb = pa_line_at(x1+vx*clrth, y1+vy*clrth, x2-vx*clrth, y2-vy*clrth, th, pcb_true);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_SUB);
			}
			return pc;
	}
	return NULL;
}

/* combine a base poly-area into the result area (pres) */
static void polytherm_base(pcb_polyarea_t **pres, const pcb_polyarea_t *src)
{
	pcb_polyarea_t *p;

	if (*pres != NULL) {
		pcb_polyarea_boolean(src, *pres, &p, PCB_PBO_UNITE);
		pcb_polyarea_free(pres);
		*pres = p;
	}
	else
		pcb_polyarea_copy0(pres, src);
}

/* combine a round clearance line set into pres; "it" is an iterator
   already initialized to a polyarea contour */
static void polytherm_round(pcb_polyarea_t **pres, pcb_poly_it_t *it, pcb_coord_t clr, pcb_bool is_diag)
{
	pcb_polyarea_t *ptmp, *p;
	double fact = 0.5, fact_ortho=0.75;
	pcb_coord_t cx, cy;
	double px, py, x, y, dx, dy, vx, vy, nx, ny, mx, my, len;
	int go, first = 1;

	clr -= 2;

	/* iterate over the vectors of the contour */
	for(go = pcb_poly_vect_first(it, &cx, &cy); go; go = pcb_poly_vect_next(it, &cx, &cy)) {
		x = cx; y = cy;
		if (first) {
			pcb_poly_vect_peek_prev(it, &cx, &cy);
			px = cx; py = cy;
			first = 0;
		}

		dx = x - px;
		dy = y - py;
		mx = (x+px)/2.0;
		my = (y+py)/2.0;

		len = sqrt(dx*dx + dy*dy);
		vx = dx / len;
		vy = dy / len;

		nx = -vy;
		ny = vx;

		/* cheat: clr-2+4 to guarantee some overlap with the poly cutout */
		if (is_diag) {
			/* one line per edge, slightly shorter than the edge */
			ptmp = pa_line_at(x - vx * clr * fact - nx * clr/2, y - vy * clr * fact - ny * clr/2, px + vx * clr *fact - nx * clr/2, py + vy * clr * fact - ny * clr/2, clr+4, pcb_false);

			pcb_polyarea_boolean(ptmp, *pres, &p, PCB_PBO_UNITE);
			pcb_polyarea_free(pres);
			pcb_polyarea_free(&ptmp);
			*pres = p;
		}
		else {
			/* two half lines per edge */
			ptmp = pa_line_at(x - nx * clr/2 , y - ny * clr/2, mx + vx * clr * fact_ortho - nx * clr/2, my + vy * clr * fact_ortho - ny * clr/2, clr+4, pcb_false);
			pcb_polyarea_boolean(ptmp, *pres, &p, PCB_PBO_UNITE);
			pcb_polyarea_free(pres);
			pcb_polyarea_free(&ptmp);
			*pres = p;

			ptmp = pa_line_at(px - nx * clr/2, py - ny * clr/2, mx - vx * clr * fact_ortho - nx * clr/2, my - vy * clr * fact_ortho - ny * clr/2, clr+4, pcb_false);
			pcb_polyarea_boolean(ptmp, *pres, &p, PCB_PBO_UNITE);
			pcb_polyarea_free(pres);
			pcb_polyarea_free(&ptmp);
			*pres = p;

			/* optical tuning: make sure the clearance is large enough around corners
			   even if lines didn't meet - just throw in a big circle */
			ptmp = pcb_poly_from_circle(x, y, clr);
			pcb_polyarea_boolean(ptmp, *pres, &p, PCB_PBO_UNITE);
			pcb_polyarea_free(pres);
			pcb_polyarea_free(&ptmp);
			*pres = p;
		}
		px = x;
		py = y;
	}
}

static void polytherm_sharp(pcb_polyarea_t **pres, pcb_poly_it_t *it, pcb_coord_t clr, pcb_bool is_diag)
{
	pcb_polyarea_t *ptmp, *p;
	pcb_coord_t cx, cy, x2c, y2c;
	double px, py, x, y, dx, dy, vx, vy, nx, ny, mx, my, len, x2, y2, vx2, vy2, len2, dx2, dy2;
	int go, first = 1;

	/* iterate over the vectors of the contour */
	for(go = pcb_poly_vect_first(it, &cx, &cy); go; go = pcb_poly_vect_next(it, &cx, &cy)) {
		x = cx; y = cy;
		if (first) {
			pcb_poly_vect_peek_prev(it, &cx, &cy);
			px = cx; py = cy;
			first = 0;
		}

		if ((x == px) && (y == py))
			continue;

		dx = x - px;
		dy = y - py;

		len = sqrt(dx*dx + dy*dy);
		vx = dx / len;
		vy = dy / len;

		if (is_diag) {
			pcb_poly_vect_peek_next(it, &x2c, &y2c);
			x2 = x2c; y2 = y2c;
			dx2 = x - x2;
			dy2 = y - y2;
			len2 = sqrt(dx2*dx2 + dy2*dy2);
			vx2 = dx2 / len2;
			vy2 = dy2 / len2;

			nx = -(vy2 - vy);
			ny = vx2 - vx;

			/* line from each corner at the average angle of the two edges from the corner */
			ptmp = pa_line_at(x-nx*clr*0.2, y-ny*clr*0.2, x + nx*clr*4, y + ny*clr*4, clr/2, pcb_false);
			pcb_polyarea_boolean(*pres, ptmp, &p, PCB_PBO_SUB);
			pcb_polyarea_free(pres);
			pcb_polyarea_free(&ptmp);
			*pres = p;
		}
		else {
			nx = -vy;
			ny = vx;
			mx = (x+px)/2.0;
			my = (y+py)/2.0;

			/* perpendicular line from the middle of each edge */
			ptmp = pa_line_at(mx, my, mx - nx*clr, my - ny*clr, clr/2, pcb_true);
			pcb_polyarea_boolean(*pres, ptmp, &p, PCB_PBO_SUB);
			pcb_polyarea_free(pres);
			pcb_polyarea_free(&ptmp);
			*pres = p;
		}
		px = x;
		py = y;
	}
}

/* generate round thermal around a polyarea specified by the iterator */
static void pcb_thermal_area_pa_round(pcb_polyarea_t **pres, pcb_poly_it_t *it, pcb_coord_t clr, pcb_bool_t is_diag)
{
	pcb_pline_t *pl;

/* cut out the poly so terminals will be displayed proerply */
	polytherm_base(pres, it->pa);

	/* generate the clear-lines */
	pl = pcb_poly_contour(it);
	if (pl != NULL)
		polytherm_round(pres, it, clr, is_diag);
}

/* generate sharp thermal around a polyarea specified by the iterator */
static void pcb_thermal_area_pa_sharp(pcb_polyarea_t **pres, pcb_poly_it_t *it, pcb_coord_t clr, pcb_bool_t is_diag)
{
	pcb_pline_t *pl;

	/* add the usual clearance glory around the polygon */
	pcb_poly_pa_clearance_construct(pres, it, clr*2);

	pl = pcb_poly_contour(it);
	if (pl != NULL)
		polytherm_sharp(pres, it, clr, is_diag);

	/* trim internal stubs */
	polytherm_base(pres, it->pa);
}

pcb_polyarea_t *pcb_thermal_area_poly(pcb_board_t *pcb, pcb_poly_t *poly, pcb_layer_id_t lid)
{
	pcb_polyarea_t *pa, *pres = NULL;
	pcb_coord_t clr = poly->Clearance;
	pcb_poly_it_t it;

	assert(poly->thermal & PCB_THERMAL_ON); /* caller should have checked this */
	switch(poly->thermal & 3) {
		case PCB_THERMAL_NOSHAPE:
		case PCB_THERMAL_SOLID: return NULL;

		case PCB_THERMAL_ROUND:
			for(pa = pcb_poly_island_first(poly, &it); pa != NULL; pa = pcb_poly_island_next(&it))
				pcb_thermal_area_pa_round(&pres, &it, clr, (poly->thermal & PCB_THERMAL_DIAGONAL));
			return pres;

		case PCB_THERMAL_SHARP:
			for(pa = pcb_poly_island_first(poly, &it); pa != NULL; pa = pcb_poly_island_next(&it))
				pcb_thermal_area_pa_sharp(&pres, &it, clr, (poly->thermal & PCB_THERMAL_DIAGONAL));
			return pres;
	}

	return NULL;
}

/* Generate a clearance around a padstack shape, with no thermal */
static pcb_polyarea_t *pcb_thermal_area_pstk_nothermal(pcb_board_t *pcb, pcb_pstk_t *ps, pcb_layer_id_t lid, pcb_pstk_shape_t *shp)
{
	pcb_poly_it_t it;
	pcb_polyarea_t *pres = NULL;

	switch(shp->shape) {
		case PCB_PSSH_CIRC:
			return pcb_poly_from_circle(ps->x + shp->data.circ.x, ps->y + shp->data.circ.y, shp->data.circ.dia/2 + ps->Clearance);
		case PCB_PSSH_LINE:
			return pa_line_at(ps->x + shp->data.line.x1, ps->y + shp->data.line.y1, ps->x + shp->data.line.x2, ps->y + shp->data.line.y2, shp->data.line.thickness + ps->Clearance*2, shp->data.line.square);
		case PCB_PSSH_POLY:
			if (shp->data.poly.pa == NULL)
				pcb_pstk_shape_update_pa(&shp->data.poly);
			if (shp->data.poly.pa == NULL)
				return NULL;
			pcb_poly_iterate_polyarea(shp->data.poly.pa, &it);
			pcb_poly_pa_clearance_construct(&pres, &it, ps->Clearance);
			return pres;
	}
	return NULL;
}

pcb_polyarea_t *pcb_thermal_area_pstk(pcb_board_t *pcb, pcb_pstk_t *ps, pcb_layer_id_t lid)
{
	unsigned char thr;
	pcb_pstk_shape_t *shp;
	pcb_polyarea_t *pres = NULL;
	pcb_layer_t *layer = pcb_get_layer(pcb->Data, lid);

	/* if we have no clearance, there's no reason to do anything */
	if (!PCB_NONPOLY_HAS_CLEARANCE(ps)) /* no clearance -> solid connection */
		return NULL;

	/* retrieve shape; assume 0 (no shape) for layers not named */
	if (lid < ps->thermals.used)
		thr = ps->thermals.shape[lid];
	else
		thr = 0;

	shp = pcb_pstk_shape_at(pcb, ps, layer);

	if (!(thr & PCB_THERMAL_ON))
		return pcb_thermal_area_pstk_nothermal(pcb, ps, lid, shp);

	switch(thr & 3) {
		case PCB_THERMAL_NOSHAPE:
			{
				pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
				return pcb_poly_from_circle(ps->x, ps->y, proto->hdia/2 + ps->Clearance);
			}
		case PCB_THERMAL_SOLID: return NULL;

		case PCB_THERMAL_ROUND:
		case PCB_THERMAL_SHARP:
			switch(shp->shape) {
				case PCB_PSSH_CIRC:
					return ThermPoly_(pcb, ps->x + shp->data.circ.x, ps->y + shp->data.circ.y, shp->data.circ.dia, ps->Clearance, pcb_themal_style_new2old(thr));
				case PCB_PSSH_LINE:
				{
					pcb_line_t ltmp;
					if (shp->data.line.square)
						ltmp.Flags = pcb_flag_make(PCB_FLAG_SQUARE);
					else
						ltmp.Flags = pcb_no_flags();
					ltmp.Point1.X = ps->x + shp->data.line.x1;
					ltmp.Point1.Y = ps->y + shp->data.line.y1;
					ltmp.Point2.X = ps->x + shp->data.line.x2;
					ltmp.Point2.Y = ps->y + shp->data.line.y2;
					ltmp.Thickness = shp->data.line.thickness;
					ltmp.Clearance = ps->Clearance;
					ltmp.thermal = thr;
					pres = pcb_thermal_area_line(pcb, &ltmp, lid);
				}
				return pres;

				case PCB_PSSH_POLY:
				{
					pcb_poly_it_t it;
					if (shp->data.poly.pa == NULL)
						pcb_pstk_shape_update_pa(&shp->data.poly);
					if (shp->data.poly.pa == NULL)
						return NULL;
					pcb_poly_iterate_polyarea(shp->data.poly.pa, &it);
					if (thr & PCB_THERMAL_ROUND)
						pcb_thermal_area_pa_round(&pres, &it, ps->Clearance, (thr & PCB_THERMAL_DIAGONAL));
					else
						pcb_thermal_area_pa_sharp(&pres, &it, ps->Clearance, (thr & PCB_THERMAL_DIAGONAL));

					if (pres != NULL)
						pcb_polyarea_move(pres, ps->x, ps->y);
				}
				return pres;
			}

	}

	return NULL;
}

pcb_polyarea_t *pcb_thermal_area(pcb_board_t *pcb, pcb_any_obj_t *obj, pcb_layer_id_t lid)
{
	switch(obj->type) {
		case PCB_OBJ_PIN:
		case PCB_OBJ_VIA:
			return pcb_thermal_area_pin(pcb, (pcb_pin_t *)obj, lid);

		case PCB_OBJ_LINE:
			return pcb_thermal_area_line(pcb, (pcb_line_t *)obj, lid);

		case PCB_OBJ_POLY:
			return pcb_thermal_area_poly(pcb, (pcb_poly_t *)obj, lid);

		case PCB_OBJ_PSTK:
			return pcb_thermal_area_pstk(pcb, (pcb_pstk_t *)obj, lid);

		case PCB_OBJ_ARC:
			break;

		default: break;
	}

	return NULL;
}

