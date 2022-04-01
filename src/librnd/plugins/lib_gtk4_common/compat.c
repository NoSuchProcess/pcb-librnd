#include <gtk/gtk.h>

#include "compat.h"

#include <librnd/plugins/lib_gtk_common/rnd_gtk.h>
#include <librnd/plugins/lib_gtk_common/in_keyboard.h>

#define EVCTRL_WIDGET  gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self))
#define EVCTRL_STATE   gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(self))

GtkWidget *gtkc_event_widget;
double gtkc_event_x, gtkc_event_y;


#ifdef GDK_WINDOWING_X11
#include <dlfcn.h>

Bool (*gtkc_XQueryPointer)(Display*,Window,Window*,Window*,int*,int*,int*,int*,unsigned int*) = NULL;
int (*gtkc_XWarpPointer)(Display*,Window,Window,int,int,unsigned int,unsigned int,int,int) = NULL;

int (*gtkc_XResizeWindow)(Display*,Window,unsigned int,unsigned int);
int (*gtkc_XMoveWindow)(Display*,Window,int,int);
Bool (*gtkc_XTranslateCoordinates)(Display*,Window,Window,int,int,int*,int*,Window*);


/* Can't directly use these symbols because there's no -lX11 as we don't know
   whether the executable will be running on X11 or Wayland */
int gtkc_resolve_X(void)
{
	static int inited = 0;

	if (!inited) {
		void *handle = dlopen(NULL, 0);
		gtkc_XQueryPointer         = dlsym(handle, "XQueryPointer");
		gtkc_XWarpPointer          = dlsym(handle, "XWarpPointer");
		gtkc_XResizeWindow         = dlsym(handle, "XResizeWindow");
		gtkc_XMoveWindow           = dlsym(handle, "XMoveWindow");
		gtkc_XTranslateCoordinates = dlsym(handle, "XTranslateCoordinates");
		inited = 1;
	}

	return (gtkc_XQueryPointer == NULL) || (gtkc_XWarpPointer == NULL) || (gtkc_XResizeWindow == NULL) || (gtkc_XMoveWindow == NULL);
}

#endif


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
	GtkWidget *widget = EVCTRL_WIDGET, *save;
	int mods, res;
	unsigned short int key_raw, kv;

	if (rnd_gtkc_key_translate(self, keyval, keycode, state, &mods, &key_raw, &kv) != 0)
		return FALSE;

	save = gtkc_event_widget; gtkc_event_widget = widget;
	res = rs->cb(widget, mods, key_raw, kv, rs->user_data);
	gtkc_event_widget = save;
	return res;
}

gint gtkc_key_release_cb(GtkEventControllerKey *self, guint keyval, guint keycode, GdkModifierType state, gpointer rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	GtkWidget *widget = EVCTRL_WIDGET, *save;
	int mods, res;
	unsigned short int key_raw, kv;

	if (rnd_gtkc_key_translate(self, keyval, keycode, state, &mods, &key_raw, &kv) != 0)
		return FALSE;

	save = gtkc_event_widget; gtkc_event_widget = widget;
	res = rs->cb(widget, mods, key_raw, kv, rs->user_data);
	gtkc_event_widget = save;
	return res;
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
	gboolean (*cb)(GtkWidget *widget, long x, long y, long z, void *user_data) = rs->cb;

	rs->cb = NULL; /* make sure we don't call cb twice */
	if (cb != NULL)
		return cb(widget, 0, 0, 0, rs->user_data);
	return 1;
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
	GtkWidget *widget = EVCTRL_WIDGET, *save;
	guint res, btn = rnd_gtk_mouse_button(gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(self)));
	GdkModifierType state = EVCTRL_STATE;
	ModifierKeysState mk = rnd_gtk_modifier_keys_state(widget, &state);

	save = gtkc_event_widget; gtkc_event_widget = widget;
	gtkc_event_x = x; gtkc_event_y = y;
	res = rs->cb(widget, rnd_round(x), rnd_round(y), btn | mk, rs->user_data);
	gtkc_event_widget = save;
	return res;
}

gboolean gtkc_mouse_release_cb(GtkGestureClick *self, gint n_press, double x, double y, gpointer rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	guint res, btn = rnd_gtk_mouse_button(gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(self)));
	GtkWidget *widget = EVCTRL_WIDGET, *save;
	GdkModifierType state = EVCTRL_STATE;
	ModifierKeysState mk = rnd_gtk_modifier_keys_state(widget, &state);

	save = gtkc_event_widget; gtkc_event_widget = widget;
	gtkc_event_x = x; gtkc_event_y = y;
	res = rs->cb(widget, rnd_round(x), rnd_round(y), btn | mk, rs->user_data);
	gtkc_event_widget = save;
	return res;
}
#else

/* Translate gdk event coords (screen coords) to widget relative coords */
static inline void gdkc_widget_coords(GtkWidget *widget, double *x, double *y)
{
	double dx, dy;
	GtkWidget *root = GTK_WIDGET(gtk_widget_get_root(widget));

	gtk_widget_translate_coordinates(root, widget, *x, *y, &dx, &dy);
	*x = dx;
	*y = dy;
}

