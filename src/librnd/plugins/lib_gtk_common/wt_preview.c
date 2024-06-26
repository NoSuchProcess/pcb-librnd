/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Alain Vigne
 *  pcb-rnd Copyright (C) 2017..2019 Tibor 'Igor2' Palinkas
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
 *
 */

/* This file copied and modified by Peter Clifton, starting from
   gui-pinout-window.c, written by Bill Wilson for the PCB Gtk port,
   then got a major refactoring by Tibor 'Igor2' Palinkas and Alain in pcb-rnd */

#include "config.h"
#include <librnd/core/rnd_conf.h>

#include "in_mouse.h"
#include "in_keyboard.h"
#include "wt_preview.h"

#include <librnd/core/compat_misc.h>
#include <librnd/core/globalconst.h>

static void get_ptr(rnd_gtk_preview_t *preview, rnd_coord_t *cx, rnd_coord_t *cy, gint *xp, gint *yp);

static void perview_update_offs(rnd_gtk_preview_t *preview)
{
	double xf, yf;

	xf = (double)preview->view.width / preview->view.canvas_width;
	yf = (double)preview->view.height / preview->view.canvas_height;
	preview->view.coord_per_px = (xf > yf ? xf : yf);

	preview->xoffs = (rnd_coord_t)(preview->view.width / 2 - preview->view.canvas_width * preview->view.coord_per_px / 2);
	preview->yoffs = (rnd_coord_t)(preview->view.height / 2 - preview->view.canvas_height * preview->view.coord_per_px / 2);
}

static void rnd_gtk_preview_update_x0y0(rnd_gtk_preview_t *preview)
{
	preview->x_min = preview->view.x0;
	preview->y_min = preview->view.y0;
	preview->x_max = preview->view.x0 + preview->view.width;
	preview->y_max = preview->view.y0 + preview->view.height;
	preview->w_pixels = preview->view.canvas_width;
	preview->h_pixels = preview->view.canvas_height;

	perview_update_offs(preview);
}

void rnd_gtk_preview_zoomto(rnd_gtk_preview_t *preview, const rnd_box_t *data_view)
{
	int orig = preview->view.inhibit_pan_common;
	preview->view.inhibit_pan_common = 1; /* avoid pan logic for the main window */

	preview->view.x0 = data_view->X1;
	preview->view.y0 = data_view->Y1;
	preview->view.width = data_view->X2 - data_view->X1;
	preview->view.height = data_view->Y2 - data_view->Y1;

	if (preview->view.width > preview->view.max_width)
		preview->view.max_width = preview->view.width;
	if (preview->view.height > preview->view.max_height)
		preview->view.max_height = preview->view.height;

	rnd_gtk_zoom_view_win(&preview->view, data_view->X1, data_view->Y1, data_view->X2, data_view->Y2, 0);
	rnd_gtk_preview_update_x0y0(preview);
	preview->view.inhibit_pan_common = orig;
}

/* modify the zoom level to coord_per_px (clamped), keeping window cursor
   position wx;wy at perview rnd_coord_t position cx;cy */
void rnd_gtk_preview_zoom_cursor(rnd_gtk_preview_t *preview, rnd_coord_t cx, rnd_coord_t cy, int wx, int wy, double coord_per_px)
{
	int orig;

	coord_per_px = rnd_gtk_clamp_zoom(&preview->view, coord_per_px);

	if (coord_per_px == preview->view.coord_per_px)
		return;

	orig = preview->view.inhibit_pan_common;
	preview->view.inhibit_pan_common = 1; /* avoid pan logic for the main window */
	preview->view.coord_per_px = coord_per_px;

	preview->view.width = preview->view.canvas_width * preview->view.coord_per_px;
	preview->view.height = preview->view.canvas_height * preview->view.coord_per_px;

	if (preview->view.width > preview->view.max_width)
		preview->view.max_width = preview->view.width;
	if (preview->view.height > preview->view.max_height)
		preview->view.max_height = preview->view.height;

	preview->view.x0 = cx - wx * preview->view.coord_per_px;
	preview->view.y0 = cy - wy * preview->view.coord_per_px;

	rnd_gtk_preview_update_x0y0(preview);
	preview->view.inhibit_pan_common = orig;
}

