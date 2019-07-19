#include "config.h"
#include <gtk/gtk.h>
#include "gui.h"
#include "render.h"
#include "wt_preview.h"


void ghid_draw_area_update(GHidPort *port, GdkRectangle *rect)
{
	gdk_window_invalidate_rect(gtk_widget_get_window(port->drawing_area), rect, FALSE);
}

void ghid_drawing_area_configure_hook(void *out)
{
	ghidgui->common.drawing_area_configure_hook(out);
}

void pcb_gtk_previews_invalidate_lr(pcb_coord_t left, pcb_coord_t right, pcb_coord_t top, pcb_coord_t bottom)
{
	pcb_box_t screen;
	screen.X1 = left; screen.X2 = right;
	screen.Y1 = top; screen.Y2 = bottom;
	pcb_gtk_preview_invalidate(&ghidgui->common, &screen);
}

void pcb_gtk_previews_invalidate_all(void)
{
	pcb_gtk_preview_invalidate(&ghidgui->common, NULL);
}

