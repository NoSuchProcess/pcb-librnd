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

#include <gtk/gtk.h>

#define gtkc_widget_get_window(w) (GDK_WINDOW(GTK_WIDGET(w)->window))

#define gtkc_widget_get_allocation(w, a) \
do { \
	*(a) = (GTK_WIDGET(w)->allocation); \
} while(0) \

#define gtkc_dialog_get_content_area(d)  ((d)->vbox)
#define gtkc_combo_box_entry_new_text()  gtk_combo_box_entry_new_text()

typedef GdkRGBA rnd_gtk_color_t;

/* INTERNAL: set fill/exp for a widget (not part of the API, do not call from elsewhere) */
static inline void gtkci_expfill(GtkWidget *parent, GtkWidget *w)
{
	int h = 1, v = 1;

	/* set fill/exp in parent box if parent is a box: figure parent orientation */
	if (GTK_IS_BOX(parent)) {
		GtkOrientation o = gtk_orientable_get_orientation(GTK_ORIENTABLE(parent));
		if (o == GTK_ORIENTATION_HORIZONTAL) v = 0;
		if (o == GTK_ORIENTATION_VERTICAL) h = 0;
	}

	if (h) {
		gtk_widget_set_halign(w, GTK_ALIGN_FILL);
		gtk_widget_set_hexpand(w, 1);
	}

	if (v) {
		gtk_widget_set_valign(w, GTK_ALIGN_FILL);
		gtk_widget_set_vexpand(w, 1);
	}
}

static inline void gtkc_box_pack_append(GtkWidget *box, GtkWidget *child, gboolean expfill, guint padding)
{
	gtk_box_append(GTK_BOX(box), child);
	if (expfill)
		gtkci_expfill(box, child);
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

#define RND_GTK_EXPOSE_EVENT_SET(obj, val) obj->expose_event = (gboolean (*)(GtkWidget *, rnd_gtk_expose_t *))val
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
	*x = round(dx);
	*y = round(dy);
}

TODO("can we remove this? - check how the resize button looks like")
static inline void gtkc_widget_add_class_style(GtkWidget *w, const char *css_class, char *css_descr)
{
}

static inline void rnd_gtk_set_selected(GtkWidget *widget, int set)
{
	/* race condition... */
	if (set) {
		gtk_label_set_selectable(GTK_LABEL(widget), 1);
		gtk_label_select_region(GTK_LABEL(widget), 0, 100000);
	}
	else
		gtk_label_set_selectable(GTK_LABEL(widget), 0);
}

/* gtk deprecated gtk_widget_hide_all() for some reason; this naive
   implementation seems to work. */
static inline void rnd_gtk_widget_hide_all(GtkWidget *widget)
{
	GtkWidget *ch;
	for(ch = gtk_widget_get_first_child(widget); ch != NULL; ch = gtk_widget_get_next_sibling(ch))
		rnd_gtk_widget_hide_all(ch);
	gtk_widget_hide(widget);
}

/* create a table with known size (all rows and cols created empty) */
static inline GtkWidget *gtkc_table_static(int rows, int cols, gboolean homog)
{
	GtkWidget *tbl = gtk_grid_new();
	gtk_grid_set_column_homogeneous(GTK_GRID(tbl), 1);

	return tbl;
}


/* Attach child in a single cell of the table */
static inline void gtkc_table_attach1(GtkWidget *table, GtkWidget *child, int row, int col)
{
	GtkWidget *bb = gtkc_hbox_new(0, 5); /* this box enables child to grow horizontally */
	gtkc_box_pack_append(bb, child, 1, 0);
	gtk_grid_attach(GTK_GRID(table), bb, row, col, 1, 1);
}

#endif  /* RND_GTK_COMPAT_H */
