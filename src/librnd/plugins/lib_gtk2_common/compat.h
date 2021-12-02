/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2019 Tibor Palinkas
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
#ifndef RND_GTK2_COMPAT_H
#define RND_GTK2_COMPAT_H

/*** lib_gtk_common compatibilty layer for GTK2 ***/

#define RND_FOR_GTK2 1

#define RND_GTK_BU_MENU_H_FN <librnd/plugins/lib_gtk2_common/bu_menu.h>

#include <gtk/gtk.h>

#include "compat_priv.h"

#define RND_GTK_KEY(keyname)   RND_GTK2_KEY(keyname)
#define rnd_gtkc_cursor_type_t GdkCursorType

#define GTKC_LITERAL_COLOR(a,r,g,b) {a,r,g,b}

#define gtkc_box_pack_append(parent_widget, child_widget, expfill, padding) \
	gtk_box_pack_start(GTK_BOX(parent_widget), child_widget, expfill, expfill, padding)

#define gtkc_box_pack_append_start(parent_widget, child_widget, expfill, padding) \
	gtk_box_pack_start(GTK_BOX(parent_widget), child_widget, expfill, expfill, padding)

#define gtkc_widget_get_allocation(w, a) \
do { \
	*(a) = (GTK_WIDGET(w)->allocation); \
} while(0) \

#define gtkc_combo_box_entry_new_text()  gtk_combo_box_entry_new_text()
#define gtkc_combo_box_get_entry(combo) GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo)))


#define gtkci_dialog_get_content_area(d)  ((d)->vbox)
static inline void gtkc_dlg_add_content(GtkDialog *dlg, GtkWidget *child)
{
	GtkWidget *content_area = gtkci_dialog_get_content_area(dlg);
	gtk_container_set_border_width(GTK_CONTAINER(child), 6);
	gtk_container_add_with_properties(GTK_CONTAINER(content_area), child, "expand", TRUE, "fill", TRUE, NULL);
}
#undef gtkci_dialog_get_content_area


typedef GdkColor rnd_gtk_color_t;

static inline GtkWidget *gtkc_hbox_new(gboolean homogenous, gint spacing)
{
	return gtk_hbox_new(homogenous, spacing);
}

static inline GtkWidget *gtkc_vbox_new(gboolean homogenous, gint spacing)
{
	return gtk_vbox_new(homogenous, spacing);
}

static inline GtkWidget *gtkc_hpaned_new()
{
	return gtk_hpaned_new();
}

static inline GtkWidget *gtkc_vpaned_new()
{
	return gtk_vpaned_new();
}

/* color button */

static inline GtkWidget *gtkc_color_button_new_with_color(rnd_gtk_color_t *color)
{
	return gtk_color_button_new_with_color(color);
}

static inline void gtkc_color_button_set_color(GtkWidget *button, rnd_gtk_color_t *color)
{
	gtk_color_button_set_color(GTK_COLOR_BUTTON(button), color);
}

static inline void gtkc_color_button_get_color(GtkWidget *button, rnd_gtk_color_t *color)
{
	gtk_color_button_get_color(GTK_COLOR_BUTTON(button), color);
}

static inline GtkWidget *gtkc_combo_box_text_new(void)
{
	return gtk_combo_box_new_text();
}

static inline void gtkc_combo_box_text_append_text(GtkWidget *combo, const gchar *text)
{
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), text);
}

static inline void gtkc_combo_box_text_prepend_text(GtkWidget *combo, const gchar *text)
{
	gtk_combo_box_prepend_text(GTK_COMBO_BOX(combo), text);
}

static inline void gtkc_combo_box_text_remove(GtkWidget *combo, gint position)
{
	gtk_combo_box_remove_text(GTK_COMBO_BOX(combo), position);
}

static inline gchar *gtkc_combo_box_text_get_active_text(GtkWidget *combo)
{
	return gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo));
}

static inline GtkWidget *gtkc_trunctext_new(const gchar *str)
{
	GtkWidget *w = gtk_label_new(str);
	gtk_widget_set_size_request(w, 1, 1);

	return w;
}

typedef GdkEventExpose rnd_gtk_expose_t;

static inline void gtkc_scrolled_window_add_with_viewport(GtkWidget *scrolled, GtkWidget *child)
{
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), child);
}

