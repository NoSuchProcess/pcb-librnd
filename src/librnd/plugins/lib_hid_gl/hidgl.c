/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2009-2011 PCB Contributers (See ChangeLog for details)
 *  Copyright (C) 2021,2022 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include <librnd/core/rnd_conf.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <librnd/hid/grid.h>
#include <librnd/hid/hid.h>
#include <librnd/poly/rtree.h>
#include <librnd/core/hidlib.h>

#include "draw.h"
#include "stenc.h"
#include "stencil_gl.h"
#include "opengl.h"

#include "hidgl.h"

#include "lib_hid_gl_conf.h"
extern conf_lib_hid_gl_t conf_lib_hid_gl;

/* Do the translation on CPU because most GPU will do only GLfloat which is
   not capable of handling large coords (even in 32 bit space) without losing
   precision. In other words: translate the input drawing's rnd_coord_t x;y
   coordinates into a local screen-coord which is always around 0 so that
   the GPU has to deal with small enough numbers that they usually fit in
   float32. (When they don't, it's because we have a huge design and zoomed
   out a lot - but then losing a few bits of integer coords won't matter anyway)
   */
static rnd_coord_t VTX, VTY;

#include "draw_INIT.h"

/* return the first draw implementation that would work with current opengl */
RND_INLINE const hidgl_draw_t *hidgl_find_draw(const RND_CFT_LIST *pref)
{
	hidgl_draw_t *d;

	hidgl_draw_init_();

	if ((pref != NULL) && (rnd_conflist_length(pref) > 0)) {
		/* order is coming from preference */
		gdl_iterator_t it;
		rnd_conf_listitem_t *p;

		rnd_conflist_foreach(pref, &it, p) {
			for(d = hidgl_draws; d != NULL; d = d->next)
				if (strcmp(d->name, p->payload) == 0)
					if (d->init() == 0) return d;
		}
	}
	else {
		/* fallback: try them one by one, in order, if there was no preference */
		for(d = hidgl_draws; d != NULL; d = d->next)
			if (d->init() == 0) return d;
	}

	/* final fallback: did not find anything working  */
	if (hidgl_draw_error.init() == 0) return &hidgl_draw_error;
	return NULL;
}

hidgl_draw_t hidgl_draw;
hidgl_stenc_t hidgl_stenc;

int hidgl_init(void)
{
	if (hidgl_draw.name == NULL) {
		const hidgl_draw_t *drw = hidgl_find_draw(&conf_lib_hid_gl.plugins.lib_hid_gl.backend.preference);

		if (drw == NULL)
			return -1;

		hidgl_draw = *drw;
	}
	return 0;
}

int hidgl_new_context(void)
{
	return hidgl_draw.new_context();
}

/* return the first stenc implementation that would work with current opengl */
RND_INLINE const hidgl_stenc_t *hidgl_find_stenc(const RND_CFT_LIST *pref, int *stencil_bits)
{
	hidgl_stenc_t *d;

	hidgl_stenc_init_();

	if ((pref != NULL) && (rnd_conflist_length(pref) > 0)) {
		/* order is coming from preference */
		gdl_iterator_t it;
		rnd_conf_listitem_t *p;

		rnd_conflist_foreach(pref, &it, p) {
			for(d = hidgl_stencs; d != NULL; d = d->next)
				if (strcmp(d->name, p->payload) == 0)
					if (d->init(stencil_bits) == 0) return d;
		}
	}
	else {
		/* fallback: try them one by one, in order, if there was no preference */
		for(d = hidgl_stencs; d != NULL; d = d->next)
			if (d->init(stencil_bits) == 0) return d;
	}

	/* final fallback: did not find anything working  */
	if (hidgl_stenc_error.init(stencil_bits) == 0) return &hidgl_stenc_error;
	return NULL;
}

int hidgl_stencil_init(void)
{
	if (hidgl_stenc.name == NULL) {
		int stencil_bits = 0;
		const hidgl_stenc_t *stc = hidgl_find_stenc(&conf_lib_hid_gl.plugins.lib_hid_gl.stencil.preference, &stencil_bits);

		if (stc == NULL)
			return -1;

		hidgl_stenc = *stc;
		return stencilgl_init(stencil_bits);
	}
	return 0;
}

