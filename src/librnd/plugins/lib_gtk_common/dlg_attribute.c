/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *
 */

/* This file was originally written by Bill Wilson for the PCB Gtk port
   then got reworked heavily by Tibor 'Igor2' Palinkas for pcb-rnd DAD */

#include "config.h"
#include <librnd/core/hidlib_conf.h>
#include "dlg_attribute.h"
#include <stdlib.h>

#include <librnd/core/rnd_printf.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/hid_dad_tree.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/event.h>

#include "hid_gtk_conf.h"

#define RND_OBJ_PROP "librnd_context"
#define RND_OBJ_PROP_CLICK "librnd_click"

typedef struct {
	void *caller_data; /* WARNING: for now, this must be the first field (see core spinbox enter_cb) */
	rnd_gtk_t *gctx;
	rnd_hid_attribute_t *attrs;
	GtkWidget **wl;     /* content widget */
	GtkWidget **wltop;  /* the parent widget, which is different from wl if reparenting (extra boxes, e.g. for framing or scrolling) was needed */
	int n_attrs;
	GtkWidget *dialog;
	int close_cb_called;
	rnd_hid_attr_val_t property[RND_HATP_max];
	void (*close_cb)(void *caller_data, rnd_hid_attr_ev_t ev);
	char *id;
	gulong destroy_handler;
	gtkc_event_xyz_t ev_resize, ev_destroy;
	unsigned inhibit_valchg:1;
	unsigned freeing_gui:1;
	unsigned being_destroyed:1;
	unsigned modal:1;
} attr_dlg_t;

#define change_cb(ctx, dst) \
	do { \
		if (ctx->property[RND_HATP_GLOBAL_CALLBACK].func != NULL) \
			ctx->property[RND_HATP_GLOBAL_CALLBACK].func(ctx, ctx->caller_data, dst); \
		if (dst->change_cb != NULL) \
			dst->change_cb(ctx, ctx->caller_data, dst); \
	} while(0) \

#define right_cb(ctx, dst) \
	do { \
		if (dst->right_cb != NULL) \
			dst->right_cb(ctx, ctx->caller_data, dst); \
	} while(0) \

#define enter_cb(ctx, dst) \
	do { \
		if (dst->enter_cb != NULL) \
			dst->enter_cb(ctx, ctx->caller_data, dst); \
	} while(0) \

static void set_flag_cb(GtkToggleButton *button, gpointer user_data)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(button), RND_OBJ_PROP);
	rnd_hid_attribute_t *dst = user_data;

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	dst->val.lng = gtk_toggle_button_get_active(button);
	change_cb(ctx, dst);
}

static void entry_changed_cb(GtkEntry *entry, rnd_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(entry), RND_OBJ_PROP);

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	free((char *)dst->val.str);
	dst->val.str = rnd_strdup(gtkc_entry_get_text(entry));
	change_cb(ctx, dst);
}

static void entry_activate_cb(GtkEntry *entry, rnd_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(entry), RND_OBJ_PROP);
	enter_cb(ctx, dst);
}

static void enum_changed_cb(GtkComboBox *combo_box, rnd_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(combo_box), RND_OBJ_PROP);

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	dst->val.lng = gtk_combo_box_get_active(combo_box);
	change_cb(ctx, dst);
}

static void notebook_changed_cb(GtkNotebook *nb, GtkWidget *page, guint page_num, rnd_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(nb), RND_OBJ_PROP);

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	/* Gets the index (starting from 0) of the current page in the notebook. If
	   the notebook has no pages, then -1 will be returned and no call-back occur. */
	if (gtk_notebook_get_current_page(nb) >= 0) {
		dst->val.lng = page_num;
		change_cb(ctx, dst);
	}
}

static void button_changed_cb(GtkButton *button, rnd_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(button), RND_OBJ_PROP);

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	change_cb(ctx, dst);
}

static gboolean label_click_cb(GtkWidget *evbox, long x, long y, long btn, void *udata_null)
{
	rnd_hid_attribute_t *dst = g_object_get_data(G_OBJECT(evbox), RND_OBJ_PROP_CLICK);
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(evbox), RND_OBJ_PROP);

	if (btn & RND_MB_LEFT) {
		dst->changed = 1;
		if (ctx->inhibit_valchg)
			return TRUE;
		change_cb(ctx, dst);
	}
	else if (btn & RND_MB_RIGHT)
		right_cb(ctx, dst);
	return TRUE;
}

static void color_changed_cb(GtkColorButton *button, rnd_hid_attribute_t *dst)
{
	attr_dlg_t *ctx = g_object_get_data(G_OBJECT(button), RND_OBJ_PROP);
	rnd_gtk_color_t clr;
	const char *str;

	dst->changed = 1;
	if (ctx->inhibit_valchg)
		return;

	gtkc_color_button_get_color(GTK_WIDGET(button), &clr);
	str = ctx->gctx->impl.get_color_name(&clr);
	rnd_color_load_str(&dst->val.clr, str);

	change_cb(ctx, dst);
}

