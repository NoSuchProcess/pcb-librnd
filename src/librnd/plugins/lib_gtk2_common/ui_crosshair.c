#include "compat.h"

static inline void gtkc_display_get_pointer(GdkDisplay *display, int *pointer_x, int *pointer_y)
{
	gdk_display_get_pointer(display, NULL, pointer_x, pointer_y, NULL);
}

static inline void gtkc_display_warp_pointer(GdkDisplay *display, int pointer_x, int pointer_y)
{
	GdkScreen *screen = gdk_display_get_default_screen(display);
	gdk_display_warp_pointer(display, screen, pointer_x, pointer_y);
}

#include <librnd/plugins/lib_gtk_common/ui_crosshair.c>
