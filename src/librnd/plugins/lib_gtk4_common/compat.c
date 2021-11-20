#include <gtk/gtk.h>

#include "compat.h"

#include <librnd/plugins/lib_gtk_common/rnd_gtk.h>
#include <librnd/plugins/lib_gtk_common/in_keyboard.h>

#define EVCTRL_WIDGET  gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self))
#define EVCTRL_STATE   gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(self))

gboolean gtkc_resize_dwg_cb(GtkWidget *widget, gint width, gint height, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	return rs->cb(widget, width, height, 0, rs->user_data);
}

gint gtkc_mouse_scroll_cb(GtkEventControllerScroll *self, gdouble dx, gdouble dy, gpointer rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	GtkWidget *widget = EVCTRL_WIDGET;
	GdkModifierType state = EVCTRL_STATE;
	ModifierKeysState mk = rnd_gtk_modifier_keys_state(widget, &state);
	return rs->cb(widget, rnd_round(dx), rnd_round(dy), mk, rs->user_data);
}

gint gtkc_mouse_enter_cb(GtkEventControllerMotion *self, gdouble x, gdouble y, gpointer rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	GtkWidget *widget = EVCTRL_WIDGET;
	return rs->cb(widget, 0, 0, 0, rs->user_data);
}

gint gtkc_mouse_leave_cb(GtkEventControllerMotion *self, gdouble x, gdouble y, gpointer rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	GtkWidget *widget = EVCTRL_WIDGET;
	return rs->cb(widget, 0, 0, 0, rs->user_data);
}

gint gtkc_mouse_motion_cb(GtkEventControllerMotion *self, gdouble x, gdouble y, gpointer rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	GtkWidget *widget = EVCTRL_WIDGET;
	return rs->cb(widget, rnd_round(x), rnd_round(y), 0, rs->user_data);
}

static inline int rnd_gtkc_key_translate(GtkEventControllerKey *self, guint keyval, guint keycode, GdkModifierType state, int *out_mods, unsigned short int *out_key_raw, unsigned short int *out_kv)
{
	unsigned short int key_raw = 0;
	GdkDevice *dev = gtk_event_controller_get_current_event_device(GTK_EVENT_CONTROLLER(self));
	GdkDisplay *display = gdk_device_get_display(dev);
	GdkKeymapKey *keys;
	guint *keyvals;
	int n_entries;

	if (keyval > 0xffff)
		return -1;

	if (gdk_display_map_keycode(display, keycode, &keys, &keyvals, &n_entries)) {
		key_raw = keyvals[0];
		g_free(keys);
		g_free(keyvals);
	}


	return rnd_gtk_key_translate(keyval, state, key_raw,  out_mods, out_key_raw, out_kv);
}


gint gtkc_key_press_fwd_cb(GtkEventControllerKey *self, guint keyval, guint keycode, GdkModifierType state, gpointer rs_)
{
	gtkc_event_xyz_fwd_t *rs = rs_;
	GtkWidget *widget = EVCTRL_WIDGET;
	int mods;
	unsigned short int key_raw, kv;

	if (rnd_gtkc_key_translate(self, keyval, keycode, state, &mods, &key_raw, &kv) != 0)
		return FALSE;

	return rs->cb(widget, mods, key_raw, kv, self, rs->user_data);
}

gint gtkc_key_press_cb(GtkEventControllerKey *self, guint keyval, guint keycode, GdkModifierType state, gpointer rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	GtkWidget *widget = EVCTRL_WIDGET;
	int mods;
	unsigned short int key_raw, kv;

	if (rnd_gtkc_key_translate(self, keyval, keycode, state, &mods, &key_raw, &kv) != 0)
		return FALSE;

	return rs->cb(widget, mods, key_raw, kv, rs->user_data);
}

gint gtkc_key_release_cb(GtkEventControllerKey *self, guint keyval, guint keycode, GdkModifierType state, gpointer rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	GtkWidget *widget = EVCTRL_WIDGET;
	int mods;
	unsigned short int key_raw, kv;

	if (rnd_gtkc_key_translate(self, keyval, keycode, state, &mods, &key_raw, &kv) != 0)
		return FALSE;

	return rs->cb(widget, mods, key_raw, kv, rs->user_data);
}

gint gtkc_win_resize_cb(GdkSurface *surf, gint width, gint height, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	GtkNative *nat = gtk_native_get_for_surface(surf);
	return rs->cb(GTK_WIDGET(nat), 0, 0, 0, rs->user_data);
}

gint gtkc_win_destroy_cb(GtkWidget *widget, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	return rs->cb(widget, 0, 0, 0, rs->user_data);
}

gint gtkc_win_delete_cb(GtkWindow *window, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	return rs->cb(GTK_WIDGET(window), 0, 0, 0, rs->user_data);
}



#if GTK4_BUG_ON_GESTURE_CLICK_FIXED
gboolean gtkc_mouse_press_cb(GtkGestureClick *self, gint n_press, double x, double y, gpointer rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	guint btn = rnd_gtk_mouse_button(gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(self)));
	GdkModifierType state = EVCTRL_STATE;
	ModifierKeysState mk = rnd_gtk_modifier_keys_state(widget, &state);

	return rs->cb(widget, rnd_round(x), rnd_round(ev->y), btn | mk, rs->user_data);
}

