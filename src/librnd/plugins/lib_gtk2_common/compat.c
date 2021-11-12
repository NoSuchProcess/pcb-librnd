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