static inline GdkWindow * gdkc_window_get_pointer(GtkWidget *w, gint *x, gint *y, GdkModifierType *mask)
{
	return gdk_window_get_pointer(gtkc_widget_get_window(w), x, y, mask);
}

static inline void rnd_gtk_set_selected(GtkWidget *widget, int set)
{
	GtkStateType st = GTK_WIDGET_STATE(widget);
	/* race condition... */
	if (set)
		gtk_widget_set_state(widget, st | GTK_STATE_SELECTED);
	else
		gtk_widget_set_state(widget, st & (~GTK_STATE_SELECTED));
}

/* gtk deprecated gtk_widget_hide_all() for some reason; this naive
   implementation seems to work. */
static inline void gtkc_widget_hide_all(GtkWidget *widget)
{
	if(GTK_IS_CONTAINER(widget)) {
		GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
		while ((children = g_list_next(children)) != NULL)
			gtkc_widget_hide_all(GTK_WIDGET(children->data));
	}
	gtk_widget_hide(widget);
}

#define gtkc_widget_show_all(w) gtk_widget_show_all(w)

/* create a table with known size (all rows and cols created empty) */
static inline GtkWidget *gtkc_table_static(int rows, int cols, gboolean homog)
{
	return gtk_table_new(rows, cols, homog);
}


/* Attach child in a single cell of the table */
static inline void gtkc_table_attach1(GtkWidget *table, GtkWidget *child, int row, int col)
{
	gtk_table_attach(GTK_TABLE(table), child, col, col+1, row, row+1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 4, 4);
}

#define GTKC_TREE_FORWARD_EVENT \
	do { \
		GtkWidgetClass *class = GTK_WIDGET_GET_CLASS(tree_view); \
		class->key_press_event(GTK_WIDGET(tree_view), fwd); \
	} while(0)


#define gtkc_entry_get_text(entry)        gtk_entry_get_text(entry)
#define gtkc_entry_set_text(entry, str)   gtk_entry_set_text(entry, str)
#define gtkc_entry_set_width_chars(e, w)  gtk_entry_set_width_chars(e, w)
#define gtkc_widget_modify_bg(w, st, c)   gtk_widget_modify_bg(w, st, c)
#define gtkc_frame_set_child(fr, ch)      gtk_container_add(GTK_CONTAINER(fr), ch)
#define gtkc_scrolled_window_new()        gtk_scrolled_window_new(NULL, NULL)
#define gtkc_scrolled_window_set_child(sw, ch) gtk_container_add(GTK_CONTAINER(sw), ch)
#define gtkc_window_set_child(sw, ch)     gtk_container_add(GTK_CONTAINER(sw), ch)
#define gtkc_button_set_child(btn, ch)    gtk_container_add(GTK_CONTAINER(btn), ch)
#define gtkc_button_set_image(btn, img)   gtk_button_set_image(btn, img)
#define gtkc_window_destroy(win)          gtk_widget_destroy(win)
#define gtkc_paned_pack1(pane, ch, resiz) gtk_paned_pack1(pane, ch, resiz, FALSE)
#define gtkc_paned_pack2(pane, ch, resiz) gtk_paned_pack2(pane, ch, resiz, FALSE)
#define gtkc_window_set_role(win, id)     gtk_window_set_role(win, id)
#define gtkc_window_resize(win, x, y)     gtk_window_resize(win, x, y)
#define gtkc_window_move(win, x, y)       gtk_window_move(win, x, y)
#define gtkc_dialog_run(dlg, modal)       gtk_dialog_run(dlg)
#define gtkc_widget_destroy(w)            gtk_widget_destroy(w)
#define gtkc_main_quit()                  gtk_main_quit()
#define gtkc_bgcolor_box_new()            gtk_event_box_new()
#define gtkc_bgcolor_box_set_child(b, ch) gtk_container_add(GTK_CONTAINER(b), ch)
#define gtkc_widget_set_focusable(w)      GTK_WIDGET_SET_FLAGS(w, GTK_CAN_FOCUS)
#define GDKC_MOD1_MASK                    GDK_MOD1_MASK
#define gtkc_check_button_set_active(b, act) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b), act)
#define gtkc_check_button_get_active(b)   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b));
#define GTKC_CHECK_BUTTON_TOGGLE_SIG      "clicked"
#define gtkc_workaround_image_scale_bug(i,p)
#define gtkc_workaround_image_button_border_bug(i,p)

static inline void gtkc_wait_pending_events(void)
{
	while(gtk_events_pending())
		gtk_main_iteration_do(0);
}


