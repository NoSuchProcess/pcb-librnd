/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2021 Tibor Palinkas
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <librnd/core/hidlib.h>
#include <genvector/vti0.h>

#include "compat.h"
#include <librnd/core/hid_cfg_action.h>
#include <librnd/hid/hid_menu.h>
#include <librnd/plugins/lib_gtk_common/rnd_gtk.h>
#include <librnd/plugins/lib_gtk_common/in_keyboard.h>
#include <librnd/plugins/lib_hid_common/menu_helper.h>

#include "bu_menu.h"

#include "bu_menu_model.c"

static void gtkci_menu_activate(rnd_gtk_menu_ctx_t *ctx, open_menu_t *om, GtkWidget *widget, lht_node_t *mnd, int is_main, int clicked);
static GtkWidget *gtkci_menu_open(rnd_gtk_menu_ctx_t *ctx, GtkWidget *widget, lht_node_t *nparent, lht_node_t *mnd, int is_main, int is_tearoff, int is_ctx_popup);
static void menu_close_subs(rnd_gtk_menu_ctx_t *ctx, lht_node_t *mnd);
static void gtkci_menu_build(rnd_gtk_menu_ctx_t *ctx, open_menu_t *om, lht_node_t *mnd);

#define RND_OM "RndOM"
#define HOVER_POPUP_DELAY_MS 500
#define gtk4_listrow_focus_bug 1

static void menu_set_label_insens(GtkWidget *l)
{
	gtkci_widget_css_add(l, "*.insens {\ncolor: #777777;\n}\n", "insens", 0);
}

static void maybe_set_chkbtn(GtkWidget *chk, int v)
{
	int curr = gtk_check_button_get_active(GTK_CHECK_BUTTON(chk));
	if (curr == v) return;
	gtk_check_button_set_active(GTK_CHECK_BUTTON(chk), v);
}

static void menu_item_update_chkbox(rnd_design_t *hidlib, lht_node_t *mnd, GtkWidget *real_row)
{
	int v;
	GtkWidget *w, *lab, *chk, *hbox = gtk_widget_get_first_child(real_row);
	const char *tf;

	if (!GTK_IS_BOX(hbox)) return;

	lab = chk = NULL;
	for(w = gtk_widget_get_first_child(hbox); w != NULL; w = gtk_widget_get_next_sibling(w)) {
		if ((chk == NULL) && (GTK_IS_CHECK_BUTTON(w)))
				chk = w;
		if ((lab == NULL) && (GTK_IS_LABEL(w)))
				lab = w;
		if ((chk != NULL) && (lab != NULL))
			break;
	}
	if (chk == NULL) return;

	tf = rnd_hid_cfg_menu_field_str(mnd, RND_MF_CHECKED);
	if (tf == NULL) return;

	v = rnd_hid_get_flag(hidlib, tf);
	if (v < 0) {
		maybe_set_chkbtn(chk, 0);
		if (lab != NULL)
			menu_set_label_insens(lab);
	}
	else
		maybe_set_chkbtn(chk, !!v);
}

static void menu_update_toggle_state(rnd_design_t *hidlib, open_menu_t *om)
{
	int n;
	GtkWidget *real_row;

	real_row = gtk_widget_get_first_child(om->lbox);
	real_row = gtk_widget_get_next_sibling(real_row);
	for(n = 1; n < om->mnd.used; n++, real_row = gtk_widget_get_next_sibling(real_row)) {
		open_menu_flag_t flg = om->flag.array[n];
		if (flg & OM_FLAG_CHKBOX)
			menu_item_update_chkbox(hidlib, om->mnd.array[n], real_row);
	}
}

void rnd_gtk_main_menu_update_toggle_state(rnd_design_t *hidlib, GtkWidget *menubar)
{
	open_menu_t *om;
	for(om = gdl_first(&open_menu); om != NULL; om = om->link.next)
		menu_update_toggle_state(hidlib, om);
}