void rnd_gtk_preview_zoom_cursor_rel(rnd_gtk_preview_t *preview, rnd_coord_t cx, rnd_coord_t cy, int wx, int wy, double factor)
{
	rnd_gtk_preview_zoom_cursor(preview, cx, cy, wx, wy, preview->view.coord_per_px * factor);
}

static void preview_set_view(rnd_gtk_preview_t *preview)
{
	rnd_box_t view;

	memcpy(&view, preview->obj, sizeof(rnd_box_t)); /* assumes the object's first field is rnd_box_t */

	rnd_gtk_preview_zoomto(preview, &view);
}

static void preview_set_data(rnd_gtk_preview_t *preview, void *obj)
{
	preview->obj = obj;
	if (obj == NULL) {
		preview->w_pixels = 0;
		preview->h_pixels = 0;
		return;
	}

	preview_set_view(preview);
}

enum {
	PROP_GPORT = 2,
	PROP_INIT_WIDGET = 3,
	PROP_EXPOSE = 4,
	PROP_KIND = 5,
	PROP_COM = 7,
	PROP_DIALOG_DRAW = 8, /* for PCB_LYT_DIALOG */
	PROP_DRAW_DATA = 9,
	PROP_CONFIG = 10
};

static GObjectClass *rnd_gtk_preview_parent_class = NULL;

/* Initialises the preview object once it is constructed. Chains up in case
   the parent class wants to do anything too. object is the preview object. */
static void rnd_gtk_preview_constructed(GObject *object)
{
	if (G_OBJECT_CLASS(rnd_gtk_preview_parent_class)->constructed != NULL)
		G_OBJECT_CLASS(rnd_gtk_preview_parent_class)->constructed(object);
}

/* Just before the rnd_gtk_preview_t GObject is finalized, free our
   allocated data, and then chain up to the parent's finalize handler. */
static void rnd_gtk_preview_finalize(GObject *object)
{
	rnd_gtk_preview_t *preview = RND_GTK_PREVIEW(object);

	/* Passing NULL for subcircuit data will clear the preview */
	preview_set_data(preview, NULL);

	G_OBJECT_CLASS(rnd_gtk_preview_parent_class)->finalize(object);
}

static void rnd_gtk_preview_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	rnd_gtk_preview_t *preview = RND_GTK_PREVIEW(object);

	switch (property_id) {
	case PROP_GPORT:
		preview->gport = (void *) g_value_get_pointer(value);
		break;
	case PROP_COM:
		preview->ctx = (void *) g_value_get_pointer(value);
		break;
	case PROP_INIT_WIDGET:
		preview->init_drawing_widget = (void *) g_value_get_pointer(value);
		break;
	case PROP_EXPOSE:
		preview->expose = (void *) g_value_get_pointer(value);
		break;
	case PROP_DRAW_DATA:
		preview->expose_data.draw_data = g_value_get_pointer(value);
		rnd_gtk_preview_redraw_all(preview);
		break;
	case PROP_DIALOG_DRAW:
		preview->expose_data.expose_cb = (void *) g_value_get_pointer(value);
		break;
	case PROP_CONFIG:
		preview->config_cb = (void *) g_value_get_pointer(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void rnd_gtk_preview_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

#define flip_apply(preview) \
do { \
	if (preview->flip_local) { \
		rnd_conf_force_set_bool(rnd_conf.editor.view.flip_x, preview->view.flip_x); \
		rnd_conf_force_set_bool(rnd_conf.editor.view.flip_y, preview->view.flip_y); \
	} \
	else if (!preview->flip_global) { \
		rnd_conf_force_set_bool(rnd_conf.editor.view.flip_x, 0); \
		rnd_conf_force_set_bool(rnd_conf.editor.view.flip_y, 0); \
	} \
} while(0)

static gboolean rnd_gtk_preview_expose(GtkWidget *widget, rnd_gtk_expose_t *ev)
{
	rnd_gtk_preview_t *preview = RND_GTK_PREVIEW(widget);
	gboolean res;
	int save_fx, save_fy;

	preview->expose_data.view.X1 = preview->x_min;
	preview->expose_data.view.Y1 = preview->y_min;
	preview->expose_data.view.X2 = preview->x_max;
	preview->expose_data.view.Y2 = preview->y_max;
	save_fx = rnd_conf.editor.view.flip_x;
	save_fy = rnd_conf.editor.view.flip_y;
	flip_apply(preview);

	preview->expose_data.design = VIEW_HIDLIB(&preview->view);
	res = preview->expose(widget, ev, rnd_app.expose_preview, &preview->expose_data);

	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_x, save_fx);
	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_y, save_fy);

	return res;
}

