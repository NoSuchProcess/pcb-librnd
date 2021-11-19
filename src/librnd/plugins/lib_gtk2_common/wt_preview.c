#include "compat.h"

#include <librnd/plugins/lib_gtk_common/wt_preview.h>

static inline void rnd_gtk_preview_redraw_all(rnd_gtk_preview_t *preview)
{
	GdkWindow *window = gtkc_widget_get_window(GTK_WIDGET(preview));
	if (window != NULL)
		gdk_window_invalidate_rect(window, NULL, FALSE);
}

static gboolean rnd_gtk_preview_expose(GtkWidget *widget, rnd_gtk_expose_t *ev);

static gboolean rnd_gtk_preview_render_cb(GtkWidget *widget, rnd_gtk_expose_t *ev)
{
	return rnd_gtk_preview_expose(widget, ev);
}

#define RND_GTK_EXPOSE_EVENT_SET(obj) obj->expose_event = (gboolean (*)(GtkWidget *, GdkEventExpose *))rnd_gtk_preview_render_cb


#include <librnd/plugins/lib_gtk_common/wt_preview.c>
