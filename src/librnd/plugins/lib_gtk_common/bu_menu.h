#ifndef RND_GTK_BU_MENU_H
#define RND_GTK_BU_MENU_H

#include <gtk/gtk.h>


#include <librnd/core/conf.h>
#include <librnd/core/hid_cfg.h>
#include <librnd/core/hid_cfg_input.h>
#include <librnd/core/conf_hid.h>

typedef struct rnd_gtk_menu_ctx_s {
	GtkWidget *menu_bar;
	rnd_conf_hid_id_t rnd_gtk_menuconf_id;
	void (*confchg_checkbox)(rnd_conf_native_t *cfg, int arr_idx);
	rnd_hidlib_t *hidlib;
} rnd_gtk_menu_ctx_t;

/* Updates the toggle/active state of all items: loops through all actions,
   passing the action (checkboxes) to update them to match current core state */
void rnd_gtk_main_menu_update_toggle_state(rnd_hidlib_t *hidlib, GtkWidget *menubar);

/* Menu widget create/remove for main menu, popup or submenu as descending the path */
int rnd_gtk_create_menu_widget(void *ctx_, int is_popup, const char *name, int is_main, lht_node_t *parent, lht_node_t *ins_after, lht_node_t *menu_item);
int rnd_gtk_remove_menu_widget(void *ctx, lht_node_t *nd);

/* Load and build all menus (main menu and popups) from the rnd_gui->menu */
GtkWidget *rnd_gtk_load_menus(rnd_gtk_menu_ctx_t *menu, rnd_hidlib_t *hidlib);

/* Return the gtk widget (if already created, else NULL) for a menu node */
GtkWidget *rnd_gtk_menu_widget(lht_node_t *node);

#endif