/* Override parent virtual class methods as needed and register GObject properties. */
static void rnd_gtk_preview_class_init(rnd_gtk_preview_class_t *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *gtk_widget_class = GTK_WIDGET_CLASS(klass);

	gobject_class->finalize = rnd_gtk_preview_finalize;
	gobject_class->set_property = rnd_gtk_preview_set_property;
	gobject_class->get_property = rnd_gtk_preview_get_property;
	gobject_class->constructed = rnd_gtk_preview_constructed;

	RND_GTK_EXPOSE_EVENT_SET(gtk_widget_class);

	rnd_gtk_preview_parent_class = (GObjectClass *) g_type_class_peek_parent(klass);

	g_object_class_install_property(gobject_class, PROP_GPORT, g_param_spec_pointer("gport", "", "", G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_COM, g_param_spec_pointer("ctx", "", "", G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_INIT_WIDGET, g_param_spec_pointer("init-widget", "", "", G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_EXPOSE, g_param_spec_pointer("expose", "", "", G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_DIALOG_DRAW, g_param_spec_pointer("dialog_draw", "", "", G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_DRAW_DATA, g_param_spec_pointer("draw_data", "", "", G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_CONFIG, g_param_spec_pointer("config", "", "", G_PARAM_WRITABLE));
}

static void update_expose_data(rnd_gtk_preview_t *prv)
{
	rnd_gtk_zoom_post(&prv->view);
	prv->expose_data.view.X1 = prv->view.x0;
	prv->expose_data.view.Y1 = prv->view.y0;
	prv->expose_data.view.X2 = prv->view.x0 + prv->view.width;
	prv->expose_data.view.Y2 = prv->view.y0 + prv->view.height;

/*	rnd_printf("EXPOSE_DATA: %$mm %$mm - %$mm %$mm (%f %$mm)\n",
		prv->expose_data.view.X1, prv->expose_data.view.Y1,
		prv->expose_data.view.X2, prv->expose_data.view.Y2,
		prv->view.coord_per_px, prv->view.x0);*/
}


static gboolean preview_resize_event_cb(GtkWidget *w, long sx, long sy, long z, void *tmp)
{
	int need_rezoom;
	rnd_gtk_preview_t *preview = (rnd_gtk_preview_t *) w;
	preview->win_w = sx;
	preview->win_h = sy;

	need_rezoom = (preview->view.canvas_width == 0) || (preview->view.canvas_height == 0);

	preview->view.canvas_width = sx;
	preview->view.canvas_height = sy;

	if (need_rezoom) {
		rnd_box_t b;
		b.X1 = preview->view.x0;
		b.Y1 = preview->view.y0;
		b.X2 = b.X1 + preview->view.width;
		b.Y2 = b.Y1 + preview->view.height;
		rnd_gtk_preview_zoomto(preview, &b);
	}
	perview_update_offs(preview);

	if (preview->config_cb != NULL)
		preview->config_cb(preview, w);

/*	update_expose_data(preview);*/
	return TRUE;
}


static gboolean button_press_(GtkWidget *w, rnd_hid_cfg_mod_t btn)
{
	rnd_gtk_preview_t *preview = (rnd_gtk_preview_t *) w;
	rnd_coord_t cx, cy;
	gint wx, wy;
	get_ptr(preview, &cx, &cy, &wx, &wy);
	void *draw_data = NULL;

	draw_data = preview->expose_data.draw_data;

	switch (btn) {
	case RND_MB_LEFT:
		if (preview->mouse_cb != NULL) {
/*				rnd_printf("bp %mm %mm\n", cx, cy); */
			if (preview->mouse_cb(w, draw_data, RND_HID_MOUSE_PRESS, cx, cy))
				gtk_widget_queue_draw(w);
		}
		break;
	case RND_MB_MIDDLE:
		preview->view.panning = 1;
		preview->grabx = cx;
		preview->graby = cy;
		preview->grabt = time(NULL);
		preview->grabmot = 0;
		break;
	case RND_MB_SCROLL_UP:
		rnd_gtk_preview_zoom_cursor_rel(preview, cx, cy, wx, wy, 0.8);
		goto do_zoom;
	case RND_MB_SCROLL_DOWN:
		rnd_gtk_preview_zoom_cursor_rel(preview, cx, cy, wx, wy, 1.25);
		goto do_zoom;
	default:
		return FALSE;
	}
	return FALSE;

do_zoom:;
	update_expose_data(preview);
	gtk_widget_queue_draw(w);

	return FALSE;
}

static gboolean button_press(GtkWidget *w, rnd_hid_cfg_mod_t btn)
{
	rnd_gtk_preview_t *preview = (rnd_gtk_preview_t *)w;
	int save_fx, save_fy;
	gboolean r;

	save_fx = rnd_conf.editor.view.flip_x;
	save_fy = rnd_conf.editor.view.flip_y;
	flip_apply(preview);

	r = button_press_(w, btn);

	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_x, save_fx);
	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_y, save_fy);

	return r;
}

static gboolean preview_button_press_cb(GtkWidget *w, long x, long y, long btn, gpointer data)
{
	return button_press(w, btn & RND_MB_ANY);
}

static gboolean preview_scroll_cb(GtkWidget *w, long x, long y, long z, gpointer data)
{
	gtk_widget_grab_focus(w);

	if (y < 0) return button_press(w, RND_MB_SCROLL_UP);
	if (y > 0) return button_press(w, RND_MB_SCROLL_DOWN);

	return FALSE;
}

static gboolean preview_button_release_cb(GtkWidget *w, long x, long y, long btn, gpointer data)
{
	rnd_gtk_preview_t *preview = (rnd_gtk_preview_t *) w;
	gint wx, wy;
	rnd_coord_t cx, cy;
	void *draw_data = NULL;
	int save_fx, save_fy;

	save_fx = rnd_conf.editor.view.flip_x;
	save_fy = rnd_conf.editor.view.flip_y;
	flip_apply(preview);

	draw_data = preview->expose_data.draw_data;

	get_ptr(preview, &cx, &cy, &wx, &wy);

	switch(btn & RND_MB_ANY) {
	case RND_MB_MIDDLE:
		preview->view.panning = 0;
		break;
	case RND_MB_RIGHT:
		if ((preview->mouse_cb != NULL) && (preview->mouse_cb(w, draw_data, RND_HID_MOUSE_POPUP, cx, cy)))
			gtk_widget_queue_draw(w);
		break;
	case RND_MB_LEFT:
		if (preview->mouse_cb != NULL) {
/*				rnd_printf("br %mm %mm\n", cx, cy); */
			if (preview->mouse_cb(w, draw_data, RND_HID_MOUSE_RELEASE, cx, cy))
				gtk_widget_queue_draw(w);
		}
		break;
	default:;
	}

	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_x, save_fx);
	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_y, save_fy);

	gtk_widget_grab_focus(w);

	return FALSE;
}

static gboolean preview_motion_cb(GtkWidget *w, long x, long y, long z, gpointer data)
{
	rnd_gtk_preview_t *preview = (rnd_gtk_preview_t *) w;
	int save_fx, save_fy;
	rnd_coord_t cx, cy;
	gint wx, wy;
	void *draw_data = NULL;

	save_fx = rnd_conf.editor.view.flip_x;
	save_fy = rnd_conf.editor.view.flip_y;
	flip_apply(preview);

	draw_data = preview->expose_data.draw_data;

	get_ptr(preview, &cx, &cy, &wx, &wy);
	if (preview->view.panning) {
		preview->grabmot++;
		preview->view.x0 = preview->grabx - wx * preview->view.coord_per_px;
		preview->view.y0 = preview->graby - wy * preview->view.coord_per_px;
		rnd_gtk_preview_update_x0y0(preview);
		update_expose_data(preview);
		gtk_widget_queue_draw(w);
	}
	else if (preview->mouse_cb != NULL) {
		if (preview->mouse_cb(w, draw_data, RND_HID_MOUSE_MOTION, cx, cy))
			gtk_widget_queue_draw(w);
	}

	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_x, save_fx);
	rnd_conf_force_set_bool(rnd_conf.editor.view.flip_y, save_fy);

	return FALSE;
}

#include <gdk/gdkkeysyms.h>

static gboolean preview_key_any_cb(GtkWidget *w, long mods, long key_raw, long kv, gpointer data, rnd_bool release)
{
	rnd_gtk_preview_t *preview = (rnd_gtk_preview_t *)w;

	if (preview->key_cb == NULL)
		return FALSE;

	if (preview->flip_local && release) {
		if (kv == RND_GTK_KEY(Tab)) {
			rnd_box_t box;

			box.X1 = preview->view.x0;
			if (preview->view.flip_y)
				box.Y1 = VIEW_HIDLIB(&preview->view)->dwg.Y2 - (preview->view.y0 + preview->view.height);
			else
				box.Y1 = preview->view.y0;
			box.X2 = box.X1 + preview->view.width;
			box.Y2 = box.Y1 + preview->view.height;

			preview->view.flip_y = !preview->view.flip_y;

			rnd_gtk_preview_zoomto(preview, &box);
			gtk_widget_queue_draw(w);
		}
	}

	if (preview->key_cb(w, preview->expose_data.draw_data, release, mods, key_raw, kv))
		gtk_widget_queue_draw(w);

	return TRUE;
}

static gboolean preview_key_press_cb(GtkWidget *w, long mods, long key_raw, long kv, gpointer data)
{
	return preview_key_any_cb(w, mods, key_raw, kv, data, 0);
}

static gboolean preview_key_release_cb(GtkWidget *w, long mods, long key_raw, long kv, gpointer data)
{
	return preview_key_any_cb(w, mods, key_raw, kv, data, 1);
}

/* API */

GType rnd_gtk_preview_get_type()
{
	static GType rnd_gtk_preview_type = 0;

	if (!rnd_gtk_preview_type) {
		static const GTypeInfo rnd_gtk_preview_info = {
			sizeof(rnd_gtk_preview_class_t),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) rnd_gtk_preview_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(rnd_gtk_preview_t),
			0,    /* n_preallocs */
			NULL, /* instance_init */
		};

		rnd_gtk_preview_type = g_type_register_static(GTKC_TYPE_DRAWING_AREA, "rnd_gtk_preview_t", &rnd_gtk_preview_info, (GTypeFlags) 0);
	}

	return rnd_gtk_preview_type;
}

