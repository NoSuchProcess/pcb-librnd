#include "compat.h"

#include <librnd/plugins/lib_gtk_common/wt_preview.h>

static inline void rnd_gtk_preview_redraw_all(rnd_gtk_preview_t *preview)
{
	GdkWindow *window = gtkc_widget_get_window(GTK_WIDGET(preview));
	if (window != NULL)
		gdk_window_invalidate_rect(window, NULL, FALSE);
}


#include <librnd/plugins/lib_gtk_common/wt_preview.c>