static GtkWidget *chk_btn_new(GtkWidget *box, gboolean active, void (*cb_func)(GtkToggleButton *, gpointer), gpointer data, const gchar *string)
{
	GtkWidget *b;

	if (string != NULL)
		b = gtk_check_button_new_with_label(string);
	else
		b = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b), active);
	gtkc_box_pack_append(box, b, FALSE, 0);
	g_signal_connect(b, "clicked", G_CALLBACK(cb_func), data);
	return b;
}

typedef struct {
	void (*cb)(void *ctx, rnd_hid_attr_ev_t ev);
	void *ctx;
	attr_dlg_t *attrdlg;
} resp_ctx_t;

/* If flags has RND_HATF_SCROLL: if inner is not NULL, it is placed
   directly in the scrolled window without adding an extra box. Useful
   when scrolled content wants to interact with the scroll, e.g. tree table
   wants to scroll where the selection is */
static GtkWidget *frame_scroll_(GtkWidget *parent, rnd_hatt_compflags_t flags, GtkWidget **wltop, GtkWidget *inner)
{
	GtkWidget *fr;
	int expfill = (flags & RND_HATF_EXPFILL);
	int topped = 0;

	if (flags & RND_HATF_FRAME) {
		fr = gtk_frame_new(NULL);
		gtkc_box_pack_append(parent, fr, expfill, 0);

		parent = gtkc_hbox_new(FALSE, 0);
		gtkc_frame_set_child(fr, parent);
		if (wltop != NULL) {
			*wltop = fr;
			topped = 1; /* remember the outmost parent */
		}
	}
	if (flags & RND_HATF_SCROLL) {
		fr = gtkc_scrolled_window_new();
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(fr), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtkc_box_pack_append(parent, fr, TRUE, 0);
		if (inner == NULL) {
			parent = gtkc_hbox_new(FALSE, 0);
			gtkc_scrolled_window_add_with_viewport(fr, parent);
		}
		else {
			parent = inner;
			gtkc_scrolled_window_set_child(fr, inner);
		}
		
		if ((wltop != NULL) && (!topped)) {
			*wltop = fr;
			topped = 1;
		}
	}

	if ((inner != NULL) && !topped) {
		/* when called from tree table without the scroll flag */
		gtkc_box_pack_append(parent, inner, expfill, 0);
		*wltop = parent = inner;
	}

	return parent;
}

static GtkWidget *frame_scroll(GtkWidget *parent, rnd_hatt_compflags_t flags, GtkWidget **wltop)
{
	return frame_scroll_(parent, flags, wltop, NULL);
}


typedef struct {
	enum {
		TB_TABLE,
		TB_TABBED,
		TB_PANE
	} type;
	union {
		struct {
			int cols, rows;
			int col, row;
		} table;
		struct {
			const char **tablab;
		} tabbed;
		struct {
			int next;
		} pane;
	} val;
} rnd_gtk_attr_tb_t;

static int rnd_gtk_attr_dlg_add(attr_dlg_t *ctx, GtkWidget *real_parent, rnd_gtk_attr_tb_t *tb_st, int start_from);

#include "dlg_attr_tree.c"
#include "dlg_attr_misc.c"
#include "dlg_attr_txt.c"
#include "dlg_attr_box.c"

