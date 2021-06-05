#ifndef RND_GTK_TOPWIN_H
#define RND_GTK_TOPWIN_H

/* OPTIONAL top window implementation */

#include <gtk/gtk.h>

#include <librnd/core/hid_cfg.h>
#include <librnd/core/hid_dad.h>

#include "rnd_gtk.h"
#include "bu_menu.h"
#include "bu_command.h"

void ghid_update_toggle_flags(rnd_hidlib_t *hidlib, rnd_gtk_topwin_t *tw, const char *cookie);
void ghid_install_accel_groups(GtkWindow *window, rnd_gtk_topwin_t *tw);
void ghid_remove_accel_groups(GtkWindow *window, rnd_gtk_topwin_t *tw);
void ghid_create_pcb_widgets(rnd_gtk_t *ctx, rnd_gtk_topwin_t *tw, GtkWidget *in_top_window);
void ghid_fullscreen_apply(rnd_gtk_topwin_t *tw);
void rnd_gtk_tw_layer_vis_update(rnd_gtk_topwin_t *tw);

void rnd_gtk_tw_interface_set_sensitive(rnd_gtk_topwin_t *tw, gboolean sensitive);

int rnd_gtk_tw_dock_enter(rnd_gtk_topwin_t *tw, rnd_hid_dad_subdialog_t *sub, rnd_hid_dock_t where, const char *id);
void rnd_gtk_tw_dock_leave(rnd_gtk_topwin_t *tw, rnd_hid_dad_subdialog_t *sub);

void rnd_gtk_tw_set_title(rnd_gtk_topwin_t *tw, const char *title);

gboolean ghid_idle_cb(void *topwin);
gboolean ghid_port_key_release_cb(GtkWidget * drawing_area, GdkEventKey * kev, rnd_gtk_topwin_t *tw);

void rnd_gtk_tw_dock_uninit(void);

#endif