/* Remove all widgets from om's listbox and rebuild them from lihata */
static void gtkc_menu_rebuild(rnd_gtk_menu_ctx_t *ctx, open_menu_t *om)
{
	GtkWidget *w, *next;
	lht_node_t *mnd = rnd_hid_cfg_menu_field(om->parent, RND_MF_SUBMENU, NULL);

	if (mnd == NULL)
		return;

	/* remove existing children */
	for(w = gtk_widget_get_first_child(om->lbox); w != NULL; w = next) {
		next = gtk_widget_get_next_sibling(w);
		gtk_list_box_remove(GTK_LIST_BOX(om->lbox), w);
	}
	om->mnd.used = 0;
	om->flag.used = 0;

	gtkci_menu_build(ctx, om, mnd);
}

/* Rebuild the open parent submenu popover/window of nd */
static int gtkc_menu_rebuild_parent(rnd_gtk_menu_ctx_t *ctx, lht_node_t *mnd)
{
	open_menu_t *om;
	lht_node_t *parent = mnd->parent->parent;

	if (!rnd_hid_cfg_has_submenus(parent))
		return 0;

	for(om = gdl_first(&open_menu); om != NULL; om = om->link.next)
		if (om->parent == parent)
			gtkc_menu_rebuild(ctx, om);

	return 0;
}

int rnd_gtk_create_menu_widget(void *ctx_, int is_popup, const char *name, int is_main, lht_node_t *parent, lht_node_t *ins_after, lht_node_t *menu_item)
{
	rnd_gtk_menu_ctx_t *ctx = ctx_;
	return gtkc_menu_rebuild_parent(ctx, menu_item);
}

int rnd_gtk_remove_menu_widget(void *ctx_, lht_node_t *nd)
{
	rnd_gtk_menu_ctx_t *ctx = &ghidgui->topwin.menu;
	open_menu_t *om;

	for(om = gdl_first(&open_menu); om != NULL; om = om->link.next) {
		if (om->parent == nd) {
			 /* this will also call gtkc_open_menu_del() from unmap */
			if (om->floating)
				gtk_window_destroy(GTK_WINDOW(om->popwin));
			else
				gtk_popover_popdown(GTK_POPOVER(om->popwin));
		}
	}

	if (nd->type != LHT_HASH)
		return 0;

	/* We are called before nd is actually removed from the lihata document;
	   mark nd so it is not created on menu rebuild */
	lht_dom_hash_put(nd, lht_dom_node_alloc(LHT_TEXT, "del"));

	return gtkc_menu_rebuild_parent(ctx, nd);
}

static GtkWidget *gtkci_menu_item_new(rnd_gtk_menu_ctx_t *ctx, const char *label, const char *accel_label, const char *update_on, const char *checked, int arrow, int sensitive, open_menu_flag_t *flag)
{
	GtkWidget *chk, *aw;
	GtkWidget *hbox = gtkc_hbox_new(FALSE, 5);
	GtkWidget *spring = gtkc_hbox_new(FALSE, 5);
	GtkWidget *l = gtk_label_new(label);
	GtkWidget *accel = gtk_label_new(accel_label);

	if ((update_on != NULL) || (checked != NULL)) {
		rnd_conf_native_t *nat = NULL;

		chk = gtk_check_button_new();
		gtk_widget_set_sensitive(chk, 0); /* gtk shouldn't toggle it on click; hbox click should be relayed to core and that should then sync all menu items */

		TODO("Code dup with gtk2: rnd_gtk_add_menu");
		if (update_on != NULL)
			nat = rnd_conf_get_field(update_on);
		else
			nat = rnd_conf_get_field(checked);

		if (nat != NULL) {
			static rnd_conf_hid_callbacks_t cbs;
			static int cbs_inited = 0;
			if (!cbs_inited) {
				memset(&cbs, 0, sizeof(rnd_conf_hid_callbacks_t));
				cbs.val_change_post = ctx->confchg_checkbox;
				cbs_inited = 1;
			}
			rnd_conf_hid_set_cb(nat, ctx->rnd_gtk_menuconf_id, &cbs);
		}
		else {
			if ((update_on == NULL) || (*update_on != '\0'))
				rnd_message(RND_MSG_WARNING, "Checkbox menu item %s not updated on any conf change - try to use the update_on field\n", checked);
		}

		*flag = OM_FLAG_CHKBOX;
	}
	else {
		chk = gtk_image_new_from_paintable(gdk_paintable_new_empty(64,64));
		*flag = 0;
	}

	if (!sensitive)
		menu_set_label_insens(l);

	gtkc_box_pack_append(hbox, chk, 0, 0);
	gtkc_box_pack_append(hbox, l, 0, 0);
	gtkc_box_pack_append(hbox, spring, 1, 0);
	gtkc_box_pack_append(hbox, accel, 0, 0);

	if (arrow) {
		aw = gtk_label_new(" > ");
		gtkc_box_pack_append(hbox, aw, 0, 0);
	}

	return hbox;
}


