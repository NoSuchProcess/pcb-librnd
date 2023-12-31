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
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */


#ifndef RND_LIB_GTK_COMMON_UI_ZOOMPAN_H
#define RND_LIB_GTK_COMMON_UI_ZOOMPAN_H

#include <librnd/core/rnd_bool.h>
#include <librnd/core/global_typedefs.h>

#define VIEW_HIDLIB(v) ((v)->local_dsg ? ((v)->dsg) : ((v)->ctx->hidlib))

#define LOCALFLIPX(v) ((v)->local_flip ? (v)->flip_x : rnd_conf.editor.view.flip_x)
#define LOCALFLIPY(v) ((v)->local_flip ? (v)->flip_y : rnd_conf.editor.view.flip_y)
#define SIDE_X_(flip, hidlib, x)      ((flip ? hidlib->dwg.X2 - (x) : (x)))
#define SIDE_Y_(flip, hidlib, y)      ((flip ? hidlib->dwg.Y2 - (y) : (y)))
#define SIDE_X(v, x)            SIDE_X_(LOCALFLIPX(v), VIEW_HIDLIB(v), (x))
#define SIDE_Y(v, y)            SIDE_Y_(LOCALFLIPY(v), VIEW_HIDLIB(v), (y))

#define DRAW_X(view, x)         (gint)((SIDE_X((view), x) - (view)->x0) / (view)->coord_per_px)
#define DRAW_Y(view, y)         (gint)((SIDE_Y((view), y) - (view)->y0) / (view)->coord_per_px)

#define EVENT_TO_DESIGN_X_(hidlib, view, x) (rnd_coord_t)rnd_round(SIDE_X_(LOCALFLIPX(view), (hidlib), (double)(x) * (view)->coord_per_px + (double)(view)->x0))
#define EVENT_TO_DESIGN_Y_(hidlib, view, y) (rnd_coord_t)rnd_round(SIDE_Y_(LOCALFLIPY(view), (hidlib), (double)(y) * (view)->coord_per_px + (double)(view)->y0))

#define EVENT_TO_DESIGN_X(view, x)  EVENT_TO_DESIGN_X_(VIEW_HIDLIB(view), view, (x))
#define EVENT_TO_DESIGN_Y(view, y)  EVENT_TO_DESIGN_Y_(VIEW_HIDLIB(view), view, (y))

typedef struct {
	/* ORDER MATTERS: first fields until ctx are special: they are saved/restored on esign switch */
	double coord_per_px;     /* Zoom level described as PCB units per screen pixel */

	rnd_coord_t x0;
	rnd_coord_t y0;
	rnd_coord_t width;
	rnd_coord_t height;
	int inited;

	struct rnd_gtk_s *ctx;

	unsigned inhibit_pan_common:1; /* when 1, do not call rnd_gtk_pan_common() */
	unsigned use_max_hidlib:1;     /* when 1, use hidlib->size_*; when 0, use the following two: */
	unsigned local_flip:1;   /* ignore hidlib's flip and use the local one */
	unsigned flip_x:1, flip_y:1; /* local version of flips when ->local_flip is enabled */
	rnd_coord_t max_width;
	rnd_coord_t max_height;

	int canvas_width, canvas_height;

	rnd_bool has_entered;
	rnd_bool panning;
	rnd_coord_t design_x, design_y;        /* design space coordinates of the mouse pointer */
	rnd_coord_t crosshair_x, crosshair_y;  /* design_space coordinates of the crosshair     */

	rnd_coord_t min_zoom;                  /* optional: how much the user can zoom in */

	/* local/global design; used only for flip calculation and initial expose ctx design setup */
	unsigned local_dsg:1;  /* if 1, use local hidlib instead of current GUI hidlib (for local dialogs) */
	rnd_design_t *dsg;     /* remember the hidlib the dialog was opened for */
} rnd_gtk_view_t;

#include "in_mouse.h"

/* take coord_per_px and validate it against other view parameters. Return
    coord_per_px or the clamped value if it was too small or too large. */
double rnd_gtk_clamp_zoom(const rnd_gtk_view_t *vw, double coord_per_px);

void rnd_gtk_pan_view_abs(rnd_gtk_view_t *v, rnd_coord_t design_x, rnd_coord_t design_y, double widget_x, double widget_y);
void rnd_gtk_pan_view_rel(rnd_gtk_view_t *v, rnd_coord_t dx, rnd_coord_t dy);
void rnd_gtk_zoom_view_abs(rnd_gtk_view_t *v, rnd_coord_t center_x, rnd_coord_t center_y, double new_zoom);
void rnd_gtk_zoom_view_rel(rnd_gtk_view_t *v, rnd_coord_t center_x, rnd_coord_t center_y, double factor);
void rnd_gtk_zoom_view_win(rnd_gtk_view_t *v, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, int setch);

/* Updates width and heigth using the new zoom level; call after a call
   to rnd_gtk_zoom_*() */
void rnd_gtk_zoom_post(rnd_gtk_view_t *v);

/* Window with max zoomout could be resized so much that it causes
   coord overflow. Detect overflow and zoom in a bit to avoid that. */
void rnd_gtk_zoom_clamp_overflow(rnd_gtk_view_t *v);

rnd_bool rnd_gtk_coords_design2event(const rnd_gtk_view_t *v, rnd_coord_t design_x, rnd_coord_t design_y, int *event_x, int *event_y);
rnd_bool rnd_gtk_coords_event2design(const rnd_gtk_view_t *v, int event_x, int event_y, rnd_coord_t * design_x, rnd_coord_t * design_y);

int rnd_gtk_get_coords(rnd_gtk_t *ctx, rnd_gtk_view_t *vw, const char *msg, rnd_coord_t *x, rnd_coord_t *y, int force);

#endif
