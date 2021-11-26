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

static void rnd_gtk_main_menu_add_node(rnd_gtk_menu_ctx_t *ctx, GtkWidget *menu_bar, const lht_node_t *base)
{
	lht_node_t *n;

	if (base->type != LHT_LIST) {
		rnd_hid_cfg_error(base, "Menu description shall be a list (li)\n");
		abort();
	}

	/* create main menu buttons */
	for(n = base->data.list.first; n != NULL; n = n->next) {
		GtkWidget *btn = gtk_button_new_with_label(n->name);
		gtkc_box_pack_append(menu_bar, btn, FALSE, 0);
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

