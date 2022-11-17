/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include "ui_zoompan.h"

#include <librnd/core/hidlib_conf.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/hidlib.h>
#include "glue_common.h"

double rnd_gtk_clamp_zoom(const rnd_gtk_view_t *vw, double coord_per_px)
{
	double min_zoom, max_zoom, max_zoom_w, max_zoom_h, out_zoom;

	min_zoom = 200;

	/* max zoom is calculated so that zoom * canvas_size * 2 doesn't overflow rnd_coord_t */
	max_zoom_w = (double)RND_COORD_MAX / (double)vw->canvas_width;
	max_zoom_h = (double)RND_COORD_MAX / (double)vw->canvas_height;
	max_zoom = MIN(max_zoom_w, max_zoom_h) / 2.0;

	out_zoom = coord_per_px;
	if (out_zoom < min_zoom)
		out_zoom = min_zoom;
	if (out_zoom > max_zoom)
		out_zoom = max_zoom;

	return out_zoom;
}


static void uiz_pan_common(rnd_gtk_view_t *v)
{
	int event_x, event_y;

	/* We need to fix up the PCB coordinates corresponding to the last
	 * event so convert it back to event coordinates temporarily. */
	rnd_gtk_coords_design2event(v, v->design_x, v->design_y, &event_x, &event_y);

	/* Don't pan so far the design is completely off the screen */
	v->x0 = MAX(VIEW_HIDLIB(v)->dwg.X1 - v->width, v->x0);
	v->y0 = MAX(VIEW_HIDLIB(v)->dwg.Y1 - v->height, v->y0);

	if (v->use_max_hidlib) {
		v->x0 = MIN(v->x0, VIEW_HIDLIB(v)->dwg.X2 + (VIEW_HIDLIB(v)->dwg.X2 - VIEW_HIDLIB(v)->dwg.X1));
		v->y0 = MIN(v->y0, VIEW_HIDLIB(v)->dwg.Y2 + (VIEW_HIDLIB(v)->dwg.Y2 - VIEW_HIDLIB(v)->dwg.Y1));
	}
	else {
		assert(v->max_width > 0);
		assert(v->max_height > 0);
		v->x0 = MIN(v->x0, v->max_width);
		v->y0 = MIN(v->y0, v->max_height);
	}

	/* Fix up noted event coordinates to match where we clamped. Alternatively
	 * we could call rnd_gtk_note_event_location (0,0,0); to get a new pointer
	 * location, but this costs us an xserver round-trip (on X11 platforms)
	 */
	rnd_gtk_coords_event2design(v, event_x, event_y, &v->design_x, &v->design_y);
	if (!v->inhibit_pan_common)
		rnd_gtk_pan_common();
}

rnd_bool rnd_gtk_coords_design2event(const rnd_gtk_view_t *v, rnd_coord_t design_x, rnd_coord_t design_y, int *event_x, int *event_y)
{
	*event_x = DRAW_X(v, design_x);
	*event_y = DRAW_Y(v, design_y);

	return rnd_true;
}

rnd_bool rnd_gtk_coords_event2design(const rnd_gtk_view_t *v, int event_x, int event_y, rnd_coord_t * design_x, rnd_coord_t * design_y)
{
	*design_x = rnd_round(EVENT_TO_DESIGN_X(v, event_x));
	*design_y = rnd_round(EVENT_TO_DESIGN_Y(v, event_y));

	return rnd_true;
}

void rnd_gtk_zoom_post(rnd_gtk_view_t *v)
{
	v->coord_per_px = rnd_gtk_clamp_zoom(v, v->coord_per_px);
	v->width = v->canvas_width * v->coord_per_px;
	v->height = v->canvas_height * v->coord_per_px;
}

/* gport->view.coord_per_px:
 * zoom value is PCB units per screen pixel.  Larger numbers mean zooming
 * out - the largest value means you are looking at the whole design.
 *
 * gport->view_width and gport->view_height are in librnd coordinates
 */