void hidgl_uninit(void)
{
	if (hidgl_draw.uninit != NULL)
		hidgl_draw.uninit();
}

void hidgl_flush_drawing(void)
{
	hidgl_draw.prim_flush();
}

void hidgl_flush(void)
{
	hidgl_draw.flush();
}

void hidgl_reset(void)
{
	hidgl_draw.reset();
}

void hidgl_set_color(float r, float g, float b, float a)
{
	hidgl_draw.set_color(r, g, b, a);
}

void hidgl_set_grid_color(float r, float g, float b)
{
	if (hidgl_draw.xor_inverts_clr)
		hidgl_draw.set_color(1.0-r, 1.0-g, 1.0-b, 1);
	else
		hidgl_draw.set_color(r, g, b, 1);
}

void hidgl_set_view(double tx, double ty, double zx, double zy, double zz)
{
	/* Translate to screen coords on CPU - see the comment at VTX declaration */
	VTX = tx;
	VTY = ty;
	hidgl_draw.set_view(0, 0, zx, zy, zz);
}

long hidgl_texture_import(unsigned char *pixels, int width, int height, int has_alpha)
{
	return hidgl_draw.texture_import(pixels, width, height, has_alpha);
}

void hidgl_texture_free(long id)
{
	hidgl_draw.texture_free(id);
}

void hidgl_push_matrix(int projection)
{
	hidgl_draw.push_matrix(projection);
}

void hidgl_pop_matrix(int projection)
{
	hidgl_draw.pop_matrix(projection);
}


static rnd_composite_op_t composite_op = RND_HID_COMP_RESET;
static rnd_bool direct_mode = rnd_true;

static GLfloat *grid_points = NULL, *grid_points3 = NULL;
static int grid_point_capacity = 0, grid_point_capacity3 = 0;

rnd_composite_op_t hidgl_get_drawing_mode()
{
	return composite_op;
}

void hidgl_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
	rnd_bool xor_mode = (composite_op == RND_HID_COMP_POSITIVE_XOR ? rnd_true : rnd_false);

	/* If the previous mode was NEGATIVE then all of the primitives drawn
	   in that mode were used only for creating the stencil and will not be
	   drawn directly to the color buffer. Therefore these primitives can be
	   discarded by rewinding the primitive buffer to the marker that was
	   set when entering NEGATIVE mode. */
	if (composite_op == RND_HID_COMP_NEGATIVE) {
		hidgl_draw.prim_flush();
		hidgl_draw.prim_rewind_to_marker();
	}

	composite_op = op;
	direct_mode = direct;

	switch (op) {
		case RND_HID_COMP_RESET:         drawgl_mode_reset(direct, screen); break;
		case RND_HID_COMP_POSITIVE_XOR:  drawgl_mode_positive_xor(direct, screen); break;
		case RND_HID_COMP_POSITIVE:      drawgl_mode_positive(direct, screen); break;
		case RND_HID_COMP_NEGATIVE:      drawgl_mode_negative(direct, screen); break;
		case RND_HID_COMP_FLUSH:         drawgl_mode_flush(direct, xor_mode, screen); break;
		default: break;
	}
}


void hidgl_fill_rect(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	x1 += VTX; y1 += VTY;
	x2 += VTX; y2 += VTY;
	/* if this is done with hidgl_draw.prim_add_fillrect(), it breaks the mask */
	hidgl_draw.prim_add_triangle(x1, y1, x1, y2, x2, y2);
	hidgl_draw.prim_add_triangle(x2, y1, x2, y2, x1, y1);
}


RND_INLINE void reserve_grid_points(int n, int n3)
{
	if (n > grid_point_capacity) {
		grid_point_capacity = n + 10;
		grid_points = realloc(grid_points, grid_point_capacity * 2 * sizeof(GLfloat));
	}
	if (n3 > grid_point_capacity3) {
		grid_point_capacity3 = n3 + 10;
		grid_points3 = realloc(grid_points3, grid_point_capacity3 * 2 * sizeof(GLfloat));
	}
}