static GtkWidget *gtkci_menu_tear_new(int teared_off)
{
	GtkWidget *l;

	if (teared_off)
		l = gtk_label_new("<span alpha=\"25%\"> &lt;&lt;&lt; </span>");
	else
		l = gtk_label_new("<span alpha=\"25%\"> &gt;&gt;&gt; </span>");
	gtk_label_set_use_markup(GTK_LABEL(l), 1);
	return l;
}

static int menu_is_sensitive(lht_node_t *mnd)
{
	const char *senss = rnd_hid_cfg_menu_field_str(mnd, RND_MF_SENSITIVE);

	if ((senss != NULL) && (strcmp(senss, "false") == 0))
		return 0;

	return 1;
}

static void stop_hover_timer(rnd_gtk_menu_ctx_t *ctx)
{
	if (ctx->hover_timer != 0) {
		g_source_remove(ctx->hover_timer);
		ctx->hover_timer = 0;
	}
}

static gboolean hover_timer_cb(void *user_data)
{
	rnd_gtk_menu_ctx_t *ctx = user_data;
	ctx->hover_timer = 0;
	gtkci_menu_activate(ctx, NULL, ctx->hover_row, ctx->hover_mnd, 0, 0);
	return FALSE;  /* Turns timer off */
}

static gboolean sel_timer_cb(void *user_data)
{
	GtkWidget *real_row = user_data;
	GtkWidget *popwin = gtk_widget_get_parent(real_row);
/*printf("  select[%p] %p -> %p!\n", popwin, real_row, gtk_list_box_get_selected_row(GTK_LIST_BOX(popwin)));*/
	gtk_list_box_select_row(GTK_LIST_BOX(popwin), GTK_LIST_BOX_ROW(real_row));
	return FALSE;  /* Turns timer off */
}


static void menu_row_hover_cb(GtkEventControllerMotion *self, gdouble x, gdouble y, gpointer user_data)
{
	lht_node_t *old, *mnd = user_data;
	rnd_gtk_menu_ctx_t *ctx = mnd->doc->root->user_data;
	GtkWidget *row = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self)); /* row hbox */
	GtkWidget *real_row = gtk_widget_get_parent(row); /* GtkListBoxRow hosting the hbox */
	GtkWidget *popwin = gtk_widget_get_parent(real_row);
	open_menu_t *om = g_object_get_data(G_OBJECT(popwin), RND_OM);
/*	printf("hover: %f %f <%p>\n", x, y, row);*/

	assert(om != NULL);

	old = ctx->hover_mnd;

	stop_hover_timer(ctx);
	ctx->hover_mnd = mnd;
	ctx->hover_row = row;
	ctx->hover_timer = g_timeout_add(HOVER_POPUP_DELAY_MS, (GSourceFunc)hover_timer_cb, ctx);

