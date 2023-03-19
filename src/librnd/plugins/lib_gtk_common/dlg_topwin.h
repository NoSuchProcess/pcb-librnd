#ifndef RND_GTK_TOPWIN_H
#define RND_GTK_TOPWIN_H

/* OPTIONAL top window implementation */

#include <gtk/gtk.h>

#include <librnd/core/hid_cfg.h>
#include <librnd/hid/hid_dad.h>

#include "rnd_gtk.h"
#include "bu_command.h"

void rnd_gtk_update_toggle_flags(rnd_design_t *hidlib, rnd_gtk_topwin_t *tw, const char *cookie);
void rnd_gtk_create_topwin_widgets(rnd_gtk_t *ctx, rnd_gtk_topwin_t *tw, GtkWidget *in_top_window);
void rnd_gtk_fullscreen_apply(rnd_gtk_topwin_t *tw);
void rnd_gtk_tw_layer_vis_update(rnd_gtk_topwin_t *tw);

void rnd_gtk_tw_interface_set_sensitive(rnd_gtk_topwin_t *tw, gboolean sensitive);

int rnd_gtk_tw_dock_enter(rnd_gtk_topwin_t *tw, rnd_hid_dad_subdialog_t *sub, rnd_hid_dock_t where, const char *id);
void rnd_gtk_tw_dock_leave(rnd_gtk_topwin_t *tw, rnd_hid_dad_subdialog_t *sub);

/* set new_dsg as current hidlib for each docked subdialog of topwin */
void rnd_gtk_tw_update_dock_hidlib(rnd_gtk_topwin_t *tw, rnd_design_t *new_dsg);


void rnd_gtk_tw_set_title(rnd_gtk_topwin_t *tw, const char *title);

gboolean rnd_gtk_idle_cb(void *topwin);
gboolean rnd_gtk_key_release_cb(GtkWidget *drawing_area, long mods, long key_raw, long kv, void *topwin);

void rnd_gtk_tw_dock_uninit(void);

#endif