gboolean gtkc_mouse_release_cb(GtkGestureClick *self, gint n_press, double x, double y, gpointer rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	guint btn = rnd_gtk_mouse_button(gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(self)));
	GdkModifierType state = EVCTRL_STATE;
	ModifierKeysState mk = rnd_gtk_modifier_keys_state(widget, &state);

	return rs->cb(widget, rnd_round(x), rnd_round(ev->y), btn | mk, rs->user_data);
}
#else
gboolean gtkc_mouse_press_cb(GtkGestureClick *self, GdkEvent *ev, gpointer rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	double x, y;
	guint btn;
	GtkWidget *widget;
	GdkEventType type = gdk_event_get_event_type(ev);
	GdkModifierType state;
	ModifierKeysState mk;

	if (type != GDK_BUTTON_PRESS) return FALSE;

	widget = EVCTRL_WIDGET;
	state = gdk_event_get_modifier_state(ev) & GDK_MODIFIER_MASK;
	mk = rnd_gtk_modifier_keys_state(widget, &state);

	gdk_event_get_position(ev, &x, &y);
	btn = rnd_gtk_mouse_button(gdk_button_event_get_button(ev));
	return rs->cb(widget, rnd_round(x), rnd_round(y), btn | mk, rs->user_data);
}

gboolean gtkc_mouse_release_cb(GtkGestureClick *self, GdkEvent *ev, gpointer rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	double x, y;
	guint btn;
	GtkWidget *widget;
	GdkEventType type = gdk_event_get_event_type(ev);
	GdkModifierType state;
	ModifierKeysState mk;

	if (type != GDK_BUTTON_RELEASE) return FALSE;

	widget = EVCTRL_WIDGET;
	state = gdk_event_get_modifier_state(ev) & GDK_MODIFIER_MASK;
	mk = rnd_gtk_modifier_keys_state(widget, &state);

	gdk_event_get_position(ev, &x, &y);
	btn = rnd_gtk_mouse_button(gdk_button_event_get_button(ev));
	return rs->cb(widget, rnd_round(x), rnd_round(y), btn | mk, rs->user_data);
}
#endif

int gtkc_clipboard_set_text(GtkWidget *widget, const char *text)
{
	GdkClipboard *cbrd = gtk_widget_get_clipboard(widget);
	gdk_clipboard_set_text(cbrd, text);
	return 0;
}


/* Some basic functionality is not available on wayland and got removed from
   gtk4. Reimplement them for X11.
   Dispatch: https://gnome.pages.gitlab.gnome.org/gtk/gdk4/x11.html
*/

#ifdef GDK_WINDOWING_X11
#	include <gdk/x11/gdkx.h>
#endif
#ifdef GDK_WINDOWING_WAYLAND
#	include <gdk/wayland/gdkwayland.h>
#endif


void gtkc_window_resize(GtkWindow *win, int x, int y)
{
#ifdef GDK_WINDOWING_X11
	GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(win));
	if (GDK_IS_X11_DISPLAY(display)) {
		GdkSurface *surf = gtkc_win_surface(GTK_WIDGET(win));
		Display *dsp = GDK_SURFACE_XDISPLAY(surf);
		Window xw = gdk_x11_surface_get_xid(surf);
		XResizeWindow(dsp, xw, x, y);
	}
#endif
/* Not available on wayland */
}

void gtkc_window_move(GtkWindow *win, int x, int y)
{
#ifdef GDK_WINDOWING_X11
	GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(win));
	if (GDK_IS_X11_DISPLAY(display)) {
		GdkSurface *surf = gtkc_win_surface(GTK_WIDGET(win));
		Display *dsp = GDK_SURFACE_XDISPLAY(surf);
		Window xw = gdk_x11_surface_get_xid(surf);
		XMoveWindow(dsp, xw, x, y);
	}
#endif
/* Not available on wayland */
}

void gtkc_window_origin(GtkWidget *wdg, int *x, int *y)
{
#ifdef GDK_WINDOWING_X11
	GdkDisplay *display = gtk_widget_get_display(wdg);
	if (GDK_IS_X11_DISPLAY(display)) {
		GdkSurface *surf = gtkc_win_surface(wdg);
		Display *dsp = GDK_SURFACE_XDISPLAY(surf);
		Window xw = gdk_x11_surface_get_xid(surf), child;
		Window rw = gdk_x11_display_get_xrootwindow(display);
		XTranslateCoordinates(dsp, xw, rw, 0, 0, x, y, &child);
		return;
	}
#endif
/* Not available on wayland */
	*x = *y = 0;
}


static void gtkci_stop_mainloop_cb(GtkWidget *widget, gpointer udata)
{
	GMainLoop *loop = udata;
	if (g_main_loop_is_running(loop))
		g_main_loop_quit(loop);
}

GtkResponseType gtkc_dialog_run(GtkDialog *dlg, int is_modal)
{
		GMainLoop *loop;

	if (is_modal)
		gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);

	loop = g_main_loop_new(NULL, FALSE);
	g_signal_connect(dlg, "destroy", G_CALLBACK(gtkci_stop_mainloop_cb), loop);
	g_main_loop_run(loop);
	g_main_loop_unref(loop);
	return GTK_RESPONSE_NONE;
}