/*printf("mnd=%p %s\n", mnd, mnd->name);*/

	if (mnd == old)
		return; /* don't trigger for staying with the same */

	if (!om->floating) {
		lht_node_t *parent = mnd->parent->parent;

		/* Gtk4 bug #6: if autohide is set, gtk_popover_show() will move the focus to
		   the next row automatically as a side effect. So if a middle item has
		   a submenu, the item is hover-selected, the submenu is open, then cusor
		   hovers up one item, the submenu is closed; but then popover automatically
		   selects the _next_ item, overriding our seleciton of the previous item.
		   It can not be turned off. Workaround: run our selection from a timer
		   after this builtin focus move */
		if (gtk4_listrow_focus_bug)
			g_timeout_add(10, (GSourceFunc)sel_timer_cb, real_row);

		if (rnd_hid_cfg_has_submenus(parent))
			menu_close_subs(ctx, parent);
	}
}

/*static void menu_row_sel_cb(GtkWidget *widget, GtkWidget *row, void *data)
{
	printf("selected[%p]: %p\n", widget, row);
}*/


static void menu_row_unhover_cb(GtkEventControllerMotion *self, gdouble x, gdouble y, gpointer user_data)
{
	lht_node_t *mnd = user_data;
	rnd_gtk_menu_ctx_t *ctx = mnd->doc->root->user_data;

	stop_hover_timer(ctx);
}

static int gtkci_menu_real_add_node(rnd_gtk_menu_ctx_t *ctx, GtkWidget *parent_w, GtkWidget *after_w, lht_node_t *mnd, open_menu_flag_t *flag)
{
	const char *text = (mnd->type == LHT_TEXT) ? mnd->data.text.value : mnd->name;
	GtkWidget *item, *ch;
	GtkListBoxRow *row;
	int after = -1;

	*flag = 0;

	if ((text != NULL) && (*text == '@'))
		return 1; /* skip creating anchors */

	/* figure where to insert by index (count widgets) */
	if (after_w != NULL) {
		int n;
		
		for(ch = gtk_widget_get_first_child(parent_w), n = 0; ch != NULL; ch = gtk_widget_get_next_sibling(ch), n++) {
			if (ch == after_w) {
				after = n;
				break;
			}
		}
	}

	if ((strcmp(text, "sep") == 0) || (strcmp(text, "-") == 0)) {
		if (after == -1) {
			/* need to know the number of items so that row can be queried */
			after = 0;
			for(ch = gtk_widget_get_first_child(parent_w); ch != NULL; ch = gtk_widget_get_next_sibling(ch))
				after++;
		}

		item = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_list_box_insert(GTK_LIST_BOX(parent_w), item, after);
		row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(parent_w), after);
		gtk_list_box_row_set_activatable(row, 0);
		gtk_list_box_row_set_selectable(row, 0);
	}
	else {
		const char *checked = rnd_hid_cfg_menu_field_str(mnd, RND_MF_CHECKED);
		const char *update_on = rnd_hid_cfg_menu_field_str(mnd, RND_MF_UPDATE_ON);
		const char *tip = rnd_hid_cfg_menu_field_str(mnd, RND_MF_TIP);
		const char *accel = "";
		GtkEventController *ctrl;
		lht_node_t *n_keydesc = rnd_hid_cfg_menu_field(mnd, RND_MF_ACCELERATOR, NULL);

		if (n_keydesc != NULL)
			accel = rnd_hid_cfg_keys_gen_accel(&rnd_gtk_keymap, n_keydesc, 1, NULL);

		item = gtkci_menu_item_new(ctx, text, accel, update_on, checked, rnd_hid_cfg_has_submenus(mnd), menu_is_sensitive(mnd), flag);
		if (tip != NULL)
			gtk_widget_set_tooltip_text(item, tip);
		gtk_list_box_insert(GTK_LIST_BOX(parent_w), item, after);

		if (*flag & OM_FLAG_CHKBOX)
			menu_item_update_chkbox(ctx->hidlib, mnd, gtk_widget_get_parent(item));

		ctrl = gtk_event_controller_motion_new();
		g_signal_connect(G_OBJECT(ctrl), "enter", G_CALLBACK(menu_row_hover_cb), mnd);
		g_signal_connect(G_OBJECT(ctrl), "leave", G_CALLBACK(menu_row_unhover_cb), mnd);
		gtk_widget_add_controller(item, ctrl);
	}

	return 0;
}