static gint preview_destroy_cb(GtkWidget *widget, long x, long y, long z, gpointer data)
{
	rnd_gtk_t *ctx = data;
	rnd_gtk_preview_t *prv = RND_GTK_PREVIEW(widget);

	rnd_gtk_preview_del(ctx, prv);
	return 0;
}


GtkWidget *rnd_gtk_preview_new(rnd_gtk_t *ctx, rnd_gtk_init_drawing_widget_t init_widget, rnd_gtk_preview_expose_t expose, rnd_hid_expose_cb_t dialog_draw, rnd_gtk_preview_config_t config, void *draw_data, rnd_design_t *local_dsg)
{
	rnd_gtk_preview_t *prv = (rnd_gtk_preview_t *)g_object_new(
		RND_GTK_TYPE_PREVIEW,
		"ctx", ctx, "gport", ctx->impl.gport, "init-widget", init_widget,
		"expose", expose, "dialog_draw", dialog_draw,
		"config", config, "draw_data", draw_data,
		"width-request", 50, "height-request", 50,
		NULL);
	prv->init_drawing_widget(GTK_WIDGET(prv), prv->gport);


TODO(": maybe expose these through the object API so the caller can set it up?")
	memset(&prv->view, 0, sizeof(prv->view));
	prv->view.width = RND_MM_TO_COORD(110);
	prv->view.height = RND_MM_TO_COORD(110);
	prv->view.local_flip = 1;
	prv->view.use_max_hidlib = 0;
	prv->view.max_width = RND_MAX_COORD;
	prv->view.max_height = RND_MAX_COORD;
	prv->view.coord_per_px = RND_MM_TO_COORD(0.25);
	prv->view.min_zoom = rnd_conf.editor.min_zoom;
	prv->view.ctx = ctx;

	if (local_dsg != NULL) {
		prv->view.local_dsg = 1;
		prv->view.dsg = local_dsg;
	}
	else {
		prv->view.local_dsg = 0;
		prv->view.dsg = ctx->hidlib;
	}

	update_expose_data(prv);

	prv->init_drawing_widget(GTK_WIDGET(prv), prv->gport);

	gtkc_setup_events(GTK_WIDGET(prv), 1, 1, 1, 1, 1, 1);

	gtkc_bind_widget_destroy(GTK_WIDGET(prv), rnd_gtkc_xy_ev(&prv->ev_destroy, preview_destroy_cb, ctx));

	gtkc_bind_mouse_scroll(GTK_WIDGET(prv), rnd_gtkc_xy_ev(&prv->rs, preview_scroll_cb, NULL));
	gtkc_bind_mouse_motion(GTK_WIDGET(prv), rnd_gtkc_xy_ev(&prv->motion, preview_motion_cb, NULL));
	gtkc_bind_mouse_press(GTK_WIDGET(prv), rnd_gtkc_xy_ev(&prv->mpress, preview_button_press_cb, NULL));
	gtkc_bind_mouse_release(GTK_WIDGET(prv), rnd_gtkc_xy_ev(&prv->mrelease, preview_button_release_cb, NULL));
	gtkc_bind_resize_dwg(GTK_WIDGET(prv), rnd_gtkc_xy_ev(&prv->sc, preview_resize_event_cb, NULL));
	gtkc_bind_key_press(GTK_WIDGET(prv), rnd_gtkc_xy_ev(&prv->kpress, preview_key_press_cb, NULL));
	gtkc_bind_key_release(GTK_WIDGET(prv), rnd_gtkc_xy_ev(&prv->krelease, preview_key_release_cb, NULL));

	/* keyboard handling needs focusable */
	gtkc_widget_set_focusable(GTK_WIDGET(prv));

	gdl_insert(&ctx->previews, prv, link);
	return GTK_WIDGET(prv);
}

