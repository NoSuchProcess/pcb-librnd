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
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <librnd/core/hidlib.h>

#include "compat.h"
#include <librnd/plugins/lib_gtk_common/rnd_gtk.h>
#include <librnd/plugins/lib_gtk_common/in_keyboard.h>
#include <librnd/plugins/lib_hid_common/menu_helper.h>

#include "bu_menu.h"

#include "bu_menu_model.c"

static void gtkci_menu_activate(rnd_gtk_menu_ctx_t *ctx, GtkWidget *widget, lht_node_t *mnd, int is_main, int clicked);
static void gtkci_menu_open(rnd_gtk_menu_ctx_t *ctx, GtkWidget *widget, lht_node_t *nparent, lht_node_t *mnd, int is_main, int is_tearoff);
static void menu_close_subs(rnd_gtk_menu_ctx_t *ctx, lht_node_t *mnd);

#define RND_OM "RndOM"
#define HOVER_POPUP_DELAY_MS 500

void rnd_gtk_main_menu_update_toggle_state(rnd_hidlib_t *hidlib, GtkWidget *menubar)
{

}

int rnd_gtk_create_menu_widget(void *ctx_, int is_popup, const char *name, int is_main, lht_node_t *parent, lht_node_t *ins_after, lht_node_t *menu_item)
{
	return -1;
}

int rnd_gtk_remove_menu_widget(void *ctx, lht_node_t *nd)
{
	return -1;
}

