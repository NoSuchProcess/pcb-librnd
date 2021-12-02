/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2019,2021 Tibor Palinkas
 *  Copyright (C) 2017 Alain Vigne
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
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */
#ifndef RND_GTK4_COMPAT_H
#define RND_GTK4_COMPAT_H

/*** lib_gtk_common compatibilty layer for GTK4 ***/

#define RND_FOR_GTK4 1

#define GTKC_LITERAL_COLOR(a,r,g,b) {.red=r/65536.0, .green=g/65536.0, .blue=b/65536.0, .alpha = a/65536.0}


#define RND_GTK_BU_MENU_H_FN <librnd/plugins/lib_gtk4_common/bu_menu.h>

#include <gtk/gtk.h>
#include <librnd/core/compat_misc.h>

#include "compat_priv.h"

#define RND_GTK_KEY(keyname)   (GDK_KEY_ ## keyname)
#define rnd_gtkc_cursor_type_t const char *

static inline void gtkc_box_pack_append(GtkWidget *box, GtkWidget *child, gboolean expfill, guint padding);

#define gtkc_widget_get_window(w) (GDK_WINDOW(GTK_WIDGET(w)->window))

#define gtkc_widget_get_allocation(w, a) gtk_widget_get_allocation(w, a)
#define gtkc_combo_box_entry_new_text()  gtk_combo_box_text_new_with_entry()
#define gtkc_combo_box_get_entry(combo)  GTK_ENTRY(gtk_combo_box_get_child(GTK_COMBO_BOX(combo)))

static inline void gtkc_dlg_add_content(GtkDialog *dlg, GtkWidget *child)
{
	GtkWidget *content_area = gtk_dialog_get_content_area(dlg);
	gtk_box_append(GTK_BOX(content_area), child);

	/* make sure content fills the whole dialog box */
	gtk_widget_set_halign(child, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand(child, 1);
	gtk_widget_set_valign(child, GTK_ALIGN_FILL);
	gtk_widget_set_vexpand(child, 1);
}

GtkResponseType gtkc_dialog_run(GtkDialog *dlg, int is_modal);


typedef GdkRGBA rnd_gtk_color_t;

static inline void gtkc_box_pack_append(GtkWidget *box, GtkWidget *child, gboolean expfill, guint padding)
{
	gtk_box_append(GTK_BOX(box), child);
	gtkci_expfill(box, child, expfill, 0);
}

static inline void gtkc_box_pack_append_start(GtkWidget *box, GtkWidget *child, gboolean expfill, guint padding)
{
	gtk_box_append(GTK_BOX(box), child);
	gtkci_expfill(box, child, expfill, 1);
}

static inline GtkWidget *gtkc_hbox_new(gboolean homogenous, gint spacing)
{
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing);
	if (homogenous) gtk_box_set_homogeneous(GTK_BOX(box), 1);
	return box;
}

static inline GtkWidget *gtkc_vbox_new(gboolean homogenous, gint spacing)
{
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing);
	if (homogenous) gtk_box_set_homogeneous(GTK_BOX(box), 1);
	return box;
}

static inline GtkWidget *gtkc_hpaned_new()
{
	return gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
}

static inline GtkWidget *gtkc_vpaned_new()
{
	return gtk_paned_new(GTK_ORIENTATION_VERTICAL);
}

/* color button */

static inline GtkWidget *gtkc_color_button_new_with_color(rnd_gtk_color_t *color)
{
	return gtk_color_button_new_with_rgba(color);
}

static inline void gtkc_color_button_set_color(GtkWidget *button, rnd_gtk_color_t *color)
{
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(button), color);
}

static inline void gtkc_color_button_get_color(GtkWidget *button, rnd_gtk_color_t *color)
{
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), color);
}

static inline GtkWidget *gtkc_combo_box_text_new(void)
{
	return gtk_combo_box_text_new();
}

static inline void gtkc_combo_box_text_append_text(GtkWidget *combo, const gchar *text)
{
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), text);
}

static inline void gtkc_combo_box_text_prepend_text(GtkWidget *combo, const gchar *text)
{
	gtk_combo_box_text_prepend_text(GTK_COMBO_BOX_TEXT(combo), text);
}

static inline void gtkc_combo_box_text_remove(GtkWidget *combo, gint position)
{
	gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(combo), position);
}

static inline gchar *gtkc_combo_box_text_get_active_text(GtkWidget *combo)
{
	return gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
}

