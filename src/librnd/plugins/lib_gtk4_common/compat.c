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
	return rs->cb(widget, 0, 0, 0, rs->user_data);
}

gint gtkc_mouse_leave_cb(GtkEventControllerMotion *self, gdouble x, gdouble y, gpointer rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	return rs->cb(widget, 0, 0, 0, rs->user_data);
}

gint gtkc_mouse_motion_cb(GtkEventControllerMotion *self, gdouble x, gdouble y, gpointer rs_)
{
	gtkc_event_xyz_t *rs = rs_;
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


gint gtkc_key_press_cb(GtkEventControllerKey *self, guint keyval, guint keycode, GdkModifierType state, gpointer rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	int mods;
	unsigned short int key_raw, kv;

	if (rnd_gtkc_key_translate(self, keyval, keycode, state, &mods, &key_raw, &kv) != 0)
		return FALSE;

	return rs->cb(widget, mods, key_raw, kv, rs->user_data);
}

gint gtkc_key_release_cb(GtkEventControllerKey *self, guint keyval, guint keycode, GdkModifierType state, gpointer rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	int mods;
	unsigned short int key_raw, kv;

	if (rnd_gtkc_key_translate(self, keyval, keycode, state, &mods, &key_raw, &kv) != 0)
		return FALSE;

	return rs->cb(widget, mods, key_raw, kv, rs->user_data);
}

gint gtkc_win_resize_cb(GdkSurface *surf, gint width, gint height, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	return rs->cb(widget, 0, 0, 0, rs->user_data);
}

gint gtkc_win_destroy_cb(GtkWidget *widget, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	return rs->cb(widget, 0, 0, 0, rs->user_data);
}

gint gtkc_win_delete_cb(GtkWindow *window, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	return rs->cb(widget, 0, 0, 0, rs->user_data);
}



#if GTK4_BUG_ON_GESTURE_CLICK_FIXED
static gboolean mouse_press_cb(GtkGestureClick *self, gint n_press, double x, double y, gpointer rs_)
{
	guint btn = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(self));
	printf("mouse press [%d] %d %f %f\n", btn, n_press, x, y);
	return TRUE;
}

static gboolean mouse_release_cb(GtkGestureClick *self, gint n_press, double x, double y, gpointer rs_)
{
	guint btn = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(self));
	printf("mouse release [%d] %d %f %f\n", btn, n_press, x, y);
	return TRUE;
}
#else
static gboolean mouse_press_cb(GtkGestureClick *self, GdkEvent *ev, gpointer rs_)
{
	double x, y;
	guint btn;
	GdkEventType type = gdk_event_get_event_type(ev);
	if (type != GDK_BUTTON_PRESS) return FALSE;

	gdk_event_get_position(ev, &x, &y);
	btn = gdk_button_event_get_button(ev);
	return TRUE;
}

static gboolean mouse_release_cb(GtkGestureClick *self, GdkEvent *ev, gpointer rs_)
{
	double x, y;
	guint btn;
	GdkEventType type = gdk_event_get_event_type(ev);
	if (type != GDK_BUTTON_RELEASE) return FALSE;

	gdk_event_get_position(ev, &x, &y);
	btn = gdk_button_event_get_button(ev);
	return TRUE;
}
#endif