gboolean gtkc_mouse_press_cb(GtkGestureClick *self, GdkEvent *ev, gpointer rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	double x, y;
	guint btn, res;
	GtkWidget *widget, *save;
	GdkEventType type = gdk_event_get_event_type(ev);
	GdkModifierType state;
	ModifierKeysState mk;

	if (type != GDK_BUTTON_PRESS) return FALSE;

	widget = EVCTRL_WIDGET;
	state = gdk_event_get_modifier_state(ev) & GDK_MODIFIER_MASK;
	mk = rnd_gtk_modifier_keys_state(widget, &state);

	gdk_event_get_position(ev, &x, &y);
	gdkc_widget_coords(widget, &x, &y);
	btn = rnd_gtk_mouse_button(gdk_button_event_get_button(ev));

	save = gtkc_event_widget; gtkc_event_widget = widget;
	gtkc_event_x = x; gtkc_event_y = y;
	res = rs->cb(widget, rnd_round(x), rnd_round(y), btn | mk, rs->user_data);
	gtkc_event_widget = save;
	return res;
}

gboolean gtkc_mouse_release_cb(GtkGestureClick *self, GdkEvent *ev, gpointer rs_)
{
	gtkc_event_xyz_t *rs = rs_;
	double x, y;
	guint btn, res;
	GtkWidget *widget, *save;
	GdkEventType type = gdk_event_get_event_type(ev);
	GdkModifierType state;
	ModifierKeysState mk;

	if (type != GDK_BUTTON_RELEASE) return FALSE;

	widget = EVCTRL_WIDGET;
	state = gdk_event_get_modifier_state(ev) & GDK_MODIFIER_MASK;
	mk = rnd_gtk_modifier_keys_state(widget, &state);

	gdk_event_get_position(ev, &x, &y);
	gdkc_widget_coords(widget, &x, &y);
	btn = rnd_gtk_mouse_button(gdk_button_event_get_button(ev));

	save = gtkc_event_widget; gtkc_event_widget = widget;
	gtkc_event_x = x; gtkc_event_y = y;
	res = rs->cb(widget, rnd_round(x), rnd_round(y), btn | mk, rs->user_data);
	gtkc_event_widget = save;
	return res;
}
#endif

#include "compat_clipboard.c"


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
	if (GDK_IS_X11_DISPLAY(display)) gtk_widget_show(GTK_WIDGET(win)); /* surface will be NULL without this */
	if (GDK_IS_X11_DISPLAY(display)) {
		GdkSurface *surf = gtkc_win_surface(GTK_WIDGET(win));
		Display *dsp = GDK_SURFACE_XDISPLAY(surf);
		Window xw = gdk_x11_surface_get_xid(surf);
		if (gtkc_resolve_X()) return;
		gtkc_XResizeWindow(dsp, xw, x, y);
	}
#endif
/* Not available on wayland */
}

void gtkc_window_move(GtkWindow *win, int x, int y)
{
#ifdef GDK_WINDOWING_X11
	GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(win));
	if (GDK_IS_X11_DISPLAY(display)) gtk_widget_show(GTK_WIDGET(win)); /* surface will be NULL without this */
	if (GDK_IS_X11_DISPLAY(display)) {
		GdkSurface *surf = gtkc_win_surface(GTK_WIDGET(win));
		Display *dsp = GDK_SURFACE_XDISPLAY(surf);
		Window xw = gdk_x11_surface_get_xid(surf);
		if (gtkc_resolve_X()) return;
		gtkc_XMoveWindow(dsp, xw, x, y);
	}
#endif
/* Not available on wayland */
}

void gtkc_widget_window_origin(GtkWidget *wdg, int *x, int *y)
{
#ifdef GDK_WINDOWING_X11
	GdkDisplay *display = gtk_widget_get_display(wdg);
	if (GDK_IS_X11_DISPLAY(display)) {
		GdkSurface *surf = gtkc_win_surface(wdg);
		Display *dsp = GDK_SURFACE_XDISPLAY(surf);
		Window xw = gdk_x11_surface_get_xid(surf), child;
		Window rw = gdk_x11_display_get_xrootwindow(display);
		if (gtkc_resolve_X()) return;
		gtkc_XTranslateCoordinates(dsp, xw, rw, 0, 0, x, y, &child);
		return;
	}
#endif
/* Not available on wayland */
	*x = *y = 0;
}

void gtkc_window_get_position(GtkWindow *win, int *x, int *y)
{
	gtkc_widget_window_origin(GTK_WIDGET(win), x, y);
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

void gtkci_widget_css_add(GtkWidget *widget, const char *css, const char *namspc, int is_low_prio)
{
	GtkStyleContext *context = gtk_widget_get_style_context(widget);
	GtkCssProvider *provider = gtk_css_provider_new();

	gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(provider), css, -1);
	gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), (is_low_prio ? GTK_STYLE_PROVIDER_PRIORITY_FALLBACK : GTK_STYLE_PROVIDER_PRIORITY_APPLICATION));
	gtk_style_context_add_class(context, namspc);
	g_object_unref(provider);
}

void gtkci_widget_css_del(GtkWidget *widget, const char *namspc)
{
	GtkStyleContext *context = gtk_widget_get_style_context(widget);
	gtk_style_context_remove_class(context, namspc);
}


void gtkc_widget_modify_bg_(GtkWidget *w, const rnd_gtk_color_t *color)
{
	char tmp[256];

	rnd_snprintf(tmp, sizeof(tmp),
		"*.wbgc {\nbackground-image: none;\nbackground-color: #%02x%02x%02x;\n}\n",
		(int)rnd_round(color->red*255.0), (int)rnd_round(color->green*255.0), (int)rnd_round(color->blue*255.0));
	gtkci_widget_css_del(w, "wbgc");
	gtkci_widget_css_add(w, tmp, "wbgc", 0);
}