static int rnd_gtk_attr_dlg_add(attr_dlg_t *ctx, GtkWidget *real_parent, rnd_gtk_attr_tb_t *tb_st, int start_from)
{
	int j, i, expfill;
	GtkWidget *combo, *widget, *entry, *vbox1, *hbox, *bparent, *parent, *tbl;

	for (j = start_from; j < ctx->n_attrs; j++) {
		if (ctx->attrs[j].type == RND_HATT_END)
			break;

		/* if we are filling a table, allocate parent boxes in row-major */
		if (tb_st != NULL) {
			switch(tb_st->type) {
				case TB_TABLE:
					parent = gtkc_vbox_new(FALSE, 0);
					gtkc_table_attach1(real_parent, parent, tb_st->val.table.row, tb_st->val.table.col);
					tb_st->val.table.col++;
					if (tb_st->val.table.col >= tb_st->val.table.cols) {
						tb_st->val.table.col = 0;
						tb_st->val.table.row++;
					}
					break;
				case TB_TABBED:
					/* Add a new notebook page with an empty vbox, using tab label present in wdata. */
					parent = gtkc_vbox_new(FALSE, 4);
					if (*tb_st->val.tabbed.tablab) {
						widget = gtk_label_new(*tb_st->val.tabbed.tablab);
						tb_st->val.tabbed.tablab++;
					}
					else
						widget = NULL;
					gtk_notebook_append_page(GTK_NOTEBOOK(real_parent), parent, widget);
					break;
				case TB_PANE:
					parent = rnd_gtk_pane_append(ctx, tb_st, real_parent);
					break;
			}
		}
		else
			parent = real_parent;

		expfill = (ctx->attrs[j].rnd_hatt_flags & RND_HATF_EXPFILL);

		/* create the actual widget from attrs */
		switch (ctx->attrs[j].type) {
			case RND_HATT_BEGIN_HBOX:
				bparent = frame_scroll(parent, ctx->attrs[j].rnd_hatt_flags, &ctx->wltop[j]);
				hbox = gtkc_hbox_new(FALSE, ((ctx->attrs[j].rnd_hatt_flags & RND_HATF_TIGHT) ? 0 : 4));
				gtkc_box_pack_append(bparent, hbox, expfill, 0);
				ctx->wl[j] = hbox;
				j = rnd_gtk_attr_dlg_add(ctx, hbox, NULL, j+1);
				break;

			case RND_HATT_BEGIN_VBOX:
				bparent = frame_scroll(parent, ctx->attrs[j].rnd_hatt_flags, &ctx->wltop[j]);
				vbox1 = gtkc_vbox_new(FALSE, ((ctx->attrs[j].rnd_hatt_flags & RND_HATF_TIGHT) ? 0 : 4));
				expfill = (ctx->attrs[j].rnd_hatt_flags & RND_HATF_EXPFILL);
				gtkc_box_pack_append(bparent, vbox1, expfill, 0);
				ctx->wl[j] = vbox1;
				j = rnd_gtk_attr_dlg_add(ctx, vbox1, NULL, j+1);
				break;

			case RND_HATT_BEGIN_HPANE:
			case RND_HATT_BEGIN_VPANE:
				j = rnd_gtk_pane_create(ctx, j, parent, (ctx->attrs[j].type == RND_HATT_BEGIN_HPANE));
				break;

			case RND_HATT_BEGIN_TABLE:
				{
					rnd_gtk_attr_tb_t ts;
					bparent = frame_scroll(parent, ctx->attrs[j].rnd_hatt_flags, &ctx->wltop[j]);
					ts.type = TB_TABLE;
					ts.val.table.cols = ctx->attrs[j].rnd_hatt_table_cols;
					ts.val.table.rows = rnd_hid_attrdlg_num_children(ctx->attrs, j+1, ctx->n_attrs) / ts.val.table.cols;
					ts.val.table.col = 0;
					ts.val.table.row = 0;
					tbl = gtkc_table_static(ts.val.table.rows, ts.val.table.cols, 1);
					gtkc_box_pack_append(bparent, tbl, expfill, ((ctx->attrs[j].rnd_hatt_flags & RND_HATF_TIGHT) ? 0 : 4));
					ctx->wl[j] = tbl;
					j = rnd_gtk_attr_dlg_add(ctx, tbl, &ts, j+1);
				}
				break;

			case RND_HATT_BEGIN_TABBED:
				{
					rnd_gtk_attr_tb_t ts;
					ts.type = TB_TABBED;
					ts.val.tabbed.tablab = ctx->attrs[j].wdata;
					ctx->wl[j] = widget = gtk_notebook_new();
					gtk_notebook_set_show_tabs(GTK_NOTEBOOK(widget), !(ctx->attrs[j].rnd_hatt_flags & RND_HATF_HIDE_TABLAB));
					if (ctx->attrs[j].rnd_hatt_flags & RND_HATF_LEFT_TAB)
						gtk_notebook_set_tab_pos(GTK_NOTEBOOK(widget), GTK_POS_LEFT);
					else
						gtk_notebook_set_tab_pos(GTK_NOTEBOOK(widget), GTK_POS_TOP);

					gtkc_box_pack_append(parent, widget, expfill, 0);
					g_signal_connect(G_OBJECT(widget), "switch-page", G_CALLBACK(notebook_changed_cb), &(ctx->attrs[j]));
					g_object_set_data(G_OBJECT(widget), RND_OBJ_PROP, ctx);
					j = rnd_gtk_attr_dlg_add(ctx, widget, &ts, j+1);
				}
				break;

			case RND_HATT_BEGIN_COMPOUND:
				j = rnd_gtk_attr_dlg_add(ctx, parent, NULL, j+1);
				break;

			case RND_HATT_LABEL:
				{
				static gtkc_event_xyz_t ev_click; /* shared among all labels, so udata is NULL */

				widget = gtkc_dad_label_new(&ctx->attrs[j]);

				ctx->wl[j] = widget;
				ctx->wltop[j] = wrap_bind_click(widget, rnd_gtkc_xy_ev(&ev_click, label_click_cb, NULL));

				g_object_set_data(G_OBJECT(widget), RND_OBJ_PROP, ctx);
				g_object_set_data(G_OBJECT(ctx->wltop[j]), RND_OBJ_PROP, ctx);
				g_object_set_data(G_OBJECT(ctx->wltop[j]), RND_OBJ_PROP_CLICK, &(ctx->attrs[j]));

				gtkc_box_pack_append(parent, ctx->wltop[j], FALSE, 0);
				gtk_widget_set_tooltip_text(widget, ctx->attrs[j].help_text);
				}
				break;

			case RND_HATT_INTEGER:
				ctx->wl[j] = gtk_label_new("ERROR: INTEGER entry");
				break;

			case RND_HATT_COORD:
				ctx->wl[j] = gtk_label_new("ERROR: COORD entry");
				break;

			case RND_HATT_STRING:
				ctx->wltop[j] = hbox = gtkc_hbox_new(FALSE, 4);
				gtkc_box_pack_append(parent, hbox, expfill, 0);

				entry = gtk_entry_new();
				gtkc_box_pack_append(hbox, entry, expfill, 0);
				g_object_set_data(G_OBJECT(entry), RND_OBJ_PROP, ctx);
				ctx->wl[j] = entry;

				if (ctx->attrs[j].val.str != NULL)
					gtkc_entry_set_text(GTK_ENTRY(entry), ctx->attrs[j].val.str);
				gtk_widget_set_tooltip_text(entry, ctx->attrs[j].help_text);
				g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(entry_changed_cb), &(ctx->attrs[j]));
				g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(entry_activate_cb), &(ctx->attrs[j]));

				if (ctx->attrs[j].hatt_flags & RND_HATF_HEIGHT_CHR)
					gtkc_entry_set_width_chars(GTK_ENTRY(entry), ctx->attrs[j].geo_width);

				break;

			case RND_HATT_BOOL:
				/* put this in a check button */
				widget = chk_btn_new(parent, ctx->attrs[j].val.lng, set_flag_cb, &(ctx->attrs[j]), NULL);
				gtk_widget_set_tooltip_text(widget, ctx->attrs[j].help_text);
				g_object_set_data(G_OBJECT(widget), RND_OBJ_PROP, ctx);
				ctx->wl[j] = widget;
				break;

			case RND_HATT_ENUM:
				ctx->wltop[j] = hbox = gtkc_hbox_new(FALSE, 4);
				gtkc_box_pack_append(parent, hbox, expfill, 0);

				combo = gtkc_combo_box_text_new();
				gtk_widget_set_tooltip_text(combo, ctx->attrs[j].help_text);
				gtkc_box_pack_append(hbox, combo, expfill, 0);
				g_object_set_data(G_OBJECT(combo), RND_OBJ_PROP, ctx);
				ctx->wl[j] = combo;

				/*
				 * Iterate through each value and add them to the
				 * combo box
				 */
				{
					const char **vals = ctx->attrs[j].wdata;
					for(i = 0; vals[i] != NULL; i++)
						gtkc_combo_box_text_append_text(combo, vals[i]);
				}
				gtk_combo_box_set_active(GTK_COMBO_BOX(combo), ctx->attrs[j].val.lng);
				g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(enum_changed_cb), &(ctx->attrs[j]));
				break;

			case RND_HATT_TREE:
				ctx->wl[j] = rnd_gtk_tree_table_create(ctx, &ctx->attrs[j], parent, j);
				break;

			case RND_HATT_PREVIEW:
				{
					rnd_gtk_preview_t *p;
					ctx->wl[j] = rnd_gtk_preview_create(ctx, &ctx->attrs[j], parent, j);
					p = (rnd_gtk_preview_t *)ctx->wl[j];
					p->flip_local = !!(ctx->attrs[j].rnd_hatt_flags & RND_HATF_PRV_LFLIP);
					p->flip_global = !!(ctx->attrs[j].rnd_hatt_flags & RND_HATF_PRV_GFLIP);
					p->view.local_flip = (p->flip_local && !p->flip_global);
					
				}
				break;

			case RND_HATT_TEXT:
				ctx->wl[j] = rnd_gtk_text_create(ctx, &ctx->attrs[j], parent);
				break;

			case RND_HATT_PICTURE:
				ctx->wl[j] = rnd_gtk_picture_create(ctx, &ctx->attrs[j], parent, j, label_click_cb, ctx);
				break;

			case RND_HATT_PICBUTTON:
				ctx->wl[j] = rnd_gtk_picbutton_create(ctx, &ctx->attrs[j], parent, j, (ctx->attrs[j].rnd_hatt_flags & RND_HATF_TOGGLE), expfill);
				g_signal_connect(G_OBJECT(ctx->wl[j]), "clicked", G_CALLBACK(button_changed_cb), &(ctx->attrs[j]));
				g_object_set_data(G_OBJECT(ctx->wl[j]), RND_OBJ_PROP, ctx);
				break;

			case RND_HATT_COLOR:
				ctx->wl[j] = rnd_gtk_color_create(ctx, &ctx->attrs[j], parent, j);
				g_signal_connect(G_OBJECT(ctx->wl[j]), "color_set", G_CALLBACK(color_changed_cb), &(ctx->attrs[j]));
				g_object_set_data(G_OBJECT(ctx->wl[j]), RND_OBJ_PROP, ctx);
				break;

			case RND_HATT_PROGRESS:
				ctx->wl[j] = rnd_gtk_progress_create(ctx, &ctx->attrs[j], parent, j);
				break;

			case RND_HATT_UNIT:
				ctx->wl[j] = gtk_label_new("ERROR: UNIT entry");
				break;

			case RND_HATT_BUTTON:
				hbox = gtkc_hbox_new(FALSE, 4);
				gtkc_box_pack_append(parent, hbox, expfill, 0);

				if (ctx->attrs[j].rnd_hatt_flags & RND_HATF_TOGGLE)
					ctx->wl[j] = gtk_toggle_button_new_with_label(ctx->attrs[j].val.str);
				else
					ctx->wl[j] = gtk_button_new_with_label(ctx->attrs[j].val.str);
				gtkc_box_pack_append(hbox, ctx->wl[j], expfill, 0);

				gtk_widget_set_tooltip_text(ctx->wl[j], ctx->attrs[j].help_text);
				g_signal_connect(G_OBJECT(ctx->wl[j]), "clicked", G_CALLBACK(button_changed_cb), &(ctx->attrs[j]));
				g_object_set_data(G_OBJECT(ctx->wl[j]), RND_OBJ_PROP, ctx);
				break;

			case RND_HATT_SUBDIALOG:
				{
					GtkWidget *subbox = gtkc_hbox_new(FALSE, 0);
					rnd_hid_dad_subdialog_t *sub = ctx->attrs[j].wdata;

					sub->dlg_hid_ctx = rnd_gtk_attr_sub_new(ctx->gctx, subbox, sub->dlg, sub->dlg_len, sub);
					ctx->wl[j] = subbox;
					gtkc_box_pack_append(parent, ctx->wl[j], FALSE, 0);
				}
				break;


			default:
				printf("rnd_gtk_attribute_dialog: unknown type of HID attribute\n");
				break;
		}
		if (ctx->wltop[j] == NULL)
			ctx->wltop[j] = ctx->wl[j];
		if (ctx->attrs[j].rnd_hatt_flags & RND_HATF_INIT_FOCUS)
			gtk_widget_grab_focus(ctx->wl[j]);
	}
	return j;
}