static void main_menu_popdown_all(rnd_gtk_menu_ctx_t *ctx)
{
	if (ctx->main_open_w != NULL) {
		gtk_popover_popdown(GTK_POPOVER(ctx->main_open_w));
		ctx->main_open_w = NULL;
		ctx->main_open_n = NULL;
	}
}

static void menu_row_click_cb(GtkWidget *widget, gpointer data)
{
	lht_node_t *mnd, **mnp;
	GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(widget));
	open_menu_t *om = g_object_get_data(G_OBJECT(widget), RND_OM);
	rnd_gtk_menu_ctx_t *ctx;
	int idx;

	if (om == NULL) {
		rnd_message(RND_MSG_ERROR, "gtk4 bu_menu internal error: om==NULL in menu_row_cb\n");
		return;
	}

	idx = gtk_list_box_row_get_index(row);
	if (idx == 0) { /* tearoff */
		ctx = om->mnd.array[0];
		if (om->floating) {
			gtk_window_destroy(GTK_WINDOW(om->popwin));
		}
		else {
			gtkci_menu_open(ctx, NULL, om->parent, rnd_hid_cfg_menu_field(om->parent, RND_MF_SUBMENU, NULL), 0, 1, 0);
			main_menu_popdown_all(ctx);
		}
		return;
	}

	mnp = (lht_node_t **)vtp0_get(&om->mnd, idx, 0);
	if ((mnp == NULL) || (*mnp == NULL)) {
		rnd_message(RND_MSG_ERROR, "gtk4 bu_menu internal error: mnp==NULL in menu_row_cb\n");
		return;
	}
	mnd = *mnp;

	ctx = mnd->doc->root->user_data;
/*	printf("Clicked menu %d: %s\n", idx, mnd->name);
	fflush(stdout);*/
	gtkci_menu_activate(ctx, om, GTK_WIDGET(row), mnd, 0, 1);
}

static gboolean menu_unparent_cb(void *user_data)
{
	gtk_widget_unparent(GTK_WIDGET(user_data));
	return FALSE; /* turn timer off */
}

static void menu_unmap(rnd_gtk_menu_ctx_t *ctx, GtkWidget *widget, int is_tearoff)
{
	open_menu_t *om = g_object_get_data(G_OBJECT(widget), RND_OM);

	/* if menu popover being destroyed is main menu top "currently open" one,
	   reset it so the menu system understands nothing is open */
	if (!is_tearoff && (ctx->main_open_w == widget)) {
		ctx->main_open_w = NULL;
		ctx->main_open_n = NULL;
	}

	stop_hover_timer(ctx);

	if (om != NULL) {
		g_object_set_data(G_OBJECT(om->lbox), RND_OM, NULL);
		g_object_set_data(G_OBJECT(widget), RND_OM, NULL);
		gtkc_open_menu_del(om);
	}
	else
		rnd_message(RND_MSG_ERROR, "gtk4 bu_menu internal error: om==NULL in menu_unmap_cb\n");


	if (!is_tearoff) {
		/* We need to unparent the popover to destroy it, else it's only hidden.
		   This looks like a GTK bug: if we unparent here immediately, other parts
		   of gtk will still try to reach fields of widget. Free it only after some
		   time. */
		g_timeout_add(1000, menu_unparent_cb, widget);
	}
}

static void menu_unmap_popover_cb(GtkWidget *widget, gpointer data)
{
	menu_unmap(data, widget, 0);
}

static void menu_unmap_tearoff_cb(GtkWidget *widget, gpointer data)
{
	menu_unmap(data, widget, 1);
}

