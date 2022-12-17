/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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

/* Low level gl rendering: dummy fallback implementation to throw error */

#include "config.h"
#include <stdlib.h>

#include <librnd/core/error.h>

#include "draw.h"

static void error_set_color(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { }
static void error_prim_add_triangle(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat x3, GLfloat y3) { }
static void error_prim_add_textrect(GLfloat x1, GLfloat y1, GLfloat u1, GLfloat v1, GLfloat x2, GLfloat y2, GLfloat u2, GLfloat v2, GLfloat x3, GLfloat y3, GLfloat u3, GLfloat v3, GLfloat x4, GLfloat y4, GLfloat u4, GLfloat v4, GLuint texture_id) { }
static void error_prim_add_line(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) { }
static void error_prim_add_rect(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) { }
static void error_prim_add_fillrect(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) { }
static void error_prim_reserve_triangles(int count) { }
static void error_prim_flush(void) { }
static void error_prim_draw_all(int stencil_bits) { }
static void error_flush(void) { }
static void error_reset(void) { }
static void error_push_matrix(int projection) { }
static void error_pop_matrix(int projection) { }
static long error_texture_import(unsigned char *pixels, int width, int height, int has_alpha) { return 0; }
static void error_texture_free(long id) { }
static void error_prim_set_marker(void) { }
static void error_prim_rewind_to_marker(void) { }
static void error_draw_points_pre(GLfloat *pts) { }
static void error_draw_points(int npts) { }
static void error_draw_points_post(void) { }
static void error_draw_lines6(GLfloat *pts, int npts, float red, float green, float blue, float alpha) { }
static void error_expose_init(int x0, int y0, int w, int h, const rnd_color_t *bg_c) { }
static void error_set_view(double tx, double ty, double zx, double zy, double zz) { }
static int error_xor_start(void) { return -1; }
static void error_xor_end(void) { }
static void error_uninit(void) { }
static int error_new_context(void) { return -1; }

static int error_init(void)
{
	rnd_message(RND_MSG_ERROR, "No opengl backend available. Not rendering any drawing.\n");
	return 0;
}

hidgl_draw_t hidgl_draw_error = {
	"error", 0,

	error_init,
	error_uninit,
	error_new_context,
	error_set_color,
	error_flush,
	error_reset,
	error_expose_init,
	error_set_view,
	error_texture_import,
	error_texture_free,
	error_push_matrix,
	error_pop_matrix,
	error_xor_start,
	error_xor_end,

	error_prim_draw_all,
	error_prim_set_marker,
	error_prim_rewind_to_marker,
	error_prim_reserve_triangles,
	error_prim_flush,

	error_prim_add_triangle,
	error_prim_add_line,
	error_prim_add_rect,
	error_prim_add_fillrect,
	error_prim_add_textrect,

	error_draw_points_pre,
	error_draw_points,
	error_draw_points_post,
	error_draw_lines6
};