static int rnd_gtk_attr_dlg_set(attr_dlg_t *ctx, int idx, const rnd_hid_attr_val_t *val, int *copied)
{
	int ret, save;
	save = ctx->inhibit_valchg;
	ctx->inhibit_valchg = 1;
	*copied = 0;

	/* create the actual widget from attrs */
	switch (ctx->attrs[idx].type) {
		case RND_HATT_BEGIN_HBOX:
		case RND_HATT_BEGIN_VBOX:
		case RND_HATT_BEGIN_TABLE:
		case RND_HATT_PICBUTTON:
		case RND_HATT_PICTURE:
		case RND_HATT_COORD:
		case RND_HATT_REAL:
		case RND_HATT_INTEGER:
		case RND_HATT_BEGIN_COMPOUND:
		case RND_HATT_SUBDIALOG:
			goto error;

		case RND_HATT_END:
			{
				rnd_hid_compound_t *cmp = ctx->attrs[idx].wdata;
				if ((cmp != NULL) && (cmp->set_value != NULL))
					cmp->set_value(&ctx->attrs[idx], ctx, idx, val);
				else
					goto error;
			}
			break;

		case RND_HATT_TREE:
			ret = rnd_gtk_tree_table_set(ctx, idx, val);
			ctx->inhibit_valchg = save;
			return ret;

		case RND_HATT_PROGRESS:
			ret = rnd_gtk_progress_set(ctx, idx, val);
			ctx->inhibit_valchg = save;
			return ret;

		case RND_HATT_PREVIEW:
			ret = rnd_gtka_preview_set(ctx, idx, val);
			ctx->inhibit_valchg = save;
			return ret;

		case RND_HATT_TEXT:
			ret = rnd_gtk_text_set(ctx, idx, val);
			ctx->inhibit_valchg = save;
			return ret;

		case RND_HATT_COLOR:
			ret = rnd_gtk_color_set(ctx, idx, val);
			ctx->inhibit_valchg = save;
			return ret;

		case RND_HATT_BEGIN_HPANE:
		case RND_HATT_BEGIN_VPANE:
			ret = rnd_gtk_pane_set(ctx, idx, val);
			ctx->inhibit_valchg = save;
			return ret;

		case RND_HATT_LABEL:
			{
				const char *txt = gtk_label_get_text(GTK_LABEL(ctx->wl[idx]));
				if (strcmp(txt, val->str) == 0)
					goto nochg;
				gtk_label_set_text(GTK_LABEL(ctx->wl[idx]), val->str);
			}
			break;



		case RND_HATT_STRING:
			{
				const char *nv, *s = gtkc_entry_get_text(GTK_ENTRY(ctx->wl[idx]));
				nv = val->str;
				if (nv == NULL)
					nv = "";
				if (strcmp(s, nv) == 0)
					goto nochg;
				gtkc_entry_set_text(GTK_ENTRY(ctx->wl[idx]), val->str);
				ctx->attrs[idx].val.str = rnd_strdup(val->str);
				*copied = 1;
			}
			break;

		case RND_HATT_BOOL:
			{
				int chk = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ctx->wl[idx]));
				if (chk == val->lng)
					goto nochg;
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctx->wl[idx]), val->lng);
			}
			break;

		case RND_HATT_ENUM:
			{
				int en = gtk_combo_box_get_active(GTK_COMBO_BOX(ctx->wl[idx]));
				if (en == val->lng)
					goto nochg;
				gtk_combo_box_set_active(GTK_COMBO_BOX(ctx->wl[idx]), val->lng);
			}
			break;

		case RND_HATT_UNIT:
			abort();
			break;

		case RND_HATT_BUTTON:
			{
				const char *s = gtk_button_get_label(GTK_BUTTON(ctx->wl[idx]));
				if (strcmp(s, val->str) == 0)
					goto nochg;
				gtk_button_set_label(GTK_BUTTON(ctx->wl[idx]), val->str);
			}
			break;

		case RND_HATT_BEGIN_TABBED:
			gtk_notebook_set_current_page(GTK_NOTEBOOK(ctx->wl[idx]), val->lng);
			break;
	}

	ctx->inhibit_valchg = save;
	return 0;

	error:;
	ctx->inhibit_valchg = save;
	return -1;

	nochg:;
	ctx->inhibit_valchg = save;
	return 1;
}

