#include "compat.h"

#include <librnd/plugins/lib_gtk_common/wt_preview.h>

static inline void rnd_gtk_preview_redraw_all(rnd_gtk_preview_t *preview)
{
	TODO("implement this for gtk4: gtk_gl_area_queue_render() - ignore rectangle, gl redraws everything");
}

static gboolean rnd_gtk_preview_expose(GtkWidget *widget, rnd_gtk_expose_t *ev);

static gboolean rnd_gtk_preview_render(GtkGLArea *area, GdkGLContext *context, void *udata)
{
	return rnd_gtk_preview_expose(GTK_WIDGET(area), NULL);
}


#define RND_GTK_EXPOSE_EVENT_SET(obj) \
	g_signal_connect(obj, "render", G_CALLBACK(rnd_gtk_preview_render), NULL);


#include <librnd/plugins/lib_gtk_common/wt_preview.c>
