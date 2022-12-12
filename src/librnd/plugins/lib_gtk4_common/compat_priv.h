/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2021 Tibor Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* internals of compat.h that are not included in the public API, gtk4 version */

void gtkci_widget_css_add(GtkWidget *widget, const char *css, const char *namspc, int is_low_prio);
void gtkci_widget_css_del(GtkWidget *widget, const char *namspc);

/* INTERNAL: set fill/exp for a widget (not part of the API, do not call from elsewhere) */
static inline void gtkci_expfill(GtkWidget *parent, GtkWidget *w, int expfill, int start)
{
	int h = expfill, v = expfill;

	/* set fill/exp in parent box if parent is a box: figure parent orientation */
	if (expfill && GTK_IS_BOX(parent)) {
		GtkOrientation o = gtk_orientable_get_orientation(GTK_ORIENTABLE(parent));
		if (o == GTK_ORIENTATION_HORIZONTAL) v = 0;
		if (o == GTK_ORIENTATION_VERTICAL) h = 0;
	}

	if (h) {
		gtk_widget_set_halign(w, GTK_ALIGN_FILL);
		gtk_widget_set_hexpand(w, 1);
	}
	else {
		gtk_widget_set_halign(w, start ? GTK_ALIGN_START : GTK_ALIGN_FILL);
		gtk_widget_set_hexpand(w, 0);
	}

	if (v) {
		gtk_widget_set_valign(w, GTK_ALIGN_FILL);
		gtk_widget_set_vexpand(w, 1);
	}
	else {
		gtk_widget_set_valign(w, start ? GTK_ALIGN_BASELINE : GTK_ALIGN_FILL);
		gtk_widget_set_vexpand(w, 0);
	}
}

/*** Create different event controllers, one per widget, on first use ***/
static inline GtkEventController *gtkc_evctrl_key(GtkWidget *w)
{
	GObject *obj = G_OBJECT(w);
	GtkEventController *ctrl = g_object_get_data(obj, "rndK");
	if (ctrl != NULL) return ctrl;
	ctrl = gtk_event_controller_key_new();
	gtk_widget_add_controller(w, ctrl);
	g_object_set_data(obj, "rndK", ctrl);
	return ctrl;
}

static inline GtkEventController *gtkc_evctrl_scroll(GtkWidget *w)
{
	GObject *obj = G_OBJECT(w);
	GtkEventController *ctrl = g_object_get_data(obj, "rndS");
	if (ctrl != NULL) return ctrl;
	ctrl = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES);
	gtk_widget_add_controller(w, ctrl);
	g_object_set_data(obj, "rndS", ctrl);
	return ctrl;
}

static inline GtkEventController *gtkc_evctrl_motion(GtkWidget *w)
{
	GObject *obj = G_OBJECT(w);
	GtkEventController *ctrl = g_object_get_data(obj, "rndM");
	if (ctrl != NULL) return ctrl;
	ctrl = gtk_event_controller_motion_new();
	gtk_widget_add_controller(w, ctrl);
	g_object_set_data(obj, "rndM", ctrl);
	return ctrl;
}

#if GTK4_BUG_ON_GESTURE_CLICK_FIXED
gboolean gtkc_mouse_press_cb(GtkGestureClick *self, gint n_press, double x, double y, gpointer user_data);
gboolean gtkc_mouse_release_cb(GtkGestureClick *self, gint n_press, double x, double y, gpointer user_data);
static inline GtkEventController *gtkc_evctrl_click(GtkWidget *w)
{
	GObject *obj = G_OBJECT(w);
	GtkGesture *gest;
	GtkEventController *ctrl = g_object_get_data(obj, "rndC");
	if (ctrl != NULL) return ctrl;
	gest = gtk_gesture_click_new();
	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gest), 0);
	ctrl = GTK_EVENT_CONTROLLER(gest);
	gtk_widget_add_controller(w, ctrl);
	g_object_set_data(obj, "rndC", ctrl);
	return ctrl;
}
#else
gboolean gtkc_mouse_press_cb(GtkGestureClick *self, GdkEvent *ev, gpointer rs_);
gboolean gtkc_mouse_release_cb(GtkGestureClick *self, GdkEvent *ev, gpointer rs_);
static inline GtkEventController *gtkc_evctrl_click(GtkWidget *w)
{
	GObject *obj = G_OBJECT(w);
	GtkEventController *ctrl = g_object_get_data(obj, "rndC");
	if (ctrl != NULL) return ctrl;
	ctrl = gtk_event_controller_legacy_new();
	gtk_widget_add_controller(w, ctrl);
	g_object_set_data(obj, "rndC", ctrl);
	return ctrl;
}
#endif

static inline GdkSurface *gtkc_win_surface(GtkWidget *window)
{
	GtkNative *nat = gtk_widget_get_native(window);
	return gtk_native_get_surface(nat);
}

gboolean gtkc_resize_dwg_cb(GtkWidget *widget, gint width, gint height, void *rs);
gint gtkc_mouse_scroll_cb(GtkEventControllerScroll *self, gdouble dx, gdouble dy, gpointer user_data);
gint gtkc_mouse_enter_cb(GtkEventControllerMotion *self, gdouble x, gdouble y, gpointer user_data);
gint gtkc_mouse_leave_cb(GtkEventControllerMotion *self, gdouble x, gdouble y, gpointer user_data);
gint gtkc_mouse_motion_cb(GtkEventControllerMotion *self, gdouble x, gdouble y, gpointer user_data);
gint gtkc_key_press_fwd_cb(GtkEventControllerKey *self, guint keyval, guint keycode, GdkModifierType state, gpointer user_data);
gint gtkc_key_press_cb(GtkEventControllerKey *self, guint keyval, guint keycode, GdkModifierType state, gpointer user_data);
gint gtkc_key_release_cb(GtkEventControllerKey *self, guint keyval, guint keycode, GdkModifierType state, gpointer user_data);
gint gtkc_win_resize_cb(GdkSurface *surf, gint width, gint height, void *rs);
gint gtkc_win_destroy_cb(GtkWidget *widget, void *rs);
gint gtkc_win_delete_cb(GtkWindow *window, void *rs);