static gint rnd_gtk_attr_dlg_configure_event_cb(GtkWidget *widget, long x, long y, long z, gpointer data)
{
	attr_dlg_t *ctx = (attr_dlg_t *)data;
	return rnd_gtk_winplace_cfg(ctx->gctx->hidlib, widget, ctx, ctx->id);
}

/* destroy the GUI widgets but do not free any hid_ctx struct */
static void rnd_gtk_attr_dlg_free_gui(attr_dlg_t *ctx)
{
	int i;

	/* make sure there are no nested rnd_gtk_attr_dlg_free() calls */
	if (ctx->freeing_gui)
		return;
	ctx->freeing_gui = 1;

	/* make sure we are not called again from the destroy signal */
	if (ctx->dialog != NULL)
		g_signal_handler_disconnect(ctx->dialog, ctx->destroy_handler);

	for(i = 0; i < ctx->n_attrs; i++) {
		switch(ctx->attrs[i].type) {
			case RND_HATT_TREE: rnd_gtk_tree_pre_free(ctx, &ctx->attrs[i], i); break;
			case RND_HATT_BUTTON: g_signal_handlers_block_by_func(G_OBJECT(ctx->wl[i]), G_CALLBACK(button_changed_cb), &(ctx->attrs[i])); break;
			case RND_HATT_PREVIEW: rnd_gtk_preview_del(ctx->gctx, RND_GTK_PREVIEW(ctx->wl[i]));
			default: break;
		}
	}

	if (!ctx->close_cb_called) {
		ctx->close_cb_called = 1;
		if (ctx->close_cb != NULL) {
			ctx->close_cb(ctx->caller_data, RND_HID_ATTR_EV_CODECLOSE);
			return; /* ctx is invalid now */
		}
	}
}


