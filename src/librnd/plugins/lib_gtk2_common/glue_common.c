#include <librnd/core/global_typedefs.h>
#include "compat.h"

/* Update adj limits to match the current zoom level */
static inline void gtkc_scb_zoom_adjustment(GtkWidget *scrollbar, rnd_coord_t view_size, rnd_coord_t dsg_min, rnd_coord_t dsg_max)
{
	GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(scrollbar));

	adj->page_size = MIN(view_size, dsg_max - dsg_min);
	adj->lower = dsg_min - view_size;
	adj->upper = dsg_max + adj->page_size - dsg_min;
	adj->step_increment = adj->page_size / 100.0;
	adj->page_increment = adj->page_size / 10.0;
	gtk_signal_emit_by_name (GTK_OBJECT(adj), "changed");
}

static inline double gtkc_scb_getval(GtkWidget *scrollbar)
{
	GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(scrollbar));
	return gtk_adjustment_get_value(adj);
}

#define gtkc_scb_setval(scb, val) gtk_range_set_value(GTK_RANGE(scb), val)

#include <librnd/plugins/lib_gtk_common/glue_common.c>

void rnd_gtkg_draw_area_update(rnd_gtk_port_t *port, GdkRectangle *rect)
{
	gdk_window_invalidate_rect(gtkc_widget_get_window(port->drawing_area), rect, FALSE);
}
