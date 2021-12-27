#include <librnd/core/global_typedefs.h>
#include "compat.h"

#if GTK4_BUG_ON_SCROLLBAR_FIXED

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

/* This is probably not needed as all fields got written via calls: */
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

printf("setal to %f\n", val);
gtk_adjustment_set_value(adj, 20);
//	gtk_adjustment_set_value(adj, val);
}

#else

#include "gtkc_scrollbar.h"

/* Update adj limits to match the current zoom level */
static inline void gtkc_scb_zoom_adjustment(GtkWidget *scb, rnd_coord_t view_size, rnd_coord_t board_size)
{
	double ps = MIN(view_size, board_size);
	gtkc_scrollbar_set_range(GTKC_SCROLLBAR(scb), -view_size, board_size + ps, ps);
}

#define gtkc_scb_getval(scb)     gtkc_scrollbar_get_val(GTKC_SCROLLBAR(scb))
#define gtkc_scb_setval(scb, v)  gtkc_scrollbar_set_val(GTKC_SCROLLBAR(scb), v)

#endif



#include <librnd/plugins/lib_gtk_common/glue_common.c>

void rnd_gtkg_draw_area_update(rnd_gtk_port_t *port, GdkRectangle *rect)
{
	gtk_gl_area_queue_render(GTK_GL_AREA(port->drawing_area));
}