static inline GtkWidget *gtkc_trunctext_new(const gchar *str)
{
	GtkWidget *w = gtk_label_new(str);
	gtk_widget_set_size_request(w, 1, 1);

	return w;
}

typedef struct {
	GtkGLArea *area;
	GdkGLContext *context;
} rnd_gtk_expose_t;

static inline void gtkc_scrolled_window_add_with_viewport(GtkWidget *scrolled, GtkWidget *child)
{
	TODO("verify that this works; pcb-rnd layer binding is a good candidate");
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), child);
}

TODO("should be void in gtk2 too")
TODO("test this, e.g. shift pressed")
static inline void gdkc_window_get_pointer(GtkWidget *w, gint *x, gint *y, GdkModifierType *mask)
{
	double dx, dy;
	GdkDisplay *dsp = gtk_widget_get_display(w);
	GdkSeat *seat = gdk_display_get_default_seat(dsp);
	GdkDevice *dev = gdk_seat_get_pointer(seat);
	GtkNative *nat = gtk_widget_get_native(w);
	GdkSurface *surf = gtk_native_get_surface(nat);

	gdk_surface_get_device_position(surf, dev, &dx, &dy, mask);

	if (x != NULL)
		*x = rnd_round(dx);

	if (y != NULL)
		*y = rnd_round(dy);
}

static inline void rnd_gtk_set_selected(GtkWidget *widget, int set)
{
	if (set) {
		gtkci_widget_css_add(widget, "@define-color theme_selected_bg_color #ff0000;\n@define-color theme_selected_fg_color #000000;\n\n", "selbgc", 1);
		gtkci_widget_css_add(widget, "*.selbg {\nbackground-image: none;\nbackground-color: @theme_selected_bg_color;\ncolor: @theme_selected_fg_color;\n}\n", "selbg", 0);
	}
	else {
		gtkci_widget_css_del(widget, "selbgc");
		gtkci_widget_css_del(widget, "selbg");
	}
}

/* gtk deprecated gtk_widget_hide_all() for some reason; this naive
   implementation seems to work. */
static inline void gtkc_widget_hide_all(GtkWidget *widget)
{
	GtkWidget *ch;
	for(ch = gtk_widget_get_first_child(widget); ch != NULL; ch = gtk_widget_get_next_sibling(ch))
		gtkc_widget_hide_all(ch);
	gtk_widget_hide(widget);
}

/* gtk deprecated gtk_widget_show_all() for some reason; this naive
   implementation seems to work. */
static inline void gtkc_widget_show_all(GtkWidget *widget)
{
/*	GtkWidget *ch;
	for(ch = gtk_widget_get_first_child(widget); ch != NULL; ch = gtk_widget_get_next_sibling(ch))
		gtkc_widget_show_all(ch);*/
	gtk_widget_show(widget);
}

/* create a table with known size (all rows and cols created empty) */
static inline GtkWidget *gtkc_table_static(int rows, int cols, gboolean homog)
{
	GtkWidget *tbl = gtk_grid_new();
	gtk_grid_set_column_homogeneous(GTK_GRID(tbl), 1);

	return tbl;
}

static inline const char *gtkc_entry_get_text(GtkEntry *entry)
{
	GtkEntryBuffer *b = gtk_entry_get_buffer(entry);
	return gtk_entry_buffer_get_text(b);
}

static inline void gtkc_entry_set_text(GtkEntry *entry, const char *str)
{
	GtkEntryBuffer *b = gtk_entry_buffer_new(str, -1);
	gtk_entry_set_buffer(GTK_ENTRY(entry), b);
	g_object_unref(b);
}

#define GTKC_TREE_FORWARD_EVENT \
	do { \
		tree_priv_t *tp = g_object_get_data(G_OBJECT(tree_view), RND_OBJ_PROP_TREE_PRIV); \
		g_signal_handler_block(fwd, tp->kpsig); \
		gtk_event_controller_key_forward(fwd, GTK_WIDGET(tree_view)); \
		g_signal_handler_unblock(fwd, tp->kpsig); \
	} while(0)