static void gtkci_menu_build(rnd_gtk_menu_ctx_t *ctx, open_menu_t *om, lht_node_t *mnd)
{
	GtkWidget *item;
	lht_node_t *n, *mark;

	if (om->ctx_popup) {
		item = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_widget_hide(item);
	}
	else
		item = gtkci_menu_tear_new(om->floating);

	gtk_list_box_append(GTK_LIST_BOX(om->lbox), item);
	vtp0_append(&om->mnd, ctx);
	vti0_append(&om->flag, 0);

	for(n = mnd->data.list.first; n != NULL; n = n->next) {
		open_menu_flag_t flag;

		if (n->type == LHT_HASH) {
			mark = lht_dom_hash_get(n, "del");
			if (mark != NULL)
				continue; /* being deleted, don't create */
		}

		if (gtkci_menu_real_add_node(ctx, om->lbox, NULL, n, &flag) == 0) {
			vtp0_append(&om->mnd, n);
			vti0_append(&om->flag, flag);
		}
	}
}

static GtkWidget *gtkci_menu_open(rnd_gtk_menu_ctx_t *ctx, GtkWidget *widget, lht_node_t *nparent, lht_node_t *mnd, int is_main, int is_tearoff, int is_ctx_popup)
{
	open_menu_t *om;
	GtkWidget *popwin, *lbox;

	if (is_main) {
		main_menu_popdown_all(ctx);
		ctx->main_open_n = nparent;
	}

	popwin = is_tearoff ? gtk_dialog_new() : gtk_popover_new();
	lbox = gtk_list_box_new();

	om = gtkc_open_menu_new(nparent, popwin, lbox, is_tearoff, is_ctx_popup);
	g_object_set_data(G_OBJECT(lbox), RND_OM, om);

	gtkci_menu_build(ctx, om, mnd);

	g_object_set_data(G_OBJECT(popwin), RND_OM, om);
	g_signal_connect(om->lbox, "row-activated", G_CALLBACK(menu_row_click_cb), NULL);
/*	g_signal_connect(om->lbox, "row-selected", G_CALLBACK(menu_row_sel_cb), NULL);*/

	if (is_tearoff) {
		g_signal_connect(popwin, "unmap", G_CALLBACK(menu_unmap_tearoff_cb), ctx);
		gtk_window_set_title(GTK_WINDOW(popwin), nparent->name);
		gtk_window_set_transient_for(GTK_WINDOW(popwin), GTK_WINDOW(ghidgui->wtop_window));
		gtkc_dlg_add_content(GTK_DIALOG(popwin), lbox);
		gtk_window_present(GTK_WINDOW(popwin));
	}
	else {
		g_signal_connect(popwin, "unmap", G_CALLBACK(menu_unmap_popover_cb), ctx);
		gtk_popover_set_child(GTK_POPOVER(popwin), lbox);
		gtk_popover_set_autohide(GTK_POPOVER(popwin), 1);
	/*	gtk_popover_set_cascade_popdown(GTK_POPOVER(popwin), 1); -> can't pop down a child without also destroying parent, not good */
		gtk_popover_set_has_arrow(GTK_POPOVER(popwin), 0);

		if (widget != NULL) {
			gtk_popover_set_position(GTK_POPOVER(popwin), is_main ? GTK_POS_BOTTOM : GTK_POS_RIGHT);
			gtk_widget_set_parent(popwin, widget);
			gtk_popover_popup(GTK_POPOVER(popwin));
		}
		/* else let the caller pop it up */
	}

	if (is_main)
		ctx->main_open_w = popwin;

	return popwin;

}

static void menu_close_subs(rnd_gtk_menu_ctx_t *ctx, lht_node_t *mnd)
{
	open_menu_t *om, *next;

	restart:;
	for(om = gdl_first(&open_menu); om != NULL; om = next) {
		next = om->link.next;
		if (om->parent == NULL) continue;
/*		printf("open: %s (%s == %s) popov=%p om=%p\n", om->parent->name, om->parent->parent->parent->name, mnd->name, om->popov, om);*/
		if (!om->destroying && !om->floating && (om->parent->parent->parent == mnd)) {
/*			printf(" Close!\n");*/
			om->destroying = 1;
			gtk_popover_popdown(GTK_POPOVER(om->popwin)); /* this will also call gtkc_open_menu_del() from unmap */
			goto restart; /* popdown() may have free'd next and the list should be short anyway so rather restart the search */
		}
	}
}

