#include "compat.h"

#include <librnd/plugins/lib_gtk_common/wt_preview.h>

static inline void rnd_gtk_preview_redraw_all(rnd_gtk_preview_t *preview)
{
	gtk_gl_area_queue_render(GTK_GL_AREA(preview));
}

static gboolean rnd_gtk_preview_expose(GtkWidget *widget, rnd_gtk_expose_t *ev);

static gboolean rnd_gtk_preview_render(GtkGLArea *area, GdkGLContext *context)
{
	return rnd_gtk_preview_expose(GTK_WIDGET(area), NULL);
}


#define RND_GTK_EXPOSE_EVENT_SET(obj) \
	((GtkGLAreaClass *)obj)->render = rnd_gtk_preview_render


#include <librnd/plugins/lib_gtk_common/wt_preview.c>