#define gtkc_entry_set_width_chars(e, w)  gtk_editable_set_width_chars(GTK_EDITABLE(e), w)
void gtkc_widget_modify_bg_(GtkWidget *w, rnd_gtk_color_t *color);
#define gtkc_widget_modify_bg(w, st, c)   gtkc_widget_modify_bg_(w, c)
#define gtkc_frame_set_child(fr, ch)      gtk_frame_set_child(GTK_FRAME(fr), ch)
#define gtkc_scrolled_window_new()        gtk_scrolled_window_new()
#define gtkc_scrolled_window_set_child(sw, ch) gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sw), GTK_WIDGET(ch))
#define gtkc_window_set_child(sw, ch)     gtk_window_set_child(GTK_WINDOW(sw), GTK_WIDGET(ch))
#define gtkc_button_set_child(btn, ch)    gtk_button_set_child(GTK_BUTTON(btn), ch)
#define gtkc_button_set_image(btn, img)   gtk_button_set_child(btn, img)
#define gtkc_window_destroy(win)          gtk_window_destroy(GTK_WINDOW(win))
#define gtkc_paned_pack1(pane, ch, resiz) gtk_paned_set_start_child(pane, ch)
#define gtkc_paned_pack2(pane, ch, resiz) gtk_paned_set_end_child(pane, ch)
#define gtkc_window_set_role(win, id)
void gtkc_window_resize(GtkWindow *win, int x, int y);
void gtkc_window_move(GtkWindow *win, int x, int y);
void gtkc_widget_window_origin(GtkWidget *wdg, int *x, int *y);
#define gtkc_main_quit()                  g_main_loop_quit(NULL)
#define gtkc_bgcolor_box_new()            gtkc_hbox_new(TRUE, 0)
#define gtkc_bgcolor_box_set_child(b, ch) gtkc_box_pack_append(b, ch, TRUE, 0)
#define gtkc_setup_events(dwg, mb, ms, mo, key, ex, ent)
#define gtkc_widget_set_focusable(w)      gtk_widget_set_focusable(w, TRUE)
#define GDKC_MOD1_MASK                    GDK_ALT_MASK
#define gtkc_check_button_set_active(b, act) gtk_check_button_set_active(GTK_CHECK_BUTTON(b), act)
#define gtkc_check_button_get_active(b)   gtk_check_button_get_active(GTK_CHECK_BUTTON(b))
#define GTKC_CHECK_BUTTON_TOGGLE_SIG      "toggled"

/* gtk4 outsmarts the code and automatically scales up small images */
static inline void gtkc_workaround_image_scale_bug(GtkWidget *img, GdkPixbuf *pxb)
{
	gtk_image_set_pixel_size(GTK_IMAGE(img), MAX(gdk_pixbuf_get_width(pxb), gdk_pixbuf_get_height(pxb)));
	gtk_widget_set_halign(img, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(img, GTK_ALIGN_CENTER);
}

/* gtk4 CSS spaceship adds unnecessary passing around small images */
static inline void gtkc_workaround_image_button_border_bug(GtkWidget *btn, GdkPixbuf *pxb)
{
	gtkci_widget_css_add(btn, "*.picbtn {\npadding: 3px;\nmargin: 0px;min-height: 2px;min-width: 2px;\n}\n", "picbtn", 0);
}


static inline void gtkc_widget_destroy(GtkWidget *w)
{
	GtkWidget *parent = gtk_widget_get_parent(w);
	if (GTK_IS_FRAME(parent))
		gtk_frame_set_child(GTK_FRAME(parent), NULL);
	else if (GTK_IS_BOX(parent))
		gtk_box_remove(GTK_BOX(parent), w);
	else
		abort();
}


static inline void gtkc_wait_pending_events(void)
{
	while(g_main_context_pending(NULL))
		g_main_context_iteration(NULL, 0);
}

/* Attach child in a single cell of the table */
static inline void gtkc_table_attach1(GtkWidget *table, GtkWidget *child, int row, int col)
{
	GtkWidget *bb = gtkc_hbox_new(0, 5); /* this box enables child to grow horizontally */
	gtkc_box_pack_append(bb, child, 1, 0);
	gtk_grid_attach(GTK_GRID(table), bb, col, row, 1, 1);
}

/* In the following all rs is gtkc_event_xyz_t, filled in by the caller */

#define gtkc_bind_resize_dwg(widget, rs) \
	g_signal_connect(G_OBJECT(GTK_WIDGET(widget)), "resize", G_CALLBACK(gtkc_resize_dwg_cb), rs)

#define gtkc_bind_mouse_scroll(widget, ev) \
	g_signal_connect(G_OBJECT(gtkc_evctrl_scroll(GTK_WIDGET(widget))), "scroll", G_CALLBACK(gtkc_mouse_scroll_cb), ev)

#define gtkc_bind_mouse_enter(widget, ev) \
	g_signal_connect(G_OBJECT(gtkc_evctrl_motion(GTK_WIDGET(widget))), "enter", G_CALLBACK(gtkc_mouse_enter_cb), ev)

#define gtkc_bind_mouse_leave(widget, ev) \
	g_signal_connect(G_OBJECT(gtkc_evctrl_motion(GTK_WIDGET(widget))), "leave", G_CALLBACK(gtkc_mouse_leave_cb), ev)

#if GTK4_BUG_ON_GESTURE_CLICK_FIXED
#	define gtkc_bind_mouse_press(widget, ev) \
		g_signal_connect(G_OBJECT(gtkc_evctrl_click(GTK_WIDGET(widget))), "pressed", G_CALLBACK(gtkc_mouse_press_cb), ev)

#	define gtkc_bind_mouse_release(widget, ev) \
		g_signal_connect(G_OBJECT(gtkc_evctrl_click(GTK_WIDGET(widget))), "released", G_CALLBACK(gtkc_mouse_release_cb), ev)
#else
#	define gtkc_bind_mouse_press(widget, ev) \
		g_signal_connect(G_OBJECT(gtkc_evctrl_click(GTK_WIDGET(widget))), "event", G_CALLBACK(gtkc_mouse_press_cb), ev)

#	define gtkc_bind_mouse_release(widget, ev) \
		g_signal_connect(G_OBJECT(gtkc_evctrl_click(GTK_WIDGET(widget))), "event", G_CALLBACK(gtkc_mouse_release_cb), ev)
#endif

#define gtkc_unbind_mouse_btn(w, ev, sig) \
		g_signal_handler_disconnect(gtkc_evctrl_click(GTK_WIDGET(widget))), sig);