/* Execute actions for a menu or open a submenu; om is often NULL */
static void gtkci_menu_activate(rnd_gtk_menu_ctx_t *ctx,open_menu_t *om, GtkWidget *widget, lht_node_t *mnd, int is_main, int clicked)
{
	if (!menu_is_sensitive(mnd)) {
/*		printf("insensitive\n");*/
		return;
	}

	if (!is_main && gtk4_listrow_focus_bug) {  /* see: gtk4 bug #6 */
		GtkWidget *real_row = widget;

		if (!GTK_IS_LIST_BOX_ROW(real_row))
			real_row = gtk_widget_get_parent(real_row); /* GtkListBoxRow hosting the hbox that's passed */
		assert(GTK_IS_LIST_BOX_ROW(real_row));

		g_timeout_add(10, (GSourceFunc)sel_timer_cb, real_row);
	}

	if (rnd_hid_cfg_has_submenus(mnd)) {
		lht_node_t *parent = mnd->parent->parent;
		if (rnd_hid_cfg_has_submenus(parent))
			menu_close_subs(ctx, parent);
		gtkci_menu_open(ctx, widget, mnd, rnd_hid_cfg_menu_field(mnd, RND_MF_SUBMENU, NULL), is_main, 0, 0);
		return;
	}

	if (clicked) {
		lht_node_t *n_action = rnd_hid_cfg_menu_field(mnd, RND_MF_ACTION, NULL);

		if (om != NULL) {
			/* gtk4 bug: the GUI may lock up if the popover is not popped down before
			   executing the action; this was the case with pcb-rnd, right click on
			   layer name, insert new layer before...
			   (This will also call gtkc_open_menu_del() from unmap) */
			if (om->floating)
				gtk_window_destroy(GTK_WINDOW(om->popwin));
			else
				gtk_popover_popdown(GTK_POPOVER(om->popwin)); /* This should be gtk_widget_unparent() but that locks up the GUI in the above case */
		}

		main_menu_popdown_all(ctx);
		rnd_hid_cfg_action(ghidgui->hidlib, n_action);
	}
}

static void enter_main_menu_cb(GtkEventController *controller, double x, double y, gpointer data)
{
	lht_node_t *mm = data;
	rnd_gtk_menu_ctx_t *ctx = mm->doc->root->user_data;

	/* If there is an open active menu from the menu bar and another menu bar
	   button is hovered, switch to that menu (opening it without a click) */
	if ((ctx->main_open_w != NULL) && (ctx->main_open_n != mm)) {
		GtkWidget *widget = gtk_event_controller_get_widget(controller);
		gtkci_menu_activate(ctx, NULL, widget, mm, 1, 1);
	}
}

static void open_main_menu_cb(GtkWidget *widget, gpointer data)
{
	lht_node_t *mm = data;
	rnd_gtk_menu_ctx_t *ctx = mm->doc->root->user_data;

	gtkci_menu_activate(ctx, NULL, widget, mm, 1, 1);
}

static void rnd_gtk_main_menu_add_node(rnd_gtk_menu_ctx_t *ctx, GtkWidget *menu_bar, const lht_node_t *base)
{
	lht_node_t *n;

	if (base->type != LHT_LIST) {
		rnd_hid_cfg_error(base, "Menu description shall be a list (li)\n");
		abort();
	}

	/* create main menu buttons */
	for(n = base->data.list.first; n != NULL; n = n->next) {
		GtkEventController *ctrl;
		GtkWidget *btn;
		
		btn = gtk_button_new_with_label(n->name);
		gtkc_box_pack_append(menu_bar, btn, FALSE, 0);
		g_signal_connect(btn, "clicked", G_CALLBACK(open_main_menu_cb), n);
		gtkci_widget_css_add(btn, "*.menubtn {\nborder: 0px; padding: 2px 6px 2px 6px;\n}\n", "menubtn", 0);

		ctrl = gtk_event_controller_motion_new();
		g_signal_connect(ctrl, "enter", G_CALLBACK(enter_main_menu_cb), n);
		gtk_widget_add_controller(GTK_WIDGET(btn), ctrl);
	}
}

