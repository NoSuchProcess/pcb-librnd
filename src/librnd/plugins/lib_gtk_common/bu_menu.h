#ifndef RND_GTK_MAIN_MENU_H__
#define RND_GTK_MAIN_MENU_H__

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

void rnd_gtk_main_menu_update_toggle_state(rnd_hidlib_t *hidlib, GtkWidget *menubar, void (*cb)(rnd_hidlib_t *hidlib, GtkAction *, const char *toggle_flag, const char *active_flag));

int rnd_gtk_remove_menu_widget(void *ctx, lht_node_t *nd);
int rnd_gtk_create_menu_widget(void *ctx_, int is_popup, const char *name, int is_main, lht_node_t *parent, lht_node_t *ins_after, lht_node_t *menu_item);

void menu_toggle_update_cb(rnd_hidlib_t *hidlib, GtkAction *act, const char *tflag, const char *aflag);

GtkWidget *rnd_gtk_load_menus(rnd_gtk_menu_ctx_t *menu, rnd_hidlib_t *hidlib);

GtkWidget *rnd_gtk_menu_widget(lht_node_t *node);

#endif
