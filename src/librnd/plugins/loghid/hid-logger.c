/*
 * a HID mostly for debugging.
 */
#include "config.h"
#include "hid-logger.h"
#include <librnd/core/rnd_printf.h>
#include <librnd/core/color.h>

#define ENUM_LOG_TEXT(e) case e: txt = #e; break

/*
 * Just having one instance for now. Keeping it
 * local is fine.
 */
static rnd_hid_t *delegatee_ = NULL;
static FILE *out_ = NULL;
extern rnd_hid_t loghid_gui;
extern rnd_hid_t loghid_exp;

#define SANITY(ret) \
	if ((delegatee_ == NULL) || (delegatee_ == &loghid_gui)) { \
		fprintf(stderr, "loghid: Invalid target HID.\n"); \
		ret; \
	} \


static const rnd_export_opt_t *log_get_export_options(rnd_hid_t *hid, int *ret, rnd_design_t *dsg, void *appspec)
{
	const rnd_export_opt_t *result;

	SANITY(return NULL);

	result = delegatee_->get_export_options(hid, ret, dsg, appspec);
	if (ret != NULL)
		rnd_fprintf(out_, "get_export_options(ret) -> ret=%d\n", *ret);
	else
		rnd_fprintf(out_, "get_export_options(ret) -> ret=n/a\n");
	return result;
}

static void log_do_exit(rnd_hid_t *hid)
{
	SANITY(return);
	rnd_fprintf(out_, "do_exit()\n");
	delegatee_->do_exit(delegatee_);
}

static void log_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	SANITY(return);
	rnd_fprintf(out_, "do_export()\n");
	delegatee_->do_export(delegatee_, design, options, appspec);
}

static int log_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	SANITY(return -1);
	rnd_fprintf(out_, "parse_arguments()\n");
	return delegatee_->parse_arguments(delegatee_, argc, argv);
}

static void log_invalidate_lr(rnd_hid_t *hid, rnd_coord_t left, rnd_coord_t right, rnd_coord_t top, rnd_coord_t bottom)
{
	SANITY(return);
	rnd_fprintf(out_, "invalidate_lr(%mm, %mm, %mm, %mm)\n", left, right, top, bottom);
	delegatee_->invalidate_lr(hid, left, right, top, bottom);
}

static void log_invalidate_all(rnd_hid_t *hid)
{
	SANITY(return);
	rnd_fprintf(out_, "invalidate_all()\n");
	delegatee_->invalidate_all(hid);
}

static void log_notify_crosshair_change(rnd_hid_t *hid, rnd_bool changes_complete)
{
	SANITY(return);
	rnd_fprintf(out_, "rnd_hid_notify_crosshair_change(%s)\n", changes_complete ? "true" : "false");
	delegatee_->notify_crosshair_change(hid, changes_complete);
}

static void log_notify_mark_change(rnd_hid_t *hid, rnd_bool changes_complete)
{
	SANITY(return);
	rnd_fprintf(out_, "log_notify_mark_change(%s)\n", changes_complete ? "true" : "false");
	delegatee_->notify_mark_change(hid, changes_complete);
}

static int log_set_layer_group(rnd_hid_t *hid, rnd_design_t *design, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	SANITY(return -1);
	rnd_fprintf(out_, "set_layer(group=%ld, layer=%ld, flags=%lx, empty=%s, xform=%p)\n", group, layer, flags, is_empty ? "true" : "false", xform);
	return delegatee_->set_layer_group(hid, design, group, purpose, purpi, layer, flags, is_empty, xform);
}

static void log_end_layer(rnd_hid_t *hid)
{
	SANITY(return);
	rnd_fprintf(out_, "end_layer()\n");
	delegatee_->end_layer(hid);
}

static rnd_hid_gc_t log_make_gc(rnd_hid_t *hid)
{
	SANITY(return NULL);
	rnd_fprintf(out_, "make_gc()\n");
	return delegatee_->make_gc(delegatee_);
}

static void log_destroy_gc(rnd_hid_gc_t gc)
{
	SANITY(return);
	rnd_fprintf(out_, "destroy_gc()\n");
	delegatee_->destroy_gc(gc);
}

static void log_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
	SANITY(return);
	if (screen != NULL)
		rnd_fprintf(out_, "set_drawing_mode(%d,%d,[%mm;%mm,%mm;%mm])\n", op, direct, screen->X1, screen->Y1, screen->X2, screen->Y2);
	else
		rnd_fprintf(out_, "set_drawing_mode(%d,%d,[NULL-screen])\n", op, direct);
	delegatee_->set_drawing_mode(hid, op, direct, screen);
}

static void log_render_burst(rnd_hid_t *hid, rnd_burst_op_t op, const rnd_box_t *screen)
{
	SANITY(return);
	rnd_fprintf(out_, "render_burst(%d,[%mm;%mm,%mm;%mm])\n", op, screen->X1, screen->Y1, screen->X2, screen->Y2);
	delegatee_->render_burst(hid, op, screen);
	rnd_gui->coord_per_pix = delegatee_->coord_per_pix;
}


static void log_set_color(rnd_hid_gc_t gc, const rnd_color_t *color)
{
	SANITY(return);
	rnd_fprintf(out_, "set_color(gc, %s)\n", color->str);
	delegatee_->set_color(gc, color);
}

