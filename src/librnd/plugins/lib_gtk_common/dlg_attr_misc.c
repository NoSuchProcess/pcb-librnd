/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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

/* Smallish misc DAD widgets */

#include "wt_preview.h"
#include "bu_pixbuf.h"

#include <librnd/plugins/lib_hid_common/dad_markup.h>

static int rnd_gtk_progress_set(attr_dlg_t *ctx, int idx, const rnd_hid_attr_val_t *val)
{
	GtkWidget *prg = ctx->wl[idx];
	double pos = val->dbl;

	if (pos < 0.0) pos = 0.0;
	else if (pos > 1.0) pos = 1.0;

	if ((pos >= 0.0) && (pos <= 1.0)) /* extra case for NaN */
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(prg), pos);
	return 0;
}


static GtkWidget *rnd_gtk_progress_create(attr_dlg_t *ctx, rnd_hid_attribute_t *attr, GtkWidget *parent, int j)
{
	GtkWidget *bparent, *prg;

	prg = gtk_progress_bar_new();

	if (attr->rnd_hatt_flags & RND_HATF_PRG_SMALL)
		gtk_widget_set_size_request(prg, 16, 6);
	else
		gtk_widget_set_size_request(prg, -1, 16);

	gtk_widget_set_tooltip_text(prg, attr->help_text);
	bparent = frame_scroll(parent, attr->rnd_hatt_flags, &ctx->wltop[j]);
	gtkc_box_pack_append(bparent, prg, TRUE, 0);
	g_object_set_data(G_OBJECT(prg), RND_OBJ_PROP, ctx);
	return prg;
}

static void rnd_gtka_preview_expose(rnd_hid_gc_t gc, rnd_hid_expose_ctx_t *e)
{
	rnd_hid_preview_t *prv = e->draw_data;
	prv->user_expose_cb(prv->attrib, prv, gc, e);
}

static rnd_bool rnd_gtka_preview_mouse(void *widget, void *draw_data, rnd_hid_mouse_ev_t kind, rnd_coord_t x, rnd_coord_t y)
{
	rnd_hid_preview_t *prv = draw_data;
	if (prv->user_mouse_cb != NULL)
		return prv->user_mouse_cb(prv->attrib, prv, kind, x, y);
	return rnd_false;
}

static rnd_bool rnd_gtka_preview_key(void *widget, void *draw_data, rnd_bool release, rnd_hid_cfg_mod_t mods, unsigned short int key_raw, unsigned short int key_tr)
{
	rnd_hid_preview_t *prv = draw_data;
	if (prv->user_key_cb != NULL)
		return prv->user_key_cb(prv->attrib, prv, release, mods, key_raw, key_tr);
	return rnd_false;
}

void rnd_gtka_preview_zoomto(rnd_hid_attribute_t *attrib, void *hid_ctx, const rnd_box_t *view)
{
	attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	GtkWidget *prv = ctx->wl[idx];
	if (view != NULL)
		rnd_gtk_preview_zoomto(RND_GTK_PREVIEW(prv), view);
	gtk_widget_queue_draw(prv);
}

static int rnd_gtka_preview_set(attr_dlg_t *ctx, int idx, const rnd_hid_attr_val_t *val)
{
	GtkWidget *prv = ctx->wl[idx];

	gtk_widget_queue_draw(prv);
	return 0;
}


void rnd_gtka_preview_config(rnd_gtk_preview_t *gp, GtkWidget *widget)
{
	rnd_hid_preview_t *prv = gp->expose_data.draw_data;
	if (prv->initial_view_valid) {
		rnd_gtk_preview_zoomto(RND_GTK_PREVIEW(widget), &prv->initial_view);
		gtk_widget_queue_draw(widget);
		prv->initial_view_valid = 0;
	}
}

