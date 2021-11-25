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
#include <librnd/plugins/lib_hid_gl/draw_gl.h>
#include <gtk/gtkgl.h>
#include <librnd/plugins/lib_hid_gl/hidgl.h>
#include <librnd/plugins/lib_hid_gl/stencil_gl.h>

#include <librnd/plugins/lib_gtk_common/hid_gtk_conf.h>

extern rnd_hid_t gtk2_gl_hid;

static rnd_hid_gc_t current_gc = NULL;

/* Sets ghidgui->port.u_gc to the "right" GC to use (wrt mask or window) */
#define USE_GC(gc) if (!use_gc(gc)) return

static rnd_coord_t grid_local_x = 0, grid_local_y = 0, grid_local_radius = 0;

typedef struct render_priv_s {
	GdkGLConfig *glconfig;
	rnd_color_t bg_color;
	rnd_color_t offlimits_color;
	rnd_color_t grid_color;
	rnd_bool trans_lines;
	rnd_bool in_context;
	int subcomposite_stencil_bit;
	unsigned long current_color_packed;
	double current_alpha_mult;
	int current_color_xor;

	/* color cache for set_color */
	rnd_clrcache_t ccache;
	int ccache_inited;
} render_priv_t;


typedef struct rnd_hid_gc_s {
	rnd_core_gc_t core_gc;
	rnd_hid_t *me_pointer;

	const rnd_color_t *pcolor;
	double alpha_mult;
	rnd_coord_t width;
} rnd_hid_gc_s;

void ghid_gl_render_burst(rnd_hid_t *hid, rnd_burst_op_t op, const rnd_box_t *screen)
{
	rnd_gui->coord_per_pix = ghidgui->port.view.coord_per_px;
}

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

int ghid_gl_set_layer_group(rnd_hid_t *hid, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	rnd_hidlib_t *hidlib = ghidgui->hidlib;
	render_priv_t *priv = ghidgui->port.render_priv;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -HIDGL_Z_NEAR);

	glScalef((rnd_conf.editor.view.flip_x ? -1. : 1.) / ghidgui->port.view.coord_per_px, (rnd_conf.editor.view.flip_y ? -1. : 1.) / ghidgui->port.view.coord_per_px, ((rnd_conf.editor.view.flip_x == rnd_conf.editor.view.flip_y) ? 1. : -1.) / ghidgui->port.view.coord_per_px);
	glTranslatef(rnd_conf.editor.view.flip_x ? ghidgui->port.view.x0 - hidlib->size_x : -ghidgui->port.view.x0, rnd_conf.editor.view.flip_y ? ghidgui->port.view.y0 - hidlib->size_y : -ghidgui->port.view.y0, 0);

	/* Put the renderer into a good state so that any drawing is done in standard mode */

	drawgl_flush();
	drawgl_reset();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_STENCIL_TEST);

	priv->trans_lines = rnd_true;
	return 1;
}

static void ghid_gl_end_layer(rnd_hid_t *hid)
{
	drawgl_flush();
	drawgl_reset();
}

void ghid_gl_destroy_gc(rnd_hid_gc_t gc)
{
	g_free(gc);
}

rnd_hid_gc_t ghid_gl_make_gc(rnd_hid_t *hid)
{
	rnd_hid_gc_t rv;

	rv = g_new0(rnd_hid_gc_s, 1);
	rv->me_pointer = &gtk2_gl_hid;
	rv->pcolor = &rnd_conf.appearance.color.background;
	rv->alpha_mult = 1.0;
	return rv;
}

void ghid_gl_draw_grid_local(rnd_hidlib_t *hidlib, rnd_coord_t cx, rnd_coord_t cy)
{
	if (hidlib->grid <= 0)
		return;

	/* cx and cy are the actual cursor snapped to wherever - round them to the nearest real grid point */
	grid_local_x = (cx / hidlib->grid) * hidlib->grid + hidlib->grid_ox;
	grid_local_y = (cy / hidlib->grid) * hidlib->grid + hidlib->grid_oy;
	grid_local_radius = rnd_gtk_conf_hid.plugins.hid_gtk.local_grid.radius;
}

