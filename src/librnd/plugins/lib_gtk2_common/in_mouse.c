#include "compat.h"

#define GDKC_HAND2       GDK_HAND2
#define GDKC_WATCH       GDK_WATCH
#define GDKC_DRAPED_BOX  GDK_DRAPED_BOX
#define GDKC_LEFT_PTR    GDK_LEFT_PTR

typedef struct {
	const char *name;
	rnd_gtkc_cursor_type_t shape;
} named_cursor_t;

static const named_cursor_t named_cursors[] = {
	{"question_arrow", GDK_QUESTION_ARROW},
	{"busy",     GDK_WATCH},
	{"left_ptr", GDK_LEFT_PTR},
	{"hand", GDK_HAND1},
	{"crosshair", GDK_CROSSHAIR},
	{"dotbox", GDK_DOTBOX},
	{"pencil", GDK_PENCIL},
	{"up_arrow", GDK_SB_UP_ARROW},
	{"ul_angle", GDK_UL_ANGLE},
	{"pirate", GDK_PIRATE},
	{"xterm", GDK_XTERM},
	{"iron_cross", GDK_IRON_CROSS},
	{NULL, 0}
};

#define RND_GTK_CURSOR_START             (GDK_LAST_CURSOR+10)
#define gdkc_cursor_new(ctx, mc)         gdk_cursor_new(mc)
#define gtkc_mc_custom_idx2shape(idx)    (RND_GTK_CURSOR_START + (idx))

#define gdkc_cursor_new_from_pixbuf(widget, pb, hx, hy) \
	gdk_cursor_new_from_pixbuf(gtk_widget_get_display(widget), pb, hx, hy)

static inline void gtkc_window_set_cursor(GtkWidget *widget, GdkCursor *curs)
{
	GdkWindow *window = gtkc_widget_get_window(widget);

	if (window == NULL)
		return; /* prevent from fatal errors if window doesn't exist */

	gdk_window_set_cursor(window, curs);
}

#include <librnd/plugins/lib_gtk_common/in_mouse.c>