static gint rnd_gtk_attr_dlg_destroy_event_cb(GtkWidget *widget, long x, long y, long z, gpointer data)
{
	rnd_gtk_attr_dlg_free_gui(data);
	return 0;
}

/* Hide (or show) a widget by idx - no check performed on idx range */
static int rnd_gtk_attr_dlg_widget_hide_(attr_dlg_t *ctx, int idx, rnd_bool hide)
{
	GtkWidget *w;

	if (ctx->attrs[idx].type == RND_HATT_BEGIN_COMPOUND)
		return -1;

	if (ctx->attrs[idx].type == RND_HATT_END) {
		rnd_hid_compound_t *cmp = ctx->attrs[idx].wdata;
		if ((cmp != NULL) && (cmp->widget_hide != NULL))
			return cmp->widget_hide(&ctx->attrs[idx], ctx, idx, hide);
		else
			return -1;
	}

	w = (ctx->wltop[idx] != NULL) ? ctx->wltop[idx] : ctx->wl[idx];
	if (w == NULL)
		return -1;

	if (hide)
		gtk_widget_hide(w);
	else
		gtk_widget_show(w);

	return 0;
}

extern fgw_ctx_t rnd_fgw;

static int rnd_gtk_attr_dlg_widget_poke_string(void *hid_ctx, GtkWidget *w, int idx, int argc, fgw_arg_t argv[])
{
	if ((argv[0].type & FGW_STR) != FGW_STR) return -1;

	switch(argv[0].val.str[0]) {
		case 's': /* select */
			if ((argc < 3) || (fgw_arg_conv(&rnd_fgw, &argv[1], FGW_INT) != 0) || (fgw_arg_conv(&rnd_fgw, &argv[2], FGW_INT) != 0))
				return -1;
			gtk_editable_select_region(GTK_EDITABLE(w), argv[1].val.nat_int, argv[1].val.nat_int + argv[2].val.nat_int);
			return 0;
	}
	return -1;
}

int rnd_gtk_attr_dlg_widget_poke(void *hid_ctx, int idx, int argc, fgw_arg_t argv[])
{
	attr_dlg_t *ctx = (attr_dlg_t *)hid_ctx;
	GtkWidget *w;

	if ((idx < 0) || (idx >= ctx->n_attrs) || (argc < 1))
		return -1;

	w = ctx->wl[idx];
	switch(ctx->attrs[idx].type) {
		case RND_HATT_STRING: return rnd_gtk_attr_dlg_widget_poke_string(hid_ctx, w, idx, argc, argv);
		default: return -1;
	}
	return -1;
}


static void rnd_gtk_initial_wstates(attr_dlg_t *ctx)
{
	int n;
	for(n = 0; n < ctx->n_attrs; n++)
		if (ctx->attrs[n].rnd_hatt_flags & RND_HATF_HIDE)
			rnd_gtk_attr_dlg_widget_hide_(ctx, n, 1);
}

