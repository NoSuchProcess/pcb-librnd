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
	long x, y;
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

	/* Reject double and triple click events */
	if (ev->type != GDK_BUTTON_PRESS)
		return TRUE;

	rnd_gtk_glob_mask = (GdkModifierType)(ev->state); /* gtk2 workaround on mac for shift pressed */

	return rs->cb(widget, ev->x, ev->y, rnd_gtk_mouse_button(ev->button), rs->user_data);
}

gint gtkc_mouse_release_cb(GtkWidget *widget, GdkEventButton *ev, void *rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	return rs->cb(widget, ev->x, ev->y, rnd_gtk_mouse_button(ev->button), rs->user_data);
}