static void log_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	const char *txt = "unknown";
	SANITY(return);
	switch (style) {
		ENUM_LOG_TEXT(rnd_cap_square);
		ENUM_LOG_TEXT(rnd_cap_round);
		ENUM_LOG_TEXT(rnd_cap_invalid);
	}
	rnd_fprintf(out_, "set_line_cap(gc, %s)\n", txt);
	delegatee_->set_line_cap(gc, style);
}

static void log_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	SANITY(return);
	rnd_fprintf(out_, "set_line_width(gc, %d)\n", width);
	delegatee_->set_line_width(gc, width);
}

static void log_set_draw_xor(rnd_hid_gc_t gc, int xor)
{
	SANITY(return);
	rnd_fprintf(out_, "set_draw_xor(gc, %s)\n", xor ? "true" : "false");
	delegatee_->set_draw_xor(gc, xor);
}

static void log_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	SANITY(return);
	rnd_fprintf(out_, "draw_line(gc, %mm, %mm, %mm, %mm)\n", x1, y1, x2, y2);
	delegatee_->draw_line(gc, x1, y1, x2, y2);
}

static void log_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t xradius, rnd_coord_t yradius, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	SANITY(return);
	rnd_fprintf(out_, "draw_arc(gc, %mm, %mm, rx=%mm, ry=%mm, start_angle=%.1f, delta_a=%.1f)\n",
		cx, cy, xradius, yradius, start_angle, delta_angle);
	delegatee_->draw_arc(gc, cx, cy, xradius, yradius, start_angle, delta_angle);
}

static void log_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	SANITY(return);
	rnd_fprintf(out_, "draw_rect(gc, %mm, %mm, %mm, %mm)\n", x1, y1, x2, y2);
	delegatee_->draw_rect(gc, x1, y1, x2, y2);
}

static void log_fill_circle(rnd_hid_gc_t gc, rnd_coord_t x, rnd_coord_t y, rnd_coord_t r)
{
	SANITY(return);
	rnd_fprintf(out_, "fill_circle(gc, %mm, %mm, %mm)\n", x, y, r);
	delegatee_->fill_circle(gc, x, y, r);
}

static void log_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y)
{
	int i;
	SANITY(return);
	rnd_fprintf(out_, "fill_polygon(gc, %d", n_coords);
	for (i = 0; i < n_coords; ++i) {
		rnd_fprintf(out_, ", (%mm, %mm)", x[i], y[i]);
	}
	rnd_fprintf(out_, ")\n");
	delegatee_->fill_polygon(gc, n_coords, x, y);
}

static void log_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	int i;
	SANITY(return);
	rnd_fprintf(out_, "fill_polygon_offs(gc, %d", n_coords);
	for (i = 0; i < n_coords; ++i) {
		rnd_fprintf(out_, ", (%mm, %mm)", x[i], y[i]);
	}
	rnd_fprintf(out_, ", {%mm,%mm})\n", dx, dy);
	delegatee_->fill_polygon_offs(gc, n_coords, x, y, dx, dy);
}

static void log_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	SANITY(return);
	rnd_fprintf(out_, "fill_rect(gc, %mm, %mm, %mm, %mm)\n", x1, y1, x2, y2);
	delegatee_->fill_rect(gc, x1, y1, x2, y2);
}

void create_log_hid(FILE *log_out, rnd_hid_t *loghid, rnd_hid_t *delegatee)
{
	out_ = log_out;
	delegatee_ = delegatee;

	if ((delegatee == NULL) || (delegatee == &loghid_gui)) {
		fprintf(stderr, "loghid: Invalid target HID.\n");
		exit(1);
	}

	/*
	 * Setting up the output hid with a copy from the original, then
	 * replace the functions we want to log.
	 * We only log 'interesting' functions for now.
	 */
	*loghid = *delegatee;
	loghid->override_render = 1;

#define REGISTER_IF_NOT_NULL(fun) loghid->fun = delegatee->fun ? log_##fun : NULL
	REGISTER_IF_NOT_NULL(get_export_options);
	REGISTER_IF_NOT_NULL(do_exit);
	REGISTER_IF_NOT_NULL(do_export);
	REGISTER_IF_NOT_NULL(parse_arguments);
	REGISTER_IF_NOT_NULL(invalidate_lr);
	REGISTER_IF_NOT_NULL(invalidate_all);
	REGISTER_IF_NOT_NULL(notify_crosshair_change);
	REGISTER_IF_NOT_NULL(notify_mark_change);
	REGISTER_IF_NOT_NULL(set_layer_group);
	REGISTER_IF_NOT_NULL(end_layer);
	REGISTER_IF_NOT_NULL(make_gc);
	REGISTER_IF_NOT_NULL(destroy_gc);
	REGISTER_IF_NOT_NULL(set_drawing_mode);
	REGISTER_IF_NOT_NULL(render_burst);
	REGISTER_IF_NOT_NULL(set_color);
	REGISTER_IF_NOT_NULL(set_line_cap);
	REGISTER_IF_NOT_NULL(set_line_width);
	REGISTER_IF_NOT_NULL(set_draw_xor);
	REGISTER_IF_NOT_NULL(draw_line);
	REGISTER_IF_NOT_NULL(draw_arc);
	REGISTER_IF_NOT_NULL(draw_rect);
	REGISTER_IF_NOT_NULL(fill_circle);
	REGISTER_IF_NOT_NULL(fill_polygon);
	REGISTER_IF_NOT_NULL(fill_polygon_offs);
	REGISTER_IF_NOT_NULL(fill_rect);

#undef REGISTER_IF_NOT_NULL
}