void *rnd_gtk_attr_dlg_new(rnd_gtk_t *gctx, const char *id, rnd_hid_attribute_t *attrs, int n_attrs, const char *title, void *caller_data, rnd_bool modal, void (*button_cb)(void *caller_data, rnd_hid_attr_ev_t ev), int defx, int defy, int minx, int miny)
{
	GtkWidget *main_vbox;
	attr_dlg_t *ctx;
	int plc[4] = {-1, -1, -1, -1};


	plc[2] = defx;
	plc[3] = defy;

	ctx = calloc(sizeof(attr_dlg_t), 1);

	ctx->gctx = gctx;
	ctx->attrs = attrs;
	ctx->n_attrs = n_attrs;
	ctx->wl = calloc(sizeof(GtkWidget *), n_attrs);
	ctx->wltop = calloc(sizeof(GtkWidget *), n_attrs);
	ctx->caller_data = caller_data;
	ctx->close_cb_called = 0;
	ctx->close_cb = button_cb;
	ctx->id = rnd_strdup(id);
	ctx->modal = modal;

	rnd_event(gctx->hidlib, RND_EVENT_DAD_NEW_DIALOG, "psp", ctx, ctx->id, plc);

	ctx->dialog = gtk_dialog_new();
	if ((modal && rnd_gtk_conf_hid.plugins.hid_gtk.dialog.transient_modal) || (!modal && rnd_gtk_conf_hid.plugins.hid_gtk.dialog.transient_modeless))
		gtk_window_set_transient_for(GTK_WINDOW(ctx->dialog), GTK_WINDOW(gctx->wtop_window));

	gtk_window_set_title(GTK_WINDOW(ctx->dialog), title);
	gtkc_window_set_role(GTK_WINDOW(ctx->dialog), id); /* optional hint for the window manager for figuring session placement/restore */
	gtk_window_set_modal(GTK_WINDOW(ctx->dialog), modal);

	if (rnd_conf.editor.auto_place) {
		if ((plc[2] > 0) && (plc[3] > 0))
			gtkc_window_resize(GTK_WINDOW(ctx->dialog), plc[2], plc[3]);
		if ((plc[0] >= 0) && (plc[1] >= 0))
			gtkc_window_move(GTK_WINDOW(ctx->dialog), plc[0], plc[1]);
	}
	else if ((defx > 0) && (defy > 0))
		gtkc_window_resize(GTK_WINDOW(ctx->dialog), defx, defy);

	gtk2c_bind_win_resize(ctx->dialog, rnd_gtkc_xy_ev(&ctx->ev_resize, rnd_gtk_attr_dlg_configure_event_cb, ctx));
	ctx->destroy_handler = gtkc_bind_win_destroy(ctx->dialog, rnd_gtkc_xy_ev(&ctx->ev_destroy, rnd_gtk_attr_dlg_destroy_event_cb, ctx));

	main_vbox = gtkc_vbox_new(FALSE, 6);
	gtkc_dlg_add_content(GTK_DIALOG(ctx->dialog), main_vbox);

	rnd_gtk_attr_dlg_add(ctx, main_vbox, NULL, 0);

	gtkc_widget_show_all(ctx->dialog);
	gtk4c_bind_win_resize(ctx->dialog, rnd_gtkc_xy_ev(&ctx->ev_resize, rnd_gtk_attr_dlg_configure_event_cb, ctx));

	rnd_gtk_initial_wstates(ctx);

	if (rnd_gtk_conf_hid.plugins.hid_gtk.dialog.auto_present)
		gtk_window_present(GTK_WINDOW(ctx->dialog));

	return ctx;
}

void *rnd_gtk_attr_sub_new(rnd_gtk_t *gctx, GtkWidget *parent_box, rnd_hid_attribute_t *attrs, int n_attrs, void *caller_data)
{
	attr_dlg_t *ctx;

	ctx = calloc(sizeof(attr_dlg_t), 1);

	ctx->gctx = gctx;
	ctx->attrs = attrs;
	ctx->n_attrs = n_attrs;
	ctx->wl = calloc(sizeof(GtkWidget *), n_attrs);
	ctx->wltop = calloc(sizeof(GtkWidget *), n_attrs);
	ctx->caller_data = caller_data;
	ctx->modal = 0;

	rnd_gtk_attr_dlg_add(ctx, parent_box, NULL, 0);

	gtkc_widget_show_all(parent_box);

	rnd_gtk_initial_wstates(ctx);

	return ctx;
}

int rnd_gtk_attr_dlg_run(void *hid_ctx)
{
	attr_dlg_t *ctx = hid_ctx;
	int modal = ctx->modal;
	GtkWidget *dialog = ctx->dialog;

	GtkResponseType res = gtkc_dialog_run(GTK_DIALOG(ctx->dialog), modal);

	/* NOTE: ctx may be invalid by now (user callback free'd it) */

	if (res == GTK_RESPONSE_NONE) {
		/* the close cb destroyed the window; real return value should be set by DAD ret override */
		return -42;
	}

	if (modal)
		gtkc_window_destroy(dialog);

	if (res == GTK_RESPONSE_OK)
		return 0;
	return -42; /* the close cb destroyed the window; real return value should be set by DAD ret override */
}

void rnd_gtk_attr_dlg_raise(void *hid_ctx)
{
	attr_dlg_t *ctx = hid_ctx;
	gtk_window_present(GTK_WINDOW(ctx->dialog));
}

