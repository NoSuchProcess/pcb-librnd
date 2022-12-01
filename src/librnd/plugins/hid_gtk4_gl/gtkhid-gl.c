#include <librnd/plugins/lib_gtk4_common/compat.h>
#include <librnd/plugins/lib_gtk_common/rnd_gtk.h>
#include <librnd/plugins/lib_gtk_common/wt_preview.h>
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
	color->red = inclr->fr;
	color->green = inclr->fg;
	color->blue = inclr->fb;
	color->alpha = 1.0;
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
	}
	else {
		r = gc->pcolor->fr;
		g = gc->pcolor->fg;
		b = gc->pcolor->fb;
	}
	a = rnd_conf.appearance.layer_alpha;

	if (!priv->in_context)
		return;


	/* We need to flush the draw buffer when changing color so that new primitives
	   don't get merged. This actually isn't a problem with the color but due to 
	   way that the final render pass iterates through the primitive buffer in 
	   reverse order. If the new primitives are merged with previous ones then they
	   will be drawn in the wrong order. */
	hidgl_flush_drawing();

	hidgl_set_color(r, g, b, a);
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

static void ghid_gl_init_drawing_widget(GtkWidget *widget, void *port_)
{
/*	rnd_gtk_port_t *port = port_;*/
	gtk_gl_area_set_has_stencil_buffer(GTK_GL_AREA(widget), 1);
}

static gboolean ghid_gl_start_drawing(rnd_gtk_port_t *port)
{
	GtkWidget *widget = port->drawing_area;

	port->render_priv->in_context = rnd_true;
	gtk_gl_area_make_current(GTK_GL_AREA(widget));

	return TRUE;
}

static void ghid_gl_end_drawing(rnd_gtk_port_t *port)
{
	hidgl_flush();

	port->render_priv->in_context = rnd_false;

	/* No need to explicitly end drawing in current GL-context */
}

static void ghid_gl_port_drawing_realize_cb(GtkWidget *widget, gpointer data)
{
	/* nop: old gtk2 bugfix */
}

static gboolean ghid_gl_drawing_area_expose_cb(GtkWidget *widget, rnd_gtk_expose_t *ev, void *vport)
{
	return ghid_gl_drawing_area_expose_cb_common(&gtk4_gl_hid, widget, ev, vport);
}

static gboolean ghid_gl_preview_expose(GtkWidget *widget, rnd_gtk_expose_t *ev, rnd_hid_preview_expose_t expcall, rnd_hid_expose_ctx_t *ctx)
{
	GtkAllocation allocation;
	rnd_design_t *hidlib = ghidgui->hidlib;
	rnd_gtk_preview_t *preview = RND_GTK_PREVIEW(widget);

	gtkc_widget_get_allocation(widget, &allocation);

	/* make GL-context "current" */
	gtk_gl_area_make_current(GTK_GL_AREA(widget));
	if (!preview->draw_inited) {
		hidgl_new_context();
		preview->draw_inited = 1;
	}

	ghid_gl_preview_expose_common(&gtk4_gl_hid, hidlib, ev, expcall, ctx, allocation.width, allocation.height);

	hidgl_flush();

	/* end drawing to current GL-context: no need */

	return FALSE;
}



/* We need to set up our state when we realize the top window GtkGLArea widget */
static void realize(GtkWidget *widget, void *udata)
{
	gtk_gl_area_make_current(GTK_GL_AREA(widget));

	/* check if we have a low level draw */
	if (hidgl_init() != 0) {
		rnd_message(RND_MSG_ERROR, "Error: hidgl_init(): failed to find a working opengl backend\n");
		return;
	}

	hidgl_new_context();
}

/* We should tear down the state when unrealizing */
static void unrealize(GtkWidget *widget, void *udata)
{
	gtk_gl_area_make_current(GTK_GL_AREA(widget));
}

static gboolean render(GtkGLArea *area, GdkGLContext *context, rnd_gtk_port_t *port)
{
	GtkWidget *widget = port->drawing_area;

	if (gtk_gl_area_get_error(area) != NULL)
		return FALSE;

	/* Clear the viewport */
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	/* Draw our object */
	ghid_gl_drawing_area_expose_cb_common(&gtk4_gl_hid, widget, NULL, port);

	/* Flush the contents of the pipeline */
	hidgl_flush();

	return TRUE;
}

static GtkWidget *ghid_gl_new_drawing_widget(rnd_gtk_impl_t *common)
{
	GtkWidget *drw = gtk_gl_area_new();

	gtk_widget_set_hexpand(drw, TRUE);
	gtk_widget_set_vexpand(drw, TRUE);
	gtk_widget_set_size_request(drw, 100, 100);

	g_signal_connect(drw, "realize", G_CALLBACK(realize), NULL);
	g_signal_connect(drw, "unrealize", G_CALLBACK(unrealize), NULL);
	g_signal_connect(drw, "render", G_CALLBACK(render), common->gport);

	return drw;
}

int ghid_gl_install(rnd_gtk_impl_t *impl, rnd_hid_t *hid)
{
	if (ghid_gl_install_common(impl, hid) != 0)
		return -1;

	if (impl != NULL) {
		impl->get_color_name = get_color_name;
		impl->drawing_area_expose = ghid_gl_drawing_area_expose_cb;
		impl->new_drawing_widget = ghid_gl_new_drawing_widget;
		impl->init_drawing_widget = ghid_gl_init_drawing_widget;
		impl->drawing_realize = ghid_gl_port_drawing_realize_cb;
		impl->preview_expose = ghid_gl_preview_expose;
		impl->init_renderer = ghid_gl_init_renderer;
		impl->shutdown_renderer = ghid_gl_shutdown_renderer;
	}

	return 0;
}
