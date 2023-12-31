/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2009-2011 PCB Contributors (See ChangeLog for details).
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
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

#ifndef RND_HID_COMMON_HIDGL_H
#define RND_HID_COMMON_HIDGL_H

#include <librnd/core/hidlib.h>
#include <librnd/hid/hid.h>

/*extern float global_depth;*/
void hidgl_draw_local_grid(rnd_design_t *hidlib, rnd_coord_t grd, rnd_coord_t cx, rnd_coord_t cy, int radius, double scale, rnd_bool cross_grid);
void hidgl_draw_grid(rnd_design_t *hidlib, rnd_coord_t grd, rnd_box_t *drawn_area, double scale, rnd_bool cross_grid);
void hidgl_set_depth(float depth);
void hidgl_draw_line(rnd_cap_style_t cap, rnd_coord_t width, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, double scale);
void hidgl_draw_arc(rnd_coord_t width, rnd_coord_t vx, rnd_coord_t vy, rnd_coord_t vrx, rnd_coord_t vry, rnd_angle_t start_angle, rnd_angle_t delta_angle, double scale);
void hidgl_draw_rect(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
void hidgl_draw_texture_rect(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, unsigned long texture_id);
void hidgl_fill_circle(rnd_coord_t vx, rnd_coord_t vy, rnd_coord_t vr, double scale);
void hidgl_fill_polygon(int n_coords, rnd_coord_t *x, rnd_coord_t *y);
void hidgl_fill_polygon_offs(int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy);
void hidgl_fill_rect(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
int hidgl_init(void);
int hidgl_new_context(void);
void hidgl_uninit(void);
int hidgl_stencil_init(void);
void hidgl_flush_drawing(void);  /* flush primitives from CPU memory to GPU memory - no glFlush(), so things are nto yet drawn */
void hidgl_flush(void);          /* equivalent of glFlush() */
void hidgl_reset(void);
void hidgl_set_color(float r, float g, float b, float a);
void hidgl_set_grid_color(float r, float g, float b);
void hidgl_set_view(double tx, double ty, double zx, double zy, double zz);
long hidgl_texture_import(unsigned char *pixels, int width, int height, int has_alpha);
void hidgl_texture_free(long id);
void hidgl_push_matrix(int projection);
void hidgl_pop_matrix(int projection);
void hidgl_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen);
rnd_composite_op_t hidgl_get_drawing_mode();

#define HIDGL_Z_NEAR 3.0

/* Prepare gl context for expose: set viewport, model, projection, stencil, color */
void hidgl_expose_init(int x0, int y0, int w, int h, const rnd_color_t *bg_c);

void hidgl_draw_crosshair(rnd_coord_t x, rnd_coord_t y, float red, float green, float blue, rnd_coord_t minx, rnd_coord_t miny, rnd_coord_t maxx, rnd_coord_t maxy);
void hidgl_draw_initial_fill(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, float red, float green, float blue);


#endif /* RND_HID_COMMON_HIDGL_H  */
