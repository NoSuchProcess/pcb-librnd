#include <librnd/core/global_typedefs.h>
#include "compat.h"

/* Update adj limits to match the current zoom level */
static inline void rnd_gtkc_zoom_adjustment(GtkAdjustment *adj, rnd_coord_t view_size, rnd_coord_t board_size)
{
	double ps = MIN(view_size, board_size);

	gtk_adjustment_set_page_size(adj, ps);
	gtk_adjustment_set_lower(adj, -view_size);
	gtk_adjustment_set_upper(adj, board_size + adj->page_size);

	gtk_adjustment_set_step_increment(adj, ps / 100.0);
	gtk_adjustment_set_page_increment(adj, ps / 10.0);

TODO("This is probably not needed as all fields got written via calls");
/*	gtk_signal_emit_by_name (GTK_OBJECT(adj), "changed");*/
}

#include <librnd/plugins/lib_gtk_common/glue_common.c>