static void gtkci_menu_install_hotkeys(rnd_gtk_menu_ctx_t *menu, const lht_node_t *mnd)
{
	lht_node_t *n;
	for(n = mnd->data.list.first; n != NULL; n = n->next) {
		lht_node_t *n_keydesc = rnd_hid_cfg_menu_field(n, RND_MF_ACCELERATOR, NULL);

		if (n_keydesc != NULL) {
			lht_node_t *n_action = rnd_hid_cfg_menu_field(n, RND_MF_ACTION, NULL);
			rnd_hid_cfg_keys_add_by_desc(&rnd_gtk_keymap, n_keydesc, n_action);
		}

		if (rnd_hid_cfg_has_submenus(n))
			gtkci_menu_install_hotkeys(menu, rnd_hid_cfg_menu_field(n, RND_MF_SUBMENU, NULL));
	}
}

GtkWidget *rnd_gtk_load_menus(rnd_gtk_menu_ctx_t *menu, rnd_design_t *hidlib)
{
	const lht_node_t *mr;
	GtkWidget *menu_bar = NULL;

	menu->hidlib = hidlib;

	rnd_hid_menu_gui_ready_to_create(rnd_gui);

	mr = rnd_hid_cfg_get_menu(rnd_gui->menu, "/main_menu");
	if (mr != NULL) {
		menu_bar = gtkc_hbox_new(0, 0);
		rnd_gtk_main_menu_add_node(menu, menu_bar, mr);
		mr->doc->root->user_data = menu;
		gtk_widget_show(menu_bar);
		gtkci_menu_install_hotkeys(menu, mr);
	}

	mr = rnd_hid_cfg_get_menu(rnd_gui->menu, "/popups");
	if (mr != NULL) {
		/* no need to create menus in advance */
		if (mr->type != LHT_LIST)
			rnd_hid_cfg_error(mr, "/popups should be a list\n");
		mr->doc->root->user_data = menu;
		gtkci_menu_install_hotkeys(menu, mr);
	}

	mr = rnd_hid_cfg_get_menu(rnd_gui->menu, "/mouse");
	if (rnd_hid_cfg_mouse_init(rnd_gui->menu, &rnd_gtk_mouse) != 0)
		rnd_message(RND_MSG_ERROR, "Error: failed to load mouse actions from the hid config lihata - mouse input will not work.");

	rnd_hid_menu_gui_ready_to_modify(rnd_gui);

	return menu_bar;
}

lht_node_t *rnd_gtk_menu_popup_pre(lht_node_t *node)
{
	return node;
}

extern GtkWidget *gtkc_event_widget;
extern double gtkc_event_x, gtkc_event_y;

void gtkc_menu_popup(void *gctx_, lht_node_t *mnd)
{
	rnd_gtk_t *gctx = gctx_;
	GtkWidget *p, *evw = gtkc_event_widget;
	GdkRectangle rect = {0};

	if (evw != NULL) {
		rect.x = gtkc_event_x;
		rect.y = gtkc_event_y;
	}
	else
		evw = gctx->topwin.drawing_area;

	p = gtkci_menu_open(&gctx->topwin.menu, NULL, mnd, rnd_hid_cfg_menu_field(mnd, RND_MF_SUBMENU, NULL), 0, 0, 1);
	gtk_widget_set_parent(p, gtkc_event_widget); TODO("should be: gctx->topwin.drawing_area to avoid a crash with newer gtk4?");
	gtk_popover_set_pointing_to(GTK_POPOVER(p), &rect);
	gtk_popover_set_position(GTK_POPOVER(p), GTK_POS_RIGHT);
	gtk_popover_set_has_arrow(GTK_POPOVER(p), 1);
	gtk_popover_popup(GTK_POPOVER(p));
}

