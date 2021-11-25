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

static const gchar *get_color_name(rnd_gtk_color_t *color)
{
	static char tmp[16];

	if (!color)
		return "#000000";

	sprintf(tmp, "#%2.2x%2.2x%2.2x", (unsigned)rnd_round(color->red * 255.0), (unsigned)rnd_round(color->green * 255.0), (unsigned)rnd_round(color->blue * 255.0));
	return tmp;
}

/* Returns TRUE only if color_string has been allocated to color. */
static rnd_bool map_color(const rnd_color_t *inclr, rnd_gtk_color_t *color)
{
	/* no need to allocate colors */
	return TRUE;
}

static void set_gl_color_for_gc(rnd_hid_gc_t gc)
{
	render_priv_t *priv = ghidgui->port.render_priv;
	double r, g, b, a;
	rnd_composite_op_t composite_op = hidgl_get_drawing_mode();
	int is_xor = (composite_op == RND_HID_COMP_POSITIVE_XOR);

	if (*gc->pcolor->str == '\0') {
		fprintf(stderr, "set_gl_color_for_gc:  gc->colorname = 0, setting to magenta\n");
		gc->pcolor = rnd_color_magenta;
	}

	if ((priv->current_color_packed == gc->pcolor->packed) && (priv->current_alpha_mult == gc->alpha_mult) && (priv->current_color_xor == is_xor))
		return;

	priv->current_color_packed = (is_xor ? ~gc->pcolor->packed : gc->pcolor->packed);
	priv->current_color_xor = is_xor;
	priv->current_alpha_mult = gc->alpha_mult;

	if (rnd_color_is_drill(gc->pcolor)) {
		r = priv->offlimits_color.fr;
		g = priv->offlimits_color.fg;
		b = priv->offlimits_color.fb;
		a = rnd_conf.appearance.drill_alpha;
	}
	else {
		r = gc->pcolor->fr;
		g = gc->pcolor->fg;
		b = gc->pcolor->fb;

		if (composite_op == RND_HID_COMP_POSITIVE_XOR) {
			r = (double)((unsigned)rnd_round(r * 255.0) ^ ((unsigned)priv->bg_color.r)) / 255.0;
			g = (double)((unsigned)rnd_round(g * 255.0) ^ ((unsigned)priv->bg_color.g)) / 255.0;
			b = (double)((unsigned)rnd_round(b * 255.0) ^ ((unsigned)priv->bg_color.b)) / 255.0;
		}
		a = rnd_conf.appearance.layer_alpha;
	}
	if (1) {
		double maxi, mult;
		a *= gc->alpha_mult;
		if (!priv->trans_lines)
			a = 1.0;
		maxi = r;
		if (g > maxi)
			maxi = g;
		if (b > maxi)
			maxi = b;
		mult = MIN(1 / a, 1 / maxi);
		r = r * mult;
		g = g * mult;
		b = b * mult;
	}

	if (!priv->in_context)
		return;


	/* We need to flush the draw buffer when changing colour so that new primitives
	   don't get merged. This actually isn't a problem with the colour but due to 
	   way that the final render pass iterates through the primitive buffer in 
	   reverse order. If the new primitives are merged with previous ones then they
	   will be drawn in the wrong order. */
	drawgl_flush();

	drawgl_set_colour(r, g, b, a);
}

static void ghid_gl_init_renderer(int *argc, char ***argv, void *vport)
{
	rnd_gtk_port_t *port = vport;
	render_priv_t *priv;

	port->render_priv = priv = g_new0(render_priv_t, 1);

	/* Setup HID function pointers specific to the GL renderer */
	gtk4_gl_hid.end_layer = ghid_gl_end_layer;
}

static void ghid_gl_shutdown_renderer(void *p)
{
	rnd_gtk_port_t *port = p;

	g_free(port->render_priv);
	port->render_priv = NULL;
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
		impl->get_color_name = get_color_name;
		impl->drawing_area_expose = ghid_gl_drawing_area_expose_cb;
		impl->new_drawing_widget = ghid_gdk_new_drawing_widget;
		impl->init_drawing_widget = gtk_gl4_dummy;
		impl->drawing_realize = gtk_gl4_dummy;
		impl->preview_expose = gtk_gl4_dummy;
		impl->init_renderer = gtk_gl4_dummy;
		impl->shutdown_renderer = gtk_gl4_dummy;
	}
}