void hidgl_draw_local_grid(rnd_design_t *hidlib, rnd_coord_t grd, rnd_coord_t cx, rnd_coord_t cy, int radius, double scale, rnd_bool cross_grid)
{
	int npoints = 0;
	rnd_coord_t x, y;

	/* PI is approximated with 3.25 here - allows a minimal overallocation, speeds up calculations */
	const int r2 = radius * radius;
	const int n = r2 * 3 + r2 / 4 + 1;

	reserve_grid_points(cross_grid ? n*5 : n, 0);

	cx += VTX;
	cy += VTY;

	for(y = -radius; y <= radius; y++) {
		int y2 = y * y;
		for(x = -radius; x <= radius; x++) {
			if (x * x + y2 < r2) {
				double px = x * hidlib->grid + cx, py = y * hidlib->grid + cy;
				grid_points[npoints * 2] = px;
				grid_points[npoints * 2 + 1] = py;
				npoints++;
				if (cross_grid) {
					grid_points[npoints * 2] = px-scale;
					grid_points[npoints * 2 + 1] = py;
					npoints++;
					grid_points[npoints * 2] = px+scale;
					grid_points[npoints * 2 + 1] = py;
					npoints++;
					grid_points[npoints * 2] = px;
					grid_points[npoints * 2 + 1] = py-scale;
					npoints++;
					grid_points[npoints * 2] = px;
					grid_points[npoints * 2 + 1] = py+scale;
					npoints++;
				}
			}
		}
	}

	hidgl_draw.xor_start();
	hidgl_draw.draw_points_pre(grid_points);
	hidgl_draw.draw_points(npoints);
	hidgl_draw.draw_points_post();
	hidgl_draw.xor_end();
}