void rnd_gtk_zoom_view_abs(rnd_gtk_view_t *v, rnd_coord_t center_x, rnd_coord_t center_y, double new_zoom)
{
	double clamped_zoom;
	double xtmp, ytmp;
	rnd_coord_t cmaxx, cmaxy;

	clamped_zoom = rnd_gtk_clamp_zoom(v, new_zoom);
	if (clamped_zoom != new_zoom)
		return;

	if (v->coord_per_px == new_zoom)
		return;

	/* Do not allow zoom level that'd overflow the coord type */
	cmaxx = v->canvas_width  * (new_zoom / 2.0);
	cmaxy = v->canvas_height * (new_zoom / 2.0);
	if ((cmaxx >= RND_COORD_MAX/2) || (cmaxy >= RND_COORD_MAX/2)) {
		return;
	}

	xtmp = (SIDE_X(v, center_x) - v->x0) / (double) v->width;
	ytmp = (SIDE_Y(v, center_y) - v->y0) / (double) v->height;

	v->coord_per_px = new_zoom;
	rnd_pixel_slop = new_zoom;
	rnd_gtk_tw_ranges_scale(ghidgui);

	v->x0 = SIDE_X(v, center_x) - xtmp * v->width;
	v->y0 = SIDE_Y(v, center_y) - ytmp * v->height;

	uiz_pan_common(v);
}



void rnd_gtk_zoom_view_rel(rnd_gtk_view_t *v, rnd_coord_t center_x, rnd_coord_t center_y, double factor)
{
	rnd_gtk_zoom_view_abs(v, center_x, center_y, v->coord_per_px * factor);
}

/* Side-correct version - long term this will be kept and the other is removed */
void rnd_gtk_zoom_view_win(rnd_gtk_view_t *v, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, int setch)
{
	double xf, yf;

	if ((v->canvas_width < 1) || (v->canvas_height < 1))
		return;

	xf = (x2 - x1) / v->canvas_width;
	yf = (y2 - y1) / v->canvas_height;
	v->coord_per_px = (xf > yf ? xf : yf);

	v->x0 = SIDE_X(v, LOCALFLIPX(v) ? x2 : x1);
	v->y0 = SIDE_Y(v, LOCALFLIPY(v) ? y2 : y1);

	uiz_pan_common(v);
	if (setch) {
		v->design_x = (x1+x2)/2;
		v->design_y = (y1+y2)/2;
		rnd_hidcore_crosshair_move_to(VIEW_HIDLIB(v), v->design_x, v->design_y, 0);
	}

	rnd_gtk_tw_ranges_scale(ghidgui);
}

void rnd_gtk_pan_view_abs(rnd_gtk_view_t *v, rnd_coord_t design_x, rnd_coord_t design_y, double widget_x, double widget_y)
{
	v->x0 = rnd_round((double)SIDE_X(v, design_x) - (double)widget_x * v->coord_per_px);
	v->y0 = rnd_round((double)SIDE_Y(v, design_y) - (double)widget_y * v->coord_per_px);

	uiz_pan_common(v);
}

void rnd_gtk_pan_view_rel(rnd_gtk_view_t *v, rnd_coord_t dx, rnd_coord_t dy)
{
	v->x0 += dx;
	v->y0 += dy;

	uiz_pan_common(v);
}

int rnd_gtk_get_coords(rnd_gtk_t *ctx, rnd_gtk_view_t *vw, const char *msg, rnd_coord_t *x, rnd_coord_t *y, int force)
{
	int res = 0;
	if ((force || !vw->has_entered) && msg) {
		if (!vw->panning) /* we are outside of the drawing area; query xy only if we are not already panning; corner case: pan ends outside of the drawing area -> get_xy makes the grey GUI lockup */
			res = rnd_gtk_get_user_xy(ctx, msg);
		if (res > 0)
			return 1;
	}
	if (vw->has_entered) {
		*x = vw->design_x;
		*y = vw->design_y;
	}
	return res;
}