static void ghid_gl_draw_grid(rnd_hidlib_t *hidlib, rnd_box_t *drawn_area)
{
	render_priv_t *priv = ghidgui->port.render_priv;

	if ((Vz(hidlib->grid) < RND_MIN_GRID_DISTANCE) || (!rnd_conf.editor.draw_grid))
		return;

	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_XOR);

	glColor3f(priv->grid_color.fr, priv->grid_color.fg, priv->grid_color.fb);

	if (rnd_gtk_conf_hid.plugins.hid_gtk.local_grid.enable)
		hidgl_draw_local_grid(hidlib, grid_local_x, grid_local_y, grid_local_radius, ghidgui->port.view.coord_per_px, rnd_conf.editor.cross_grid);
	else
		hidgl_draw_grid(hidlib, drawn_area, ghidgui->port.view.coord_per_px, rnd_conf.editor.cross_grid);

	glDisable(GL_COLOR_LOGIC_OP);
}

static void rnd_gl_draw_texture(rnd_hidlib_t *hidlib, rnd_gtk_pixmap_t *gpm, rnd_coord_t ox, rnd_coord_t oy, rnd_coord_t bw, rnd_coord_t bh)
{
	hidgl_draw_texture_rect(ox, oy, ox+bw, oy+bh, gpm->cache.lng);
}

static void ghid_gl_draw_pixmap(rnd_hidlib_t *hidlib, rnd_gtk_pixmap_t *gpm, rnd_coord_t ox, rnd_coord_t oy, rnd_coord_t bw, rnd_coord_t bh)
{
	GLuint texture_handle = gpm->cache.lng;
	if (texture_handle == 0) {
		int width = gpm->pxm->sx;
		int height = gpm->pxm->sy;
		int rowstride = gdk_pixbuf_get_rowstride(gpm->image);
		int bits_per_sample = gdk_pixbuf_get_bits_per_sample(gpm->image);
		int n_channels = gdk_pixbuf_get_n_channels(gpm->image);
		unsigned char *pixels = gdk_pixbuf_get_pixels(gpm->image);

		g_warn_if_fail(bits_per_sample == 8);
		g_warn_if_fail(rowstride == width * n_channels);

		glGenTextures(1, &texture_handle);
		glBindTexture(GL_TEXTURE_2D, texture_handle);

		/* XXX: We should probably determine what the maximum texture supported is,
		        and if our image is larger, shrink it down using GDK pixbuf routines
		        rather than having it fail below. */
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, (n_channels == 4) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, pixels);
		gpm->cache.lng = texture_handle;
	}

	rnd_gl_draw_texture(hidlib, gpm, ox, oy, bw, bh);
}

static void ghid_gl_draw_bg_image(rnd_hidlib_t *hidlib)
{
	if (ghidgui->bg_pixmap.image != NULL)
		ghid_gl_draw_pixmap(hidlib, &ghidgui->bg_pixmap, 0, 0, hidlib->size_x, hidlib->size_y);
}

static void ghid_gl_draw_bg_solid_color(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, float red, float green, float blue)
{
	float points[4][6];
	int i;
	for(i=0;i<4;++i)
	{
		points[i][2] = red;
		points[i][3] = green;
		points[i][4] = blue;
		points[i][5] = 1.0f;
	}

	points[0][0] = x1;
	points[0][1] = y1;
	points[1][0] = x2;
	points[1][1] = y1;
	points[2][0] = x2;
	points[2][1] = y2;
	points[3][0] = x1;
	points[3][1] = y2;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(float) * 6, points);
	glColorPointer(4, GL_FLOAT, sizeof(float) * 6, &points[0][2]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

/* Config helper functions for when the user changes color preferences.
   set_special colors used in the gtkhid. */
static void set_special_grid_color(void)
{
	render_priv_t *priv = ghidgui->port.render_priv;
	unsigned r, g, b;

	r = priv->grid_color.r ^ priv->bg_color.r;
	g = priv->grid_color.g ^ priv->bg_color.g;
	b = priv->grid_color.b ^ priv->bg_color.b;
	rnd_color_load_int(&priv->grid_color, r, g, b, 255);
}

void ghid_gl_set_special_colors(rnd_conf_native_t *cfg)
{
	render_priv_t *priv = ghidgui->port.render_priv;

	if (((RND_CFT_COLOR *) cfg->val.color == &rnd_conf.appearance.color.background)) {
		priv->bg_color = cfg->val.color[0];
	}
	else if ((RND_CFT_COLOR *)cfg->val.color == &rnd_conf.appearance.color.off_limit) {
		priv->offlimits_color = cfg->val.color[0];
	}
	else if (((RND_CFT_COLOR *)cfg->val.color == &rnd_conf.appearance.color.grid)) {
		priv->grid_color = cfg->val.color[0];
		set_special_grid_color();
	}
}

typedef struct {
	int color_set;
	rnd_gtk_color_t color;
	int xor_set;
	rnd_gtk_color_t xor_color;
	double red;
	double green;
	double blue;
} rnd_gtk_color_cache_t;

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


	/* We need to flush the draw buffer when changing colour so that new primitives
	   don't get merged. This actually isn't a problem with the colour but due to 
	   way that the final render pass iterates through the primitive buffer in 
	   reverse order. If the new primitives are merged with previous ones then they
	   will be drawn in the wrong order. */
	drawgl_flush();

	drawgl_set_colour(r, g, b, a);
}

