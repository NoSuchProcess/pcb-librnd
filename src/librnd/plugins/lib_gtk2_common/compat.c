#include "compat.h"
#include <librnd/plugins/lib_gtk_common/rnd_gtk.h>
#include <librnd/plugins/lib_gtk_common/in_keyboard.h>



gboolean gtkc_resize_dwg_cb(GtkWidget *widget, GdkEventConfigure *ev, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	return rs->cb(widget, ev->width, ev->height, 0, rs->user_data);
}

gint gtkc_mouse_scroll_cb(GtkWidget *widget, GdkEventScroll *ev, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	long x = 0, y = 0;
	ModifierKeysState mk;
	GdkModifierType state;

	state = (GdkModifierType) (ev->state);
	mk = rnd_gtk_modifier_keys_state(widget, &state);

	switch (ev->direction) {
		case GDK_SCROLL_UP:    y = -1; break;
		case GDK_SCROLL_DOWN:  y = +1; break;
		case GDK_SCROLL_LEFT:  x = -1; break;
		case GDK_SCROLL_RIGHT: x = +1; break;
	}

	return rs->cb(widget, x, y, mk, rs->user_data);
}

gint gtkc_mouse_enter_cb(GtkWidget *widget, GdkEventCrossing *ev, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	int need_update;

	/* See comment in gtkc_mouse_leave_cb */
	if (ev->mode != GDK_CROSSING_NORMAL && ev->mode != GDK_CROSSING_UNGRAB && ev->detail != GDK_NOTIFY_NONLINEAR)
		return FALSE;

	/* Following expression is true if you open a menu from the menu bar,
	   move the mouse to the viewport and click on it. This closes the menu
	   and moves the pointer to the viewport without the pointer going over
	   the edge of the viewport */
	need_update = ((ev->mode == GDK_CROSSING_UNGRAB) && (ev->detail == GDK_NOTIFY_NONLINEAR));
	return rs->cb(widget, need_update, 0, 0, rs->user_data);
}

gint gtkc_mouse_leave_cb(GtkWidget *widget, GdkEventCrossing *ev, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;

	/* Window leave events can also be triggered because of focus grabs. Some
	   X applications occasionally grab the focus and so trigger this function.
	   At least GNOME's window manager is known to do this on every mouse click.
	   See http://bugzilla.gnome.org/show_bug.cgi?id=102209 */
	if (ev->mode != GDK_CROSSING_NORMAL)
		return FALSE;

	return rs->cb(widget, 0, 0, 0, rs->user_data);
}


gint gtkc_mouse_motion_cb(GtkWidget *widget, GdkEventMotion *ev, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;

	gdk_event_request_motions(ev);

	return rs->cb(widget, ev->x, ev->y, 0, rs->user_data);
}

gint gtkc_mouse_press_cb(GtkWidget *widget, GdkEventButton *ev, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	GdkModifierType state;
	long btn;

	/* Reject double and triple click events */
	if (ev->type != GDK_BUTTON_PRESS)
		return TRUE;

	btn = rnd_gtk_mouse_button(ev->button);
	state = (GdkModifierType) (ev->state);
	btn |= rnd_gtk_modifier_keys_state(widget, &state);

	rnd_gtk_glob_mask = state; /* gtk2 workaround on mac for shift pressed */

	return rs->cb(widget, ev->x, ev->y, btn, rs->user_data);
}

gint gtkc_mouse_release_cb(GtkWidget *widget, GdkEventButton *ev, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	GdkModifierType state;
	long btn;

	btn = rnd_gtk_mouse_button(ev->button);
	state = (GdkModifierType) (ev->state);
	btn |= rnd_gtk_modifier_keys_state(widget, &state);

	return rs->cb(widget, ev->x, ev->y, btn, rs->user_data);
}


static inline int rnd_gtkc_key_translate(const GdkEventKey *kev, int *out_mods, unsigned short int *out_key_raw, unsigned short int *out_kv)
{
	guint *keyvals;
	GdkKeymapKey *keys;
	gint n_entries;
	unsigned short int key_raw = 0;

	if (kev->keyval > 0xffff)
		return -1;

	/* Retrieve the basic character (level 0) corresponding to physical key stroked. */
	if (gdk_keymap_get_entries_for_keycode(gdk_keymap_get_default(), kev->hardware_keycode, &keys, &keyvals, &n_entries)) {
		key_raw = keyvals[0];
		g_free(keys);
		g_free(keyvals);
	}

	return rnd_gtk_key_translate(kev->keyval, kev->state, key_raw,   out_mods, out_key_raw, out_kv);
}

gint gtkc_key_press_fwd_cb(GtkWidget *widget, GdkEventKey *kev, void *rs_)
{
	gtkc_event_xyz_fwd_t *rs = rs_;
	int mods;
	unsigned short int key_raw, kv;

	if (rnd_gtkc_key_translate(kev, &mods, &key_raw, &kv) != 0)
		return FALSE;

	return rs->cb(widget, mods, key_raw, kv, kev, rs->user_data);
}

gint gtkc_key_press_cb(GtkWidget *widget, GdkEventKey *kev, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	int mods;
	unsigned short int key_raw, kv;

	if (rnd_gtkc_key_translate(kev, &mods, &key_raw, &kv) != 0)
		return FALSE;

	return rs->cb(widget, mods, key_raw, kv, rs->user_data);
}

gint gtkc_key_release_cb(GtkWidget *widget, GdkEventKey *kev, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	int mods;
	unsigned short int key_raw, kv;

	if (rnd_gtkc_key_translate(kev, &mods, &key_raw, &kv) != 0)
		return FALSE;

	return rs->cb(widget, mods, key_raw, kv, rs->user_data);
}


gint gtkc_win_resize_cb(GtkWidget *widget, GdkEventConfigure *ev, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	return rs->cb(widget, 0, 0, 0, rs->user_data);
}

gint gtkc_win_destroy_cb(GtkWidget *widget, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	return rs->cb(widget, 0, 0, 0, rs->user_data);
}

gint gtkc_win_delete_cb(GtkWidget *widget, GdkEvent *ev, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	return rs->cb(widget, 0, 0, 0, rs->user_data);
}


int gtkc_clipboard_set_text(GtkWidget *widget, const char *text)
{
	GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	if (clipboard == NULL)
		return -1;
	gtk_clipboard_set_text(clipboard, text, -1);
	return 0;
}

char *gtkc_clipboard_get_text(GtkWidget *wdg)
{
	GtkClipboard *cbrd = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

	if (gtk_clipboard_wait_is_text_available(cbrd))
		return (char *)gtk_clipboard_wait_for_text(cbrd);

	return NULL;
}
