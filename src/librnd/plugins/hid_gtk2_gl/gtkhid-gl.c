#include "config.h"

#include <stdio.h>

#include <librnd/core/color_cache.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/hidlib_conf.h>
#include <librnd/core/pixmap.h>
#include <librnd/core/globalconst.h>

#include <librnd/plugins/lib_hid_common/clip.h>
#include <librnd/plugins/lib_gtk_common/hid_gtk_conf.h>
#include <librnd/plugins/lib_gtk_common/lib_gtk_config.h>

#include <librnd/plugins/lib_gtk2_common/compat.h>
#include <librnd/plugins/lib_gtk_common/rnd_gtk.h>
#include <librnd/plugins/lib_gtk_common/coord_conv.h>
#include <librnd/plugins/lib_gtk_common/glue_common.h>

#include <librnd/plugins/lib_hid_gl/opengl.h>
#include <gtk/gtkgl.h>
#include <librnd/plugins/lib_hid_gl/hidgl.h>

#include <librnd/plugins/lib_gtk_common/hid_gtk_conf.h>

extern rnd_hid_t gtk2_gl_hid;

/* Sets ghidgui->port.u_gc to the "right" GC to use (wrt mask or window) */
#define USE_GC(gc) \
do { \
	if (gc->me_pointer != &gtk2_gl_hid) { \
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

	sprintf(tmp, "#%2.2x%2.2x%2.2x", (color->red >> 8) & 0xff, (color->green >> 8) & 0xff, (color->blue >> 8) & 0xff);
	return tmp;
}

/* Returns TRUE only if color_string has been allocated to color. */
static rnd_bool map_color(const rnd_color_t *inclr, rnd_gtk_color_t *color)
{
	static GdkColormap *colormap = NULL;

	if (!color || !ghidgui->port.top_window)
		return FALSE;
	if (colormap == NULL)
		colormap = gtk_widget_get_colormap(ghidgui->port.top_window);
	if (color->red || color->green || color->blue)
		gdk_colormap_free_colors(colormap, color, 1);

	color->red = rnd_gtk_color8to16(inclr->r);
	color->green = rnd_gtk_color8to16(inclr->g);
	color->blue = rnd_gtk_color8to16(inclr->b);
	gdk_color_alloc(colormap, color);

	return TRUE;
}

static void set_gl_color_for_gc(rnd_hid_gc_t gc)
{
	render_priv_t *priv = ghidgui->port.render_priv;
	static GdkColormap *colormap = NULL;
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

	if (colormap == NULL)
		colormap = gtk_widget_get_colormap(ghidgui->port.top_window);

	if (rnd_color_is_drill(gc->pcolor)) {
		r = priv->offlimits_color.fr;
		g = priv->offlimits_color.fg;
		b = priv->offlimits_color.fb;
		a = rnd_conf.appearance.drill_alpha;
	}
	else {
		rnd_gtk_color_cache_t *cc;

		if (!priv->ccache_inited) {
			rnd_clrcache_init(&priv->ccache, sizeof(rnd_gtk_color_cache_t), NULL);
			priv->ccache_inited = 1;
		}
		cc = rnd_clrcache_get(&priv->ccache, gc->pcolor, 1);
		if (!cc->color_set) {
			const char *clr_str = gc->pcolor->str;
			char clr_tmp[8];

			if (gc->pcolor->str[7] != '\0') {
				/* convert #rrggbbaa: truncate aa, gdk_color_parse doesn't like it */
				memcpy(clr_tmp, gc->pcolor->str, 7);
				clr_tmp[7] = '\0';
				clr_str = clr_tmp;
			}
			if (gdk_color_parse(clr_str, &cc->color))
				gdk_color_alloc(colormap, &cc->color);
			else
				gdk_color_white(colormap, &cc->color);
			cc->red = cc->color.red / 65535.;
			cc->green = cc->color.green / 65535.;
			cc->blue = cc->color.blue / 65535.;
			cc->color_set = 1;
		}
		if (composite_op == RND_HID_COMP_POSITIVE_XOR) {
			if (!cc->xor_set) {
				cc->xor_color.red = cc->color.red ^ ((unsigned)priv->bg_color.r << 8);
				cc->xor_color.green = cc->color.green ^ ((unsigned)priv->bg_color.g << 8);
				cc->xor_color.blue = cc->color.blue ^ ((unsigned)priv->bg_color.b << 8);
				gdk_color_alloc(colormap, &cc->xor_color);
				cc->red = cc->color.red / 65535.;
				cc->green = cc->color.green / 65535.;
				cc->blue = cc->color.blue / 65535.;
				cc->xor_set = 1;
			}
			r = cc->xor_color.red / 65535.0;
			g = cc->xor_color.green / 65535.0;
			b = cc->xor_color.blue / 65535.0;
		}
		else {
			r = cc->red;
			g = cc->green;
			b = cc->blue;
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
#if 1
		r = r * mult;
		g = g * mult;
		b = b * mult;
#endif
	}

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

	gtk_gl_init(argc, argv);

	/* setup GL-context */
	priv->glconfig = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGBA | GDK_GL_MODE_STENCIL | GDK_GL_MODE_DOUBLE);
	if (!priv->glconfig) {
		printf("Could not setup GL-context!\n");
		return; /* Should we abort? */
	}

	/* Setup HID function pointers specific to the GL renderer */
	gtk2_gl_hid.end_layer = ghid_gl_end_layer;
}

static void ghid_gl_shutdown_renderer(void *p)
{
	rnd_gtk_port_t *port = p;

	g_free(port->render_priv);
	port->render_priv = NULL;
}

static void ghid_gl_init_drawing_widget(GtkWidget *widget, void *port_)
{
	rnd_gtk_port_t *port = port_;
	render_priv_t *priv = port->render_priv;

	gtk_widget_set_gl_capability(widget, priv->glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE);
}

static gboolean ghid_gl_start_drawing(rnd_gtk_port_t *port)
{
	GtkWidget *widget = port->drawing_area;
	GdkGLContext *pGlContext = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *pGlDrawable = gtk_widget_get_gl_drawable(widget);

	/* make GL-context "current" */
	if (!gdk_gl_drawable_gl_begin(pGlDrawable, pGlContext))
		return FALSE;

	port->render_priv->in_context = rnd_true;

	return TRUE;
}

static void ghid_gl_end_drawing(rnd_gtk_port_t *port)
{
	GtkWidget *widget = port->drawing_area;
	GdkGLDrawable *pGlDrawable = gtk_widget_get_gl_drawable(widget);

	if (gdk_gl_drawable_is_double_buffered(pGlDrawable))
		gdk_gl_drawable_swap_buffers(pGlDrawable);
	else
		hidgl_flush();

	port->render_priv->in_context = rnd_false;

	/* end drawing to current GL-context */
	gdk_gl_drawable_gl_end(pGlDrawable);
}

/* This realize callback is used to work around a crash bug in some mesa
   versions (observed on a machine running the intel i965 driver. It isn't
   obvious why it helps, but somehow fiddling with the GL context here solves
   the issue. The problem appears to have been fixed in recent mesa versions. */
static void ghid_gl_port_drawing_realize_cb(GtkWidget *widget, gpointer data)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(widget);

	if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
		return;

	gdk_gl_drawable_gl_end(gldrawable);
	return;
}