#define gtkc_bind_mouse_motion(widget, ev) \
	g_signal_connect(G_OBJECT(gtkc_evctrl_motion(GTK_WIDGET(widget))), "motion", G_CALLBACK(gtkc_mouse_motion_cb), ev)


#define gtkc_bind_key_press_fwd(widget, ev) \
	g_signal_connect(G_OBJECT(gtkc_evctrl_key(GTK_WIDGET(widget))), "key-pressed", G_CALLBACK(gtkc_key_press_fwd_cb), ev)

#define gtkc_bind_key_press(widget, ev) \
	g_signal_connect(G_OBJECT(gtkc_evctrl_key(GTK_WIDGET(widget))), "key-pressed", G_CALLBACK(gtkc_key_press_cb), ev)

#define gtkc_bind_key_release(widget, ev) \
	g_signal_connect(G_OBJECT(gtkc_evctrl_key(GTK_WIDGET(widget))), "key-released", G_CALLBACK(gtkc_key_release_cb), ev)

#define gtkc_unbind_key(w, ev, sig)  g_signal_handler_disconnect(G_OBJECT(gtkc_evctrl_key(GTK_WIDGET(w))), sig)



#define gtk2c_bind_win_resize(widget, ev)
#define gtk4c_bind_win_resize(widget, ev) \
	g_signal_connect(G_OBJECT(gtkc_win_surface(GTK_WIDGET(widget))), "layout", G_CALLBACK(gtkc_win_resize_cb), ev)

#define gtkc_bind_win_destroy(widget, ev) \
	g_signal_connect(G_OBJECT(widget), "destroy", G_CALLBACK(gtkc_win_destroy_cb), ev)

#define gtkc_bind_widget_destroy(widget, ev) gtkc_bind_win_destroy(widget, ev)

#define gtkc_bind_win_delete(widget, ev) \
	g_signal_connect(G_OBJECT(widget), "close-request", G_CALLBACK(gtkc_win_delete_cb), ev)

struct gtkc_event_xyz_s;

/* Wrap w so that clicks on it are triggering a callback */
static inline GtkWidget *wrap_bind_click(GtkWidget *w, struct gtkc_event_xyz_s *ev)
{
	gtkc_bind_mouse_press(w, ev);
	return w;
}

int gtkc_clipboard_set_text(GtkWidget *widget, const char *text);
int gtkc_clipboard_get_text(GtkWidget *wdg, void **data, size_t *len);

#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
extern Bool (*gtkc_XQueryPointer)(Display*,Window,Window*,Window*,int*,int*,int*,int*,unsigned int*);
extern int (*gtkc_XWarpPointer)(Display*,Window,Window,int,int,unsigned int,unsigned int,int,int);
int gtkc_resolve_X(void);
#endif

#endif  /* RND_GTK_COMPAT_H */
