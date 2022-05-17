/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2022 Tibor 'Igor2' Palinkas
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

/* Boxes and group widgets */

#define RND_OBJ_PROP_PANE_PRIV "librnd_pane_priv"

typedef struct {
	attr_dlg_t *ctx;
	int idx;

	double set_pos; /* target for the ongoing set position call */

	guint set_timer; /* timer 1 */
	guint get_timer; /* timer 2 */
	unsigned set_timer_running:1;
	unsigned get_timer_running:1;
} paned_wdata_t;

static int rnd_gtk_pane_set_(attr_dlg_t *ctx, int idx, double new_pos, int allow_timer);

static gint paned_get_size(paned_wdata_t *pctx)
{
	GtkWidget *pane = pctx->ctx->wl[pctx->idx];
	GtkAllocation a;

	gtkc_widget_get_allocation(pane, &a);

	switch(pctx->ctx->attrs[pctx->idx].type) {
		case RND_HATT_BEGIN_HPANE: return a.width;
		case RND_HATT_BEGIN_VPANE: return a.height;
		default: abort();
	}
}

static void paned_stop_timer(paned_wdata_t *pctx, int id)
{
	if ((id & 1) && pctx->set_timer_running) {
		g_source_remove(pctx->set_timer);
		pctx->set_timer_running = 0;
	}
	if ((id & 2) && pctx->get_timer_running) {
		g_source_remove(pctx->get_timer);
		pctx->get_timer_running = 0;
	}
}

static gboolean paned_setpos_cb(paned_wdata_t *pctx)
{
	rnd_gtk_pane_set_(pctx->ctx, pctx->idx, pctx->set_pos, 0);
	pctx->set_timer_running = 0;
	return FALSE;  /* Turns timer off */
}

static gboolean paned_getpos_cb(paned_wdata_t *pctx)
{
	GtkWidget *pane = pctx->ctx->wl[pctx->idx];
	int px = gtk_paned_get_position(GTK_PANED(pane));
	rnd_trace("paned resize event %d!\n", px);
	pctx->get_timer_running = 0;
	return FALSE;  /* Turns timer off */
}


static int rnd_gtk_pane_set_(attr_dlg_t *ctx, int idx, double new_pos, int allow_timer)
{
	GtkWidget *pane = ctx->wl[idx];
	paned_wdata_t *pctx = g_object_get_data(G_OBJECT(pane), RND_OBJ_PROP_PANE_PRIV);
	double ratio = new_pos;
	gint p, minp, maxp;

	if (ratio < 0.0) ratio = 0.0;
	else if (ratio > 1.0) ratio = 1.0;

	g_object_get(G_OBJECT(pane), "min-position", &minp, "max-position", &maxp, NULL);
	p = paned_get_size(pctx);

	if (p <= 0) {
TODO("gtk4 #48: not yet created; temporary workaround: timer; final fix should be related to the remember-pane-setting (like window geometry)");
		if (allow_timer) {
			paned_stop_timer(pctx, 1);
			pctx->set_pos = ratio;
			pctx->set_timer = g_timeout_add(50, (GSourceFunc)paned_setpos_cb, pctx);
			pctx->set_timer_running = 1;
		}
		return 0; /* do not set before widget is created */
	}

	p = (double)p * ratio;
	if (p < minp) p = minp;
	if (p > maxp) p = maxp;

	gtk_paned_set_position(GTK_PANED(pane), p);
	return 0;
}

static int rnd_gtk_pane_set(attr_dlg_t *ctx, int idx, const rnd_hid_attr_val_t *val)
{
	return rnd_gtk_pane_set_(ctx, idx, val->dbl, 1);
}


static GtkWidget *rnd_gtk_pane_append(attr_dlg_t *ctx, rnd_gtk_attr_tb_t *ts, GtkWidget *parent)
{
	GtkWidget *page = gtkc_vbox_new(FALSE, 4);
	switch(ts->val.pane.next) {
		case 1: gtkc_paned_pack1(parent, page, TRUE); break;
		case 2: gtkc_paned_pack2(parent, page, TRUE); break;
		default:
			rnd_message(RND_MSG_ERROR, "Wrong number of pages for a paned widget (%d): must be exactly 2\n", ts->val.pane.next);
	}
	ts->val.pane.next++;
	return page;
}

void rnd_gtk_pane_move_cb(GObject *self, void *pspec, gpointer user_data)
{
	paned_wdata_t *pctx = g_object_get_data(self, RND_OBJ_PROP_PANE_PRIV);

	if (pctx->ctx->attrs[pctx->idx].name == NULL)
		return; /* do not remember unnamed panes, they are not saved */

	/* generate an event only once in every 500 ms */
	if (!pctx->get_timer_running) {
		pctx->get_timer = g_timeout_add(500, (GSourceFunc)paned_getpos_cb, pctx);
		pctx->get_timer_running = 1;
	}
}



static void rnd_gtk_pane_pre_free(attr_dlg_t *ctx, rnd_hid_attribute_t *attr, int j)
{
	GtkWidget *widget = ctx->wl[j];
	paned_wdata_t *pctx = g_object_get_data(G_OBJECT(widget), RND_OBJ_PROP_PANE_PRIV);

	paned_stop_timer(pctx, 0xff); /* stop all timers */
	free(pctx);
	g_object_set_data(G_OBJECT(widget), RND_OBJ_PROP_PANE_PRIV, NULL);
}


static int rnd_gtk_pane_create(attr_dlg_t *ctx, int j, GtkWidget *parent, int ishor)
{
	GtkWidget *bparent, *widget;
	rnd_gtk_attr_tb_t ts;
	paned_wdata_t *pctx;

	pctx = calloc(sizeof(paned_wdata_t), 1);
	pctx->ctx = ctx;
	pctx->idx = j;
	ctx->attrs[j].wdata = pctx;

	ts.type = TB_PANE;
	ts.val.pane.next = 1;
	ctx->wl[j] = widget = ishor ? gtkc_hpaned_new() : gtkc_vpaned_new();

	g_object_set_data(G_OBJECT(widget), RND_OBJ_PROP_PANE_PRIV, pctx);

	bparent = frame_scroll(parent, ctx->attrs[j].rnd_hatt_flags, &ctx->wltop[j]);
	gtkc_box_pack_append(bparent, widget, TRUE, 0);
	g_object_set_data(G_OBJECT(widget), RND_OBJ_PROP, ctx);
	j = rnd_gtk_attr_dlg_add(ctx, widget, &ts, j+1);

	g_signal_connect(widget, "notify::position", G_CALLBACK(rnd_gtk_pane_move_cb), &ctx->attrs[j]);

	return j;
}