static gboolean ghid_gl_drawing_area_expose_cb(GtkWidget *widget, rnd_gtk_expose_t *ev, void *vport)
{
	return ghid_gl_drawing_area_expose_cb_common(&gtk2_gl_hid, widget, ev, vport);
}


static gboolean ghid_gl_preview_expose(GtkWidget *widget, rnd_gtk_expose_t *ev, rnd_hid_expose_t expcall, rnd_hid_expose_ctx_t *ctx)
{
	GdkGLContext *pGlContext = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *pGlDrawable = gtk_widget_get_gl_drawable(widget);
	GtkAllocation allocation;
	rnd_hidlib_t *hidlib = ghidgui->hidlib;

	gtkc_widget_get_allocation(widget, &allocation);

	/* make GL-context "current" */
	if (!gdk_gl_drawable_gl_begin(pGlDrawable, pGlContext)) {
		return FALSE;
	}

	ghid_gl_preview_expose_common(&gtk2_gl_hid, hidlib, ev, expcall, ctx, allocation.width, allocation.height);

	if (gdk_gl_drawable_is_double_buffered(pGlDrawable))
		gdk_gl_drawable_swap_buffers(pGlDrawable);
	else
		hidgl_flush();

	/* end drawing to current GL-context */
	gdk_gl_drawable_gl_end(pGlDrawable);

	return FALSE;
}

static GtkWidget *ghid_gl_new_drawing_widget(rnd_gtk_impl_t *impl)
{
	GtkWidget *w = gtk_drawing_area_new();

	g_signal_connect(G_OBJECT(w), "expose_event", G_CALLBACK(impl->drawing_area_expose), impl->gport);

	return w;
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