void ghid_gl_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
	if (color == NULL) {
		fprintf(stderr, "ghid_gl_set_color():  name = NULL, setting to magenta\n");
		color = rnd_color_magenta;
	}

	gc->pcolor = color;
	set_gl_color_for_gc(gc);
}

void ghid_gl_set_alpha_mult(rnd_hid_gc_t gc, double alpha_mult)
{
	gc->alpha_mult = alpha_mult;
	set_gl_color_for_gc(gc);
}

void ghid_gl_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	/* nop: each line render gets the cap style from the core gc because
	   low level line draw function has to build the ap from triangles */
}

void ghid_gl_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	gc->width = width < 0 ? (-width) * ghidgui->port.view.coord_per_px : width;
}


void ghid_gl_set_draw_xor(rnd_hid_gc_t gc, int xor)
{
	/* NOT IMPLEMENTED */

	/* Only presently called when setting up a crosshair GC.
	   We manage our own drawing model for that anyway. */
}

static void ghid_gl_invalidate_current_gc(void)
{
	current_gc = NULL;
}

static int use_gc(rnd_hid_gc_t gc)
{
	if (gc->me_pointer != &gtk2_gl_hid) {
		fprintf(stderr, "Fatal: GC from another HID passed to GTK HID\n");
		abort();
	}

	if (current_gc == gc)
		return 1;

	current_gc = gc;

	set_gl_color_for_gc(gc);
	return 1;
}


static void ghid_gl_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	USE_GC(gc);

	hidgl_draw_line(gc->core_gc.cap, gc->width, x1, y1, x2, y2, ghidgui->port.view.coord_per_px);
}

static void ghid_gl_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t xradius, rnd_coord_t yradius, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	USE_GC(gc);

	hidgl_draw_arc(gc->width, cx, cy, xradius, yradius, start_angle, delta_angle, ghidgui->port.view.coord_per_px);
}

static void ghid_gl_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	USE_GC(gc);

	hidgl_draw_rect(x1, y1, x2, y2);
}


static void ghid_gl_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	USE_GC(gc);

	hidgl_fill_circle(cx, cy, radius, ghidgui->port.view.coord_per_px);
}


static void ghid_gl_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y)
{
	USE_GC(gc);

	hidgl_fill_polygon(n_coords, x, y);
}

static void ghid_gl_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	USE_GC(gc);

	hidgl_fill_polygon_offs(n_coords, x, y, dx, dy);
}

static void ghid_gl_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	USE_GC(gc);

	hidgl_fill_rect(x1, y1, x2, y2);
}

static int preview_lock = 0;

void ghid_gl_invalidate_all(rnd_hid_t *hid)
{
	if (ghidgui && ghidgui->topwin.menu.menu_bar) {
		rnd_gtkg_draw_area_update(&ghidgui->port, NULL);
		if (!preview_lock) {
			preview_lock++;
			rnd_gtk_previews_invalidate_all();
			preview_lock--;
		}
	}
}

void ghid_gl_invalidate_lr(rnd_hid_t *hid, rnd_coord_t left, rnd_coord_t right, rnd_coord_t top, rnd_coord_t bottom)
{
	ghid_gl_invalidate_all(hid);
	if (!preview_lock) {
		preview_lock++;
		rnd_gtk_previews_invalidate_all();
		preview_lock--;
	}
}

