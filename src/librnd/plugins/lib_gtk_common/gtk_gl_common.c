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

#include <librnd/plugins/lib_gtk_common/rnd_gtk.h>
#include <librnd/plugins/lib_gtk_common/coord_conv.h>
#include <librnd/plugins/lib_gtk_common/glue_common.h>

#include <librnd/plugins/lib_hid_gl/hidgl.h>

#include <librnd/plugins/lib_gtk_common/hid_gtk_conf.h>

/* These functions are defined in the file that #includes us: */
static void set_gl_color_for_gc(rnd_hid_gc_t gc);
static rnd_bool map_color(const rnd_color_t *inclr, rnd_gtk_color_t *color);
static gboolean ghid_gl_start_drawing(rnd_gtk_port_t *port);
static void ghid_gl_end_drawing(rnd_gtk_port_t *port);



static rnd_hid_gc_t current_gc = NULL;

static rnd_coord_t grid_local_x = 0, grid_local_y = 0, grid_local_radius = 0;

typedef struct render_priv_s {
	void *glconfig;                       /* gtk2: GdkGLConfig * */
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

int ghid_gl_set_layer_group(rnd_hid_t *hid, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	rnd_hidlib_t *hidlib = ghidgui->hidlib;
	render_priv_t *priv = ghidgui->port.render_priv;
	double tx, ty, zx, zy, zz;

	tx = rnd_conf.editor.view.flip_x ? ghidgui->port.view.x0 - hidlib->size_x : -ghidgui->port.view.x0;
	ty = rnd_conf.editor.view.flip_y ? ghidgui->port.view.y0 - hidlib->size_y : -ghidgui->port.view.y0;
	zx = (rnd_conf.editor.view.flip_x ? -1. : 1.) / ghidgui->port.view.coord_per_px;
	zy = (rnd_conf.editor.view.flip_y ? -1. : 1.) / ghidgui->port.view.coord_per_px;
	zz = ((rnd_conf.editor.view.flip_x == rnd_conf.editor.view.flip_y) ? 1. : -1.) / ghidgui->port.view.coord_per_px;
	hidgl_set_view(tx, ty, zx, zy, zz);

	/* Put the renderer into a good state so that any drawing is done in standard mode */

	hidgl_flush_drawing();
	hidgl_reset();

	priv->trans_lines = rnd_true;
	return 1;
}

static void ghid_gl_end_layer(rnd_hid_t *hid)
{
	hidgl_flush_drawing();
	hidgl_reset();
}

void ghid_gl_destroy_gc(rnd_hid_gc_t gc)
{
	g_free(gc);
}

rnd_hid_gc_t ghid_gl_make_gc(rnd_hid_t *hid)
{
	rnd_hid_gc_t rv;

	rv = g_new0(rnd_hid_gc_s, 1);
	rv->me_pointer = hid;
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

	hidgl_set_grid_color(priv->grid_color.fr, priv->grid_color.fg, priv->grid_color.fb);

	if (rnd_gtk_conf_hid.plugins.hid_gtk.local_grid.enable)
		hidgl_draw_local_grid(hidlib, grid_local_x, grid_local_y, grid_local_radius, ghidgui->port.view.coord_per_px, rnd_conf.editor.cross_grid);
	else
		hidgl_draw_grid(hidlib, drawn_area, ghidgui->port.view.coord_per_px, rnd_conf.editor.cross_grid);
}

static void rnd_gl_draw_texture(rnd_hidlib_t *hidlib, rnd_gtk_pixmap_t *gpm, rnd_coord_t ox, rnd_coord_t oy, rnd_coord_t bw, rnd_coord_t bh)
{
	hidgl_draw_texture_rect(ox, oy, ox+bw, oy+bh, gpm->cache.lng);
}

static void ghid_gl_draw_pixmap(rnd_hidlib_t *hidlib, rnd_gtk_pixmap_t *gpm, rnd_coord_t ox, rnd_coord_t oy, rnd_coord_t bw, rnd_coord_t bh)
{
	if (gpm->cache.lng == 0) {
		int width = gpm->pxm->sx;
		int height = gpm->pxm->sy;
		int rowstride = gdk_pixbuf_get_rowstride(gpm->image);
		int bits_per_sample = gdk_pixbuf_get_bits_per_sample(gpm->image);
		int n_channels = gdk_pixbuf_get_n_channels(gpm->image);
		unsigned char *pixels = gdk_pixbuf_get_pixels(gpm->image);

		g_warn_if_fail(bits_per_sample == 8);
		g_warn_if_fail(rowstride == width * n_channels);

		TODO(
			"We should probably determine what the maximum texture supported is,"
			"and if our image is larger, shrink it down using GDK pixbuf routines"
			"rather than having it fail below.");
		gpm->cache.lng = hidgl_texture_import(pixels, width, height, (n_channels == 4));
	}

	rnd_gl_draw_texture(hidlib, gpm, ox, oy, bw, bh);
}

static void ghid_gl_draw_bg_image(rnd_hidlib_t *hidlib)
{
	if (ghidgui->bg_pixmap.image != NULL)
		ghid_gl_draw_pixmap(hidlib, &ghidgui->bg_pixmap, 0, 0, hidlib->size_x, hidlib->size_y);
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

	hidgl_reset();
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

static void ghid_gl_screen_update(void)
{
}

/* Settles background color + inital GL configuration, to allow further drawing in GL area.
    (w, h) describes the total area concerned, while (xr, yr, wr, hr) describes area requested by an expose event.
    The color structure holds the wanted solid back-ground color, used to first paint the exposed drawing area. */
static void rnd_gl_draw_expose_init(int w, int h, int xr, int yr, int wr, int hr, rnd_color_t *bg_c)
{
	hidgl_stencil_init();
	hidgl_expose_init(w, h, bg_c);
}

static gboolean ghid_gl_drawing_area_expose_cb_common(rnd_hid_t *hid, GtkWidget *widget, rnd_gtk_expose_t *ev, void *vport)
{
	rnd_gtk_port_t *port = vport;
	rnd_hidlib_t *hidlib = ghidgui->hidlib;
	render_priv_t *priv = port->render_priv;
	GtkAllocation allocation;
	rnd_hid_expose_ctx_t ctx;
	double tx, ty, zx, zy, zz;

	gtkc_widget_get_allocation(widget, &allocation);

	ghid_gl_start_drawing(port);

	ctx.view.X1 = MIN(Px(0), Px(allocation.width + 1));
	ctx.view.Y1 = MIN(Py(0), Py(allocation.height + 1));
	ctx.view.X2 = MAX(Px(0), Px(allocation.width + 1));
	ctx.view.Y2 = MAX(Py(0), Py(allocation.height + 1));

	rnd_gl_draw_expose_init(allocation.width, allocation.height, 0, 0, allocation.width, allocation.height, &priv->offlimits_color);

	zx = (rnd_conf.editor.view.flip_x ? -1. : 1.) / port->view.coord_per_px;
	zy = (rnd_conf.editor.view.flip_y ? -1. : 1.) / port->view.coord_per_px;
	zz = ((rnd_conf.editor.view.flip_x == rnd_conf.editor.view.flip_y) ? 1. : -1.) / port->view.coord_per_px;
	tx = rnd_conf.editor.view.flip_x ? port->view.x0 - hidlib->size_x : -port->view.x0;
	ty = rnd_conf.editor.view.flip_y ? port->view.y0 - hidlib->size_y : -port->view.y0;
	hidgl_set_view(tx, ty, zx, zy, zz);

	/* Draw PCB background, before PCB primitives */
	hidgl_draw_initial_fill(0, 0, hidlib->size_x, hidlib->size_y, priv->bg_color.fr, priv->bg_color.fg, priv->bg_color.fb);
	ghid_gl_draw_bg_image(hidlib);

	ghid_gl_invalidate_current_gc();
	hidgl_push_matrix(1);
	rnd_app.expose_main(hid, &ctx, NULL);
	hidgl_flush_drawing();
	hidgl_pop_matrix(1);

	ghid_gl_draw_grid(hidlib, &ctx.view);

	ghid_gl_invalidate_current_gc();

	if (rnd_app.draw_attached != NULL)
		rnd_app.draw_attached(hidlib, 0);
	if (rnd_app.draw_marks != NULL)
		rnd_app.draw_marks(hidlib, 0);
	hidgl_flush_drawing();

	ghid_gl_show_crosshair(hidlib, TRUE, ctx.view.X1, ctx.view.Y1, ctx.view.X2, ctx.view.Y2);

	hidgl_flush_drawing();

	ghid_gl_end_drawing(port);

	return FALSE;
}

/* Assumes gl context is set up for drawing in the target widget */
static void ghid_gl_preview_expose_common(rnd_hid_t *hid, rnd_hidlib_t *hidlib, rnd_gtk_expose_t *ev, rnd_hid_expose_t expcall, rnd_hid_expose_ctx_t *ctx, long widget_xs, long widget_ys)
{
	render_priv_t *priv = ghidgui->port.render_priv;
	double xz, yz, vw, vh;
	rnd_coord_t ox1 = ctx->view.X1, oy1 = ctx->view.Y1, ox2 = ctx->view.X2, oy2 = ctx->view.Y2;
	rnd_coord_t save_cpp;
	rnd_gtk_view_t save_view;
	int save_width, save_height;
	double tx, ty, zx, zy, zz;

	/* Setup zoom factor for drawing routines */
	vw = ctx->view.X2 - ctx->view.X1;
	vh = ctx->view.Y2 - ctx->view.Y1;
	xz = vw / (double)widget_xs;
	yz = vh / (double)widget_ys;

	save_view = ghidgui->port.view;
	save_width = ghidgui->port.view.canvas_width;
	save_height = ghidgui->port.view.canvas_height;
	save_cpp = rnd_gui->coord_per_pix;

	if (xz > yz)
		ghidgui->port.view.coord_per_px = xz;
	else
		ghidgui->port.view.coord_per_px = yz;

	ghidgui->port.view.canvas_width = widget_xs;
	ghidgui->port.view.canvas_height = widget_ys;
	ghidgui->port.view.width = (double)widget_xs * ghidgui->port.view.coord_per_px;
	ghidgui->port.view.height = (double)widget_ys * ghidgui->port.view.coord_per_px;
	ghidgui->port.view.x0 = (vw - ghidgui->port.view.width) / 2 + ctx->view.X1;
	ghidgui->port.view.y0 = (vh - ghidgui->port.view.height) / 2 + ctx->view.Y1;

	RND_GTKC_PREVIEW_TUNE_EXTENT(ctx, widget_xs, widget_ys);

	ghidgui->port.render_priv->in_context = rnd_true;

	hidgl_expose_init(widget_xs, widget_ys, &priv->bg_color);

	/* call the drawing routine */
	ghid_gl_invalidate_current_gc();
	hidgl_push_matrix(0);

	zx = (rnd_conf.editor.view.flip_x ? -1. : 1.) / ghidgui->port.view.coord_per_px;
	zy = (rnd_conf.editor.view.flip_y ? -1. : 1.) / ghidgui->port.view.coord_per_px;
	zz = 1;
	tx = rnd_conf.editor.view.flip_x ? ghidgui->port.view.x0 - hidlib->size_x : -ghidgui->port.view.x0;
	ty = rnd_conf.editor.view.flip_y ? ghidgui->port.view.y0 - hidlib->size_y : -ghidgui->port.view.y0;
	hidgl_set_view(tx, ty, zx, zy, zz);

	rnd_gui->coord_per_pix = ghidgui->port.view.coord_per_px;
	expcall(hid, ctx);

	hidgl_flush_drawing();
	hidgl_pop_matrix(0);

	ghidgui->port.render_priv->in_context = rnd_false;

	/* restore the original context and priv */
	ctx->view.X1 = ox1;
	ctx->view.X2 = ox2;
	ctx->view.Y1 = oy1;
	ctx->view.Y2 = oy2;
	rnd_gui->coord_per_pix = save_cpp;
	ghidgui->port.view = save_view;
	ghidgui->port.view.canvas_width = save_width;
	ghidgui->port.view.canvas_height = save_height;
}

int ghid_gl_install_common(rnd_gtk_impl_t *impl, rnd_hid_t *hid)
{
	if (impl != NULL) {
		impl->set_special_colors = ghid_gl_set_special_colors;
		impl->screen_update = ghid_gl_screen_update;
		impl->draw_grid_local = ghid_gl_draw_grid_local;
		impl->drawing_area_configure_hook = ghid_gl_drawing_area_configure_hook;
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

	return 0;
}