static GtkWidget *rnd_gtk_preview_create(attr_dlg_t *ctx, rnd_hid_attribute_t *attr, GtkWidget *parent, int j)
{
	GtkWidget *bparent, *prv;
	rnd_gtk_preview_t *p;
	rnd_hid_preview_t *hp = attr->wdata;

	hp->hid_wdata = ctx;
	hp->hid_zoomto_cb = rnd_gtka_preview_zoomto;
	
	bparent = frame_scroll(parent, attr->rnd_hatt_flags, &ctx->wltop[j]);
	prv = rnd_gtk_preview_new(ctx->gctx, ctx->gctx->impl.init_drawing_widget, ctx->gctx->impl.preview_expose, rnd_gtka_preview_expose, rnd_gtka_preview_config, attr->wdata, hp->loc_dsg);
	gtkc_box_pack_append(bparent, prv, TRUE, 0);
	gtk_widget_set_tooltip_text(prv, attr->help_text);
	p = (rnd_gtk_preview_t *) prv;
	p->mouse_cb = rnd_gtka_preview_mouse;
	p->key_cb = rnd_gtka_preview_key;

/*	p->overlay_draw_cb = pcb_stub_draw_csect_overlay;*/
TODO("TODO make these configurable:")
	p->x_min = 0;
	p->y_min = 0;
	p->x_max = RND_MM_TO_COORD(100);
	p->y_max = RND_MM_TO_COORD(100);
	p->w_pixels = RND_MM_TO_COORD(10);
	p->h_pixels = RND_MM_TO_COORD(10);
	p->redraw_with_design = !!(attr->hatt_flags & RND_HATF_PRV_DESIGN);

	gtk_widget_set_size_request(prv, hp->min_sizex_px, hp->min_sizey_px);
	return prv;
}

static GtkWidget *rnd_gtk_picture_create(attr_dlg_t *ctx, rnd_hid_attribute_t *attr, GtkWidget *parent, int j, gboolean (*click_cb)(GtkWidget*,long,long,long,void*), void *click_ctx)
{
	GtkWidget *bparent, *pic, *evb;
	GdkPixbuf *pixbuf;
	bparent = frame_scroll(parent, attr->rnd_hatt_flags, &ctx->wltop[j]);
	int expfill = (attr->rnd_hatt_flags & RND_HATF_EXPFILL);
	static struct gtkc_event_xyz_s ev_click; /* shared among all pictures, so udata is NULL */

	pixbuf = rnd_gtk_xpm2pixbuf(attr->wdata, 1);
	pic = gtk_image_new_from_pixbuf(pixbuf);
	gtkc_workaround_image_scale_bug(pic, pixbuf);
	evb = wrap_bind_click(pic, rnd_gtkc_xy_ev(&ev_click, click_cb, NULL));
	g_object_set_data(G_OBJECT(evb), RND_OBJ_PROP, click_ctx);
	g_object_set_data(G_OBJECT(evb), RND_OBJ_PROP_CLICK, attr);

	gtkc_box_pack_append(bparent, evb, expfill, 0);
	gtk_widget_set_tooltip_text(pic, attr->help_text);

	return evb;
}


static GtkWidget *rnd_gtk_picbutton_create(attr_dlg_t *ctx, rnd_hid_attribute_t *attr, GtkWidget *parent, int j, int toggle, int expfill)
{
	GtkWidget *bparent, *button, *img;
	GdkPixbuf *pixbuf;

	bparent = frame_scroll(parent, attr->rnd_hatt_flags, &ctx->wltop[j]);

	pixbuf = rnd_gtk_xpm2pixbuf(attr->wdata, 1);
	img = gtk_image_new_from_pixbuf(pixbuf);
	gtkc_workaround_image_scale_bug(img, pixbuf);

	if (toggle)
		button = gtk_toggle_button_new();
	else
		button = gtk_button_new();

	gtkc_button_set_child(button, img);

	gtkc_workaround_image_button_border_bug(button, pixbuf);

	gtkc_box_pack_append(bparent, button, expfill, 0);
	gtk_widget_set_tooltip_text(button, attr->help_text);

	return button;
}

static GtkWidget *rnd_gtk_color_create(attr_dlg_t *ctx, rnd_hid_attribute_t *attr, GtkWidget *parent, int j)
{
	GtkWidget *bparent, *button;
	rnd_gtk_color_t gclr;

	bparent = frame_scroll(parent, attr->rnd_hatt_flags, &ctx->wltop[j]);

	memset(&gclr, 0, sizeof(gclr));
	ctx->gctx->impl.map_color(rnd_color_black, &gclr);

	button = gtkc_color_button_new_with_color(&gclr);
	gtk_color_button_set_title(GTK_COLOR_BUTTON(button), NULL);

	gtkc_box_pack_append(bparent, button, TRUE, 0);
	gtk_widget_set_tooltip_text(button, attr->help_text);

	return button;
}


static int rnd_gtk_color_set(attr_dlg_t *ctx, int idx, const rnd_hid_attr_val_t *val)
{
	rnd_gtk_color_t gclr;
	GtkWidget *btn = ctx->wl[idx];

	memset(&gclr, 0, sizeof(gclr));
	ctx->gctx->impl.map_color(&val->clr, &gclr);
	gtkc_color_button_set_color(btn, &gclr);

	return 0;
}