static void ghid_gl_notify_crosshair_change(rnd_hid_t *hid, rnd_bool changes_complete)
{
	/* We sometimes get called before the GUI is up */
	if (ghidgui->port.drawing_area == NULL)
		return;

	/* FIXME: We could just invalidate the bounds of the crosshair attached objects? */
	ghid_gl_invalidate_all(hid);
}

static void ghid_gl_notify_mark_change(rnd_hid_t *hid, rnd_bool changes_complete)
{
	/* We sometimes get called before the GUI is up */
	if (ghidgui->port.drawing_area == NULL)
		return;

	/* FIXME: We could just invalidate the bounds of the mark? */
	ghid_gl_invalidate_all(hid);
}

static void ghid_gl_show_crosshair(rnd_hidlib_t *hidlib, gboolean paint_new_location, rnd_coord_t minx, rnd_coord_t miny, rnd_coord_t maxx, rnd_coord_t maxy)
{
	static int done_once = 0;
	static rnd_gtk_color_t cross_color;
	static unsigned long cross_color_packed;

	if (!paint_new_location)
		return;

	if (!done_once || (cross_color_packed != rnd_conf.appearance.color.cross.packed)) {
		done_once = 1;
		map_color(&rnd_conf.appearance.color.cross, &cross_color);
		cross_color_packed = rnd_conf.appearance.color.cross.packed;
	}

	if (ghidgui->topwin.active && ghidgui->port.view.has_entered) {
		hidgl_draw_crosshair(
			ghidgui->port.view.crosshair_x, ghidgui->port.view.crosshair_y,
			cross_color.red / 65535.0f, cross_color.green / 65535.0f, cross_color.blue / 65535.0f,
			minx, miny, maxx, maxy);
	}

	TODO("This shouldn't be here, but it's missing from somewhere else; if removed, moving out from the drawing area and clicking a tool button will recolor the backgrouind on intel");
	glDisable(GL_COLOR_LOGIC_OP);
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

static void ghid_gl_drawing_area_configure_hook(void *port)
{
	static int done_once = 0;
	rnd_gtk_port_t *p = port;
	render_priv_t *priv = p->render_priv;

	ghidgui->port.drawing_allowed = rnd_true;

	if (!done_once) {
		priv->bg_color = rnd_conf.appearance.color.background;
		priv->offlimits_color = rnd_conf.appearance.color.off_limit;
		priv->grid_color = rnd_conf.appearance.color.grid;
		set_special_grid_color();
		done_once = 1;
	}
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
		glFlush();

	port->render_priv->in_context = rnd_false;

	/* end drawing to current GL-context */
	gdk_gl_drawable_gl_end(pGlDrawable);
}

static void ghid_gl_screen_update(void)
{
}

/* Settles background color + inital GL configuration, to allow further drawing in GL area.
    (w, h) describes the total area concerned, while (xr, yr, wr, hr) describes area requested by an expose event.
    The color structure holds the wanted solid back-ground color, used to first paint the exposed drawing area. */
static void rnd_gl_draw_expose_init(rnd_hid_t *hid, int w, int h, int xr, int yr, int wr, int hr, rnd_color_t *bg_c)
{
	hidgl_init();
	hidgl_expose_init(w, h, bg_c);
}

static gboolean ghid_gl_drawing_area_expose_cb(GtkWidget *widget, rnd_gtk_expose_t *ev, void *vport)
{
	rnd_gtk_port_t *port = vport;
	rnd_hidlib_t *hidlib = ghidgui->hidlib;
	render_priv_t *priv = port->render_priv;
	GtkAllocation allocation;
	rnd_hid_expose_ctx_t ctx;

	gtkc_widget_get_allocation(widget, &allocation);

	ghid_gl_start_drawing(port);

	ctx.view.X1 = MIN(Px(0), Px(allocation.width + 1));
	ctx.view.Y1 = MIN(Py(0), Py(allocation.height + 1));
	ctx.view.X2 = MAX(Px(0), Px(allocation.width + 1));
	ctx.view.Y2 = MAX(Py(0), Py(allocation.height + 1));

	rnd_gl_draw_expose_init(&gtk2_gl_hid, allocation.width, allocation.height, 0, 0, allocation.width, allocation.height, &priv->offlimits_color);

	glScalef((rnd_conf.editor.view.flip_x ? -1. : 1.) / port->view.coord_per_px, (rnd_conf.editor.view.flip_y ? -1. : 1.) / port->view.coord_per_px, ((rnd_conf.editor.view.flip_x == rnd_conf.editor.view.flip_y) ? 1. : -1.) / port->view.coord_per_px);
	glTranslatef(rnd_conf.editor.view.flip_x ? port->view.x0 - hidlib->size_x : -port->view.x0, rnd_conf.editor.view.flip_y ? port->view.y0 - hidlib->size_y : -port->view.y0, 0);

	/* Draw PCB background, before PCB primitives */
	ghid_gl_draw_bg_solid_color(0, 0, hidlib->size_x, hidlib->size_y, priv->bg_color.fr, priv->bg_color.fg, priv->bg_color.fb);
	ghid_gl_draw_bg_image(hidlib);

	ghid_gl_invalidate_current_gc();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	rnd_app.expose_main(&gtk2_gl_hid, &ctx, NULL);
	drawgl_flush();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	ghid_gl_draw_grid(hidlib, &ctx.view);

	ghid_gl_invalidate_current_gc();

	if (rnd_app.draw_attached != NULL)
		rnd_app.draw_attached(hidlib, 0);
	if (rnd_app.draw_marks != NULL)
		rnd_app.draw_marks(hidlib, 0);
	drawgl_flush();

	ghid_gl_show_crosshair(hidlib, TRUE, ctx.view.X1, ctx.view.Y1, ctx.view.X2, ctx.view.Y2);

	drawgl_flush();

	ghid_gl_end_drawing(port);

	return FALSE;
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

static gboolean ghid_gl_preview_expose(GtkWidget *widget, rnd_gtk_expose_t *ev, rnd_hid_expose_t expcall, rnd_hid_expose_ctx_t *ctx)
{
	GdkGLContext *pGlContext = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *pGlDrawable = gtk_widget_get_gl_drawable(widget);
	GtkAllocation allocation;
	render_priv_t *priv = ghidgui->port.render_priv;
	rnd_hidlib_t *hidlib = ghidgui->hidlib;
	rnd_gtk_view_t save_view;
	int save_width, save_height;
	double xz, yz, vw, vh;
	rnd_coord_t ox1 = ctx->view.X1, oy1 = ctx->view.Y1, ox2 = ctx->view.X2, oy2 = ctx->view.Y2;
	rnd_coord_t save_cpp;

	vw = ctx->view.X2 - ctx->view.X1;
	vh = ctx->view.Y2 - ctx->view.Y1;

	save_view = ghidgui->port.view;
	save_width = ghidgui->port.view.canvas_width;
	save_height = ghidgui->port.view.canvas_height;
	save_cpp = rnd_gui->coord_per_pix;

	/* Setup zoom factor for drawing routines */
	gtkc_widget_get_allocation(widget, &allocation);
	xz = vw / (double)allocation.width;
	yz = vh / (double)allocation.height;

	if (xz > yz)
		ghidgui->port.view.coord_per_px = xz;
	else
		ghidgui->port.view.coord_per_px = yz;

	ghidgui->port.view.canvas_width = allocation.width;
	ghidgui->port.view.canvas_height = allocation.height;
	ghidgui->port.view.width = allocation.width * ghidgui->port.view.coord_per_px;
	ghidgui->port.view.height = allocation.height * ghidgui->port.view.coord_per_px;
	ghidgui->port.view.x0 = (vw - ghidgui->port.view.width) / 2 + ctx->view.X1;
	ghidgui->port.view.y0 = (vh - ghidgui->port.view.height) / 2 + ctx->view.Y1;

	RND_GTK_PREVIEW_TUNE_EXTENT(ctx, allocation);

	/* make GL-context "current" */
	if (!gdk_gl_drawable_gl_begin(pGlDrawable, pGlContext)) {
		return FALSE;
	}
	ghidgui->port.render_priv->in_context = rnd_true;

	hidgl_expose_init(allocation.width, allocation.height, &priv->bg_color);

	/* call the drawing routine */
	ghid_gl_invalidate_current_gc();
	glPushMatrix();
	glScalef((rnd_conf.editor.view.flip_x ? -1. : 1.) / ghidgui->port.view.coord_per_px, (rnd_conf.editor.view.flip_y ? -1. : 1.) / ghidgui->port.view.coord_per_px, 1);
	glTranslatef(rnd_conf.editor.view.flip_x ? ghidgui->port.view.x0 - hidlib->size_x : -ghidgui->port.view.x0, rnd_conf.editor.view.flip_y ? ghidgui->port.view.y0 - hidlib->size_y : -ghidgui->port.view.y0, 0);

	rnd_gui->coord_per_pix = ghidgui->port.view.coord_per_px;
	expcall(&gtk2_gl_hid, ctx);

	drawgl_flush();
	glPopMatrix();

	if (gdk_gl_drawable_is_double_buffered(pGlDrawable))
		gdk_gl_drawable_swap_buffers(pGlDrawable);
	else
		glFlush();

	/* end drawing to current GL-context */
	ghidgui->port.render_priv->in_context = rnd_false;
	gdk_gl_drawable_gl_end(pGlDrawable);

	/* restore the original context and priv */
	ctx->view.X1 = ox1;
	ctx->view.X2 = ox2;
	ctx->view.Y1 = oy1;
	ctx->view.Y2 = oy2;
	rnd_gui->coord_per_pix = save_cpp;
	ghidgui->port.view = save_view;
	ghidgui->port.view.canvas_width = save_width;
	ghidgui->port.view.canvas_height = save_height;

	return FALSE;
}

static GtkWidget *ghid_gl_new_drawing_widget(rnd_gtk_impl_t *impl)
{
	GtkWidget *w = gtk_drawing_area_new();

	g_signal_connect(G_OBJECT(w), "expose_event", G_CALLBACK(impl->drawing_area_expose), impl->gport);

	return w;
}


void ghid_gl_install(rnd_gtk_impl_t *impl, rnd_hid_t *hid)
{

	if (impl != NULL) {
		impl->new_drawing_widget = ghid_gl_new_drawing_widget;
		impl->init_drawing_widget = ghid_gl_init_drawing_widget;
		impl->drawing_realize = ghid_gl_port_drawing_realize_cb;
		impl->drawing_area_expose = ghid_gl_drawing_area_expose_cb;
		impl->preview_expose = ghid_gl_preview_expose;
		impl->set_special_colors = ghid_gl_set_special_colors;
		impl->init_renderer = ghid_gl_init_renderer;
		impl->screen_update = ghid_gl_screen_update;
		impl->draw_grid_local = ghid_gl_draw_grid_local;
		impl->drawing_area_configure_hook = ghid_gl_drawing_area_configure_hook;
		impl->shutdown_renderer = ghid_gl_shutdown_renderer;
		impl->get_color_name = get_color_name;
		impl->map_color = map_color;
		impl->draw_pixmap = ghid_gl_draw_pixmap;
	}

	if (hid != NULL) {
		hid->invalidate_lr = ghid_gl_invalidate_lr;
		hid->invalidate_all = ghid_gl_invalidate_all;
		hid->notify_crosshair_change = ghid_gl_notify_crosshair_change;
		hid->notify_mark_change = ghid_gl_notify_mark_change;
		hid->set_layer_group = ghid_gl_set_layer_group;
		hid->make_gc = ghid_gl_make_gc;
		hid->destroy_gc = ghid_gl_destroy_gc;
		hid->set_color = ghid_gl_set_color;
		hid->set_line_cap = ghid_gl_set_line_cap;
		hid->set_line_width = ghid_gl_set_line_width;
		hid->set_draw_xor = ghid_gl_set_draw_xor;
		hid->draw_line = ghid_gl_draw_line;
		hid->draw_arc = ghid_gl_draw_arc;
		hid->draw_rect = ghid_gl_draw_rect;
		hid->fill_circle = ghid_gl_fill_circle;
		hid->fill_polygon = ghid_gl_fill_polygon;
		hid->fill_polygon_offs = ghid_gl_fill_polygon_offs;
		hid->fill_rect = ghid_gl_fill_rect;

		hid->set_drawing_mode = hidgl_set_drawing_mode;
		hid->render_burst = ghid_gl_render_burst;
	}
}