static GtkWidget *gtkci_menu_item_new(const char *label, const char *accel_label, int check, int arrow, int sensitive)
{
	GtkWidget *chk, *aw;
	GtkWidget *hbox = gtkc_hbox_new(FALSE, 5);
	GtkWidget *spring = gtkc_hbox_new(FALSE, 5);
	GtkWidget *l = gtk_label_new(label);
	GtkWidget *accel = gtk_label_new(accel_label);

	if (!check) {
		chk = gtk_image_new_from_paintable(gdk_paintable_new_empty(64,64));
	}
	else
		chk = gtk_check_button_new();

	if (!sensitive)
		gtkci_widget_css_add(l, "*.insens {\ncolor: #777777;\n}\n", "insens");

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
	gtkci_menu_activate(ctx, ctx->hover_row, ctx->hover_mnd, 0, 0);
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

		/* Gtk4 bug: if autohide is set, gtk_popover_show() will move the focus to
		   the next row automatically as a side effect. So if a middle item has
		   a submenu, the item is hover-selected, the submenu is open, then cusor
		   hovers up one item, the submenu is closed; but then popover automatically
		   selects the _next_ item, overriding our seleciton of the previous item.
		   It can not be turned off. Workaround: run our selection from a timer
		   after this builtin focus move */
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

static void gtkci_menu_real_add_node(rnd_gtk_menu_ctx_t *ctx, GtkWidget *parent_w, GtkWidget *after_w, lht_node_t *mnd)
{
	const char *text = (mnd->type == LHT_TEXT) ? mnd->data.text.value : mnd->name;
	GtkWidget *item, *ch;
	GtkListBoxRow *row;
	int after = -1;

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

		item = gtkci_menu_item_new(text, accel, (checked != NULL), rnd_hid_cfg_has_submenus(mnd), menu_is_sensitive(mnd));
		if (tip != NULL)
			gtk_widget_set_tooltip_text(item, tip);
		gtk_list_box_insert(GTK_LIST_BOX(parent_w), item, after);

		ctrl = gtk_event_controller_motion_new();
		g_signal_connect(G_OBJECT(ctrl), "enter", G_CALLBACK(menu_row_hover_cb), mnd);
		g_signal_connect(G_OBJECT(ctrl), "leave", G_CALLBACK(menu_row_unhover_cb), mnd);
		gtk_widget_add_controller(item, ctrl);
	}
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
			gtkci_menu_open(ctx, NULL, om->parent, rnd_hid_cfg_menu_field(om->parent, RND_MF_SUBMENU, NULL), 0, 1);
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
	printf("Clicked menu %d: %s\n", idx, mnd->name);
	fflush(stdout);
	gtkci_menu_activate(ctx, row, mnd, 0, 1);
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

static void gtkci_menu_open(rnd_gtk_menu_ctx_t *ctx, GtkWidget *widget, lht_node_t *nparent, lht_node_t *mnd, int is_main, int is_tearoff)
{
	GtkWidget *popwin, *lbox, *item;
	GtkListBoxRow *row;
	lht_node_t *n;
	open_menu_t *om;

	if (is_main) {
		main_menu_popdown_all(ctx);
		ctx->main_open_n = nparent;
	}

	popwin = is_tearoff ? gtk_dialog_new() : gtk_popover_new();
	lbox = gtk_list_box_new();

	om = gtkc_open_menu_new(nparent, popwin, lbox, is_tearoff);
	g_object_set_data(G_OBJECT(lbox), RND_OM, om);

	item = gtkci_menu_tear_new(is_tearoff);
	gtk_list_box_append(GTK_LIST_BOX(lbox), item);
	vtp0_append(&om->mnd, ctx);

	for(n = mnd->data.list.first; n != NULL; n = n->next) {
		gtkci_menu_real_add_node(ctx, lbox, NULL, n);
		vtp0_append(&om->mnd, n);
	}

	g_signal_connect(lbox, "row-activated", G_CALLBACK(menu_row_click_cb), NULL);
/*	g_signal_connect(lbox, "row-selected", G_CALLBACK(menu_row_sel_cb), NULL);*/

	g_object_set_data(G_OBJECT(popwin), RND_OM, om);

	if (is_tearoff) {
		g_signal_connect(popwin, "unmap", G_CALLBACK(menu_unmap_tearoff_cb), ctx);
		gtk_window_set_title(GTK_WINDOW(popwin), nparent->name);
		gtk_window_set_transient_for(GTK_WINDOW(popwin), GTK_WINDOW(ghidgui->wtop_window));
		gtkc_dlg_add_content(GTK_DIALOG(popwin), lbox);
		gtk_window_present(GTK_WINDOW(popwin));
	}
	else {
		g_signal_connect(popwin, "unmap", G_CALLBACK(menu_unmap_popover_cb), ctx);
		gtk_popover_set_position(GTK_POPOVER(popwin), is_main ? GTK_POS_BOTTOM : GTK_POS_RIGHT);
		gtk_widget_set_parent(popwin, widget);
		gtk_popover_set_child(GTK_POPOVER(popwin), lbox);
		gtk_popover_set_autohide(GTK_POPOVER(popwin), 1);
	/*	gtk_popover_set_cascade_popdown(GTK_POPOVER(popwin), 1); -> can't pop down a child without also destroying parent, not good */
		gtk_popover_set_has_arrow(GTK_POPOVER(popwin), 0);
		gtk_popover_popup(GTK_POPOVER(popwin));
	}

	if (is_main)
		ctx->main_open_w = popwin;
}

static void menu_close_subs(rnd_gtk_menu_ctx_t *ctx, lht_node_t *mnd)
{
	open_menu_t *om, *next;

	for(om = gdl_first(&open_menu); om != NULL; om = next) {
		next = om->link.next;
		if (om->parent == NULL) continue;
/*		printf("open: %s (%s == %s) popov=%p om=%p\n", om->parent->name, om->parent->parent->parent->name, mnd->name, om->popov, om);*/
		if (!om->floating && (om->parent->parent->parent == mnd)) {
/*			printf(" Close!\n");*/
			gtk_popover_popdown(GTK_POPOVER(om->popwin)); /* this will also call gtkc_open_menu_del() from unmap */
		}
	}
}

static void gtkci_menu_activate(rnd_gtk_menu_ctx_t *ctx, GtkWidget *widget, lht_node_t *mnd, int is_main, int clicked)
{
	if (!menu_is_sensitive(mnd)) {
		printf("insensitive\n");
		return;
	}

	if (rnd_hid_cfg_has_submenus(mnd)) {
		lht_node_t *parent = mnd->parent->parent;
		if (rnd_hid_cfg_has_submenus(parent))
			menu_close_subs(ctx, parent);
		gtkci_menu_open(ctx, widget, mnd, rnd_hid_cfg_menu_field(mnd, RND_MF_SUBMENU, NULL), is_main, 0);
		return;
	}

	if (clicked) {
		lht_node_t *n_action = rnd_hid_cfg_menu_field(mnd, RND_MF_ACTION, NULL);
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
		gtkci_menu_activate(ctx, widget, mm, 1, 1);
	}
}

static void open_main_menu_cb(GtkWidget *widget, gpointer data)
{
	lht_node_t *mm = data;
	rnd_gtk_menu_ctx_t *ctx = mm->doc->root->user_data;

	gtkci_menu_activate(ctx, widget, mm, 1, 1);
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

GtkWidget *rnd_gtk_load_menus(rnd_gtk_menu_ctx_t *menu, rnd_hidlib_t *hidlib)
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

GtkWidget *rnd_gtk_menu_widget(lht_node_t *node)
{
	return NULL;
}

void gtkc_menu_popup(void *gctx, GtkWidget *menu)
{
}

