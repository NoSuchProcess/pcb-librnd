#include "config.h"
#include <gtk/gtk.h>
#include "gui.h"
#include "render.h"

void ghid_draw_area_update(GHidPort *port, GdkRectangle *rect)
{
	gdk_window_invalidate_rect(gtk_widget_get_window(port->drawing_area), rect, FALSE);
}

void ghid_drawing_area_configure_hook(void *out)
{
	ghidgui->common.drawing_area_configure_hook(out);
}
