/*#include <librnd/plugins/hid_gtk2_gl/gtkhid-gl.c>*/

#include <librnd/plugins/lib_gtk4_common/compat.h>
#include <librnd/plugins/lib_gtk_common/rnd_gtk.h>
#include <epoxy/gl.h>

extern rnd_hid_t gtk4_gl_hid;

/* Sets ghidgui->port.u_gc to the "right" GC to use (wrt mask or window) */
#define USE_GC(gc) \
do { \
	if (gc->me_pointer != &gtk4_gl_hid) { \
		fprintf(stderr, "Fatal: GC from another HID passed to GTK HID\n"); \
		abort(); \
	} \
	if (!use_gc(gc)) return; \
} while(0)

#include <librnd/plugins/lib_gtk_common/gtk_gl_common.c>

/* We need to set up our state when we realize the GtkGLArea widget */
static void realize(GtkWidget *widget)
{
	gtk_gl_area_make_current(GTK_GL_AREA(widget));
}

/* We should tear down the state when unrealizing */
static void unrealize(GtkWidget *widget)
{
	gtk_gl_area_make_current(GTK_GL_AREA(widget));
}

static gboolean render(GtkGLArea *area, GdkGLContext *context)
{
	if (gtk_gl_area_get_error(area) != NULL)
		return FALSE;

	/* Clear the viewport */
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	/* Draw our object */
/*	draw_triangle();*/

	/* Flush the contents of the pipeline */
	glFlush();

	return TRUE;
}

static GtkWidget *ghid_gdk_new_drawing_widget(rnd_gtk_impl_t *common)
{
	GtkWidget *drw = gtk_gl_area_new();

	gtk_widget_set_hexpand(drw, TRUE);
	gtk_widget_set_vexpand(drw, TRUE);
	gtk_widget_set_size_request(drw, 100, 100);

	g_signal_connect(drw, "realize", G_CALLBACK(realize), NULL);
	g_signal_connect(drw, "unrealize", G_CALLBACK(unrealize), NULL);
	g_signal_connect(drw, "render", G_CALLBACK(render), NULL);

	return drw;
}

static gboolean ghid_gl_drawing_area_expose_cb(GtkWidget *widget, rnd_gtk_expose_t *ev, void *vport)
{
	return ghid_gl_drawing_area_expose_cb_common(&gtk4_gl_hid, widget, ev, vport);
}


static int gtk_gl4_dummy(int foo, ...)
{
	return 0;
}

void ghid_gl_install(rnd_gtk_impl_t *impl, rnd_hid_t *hid)
{
	ghid_gl_install_common(impl, hid);

fprintf(stderr, "No GL rendering for gtk4 yet\n");
	if (impl != NULL) {
		impl->new_drawing_widget = ghid_gdk_new_drawing_widget;
		impl->init_drawing_widget = gtk_gl4_dummy;
		impl->drawing_realize = gtk_gl4_dummy;
		impl->preview_expose = gtk_gl4_dummy;
		impl->init_renderer = gtk_gl4_dummy;
		impl->shutdown_renderer = gtk_gl4_dummy;
	}
}