void rnd_gtk_preview_get_natsize(rnd_gtk_preview_t *preview, int *width, int *height)
{
	*width = preview->w_pixels;
	*height = preview->h_pixels;
}

/* Has to be at the end since it undef's SIDE_X */
static void get_ptr(rnd_gtk_preview_t *preview, rnd_coord_t *cx, rnd_coord_t *cy, gint *xp, gint *yp)
{
	gdkc_window_get_pointer(GTK_WIDGET(preview), xp, yp, NULL);
#undef SIDE_X_
#undef SIDE_Y_
#define SIDE_X_(flip, hidlib, x) x
#define SIDE_Y_(flip, hidlib, y) y
	*cx = EVENT_TO_DESIGN_X(&preview->view, *xp) + preview->xoffs;
	*cy = EVENT_TO_DESIGN_Y(&preview->view, *yp) + preview->yoffs;
#undef SIDE_X_
#undef SIDE_Y_
}

void rnd_gtk_preview_invalidate(rnd_gtk_t *ctx, const rnd_box_t *screen)
{
	rnd_gtk_preview_t *prv;

	for(prv = gdl_first(&ctx->previews); prv != NULL; prv = prv->link.next) {
		if (!prv->redraw_with_design || prv->redrawing) continue;
		if (screen != NULL) {
			rnd_box_t pb;
			pb.X1 = prv->view.x0;
			pb.Y1 = prv->view.y0;
			pb.X2 = prv->view.x0 + prv->view.width;
			pb.Y2 = prv->view.y0 + prv->view.height;
			if (rnd_box_intersect(screen, &pb)) {
				prv->redrawing = 1;
				rnd_gtk_preview_expose(GTK_WIDGET(prv), NULL);
				prv->redrawing = 0;
			}
		}
		else {
			prv->redrawing = 1;
			rnd_gtk_preview_expose(GTK_WIDGET(prv), NULL);
			prv->redrawing = 0;
		}
	}
}

void rnd_gtk_previews_flip(rnd_gtk_t *ctx)
{
	rnd_gtk_preview_t *prv;

	for(prv = gdl_first(&ctx->previews); prv != NULL; prv = prv->link.next) {
		if (prv->flip_global) {
			rnd_box_t box;

			box.X1 = prv->view.x0;
			if (!rnd_conf.editor.view.flip_y)
				box.Y1 = VIEW_HIDLIB(&prv->view)->dwg.Y2 - (prv->view.y0 + prv->view.height);
			else
				box.Y1 = prv->view.y0;
			box.X2 = box.X1 + prv->view.width;
			box.Y2 = box.Y1 + prv->view.height;

			rnd_gtk_preview_zoomto(prv, &box);
		}
	}
}


void rnd_gtk_preview_del(rnd_gtk_t *ctx, rnd_gtk_preview_t *prv)
{
	if (prv->link.parent == &ctx->previews)
		gdl_remove(&ctx->previews, prv, link);
}