void hidgl_draw_grid(rnd_design_t *hidlib, rnd_coord_t grd, rnd_box_t *drawn_area, double scale, rnd_bool cross_grid)
{
	rnd_coord_t x1, y1, x2, y2, n, i, n3;
	double x, y;

	hidgl_draw.xor_start();

	x1 = rnd_grid_fit(MAX(hidlib->dwg.X1, drawn_area->X1), grd, hidlib->grid_ox);
	y1 = rnd_grid_fit(MAX(hidlib->dwg.Y1, drawn_area->Y1), grd, hidlib->grid_oy);
	x2 = rnd_grid_fit(MIN(hidlib->dwg.X2, drawn_area->X2), grd, hidlib->grid_ox);
	y2 = rnd_grid_fit(MIN(hidlib->dwg.Y2, drawn_area->Y2), grd, hidlib->grid_oy);

	if (x1 > x2) {
		rnd_coord_t tmp = x1;
		x1 = x2;
		x2 = tmp;
	}

	if (y1 > y2) {
		rnd_coord_t tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

	x1 += VTX; y1 += VTY;
	x2 += VTX; y2 += VTY;

	n = (int)((x2 - x1) / grd + 0.5) + 1;
	reserve_grid_points(n, cross_grid ? n*2 : 0);

	/* draw grid center points and y offset points */
	hidgl_draw.draw_points_pre(grid_points);

	n = 0;
	for(x = x1; x <= x2; x += grd, ++n)
		grid_points[2 * n + 0] = x;

	for(y = y1; y <= y2; y += grd) {
		for(i = 0; i < n; i++)
			grid_points[2 * i + 1] = y;
		hidgl_draw.draw_points(n);

		if (cross_grid) { /* vertical extension */
			for(i = 0; i < n; i++)
				grid_points[2 * i + 1] = y-scale;
			hidgl_draw.draw_points(n);
			for(i = 0; i < n; i++)
				grid_points[2 * i + 1] = y+scale;
			hidgl_draw.draw_points(n);
		}
	}
	hidgl_draw.draw_points_post();


	if (cross_grid) {
		/* draw grid points around the crossings in x.x pattern (horizontal arms) */
		hidgl_draw.draw_points_pre(grid_points3);

		n3 = 0;
		for(x = x1; x <= x2; x += grd) {
			grid_points3[2 * n3 + 0] = x - scale; n3++;
			grid_points3[2 * n3 + 0] = x + scale; n3++;
		}

		for(y = y1; y <= y2; y += grd) {
			for(i = 0; i < n3; i++)
				grid_points3[2 * i + 1] = y;
			hidgl_draw.draw_points(n3);
		}

		hidgl_draw.draw_points_post();
	}

	hidgl_draw.xor_end();
}

#define MAX_PIXELS_ARC_TO_CHORD 0.5
#define MIN_SLICES 6
int calc_slices(float pix_radius, float sweep_angle)
{
	float slices;

	if (pix_radius <= MAX_PIXELS_ARC_TO_CHORD)
		return MIN_SLICES;

	slices = sweep_angle / acosf(1 - MAX_PIXELS_ARC_TO_CHORD / pix_radius) / 2.;
	return (int)ceilf(slices);
}

#define MIN_TRIANGLES_PER_CAP 3
#define MAX_TRIANGLES_PER_CAP 90
static void draw_round_cap(rnd_coord_t width, rnd_coord_t x, rnd_coord_t y, rnd_angle_t angle, double scale)
{
	float last_capx, last_capy;
	float capx, capy;
	float radius = width / 2.;
	int slices = calc_slices(radius / scale, M_PI);
	int i;

	if (slices < MIN_TRIANGLES_PER_CAP)
		slices = MIN_TRIANGLES_PER_CAP;

	if (slices > MAX_TRIANGLES_PER_CAP)
		slices = MAX_TRIANGLES_PER_CAP;

	hidgl_draw.prim_reserve_triangles(slices);

	last_capx = radius * cosf(angle * M_PI / 180.) + x;
	last_capy = -radius * sinf(angle * M_PI / 180.) + y;
	for(i = 0; i < slices; i++) {
		capx = radius * cosf(angle * M_PI / 180. + ((float)(i + 1)) * M_PI / (float)slices) + x;
		capy = -radius * sinf(angle * M_PI / 180. + ((float)(i + 1)) * M_PI / (float)slices) + y;
		hidgl_draw.prim_add_triangle(last_capx, last_capy, capx, capy, x, y);
		last_capx = capx;
		last_capy = capy;
	}
}

#define NEEDS_CAP(width, coord_per_pix) (width > coord_per_pix)

void hidgl_draw_line(rnd_cap_style_t cap, rnd_coord_t width, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, double scale)
{
	double angle;
	float deltax, deltay, length;
	float wdx, wdy;
	int round_caps = 0;
	rnd_coord_t orig_width = width;

	x1 += VTX; y1 += VTY;
	x2 += VTX; y2 += VTY;

	if ((width == 0) || (!NEEDS_CAP(orig_width, scale)))
		hidgl_draw.prim_add_line(x1, y1, x2, y2);
	else {
		if (width < scale)
			width = scale;

		deltax = x2 - x1;
		deltay = y2 - y1;

		length = sqrt(deltax * deltax + deltay * deltay);

		if (length == 0) {
			/* Assume the orientation of the line is horizontal */
			angle = 0;
			wdx = 0;
			wdy = width / 2.;
			length = 1.;
			deltax = 1.;
			deltay = 0.;
		}
		else {
			wdy = deltax * width / 2. / length;
			wdx = -deltay * width / 2. / length;

			if (deltay == 0.)
				angle = (deltax < 0) ? 270. : 90.;
			else
				angle = 180. / M_PI * atanl(deltax / deltay);

			if (deltay < 0)
				angle += 180.;
		}

		switch (cap) {
			case rnd_cap_round:
				round_caps = 1;
				break;

			case rnd_cap_square:
				x1 -= deltax * width / 2. / length;
				y1 -= deltay * width / 2. / length;
				x2 += deltax * width / 2. / length;
				y2 += deltay * width / 2. / length;
				break;

			default:
				assert(!"unhandled cap");
				round_caps = 1;
		}

		hidgl_draw.prim_add_triangle(x1 - wdx, y1 - wdy, x2 - wdx, y2 - wdy, x2 + wdx, y2 + wdy);
		hidgl_draw.prim_add_triangle(x1 - wdx, y1 - wdy, x2 + wdx, y2 + wdy, x1 + wdx, y1 + wdy);

		if (round_caps) {
			draw_round_cap(width, x1, y1, angle, scale);
			draw_round_cap(width, x2, y2, angle + 180., scale);
		}
	}
}

#define MIN_SLICES_PER_ARC 6
#define MAX_SLICES_PER_ARC 360
void hidgl_draw_arc(rnd_coord_t width, rnd_coord_t x, rnd_coord_t y, rnd_coord_t rx, rnd_coord_t ry, rnd_angle_t start_angle, rnd_angle_t delta_angle, double scale)
{
	float last_inner_x, last_inner_y;
	float last_outer_x, last_outer_y;
	float inner_x, inner_y;
	float outer_x, outer_y;
	float inner_r;
	float outer_r;
	float cos_ang, sin_ang;
	float start_angle_rad;
	float delta_angle_rad;
	float angle_incr_rad;
	int slices;
	int i;
	int hairline = 0;
	rnd_coord_t orig_width = width;

	x += VTX; y += VTY;

	/* TODO: Draw hairlines as lines instead of triangles ? */

	if (width == 0.0)
		hairline = 1;

	if (width < scale)
		width = scale;

	inner_r = rx - width / 2.;
	outer_r = rx + width / 2.;

	if (delta_angle < 0) {
		start_angle += delta_angle;
		delta_angle = -delta_angle;
	}

	start_angle_rad = start_angle * M_PI / 180.;
	delta_angle_rad = delta_angle * M_PI / 180.;

	slices = calc_slices((rx + width / 2.) / scale, delta_angle_rad);

	if (slices < MIN_SLICES_PER_ARC)
		slices = MIN_SLICES_PER_ARC;

	if (slices > MAX_SLICES_PER_ARC)
		slices = MAX_SLICES_PER_ARC;

	hidgl_draw.prim_reserve_triangles(2 * slices);

	angle_incr_rad = delta_angle_rad / (float)slices;

	cos_ang = cosf(start_angle_rad);
	sin_ang = sinf(start_angle_rad);
	last_inner_x = -inner_r * cos_ang + x;
	last_inner_y = inner_r * sin_ang + y;
	last_outer_x = -outer_r * cos_ang + x;
	last_outer_y = outer_r * sin_ang + y;
	for(i = 1; i <= slices; i++) {
		cos_ang = cosf(start_angle_rad + ((float)(i)) * angle_incr_rad);
		sin_ang = sinf(start_angle_rad + ((float)(i)) * angle_incr_rad);
		inner_x = -inner_r * cos_ang + x;
		inner_y = inner_r * sin_ang + y;
		outer_x = -outer_r * cos_ang + x;
		outer_y = outer_r * sin_ang + y;

		hidgl_draw.prim_add_triangle(last_inner_x, last_inner_y, last_outer_x, last_outer_y, outer_x, outer_y);
		hidgl_draw.prim_add_triangle(last_inner_x, last_inner_y, inner_x, inner_y, outer_x, outer_y);

		last_inner_x = inner_x;
		last_inner_y = inner_y;
		last_outer_x = outer_x;
		last_outer_y = outer_y;
	}

	/* Don't bother capping hairlines */
	if (hairline)
		return;

	if (NEEDS_CAP(orig_width, scale)) {
		draw_round_cap(width, x + rx * -cosf(start_angle_rad), y + rx * sinf(start_angle_rad), start_angle, scale);
		draw_round_cap(width, x + rx * -cosf(start_angle_rad + delta_angle_rad), y + rx * sinf(start_angle_rad + delta_angle_rad), start_angle + delta_angle + 180., scale);
	}
}

void hidgl_draw_rect(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	hidgl_draw.prim_add_rect(x1 + VTX, y1 + VTY, x2 + VTX, y2 + VTY);
}

void hidgl_draw_texture_rect(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, unsigned long texture_id)
{
	hidgl_draw.prim_add_textrect(x1 + VTX, y1 + VTY, 0.0, 0.0, x2 + VTX, y1 + VTY, 1.0, 0.0, x2 + VTX, y2 + VTY, 1.0, 1.0, x1 + VTX, y2 + VTY, 0.0, 1.0, texture_id);
}

void hidgl_fill_circle(rnd_coord_t vx, rnd_coord_t vy, rnd_coord_t vr, double scale)
{
#define MIN_TRIANGLES_PER_CIRCLE 6
#define MAX_TRIANGLES_PER_CIRCLE 360

	float last_x, last_y;
	float radius = vr;
	int slices;
	int i;

	vx += VTX; vy += VTY;

	assert((composite_op == RND_HID_COMP_POSITIVE) || (composite_op == RND_HID_COMP_POSITIVE_XOR) || (composite_op == RND_HID_COMP_NEGATIVE));

	slices = calc_slices(vr / scale, 2 * M_PI);

	if (slices < MIN_TRIANGLES_PER_CIRCLE)
		slices = MIN_TRIANGLES_PER_CIRCLE;

	if (slices > MAX_TRIANGLES_PER_CIRCLE)
		slices = MAX_TRIANGLES_PER_CIRCLE;

	hidgl_draw.prim_reserve_triangles(slices);

	last_x = vx + vr;
	last_y = vy;

	for(i = 0; i < slices; i++) {
		float x, y;
		x = radius * cosf(((float)(i + 1)) * 2. * M_PI / (float)slices) + vx;
		y = radius * sinf(((float)(i + 1)) * 2. * M_PI / (float)slices) + vy;
		hidgl_draw.prim_add_triangle(vx, vy, last_x, last_y, x, y);
		last_x = x;
		last_y = y;
	}
}


#define MAX_COMBINED_MALLOCS 2500
static void *combined_to_free[MAX_COMBINED_MALLOCS];
static int combined_num_to_free = 0;

static GLenum tessVertexType;
static int stashed_vertices;
static int triangle_comp_idx;

static void myError(GLenum errno)
{
	printf("gluTess error: %s\n", gluErrorString(errno));
}

static void myFreeCombined()
{
	while(combined_num_to_free)
		free(combined_to_free[--combined_num_to_free]);
}

static void myCombine(GLdouble coords[3], void *vertex_data[4], GLfloat weight[4], void **dataOut)
{
#define MAX_COMBINED_VERTICES 2500
	static GLdouble combined_vertices[3 * MAX_COMBINED_VERTICES];
	static int num_combined_vertices = 0;

	GLdouble *new_vertex;

	if (num_combined_vertices < MAX_COMBINED_VERTICES) {
		new_vertex = &combined_vertices[3 * num_combined_vertices];
		num_combined_vertices++;
	}
	else {
		new_vertex = malloc(3 * sizeof(GLdouble));

		if (combined_num_to_free < MAX_COMBINED_MALLOCS)
			combined_to_free[combined_num_to_free++] = new_vertex;
		else
			printf("myCombine leaking %lu bytes of memory\n", 3 * sizeof(GLdouble));
	}

	new_vertex[0] = coords[0];
	new_vertex[1] = coords[1];
	new_vertex[2] = coords[2];

	*dataOut = new_vertex;
}

static void myBegin(GLenum type)
{
	tessVertexType = type;
	stashed_vertices = 0;
	triangle_comp_idx = 0;
}

static void myVertex(GLdouble *vertex_data)
{
	static GLfloat triangle_vertices[2 *3];

	if (tessVertexType == GL_TRIANGLE_STRIP || tessVertexType == GL_TRIANGLE_FAN) {
		if (stashed_vertices < 2) {
			triangle_vertices[triangle_comp_idx++] = vertex_data[0];
			triangle_vertices[triangle_comp_idx++] = vertex_data[1];
			stashed_vertices++;
		}
		else {
			hidgl_draw.prim_add_triangle(triangle_vertices[0], triangle_vertices[1], triangle_vertices[2], triangle_vertices[3], vertex_data[0], vertex_data[1]);
			if (tessVertexType == GL_TRIANGLE_STRIP) {
				/* STRIP saves the last two vertices for re-use in the next triangle */
				triangle_vertices[0] = triangle_vertices[2];
				triangle_vertices[1] = triangle_vertices[3];
			}
			/* Both FAN and STRIP save the last vertex for re-use in the next triangle */
			triangle_vertices[2] = vertex_data[0];
			triangle_vertices[3] = vertex_data[1];
		}
	}
	else if (tessVertexType == GL_TRIANGLES) {
		triangle_vertices[triangle_comp_idx++] = vertex_data[0];
		triangle_vertices[triangle_comp_idx++] = vertex_data[1];
		stashed_vertices++;
		if (stashed_vertices == 3) {
			hidgl_draw.prim_add_triangle(triangle_vertices[0], triangle_vertices[1], triangle_vertices[2], triangle_vertices[3], triangle_vertices[4], triangle_vertices[5]);
			triangle_comp_idx = 0;
			stashed_vertices = 0;
		}
	}
	else
		printf("Vertex received with unknown type\n");
}

/* Intentional code duplication for performance */
void hidgl_fill_polygon(int n_coords, rnd_coord_t *x, rnd_coord_t *y)
{
	int i;
	GLUtesselator *tobj;
	GLdouble *vertices;

	assert(n_coords > 0);

	vertices = malloc(sizeof(GLdouble) * n_coords * 3);

	tobj = gluNewTess();
	gluTessCallback(tobj, GLU_TESS_BEGIN, (_GLUfuncptr) myBegin);
	gluTessCallback(tobj, GLU_TESS_VERTEX, (_GLUfuncptr) myVertex);
	gluTessCallback(tobj, GLU_TESS_COMBINE, (_GLUfuncptr) myCombine);
	gluTessCallback(tobj, GLU_TESS_ERROR, (_GLUfuncptr) myError);

	gluTessBeginPolygon(tobj, NULL);
	gluTessBeginContour(tobj);

	for(i = 0; i < n_coords; i++) {
		vertices[0 + i * 3] = x[i] + VTX;
		vertices[1 + i * 3] = y[i] + VTY;
		vertices[2 + i * 3] = 0.;
		gluTessVertex(tobj, &vertices[i * 3], &vertices[i * 3]);
	}

	gluTessEndContour(tobj);
	gluTessEndPolygon(tobj);
	gluDeleteTess(tobj);

	myFreeCombined();
	free(vertices);
}

/* Intentional code duplication for performance */
void hidgl_fill_polygon_offs(int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	int i;
	GLUtesselator *tobj;
	GLdouble *vertices;

	assert(n_coords > 0);

	vertices = malloc(sizeof(GLdouble) * n_coords * 3);

	tobj = gluNewTess();
	gluTessCallback(tobj, GLU_TESS_BEGIN, (_GLUfuncptr) myBegin);
	gluTessCallback(tobj, GLU_TESS_VERTEX, (_GLUfuncptr) myVertex);
	gluTessCallback(tobj, GLU_TESS_COMBINE, (_GLUfuncptr) myCombine);
	gluTessCallback(tobj, GLU_TESS_ERROR, (_GLUfuncptr) myError);

	gluTessBeginPolygon(tobj, NULL);
	gluTessBeginContour(tobj);

	dx += VTX;
	dy += VTY;

	for(i = 0; i < n_coords; i++) {
		vertices[0 + i * 3] = x[i] + dx;
		vertices[1 + i * 3] = y[i] + dy;
		vertices[2 + i * 3] = 0.;
		gluTessVertex(tobj, &vertices[i * 3], &vertices[i * 3]);
	}

	gluTessEndContour(tobj);
	gluTessEndPolygon(tobj);
	gluDeleteTess(tobj);

	myFreeCombined();
	free(vertices);
}

void hidgl_expose_init(int x0, int y0, int w, int h, const rnd_color_t *bg_c)
{
	hidgl_draw.expose_init(x0, y0, w, h, bg_c);
}

void hidgl_draw_crosshair(rnd_coord_t x, rnd_coord_t y, float red, float green, float blue, rnd_coord_t minx, rnd_coord_t miny, rnd_coord_t maxx, rnd_coord_t maxy)
{
	GLfloat points[4][6];

	points[0][0] = x + VTX;
	points[0][1] = miny + VTY;
	points[1][0] = x + VTX;
	points[1][1] = maxy + VTY;
	points[2][0] = minx + VTX;
	points[2][1] = y + VTY;
	points[3][0] = maxx + VTX;
	points[3][1] = y + VTY;

	if (hidgl_draw.xor_start() == 0) {
		red = 1.0 - red;
		green = 1.0 - green;
		blue = 1.0 - blue;
	}
	hidgl_draw.draw_lines6((GLfloat *)points, 4, red, green, blue, 1.0);
	hidgl_draw.xor_end();
}

void hidgl_draw_initial_fill(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, float r, float g, float b)
{
	hidgl_draw.xor_end();

	x1 += VTX; y1 += VTY;
	x2 += VTX; y2 += VTY;

	/* we can cheat here: this is called only once, before other drawing commands
	   to fill the background. */
	hidgl_draw.set_color(r, g, b, 1.0f);

	/* fillrect breaks for some reason, drawing two triangles is more stable */
	hidgl_draw.prim_add_triangle(x1, y1, x1, y2, x2, y2);
	hidgl_draw.prim_add_triangle(x2, y1, x2, y2, x1, y1);
}

