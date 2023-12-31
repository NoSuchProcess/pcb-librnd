#ifndef RND_GTK_IN_MOUSE_H
#define RND_GTK_IN_MOUSE_H

#include <librnd/hid/hid_cfg_input.h>
#include "rnd_gtk.h"

#include <gtk/gtk.h>
#include <gdk/gdkevents.h>


extern rnd_hid_cfg_mouse_t rnd_gtk_mouse;
extern int rnd_gtk_wheel_zoom;

rnd_hid_cfg_mod_t rnd_gtk_mouse_button(int ev_button);

int rnd_gtk_get_user_xy(rnd_gtk_t *ctx, const char *msg);

gint rnd_gtk_window_mouse_scroll_cb(GtkWidget *widget, long dx, long dy, long modkey, void *out);

gboolean rnd_gtk_button_press_cb(GtkWidget *drawing_area, long x, long y, long btn, gpointer data);
gboolean rnd_gtk_button_release_cb(GtkWidget *drawing_area, long x, long y, long btn, gpointer data);

void rnd_gtk_reg_mouse_cursor(rnd_gtk_t *ctx, int idx, const char *name, const unsigned char *pixel, const unsigned char *mask);
void rnd_gtk_set_mouse_cursor(rnd_gtk_t *ctx, int idx);

void rnd_gtk_mode_cursor(rnd_gtk_t *ctx); /* Changes the normal cursor appearance according to last set mode, but respect override */
void rnd_gtk_restore_cursor(rnd_gtk_t *ctx); /* Remove override and restore the mode cursor */


#endif
