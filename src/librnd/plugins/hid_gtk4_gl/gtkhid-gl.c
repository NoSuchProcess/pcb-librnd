/*#include <librnd/plugins/hid_gtk2_gl/gtkhid-gl.c>*/

#include <librnd/plugins/lib_gtk4_common/compat.h>
#include <librnd/plugins/lib_gtk_common/rnd_gtk.h>
#include <epoxy/gl.h>

void ghid_gl_destroy_gc(rnd_hid_gc_t gc)
{
	free(gc);
}

rnd_hid_gc_t ghid_gl_make_gc(rnd_hid_t *hid)
{
	return calloc(128, 1);
}



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


static int gtk_gl4_dummy(int foo, ...)
{
	return 0;
}

void ghid_gl_install(rnd_gtk_impl_t *impl, rnd_hid_t *hid)
{
TODO("remove this and link gtkhid-gl instead");
fprintf(stderr, "No GL rendering for gtk4 yet\n");
	if (impl != NULL) {
		impl->new_drawing_widget = ghid_gdk_new_drawing_widget;
		impl->init_drawing_widget = gtk_gl4_dummy;
		impl->drawing_realize = gtk_gl4_dummy;
		impl->drawing_area_expose = gtk_gl4_dummy;
		impl->preview_expose = gtk_gl4_dummy;
		impl->set_special_colors = gtk_gl4_dummy;
		impl->init_renderer = gtk_gl4_dummy;
		impl->screen_update = gtk_gl4_dummy;
		impl->draw_grid_local = gtk_gl4_dummy;
		impl->drawing_area_configure_hook = gtk_gl4_dummy;
		impl->shutdown_renderer = gtk_gl4_dummy;
		impl->get_color_name = gtk_gl4_dummy;
		impl->map_color = gtk_gl4_dummy;
		impl->draw_pixmap = gtk_gl4_dummy;
	}

	if (hid != NULL) {
		hid->invalidate_lr = gtk_gl4_dummy;
		hid->invalidate_all = gtk_gl4_dummy;
		hid->notify_crosshair_change = gtk_gl4_dummy;
		hid->notify_mark_change = gtk_gl4_dummy;
		hid->set_layer_group = gtk_gl4_dummy;
		hid->make_gc = ghid_gl_make_gc;
		hid->destroy_gc = ghid_gl_destroy_gc;
		hid->set_color = gtk_gl4_dummy;
		hid->set_line_cap = gtk_gl4_dummy;
		hid->set_line_width = gtk_gl4_dummy;
		hid->set_draw_xor = gtk_gl4_dummy;
		hid->draw_line = gtk_gl4_dummy;
		hid->draw_arc = gtk_gl4_dummy;
		hid->draw_rect = gtk_gl4_dummy;
		hid->fill_circle = gtk_gl4_dummy;
		hid->fill_polygon = gtk_gl4_dummy;
		hid->fill_polygon_offs = gtk_gl4_dummy;
		hid->fill_rect = gtk_gl4_dummy;

		hid->set_drawing_mode = gtk_gl4_dummy;
		hid->render_burst = gtk_gl4_dummy;
	}

}
