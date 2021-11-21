#include <librnd/core/global_typedefs.h>
#include "compat.h"

/* Update adj limits to match the current zoom level */
static inline void gtkc_scb_zoom_adjustment(GtkWidget *scrollb, rnd_coord_t view_size, rnd_coord_t board_size)
{
	GtkAdjustment *adj = gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(scrollb));
	double ps = MIN(view_size, board_size);

	gtk_adjustment_set_page_size(adj, ps);
	gtk_adjustment_set_lower(adj, -view_size);
	gtk_adjustment_set_upper(adj, board_size + ps);

	gtk_adjustment_set_step_increment(adj, ps / 100.0);
	gtk_adjustment_set_page_increment(adj, ps / 10.0);

TODO("This is probably not needed as all fields got written via calls");
/*	gtk_signal_emit_by_name (GTK_OBJECT(adj), "changed");*/
}

static inline double gtkc_scb_getval(GtkWidget *scrollb)
{
	GtkAdjustment *adj = gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(scrollb));
	return gtk_adjustment_get_value(adj);
}

static inline void gtkc_scb_setval(GtkWidget *scrollb, double val)
{
	GtkAdjustment *adj = gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(scrollb));
	gtk_adjustment_set_value(adj, val);
}

#include <librnd/plugins/lib_gtk_common/glue_common.c>

void rnd_gtkg_draw_area_update(rnd_gtk_port_t *port, GdkRectangle *rect)
{
	gtk_gl_area_queue_render(GTK_GL_AREA(port->drawing_area));
}
