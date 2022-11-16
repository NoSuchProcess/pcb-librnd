/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017,2018 Tibor 'Igor2' Palinkas
 *  pcb-rnd Copyright (C) 2017 Alain Vigne
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
 *
 */

/* This file was originally written by Peter Clifton
   then got a major refactoring by Tibor 'Igor2' Palinkas and Alain in pcb-rnd
*/

#ifndef RND_GTK_WT_REVIEW_H
#define RND_GTK_WT_REVIEW_H

#include <gtk/gtk.h>
#include <genlist/gendlist.h>
#include <librnd/core/hid.h>
#include "ui_zoompan.h"
#include "rnd_gtk.h"

#define RND_GTK_TYPE_PREVIEW           (rnd_gtk_preview_get_type())
#define RND_GTK_PREVIEW(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), RND_GTK_TYPE_PREVIEW, rnd_gtk_preview_t))
#define RND_GTK_PREVIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  RND_GTK_TYPE_PREVIEW, rnd_gtk_preview_class_t))
#define RND_GTK_IS_PREVIEW(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RND_GTK_TYPE_PREVIEW))
#define RND_GTK_PREVIEW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  RND_GTK_TYPE_PREVIEW, rnd_gtk_preview_class_t))

typedef struct rnd_gtk_preview_class_s rnd_gtk_preview_class_t;
typedef struct rnd_gtk_preview_s rnd_gtk_preview_t;

struct rnd_gtk_preview_class_s {
	GtkcDrawingAreaClass parent_class;
};

typedef void (*rnd_gtk_init_drawing_widget_t)(GtkWidget *widget, void *port);
typedef void (*rnd_gtk_preview_config_t)(rnd_gtk_preview_t *gp, GtkWidget *widget);
typedef gboolean(*rnd_gtk_preview_expose_t)(GtkWidget *widget, rnd_gtk_expose_t *ev, rnd_hid_expose_t expcall, rnd_hid_expose_ctx_t *ctx);
typedef rnd_bool(*rnd_gtk_preview_mouse_ev_t)(void *widget, void *draw_data, rnd_hid_mouse_ev_t kind, rnd_coord_t x, rnd_coord_t y);
typedef rnd_bool(*rnd_gtk_preview_key_ev_t)(void *widget, void *draw_data, rnd_bool release, rnd_hid_cfg_mod_t mods, unsigned short int key_raw, unsigned short int key_tr);

struct rnd_gtk_preview_s {
	GtkcDrawingArea parent_instance;

	rnd_hid_expose_ctx_t expose_data;
	rnd_gtk_view_t view;

	rnd_coord_t x_min, y_min, x_max, y_max;   /* for the obj preview: bounding box */
	gint w_pixels, h_pixels;                  /* natural size of object preview */
	gint win_w, win_h;

	rnd_coord_t xoffs, yoffs; /* difference between desired x0;y0 and the actual window's top left coords */

	void *gport;
	rnd_gtk_init_drawing_widget_t init_drawing_widget;
	rnd_gtk_preview_config_t config_cb;
	rnd_gtk_preview_expose_t expose;
	rnd_gtk_preview_mouse_ev_t mouse_cb;
	rnd_gtk_preview_key_ev_t key_cb;
	rnd_hid_expose_t overlay_draw_cb;
	rnd_coord_t grabx, graby;
	time_t grabt;
	long grabmot;

	void *obj; /* object being displayed in the preview */

	rnd_gtk_t *ctx;
	gtkc_event_xyz_t rs, sc, motion, mpress, mrelease, kpress, krelease, ev_destroy;
	gdl_elem_t link; /* in the list of all previews in ->ctx->previews */
	unsigned redraw_with_design:1;
	unsigned redrawing:1;
	unsigned draw_inited:1; /* used to track new_context() state for opengl backends */

	unsigned flip_global:1;  /* flip status is tied to board's flip; ignored if flip_local is set */
	unsigned flip_local:1;   /* local flip enabled on tab; if both local and global flips are off, the preview is permanently on no-flip mode */
};

GType rnd_gtk_preview_get_type(void);

/* Queries the natural size of a preview widget */
void rnd_gtk_preview_get_natsize(rnd_gtk_preview_t *preview, int *width, int *height);

GtkWidget *rnd_gtk_preview_new(rnd_gtk_t *ctx, rnd_gtk_init_drawing_widget_t init_widget,
	rnd_gtk_preview_expose_t expose, rnd_hid_expose_t dialog_draw, rnd_gtk_preview_config_t config, void *draw_data);

void rnd_gtk_preview_zoomto(rnd_gtk_preview_t *preview, const rnd_box_t *data_view);

/* invalidate (redraw) all preview widgets whose current view overlaps with
   the screen box; if screen is NULL, redraw all */
void rnd_gtk_preview_invalidate(rnd_gtk_t *ctx, const rnd_box_t *screen);

/* called when the global view got flipped - updates all previews with global flip follow */
void rnd_gtk_previews_flip(rnd_gtk_t *ctx);


void rnd_gtk_preview_del(rnd_gtk_t *ctx, rnd_gtk_preview_t *prv);

#endif /* RND_GTK_WT_REVIEW_H */
