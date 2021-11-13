#include <gtk/gtk.h>
#include <librnd/plugins/lib_gtk_common/bu_pixbuf.h>
#include "compat.h"

/* XPM */
static const char *resize_grip_xpm[] = {
"17 17 3 1",
" 	c None",
".	c #FFFFFF",
"+	c #9E9A91",
"                .",
"               .+",
"              .++",
"             .++ ",
"            .++  ",
"           .++  .",
"          .++  .+",
"         .++  .++",
"        .++  .++ ",
"       .++  .++  ",
"      .++  .++  .",
"     .++  .++  .+",
"    .++  .++  .++",
"   .++  .++  .++ ",
"  .++  .++  .++  ",
" .++  .++  .++   ",
".++  .++  .++    "};

static gboolean resize_grip_button_press(GtkWidget *area, GdkEventButton *event, gpointer user_data)
{
	if (event->type != GDK_BUTTON_PRESS)
		return TRUE;

	switch (event->button) {
		case 1:
			gtk_window_begin_resize_drag(GTK_WINDOW(gtk_widget_get_toplevel(area)),
				GDK_WINDOW_EDGE_SOUTH_EAST,
				event->button, event->x_root, event->y_root, event->time);
			break;

		case 2:
			gtk_window_begin_move_drag(GTK_WINDOW(gtk_widget_get_toplevel(area)),
					event->button, event->x_root, event->y_root, event->time);
			break;
	}

	return TRUE;
}

void gtkc_create_resize_grip(GtkWidget *parent)
{
	GtkWidget *resize_grip_vbox = gtkc_vbox_new(FALSE, 0);
	GtkWidget *resize_grip = gtk_event_box_new();
	GdkPixbuf *resize_grip_pixbuf = rnd_gtk_xpm2pixbuf(resize_grip_xpm, 1);
	GtkWidget *resize_grip_image = gtk_image_new_from_pixbuf(resize_grip_pixbuf);

	g_object_unref(resize_grip_pixbuf);
	gtk_container_add(GTK_CONTAINER(resize_grip), resize_grip_image);
	gtk_widget_add_events(resize_grip, GDK_BUTTON_PRESS_MASK);
	g_signal_connect(resize_grip, "button_press_event", G_CALLBACK(resize_grip_button_press), NULL);
	gtk_widget_set_tooltip_text(resize_grip, "Left-click to resize the main window\nMid-click to move the window");
	gtk_box_pack_end(GTK_BOX(resize_grip_vbox), resize_grip, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(parent), resize_grip_vbox, FALSE, FALSE, 0);
}


#include <librnd/plugins/lib_gtk_common/dlg_topwin.c>