/*** Event/signal compatibility ***/

/* Make sure the specified widget is capable of accepting event classes
   that are enabled in the args */
static inline void gtkc_setup_events(GtkWidget *dwg, int mbutton, int mscroll, int motion, int key, int expose, int enter)
{
	gint events = GDK_FOCUS_CHANGE_MASK;

	if (expose)  events |= GDK_EXPOSURE_MASK;
	if (enter)   events |= GDK_LEAVE_NOTIFY_MASK | GDK_ENTER_NOTIFY_MASK;
	if (mbutton) events |= GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK;
	if (mscroll) events |= GDK_SCROLL_MASK;
	if (key)     events |= GDK_KEY_RELEASE_MASK | GDK_KEY_PRESS_MASK;
	if (motion)  events |= GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK;

	gtk_widget_add_events(dwg, events);
}

/* In the following all rs is gtkc_event_xyz_t, filled in by the caller */

#define gtkc_bind_resize_dwg(widget, rs) \
	g_signal_connect(G_OBJECT(widget), "configure_event", G_CALLBACK(gtkc_resize_dwg_cb), rs);

#define gtkc_bind_mouse_scroll(widget, ev) \
	g_signal_connect(G_OBJECT(widget), "scroll_event", G_CALLBACK(gtkc_mouse_scroll_cb), ev);

#define gtkc_bind_mouse_enter(widget, ev) \
	g_signal_connect(G_OBJECT(widget), "enter_notify_event", G_CALLBACK(gtkc_mouse_enter_cb), ev);

#define gtkc_bind_mouse_leave(widget, ev) \
	g_signal_connect(G_OBJECT(widget), "leave_notify_event", G_CALLBACK(gtkc_mouse_leave_cb), ev);

#define gtkc_bind_mouse_press(widget, ev) \
	g_signal_connect(G_OBJECT(widget), "button_press_event", G_CALLBACK(gtkc_mouse_press_cb), ev);

#define gtkc_bind_mouse_release(widget, ev) \
	g_signal_connect(G_OBJECT(widget), "button_release_event", G_CALLBACK(gtkc_mouse_release_cb), ev);

#define gtkc_bind_mouse_motion(widget, ev) \
	g_signal_connect(G_OBJECT(widget), "motion_notify_event", G_CALLBACK(gtkc_mouse_motion_cb), ev);


#define gtkc_bind_key_press_fwd(widget, ev) \
	g_signal_connect(G_OBJECT(widget), "key_press_event", G_CALLBACK(gtkc_key_press_fwd_cb), ev);

#define gtkc_bind_key_press(widget, ev) \
	g_signal_connect(G_OBJECT(widget), "key_press_event", G_CALLBACK(gtkc_key_press_cb), ev);

#define gtkc_bind_key_release(widget, ev) \
	g_signal_connect(G_OBJECT(widget), "key_release_event", G_CALLBACK(gtkc_key_release_cb), ev);

#define gtkc_unbind_key(w, ev, sig)  g_signal_handler_disconnect(w, sig)


#define gtk4c_bind_win_resize(widget, ev)
#define gtk2c_bind_win_resize(widget, ev) \
	g_signal_connect(G_OBJECT(widget), "configure_event", G_CALLBACK(gtkc_win_resize_cb), ev);


#define gtkc_bind_win_destroy(widget, ev) \
	g_signal_connect(G_OBJECT(widget), "destroy", G_CALLBACK(gtkc_win_destroy_cb), ev);

#define gtkc_bind_widget_destroy(widget, ev) gtkc_bind_win_destroy(widget, ev)

#define gtkc_bind_win_delete(widget, ev) \
	g_signal_connect(G_OBJECT(widget), "delete_event", G_CALLBACK(gtkc_win_delete_cb), ev);

struct gtkc_event_xyz_s;

/* Wrap w so that clicks on it are triggering a callback */
static inline GtkWidget *wrap_bind_click(GtkWidget *w, struct gtkc_event_xyz_s *ev)
{
	GtkWidget *event_box = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(event_box), w);

	gtkc_bind_mouse_press(event_box, ev);

	return event_box;
}

int gtkc_clipboard_set_text(GtkWidget *widget, const char *text);
int gtkc_clipboard_get_text(GtkWidget *wdg, void **data, size_t *len);

#endif  /* RND_GTK_COMPAT_H */
