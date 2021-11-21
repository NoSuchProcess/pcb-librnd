/*#include <librnd/plugins/hid_gtk2_gl/gtkhid-gl.c>*/

#include <librnd/plugins/lib_gtk4_common/compat.h>
#include <librnd/plugins/lib_gtk_common/rnd_gtk.h>

void ghid_gl_destroy_gc(rnd_hid_gc_t gc)
{
	free(gc);
}

rnd_hid_gc_t ghid_gl_make_gc(rnd_hid_t *hid)
{
	return calloc(128, 1);
}


static GtkWidget *ghid_gdk_new_drawing_widget(rnd_gtk_impl_t *common)
{
	return gtk_gl_area_new();
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