void rnd_gtk_attr_dlg_close(void *hid_ctx)
{
	attr_dlg_t *ctx = hid_ctx;

	if (ctx->dialog != NULL) {
		GtkWidget *dlg = ctx->dialog; /* the destroy callback may free ctx */
		ctx->dialog = NULL;
		gtkc_window_destroy(dlg);
	}
}

void rnd_gtk_attr_dlg_free(void *hid_ctx)
{
	attr_dlg_t *ctx = hid_ctx;

	if (ctx->being_destroyed)
		return;
	ctx->being_destroyed = 1;

	if ((ctx->dialog != NULL) && (!ctx->freeing_gui)) {
		gtkc_window_destroy(ctx->dialog);
		while(!ctx->freeing_gui) /* wait for the destroy event to get delivered */
			while(gtk_events_pending())
				gtk_main_iteration_do(0);
	}

	free(ctx->id);
	free(ctx->wl);
	free(ctx->wltop);
	free(ctx);
}

void rnd_gtk_attr_dlg_property(void *hid_ctx, rnd_hat_property_t prop, const rnd_hid_attr_val_t *val)
{
	attr_dlg_t *ctx = hid_ctx;

	if ((prop >= 0) && (prop < RND_HATP_max))
		ctx->property[prop] = *val;
}

int rnd_gtk_attr_dlg_widget_state(void *hid_ctx, int idx, int enabled)
{
	attr_dlg_t *ctx = hid_ctx;
	if ((idx < 0) || (idx >= ctx->n_attrs) || (ctx->wl[idx] == NULL))
		return -1;

	if (ctx->attrs[idx].type == RND_HATT_BEGIN_COMPOUND)
		return -1;

	if (ctx->attrs[idx].type == RND_HATT_END) {
		rnd_hid_compound_t *cmp = ctx->attrs[idx].wdata;
		if ((cmp != NULL) && (cmp->widget_state != NULL))
			cmp->widget_state(&ctx->attrs[idx], ctx, idx, enabled);
		else
			return -1;
	}

	gtk_widget_set_sensitive(ctx->wl[idx], enabled);

	switch(ctx->attrs[idx].type) {
		case RND_HATT_LABEL:
			rnd_gtk_set_selected(ctx->wltop[idx], (enabled == 2));
			break;
		case RND_HATT_PICBUTTON:
		case RND_HATT_BUTTON:
			if (ctx->attrs[idx].rnd_hatt_flags & RND_HATF_TOGGLE)
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctx->wl[idx]), (enabled == 2));
			break;
		default:;
	}

	return 0;
}


int rnd_gtk_attr_dlg_widget_hide(void *hid_ctx, int idx, rnd_bool hide)
{
	attr_dlg_t *ctx = hid_ctx;

	if ((idx < 0) || (idx >= ctx->n_attrs))
		return -1;

	return rnd_gtk_attr_dlg_widget_hide_(ctx, idx, hide);
}

int rnd_gtk_attr_dlg_set_value(void *hid_ctx, int idx, const rnd_hid_attr_val_t *val)
{
	attr_dlg_t *ctx = hid_ctx;
	int res, copied;

	if ((idx < 0) || (idx >= ctx->n_attrs))
		return -1;

	res = rnd_gtk_attr_dlg_set(ctx, idx, val, &copied);

	if (res == 0) {
		if (!copied)
			ctx->attrs[idx].val = *val;
		return 0;
	}
	else if (res == 1)
		return 0; /* no change */

	return -1;
}

void rnd_gtk_attr_dlg_set_help(void *hid_ctx, int idx, const char *val)
{
	attr_dlg_t *ctx = hid_ctx;

	if ((idx < 0) || (idx >= ctx->n_attrs))
		return;

	gtk_widget_set_tooltip_text(ctx->wl[idx], val);
}

void rnd_gtk_dad_fixcolor(void *hid_ctx, const rnd_gtk_color_t *color)
{
	attr_dlg_t *ctx = hid_ctx;

	int n;
	for(n = 0; n < ctx->n_attrs; n++) {
		switch(ctx->attrs[n].type) {
			case RND_HATT_BUTTON:
			case RND_HATT_LABEL:
			case RND_HATT_PICTURE:
				gtkc_widget_modify_bg(ctx->wltop[n], GTK_STATE_NORMAL, color);
			default:;
		}
	}
}

int rnd_gtk_winplace_cfg(rnd_hidlib_t *hidlib, GtkWidget *widget, void *ctx, const char *id)
{
	GtkAllocation allocation;

	gtkc_widget_get_allocation(widget, &allocation);

	/* For whatever reason, get_allocation doesn't set these. Gtk. */
	gtk_window_get_position(GTK_WINDOW(widget), &allocation.x, &allocation.y);

	rnd_event(hidlib, RND_EVENT_DAD_NEW_GEO, "psiiii", ctx, id,
		(int)allocation.x, (int)allocation.y, (int)allocation.width, (int)allocation.height);

	return 0;
}

rnd_hidlib_t *rnd_gtk_attr_get_dad_hidlib(void *hid_ctx)
{
	attr_dlg_t *ctx = hid_ctx;
	return ctx->gctx->hidlib;
}
