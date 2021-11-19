#include "compat.h"

#define GDKC_HAND2       "pointer"
#define GDKC_WATCH       "progress"
#define GDKC_DRAPED_BOX  "all-scroll"
#define GDKC_LEFT_PTR    "default"

typedef struct {
	const char *name;
	rnd_gtkc_cursor_type_t shape;
} named_cursor_t;

static const named_cursor_t named_cursors[] = {
	{"question_arrow", "help"},
	{"left_ptr",       "default"},
	{"hand",           "grabbing"},
	{"crosshair",      "crosshair"},
	{"dotbox",         "cell"},
	{"pencil",         "context-menu"},
	{"up_arrow",       "n-resize"},
	{"ul_angle",       "nw-resize"},
	{"pirate",         "not-allowed"},
	{"xterm",          "text"},
	{"iron_cross",     "move"},
	{NULL, 0}
};

static inline GdkCursor *gdkc_cursor_new(void *ctx, const char *name)
{
	return gdk_cursor_new_from_name(name, NULL);
}

static const char GTKC_MC_CUSTOM_SHAPE_NAME[] = "rnd-custom-cursor";

#define gtkc_mc_custom_idx2shape(idx)    GTKC_MC_CUSTOM_SHAPE_NAME
#define gtkc_window_set_cursor(wdg, cur) gtk_widget_set_cursor(wdg, cur)


static GdkCursor *gdkc_cursor_new_from_pixbuf(GtkWidget *widget, GdkPixbuf *pb, int hx, int hy)
{
	GdkCursor *res;
	GdkTexture *texture = gdk_texture_new_for_pixbuf(pb);
	res = gdk_cursor_new_from_texture(texture, hx, hy, NULL);
	g_object_unref(pb);
	g_object_unref(texture);
	return res;
}


#include <librnd/plugins/lib_gtk_common/in_mouse.c>
