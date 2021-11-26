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

static GtkWidget *gtkci_menu_item_new(const char *label, const char *accel_label, int check, int arrow)
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

	gtkc_box_pack_append(hbox, chk, 0, 0);
	gtkc_box_pack_append(hbox, l, 0, 0);
	gtkc_box_pack_append(hbox, spring, 1, 0);
	gtkc_box_pack_append(hbox, accel, 0, 0);

	if (arrow)
		aw = gtk_label_new(" > ");
	else
		aw = gtk_label_new("     ");

	gtkc_box_pack_append(hbox, aw, 0, 0);

	return hbox;
}


static GtkWidget *gtkci_menu_tear_new(void)
{
	GtkWidget *l = gtk_label_new("<span alpha=\"25%\"> &gt;&gt;&gt; </span>");
	gtk_label_set_use_markup(GTK_LABEL(l), 1);
	return l;
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
		const char *label = rnd_hid_cfg_menu_field_str(mnd, RND_MF_SENSITIVE);
		const char *tip = rnd_hid_cfg_menu_field_str(mnd, RND_MF_TIP);
		const char *accel = "";
		lht_node_t *n_keydesc = rnd_hid_cfg_menu_field(mnd, RND_MF_ACCELERATOR, NULL);

		if (n_keydesc != NULL)
			accel = rnd_hid_cfg_keys_gen_accel(&rnd_gtk_keymap, n_keydesc, 1, NULL);

		item = gtkci_menu_item_new(text, accel, 1, 0);
		gtk_list_box_insert(GTK_LIST_BOX(parent_w), item, after);
	}
}

static void menu_row_cb(GtkWidget *widget, gpointer data)
{
	GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(widget));

	printf("Clicked menu %d\n", gtk_list_box_row_get_index(row));
	fflush(stdout);
}


static void gtkci_menu_open(rnd_gtk_menu_ctx_t *ctx, GtkWidget *widget, lht_node_t *nparent, lht_node_t *mnd, int is_main)
{
	GtkWidget *popow, *vbox, *item;
	GtkListBoxRow *row;
	lht_node_t *n;

	if (is_main) {
		if (ctx->main_open_w != NULL) {
			gtk_popover_popdown(ctx->main_open_w);
			ctx->main_open_w = NULL;
		}
		ctx->main_open_n = nparent;
	}


	vbox = gtk_list_box_new();

	item = gtkci_menu_tear_new();
	gtk_list_box_append(GTK_LIST_BOX(vbox), item);

	for(n = mnd->data.list.first; n != NULL; n = n->next)
		gtkci_menu_real_add_node(ctx, vbox, NULL, n);

	g_signal_connect(vbox, "row-activated", G_CALLBACK(menu_row_cb), NULL);

	popow = gtk_popover_new();
	gtk_popover_set_position(GTK_POPOVER(popow), GTK_POS_BOTTOM);
	gtk_widget_set_parent(popow, widget);
	gtk_popover_set_child(GTK_POPOVER(popow), vbox);
	gtk_popover_set_autohide(GTK_POPOVER(popow), 1);
	gtk_popover_set_cascade_popdown(GTK_POPOVER(popow), 1);
	gtk_popover_set_has_arrow(GTK_POPOVER(popow), 0);
	g_signal_connect(popow, "unmap", G_CALLBACK(gtk_widget_unparent), NULL);
	gtk_popover_popup(GTK_POPOVER(popow));

	if (is_main)
		ctx->main_open_w = popow;
}

static void gtkci_menu_activate(rnd_gtk_menu_ctx_t *ctx, GtkWidget *widget, lht_node_t *mnd, int is_main)
{
	if (rnd_hid_cfg_has_submenus(mnd)) {
		gtkci_menu_open(ctx, widget, mnd, rnd_hid_cfg_menu_field(mnd, RND_MF_SUBMENU, NULL), is_main);
		return;
	}
	printf("Activate menu!\n");
}

static void enter_main_menu_cb(GtkEventController *controller, double x, double y, gpointer data)
{
	lht_node_t *mm = data;
	rnd_gtk_menu_ctx_t *ctx = mm->doc->root->user_data;

	/* If there is an open active menu from the menu bar and another menu bar
	   button is hovered, switch to that menu (opening it without a click) */
	if ((ctx->main_open_w != NULL) && (ctx->main_open_n != mm)) {
		GtkWidget *widget = gtk_event_controller_get_widget(controller);
		gtkci_menu_activate(ctx, widget, mm, 1);
	}
}

static void open_main_menu_cb(GtkWidget *widget, gpointer data)
{
	lht_node_t *mm = data;
	rnd_gtk_menu_ctx_t *ctx = mm->doc->root->user_data;

	gtkci_menu_activate(ctx, widget, mm, 1);
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
		gtk_event_controller_set_propagation_limit(ctrl, GTK_LIMIT_NONE);
		g_signal_connect(ctrl, "enter", G_CALLBACK(enter_main_menu_cb), n);
		gtk_widget_add_controller(GTK_WIDGET(btn), ctrl);
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
	}

	mr = rnd_hid_cfg_get_menu(rnd_gui->menu, "/popups");
	if (mr != NULL) {
		/* no need to create menus in advance */
		if (mr->type != LHT_LIST)
			rnd_hid_cfg_error(mr, "/popups should be a list\n");
		mr->doc->root->user_data = menu;
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

