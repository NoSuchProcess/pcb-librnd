#include <librnd/core/global_typedefs.h>
#include "compat.h"

/* Update adj limits to match the current zoom level */
static inline void rnd_gtkc_zoom_adjustment(GtkAdjustment *adj, rnd_coord_t view_size, rnd_coord_t board_size)
{
	adj->page_size = MIN(view_size, board_size);
	adj->lower = -view_size;
	adj->upper = board_size + adj->page_size;
	adj->step_increment = adj->page_size / 100.0;
	adj->page_increment = adj->page_size / 10.0;
	gtk_signal_emit_by_name (GTK_OBJECT(adj), "changed");
}

#include <librnd/plugins/lib_gtk_common/glue_common.c>