/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2009-2017 PCB Contributers (See ChangeLog for details)
 *  Copyright (C) 2017 Adrian Purser
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
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Low level gl rendering: direct access, pre-vao, works with gtk2.
   Required opengl API version: 2.0 */

#include "config.h"
#include <stdlib.h>

#include <librnd/core/color.h>

#include "opengl_debug.h"

#include "hidgl.h"
#include "stencil_gl.h"

#include "draw.h"

#include "lib_hid_gl_conf.h"
extern conf_lib_hid_gl_t conf_lib_hid_gl;

/* Vertex Buffer Data
   The vertex buffer is a dynamic array of vertices. Each vertex contains
   position and color information. */
typedef struct {
	GLfloat x, y;
	GLfloat u, v;
	GLfloat r, g, b, a;
} vertex_t;

#include "vertbuf.c"
#include "primbuf.c"

static GLfloat red = 0.0f, green = 0.0f, blue = 0.0f, alpha = 0.75f;

RND_INLINE void vertbuf_add(GLfloat x, GLfloat y)
{
	vertex_t *p_vert = vertbuf_allocate(1);
	if (p_vert) {
		p_vert->x = x;
		p_vert->y = y;
		p_vert->r = red;
		p_vert->g = green;
		p_vert->b = blue;
		p_vert->a = alpha;
	}
}

RND_INLINE void vertbuf_add_xyuv(GLfloat x, GLfloat y, GLfloat u, GLfloat v)
{
	vertex_t *p_vert = vertbuf_allocate(1);
	if (p_vert) {
		p_vert->x = x;
		p_vert->y = y;
		p_vert->u = u;
		p_vert->v = v;
		p_vert->r = 1.0;
		p_vert->g = 1.0;
		p_vert->b = 1.0;
		p_vert->a = 1.0;
	}
}

static void direct_set_color(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	red = r;
	green = g;
	blue = b;
	alpha = a;
}

static void direct_prim_add_triangle(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat x3, GLfloat y3)
{
	/* Debug Drawing */
#if 0
	primbuf_add(GL_LINES,vertbuf.size,6);
	vertbuf_reserve_extra(6);
	vertbuf_add(x1,y1);  vertbuf_add(x2,y2);
	vertbuf_add(x2,y2);  vertbuf_add(x3,y3);
	vertbuf_add(x3,y3);  vertbuf_add(x1,y1);
#endif

	primbuf_add(GL_TRIANGLES, vertbuf.size, 3, 0);
	vertbuf_reserve_extra(3);
	vertbuf_add(x1, y1);
	vertbuf_add(x2, y2);
	vertbuf_add(x3, y3);
}

static void direct_prim_add_textrect(GLfloat x1, GLfloat y1, GLfloat u1, GLfloat v1,
	GLfloat x2, GLfloat y2, GLfloat u2, GLfloat v2,
	GLfloat x3, GLfloat y3, GLfloat u3, GLfloat v3,
	GLfloat x4, GLfloat y4, GLfloat u4, GLfloat v4,
	GLuint texture_id)
{
	primbuf_add(GL_TRIANGLE_FAN, vertbuf.size, 4, texture_id);
	vertbuf_reserve_extra(4);
	vertbuf_add_xyuv(x1, y1, u1, v1);
	vertbuf_add_xyuv(x2, y2, u2, v2);
	vertbuf_add_xyuv(x3, y3, u3, v3);
	vertbuf_add_xyuv(x4, y4, u4, v4);
}

static void direct_prim_add_line(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	primbuf_add(GL_LINES, vertbuf.size, 2, 0);
	vertbuf_reserve_extra(2);
	vertbuf_add(x1, y1);
	vertbuf_add(x2, y2);
}

static void direct_prim_add_rect(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	primbuf_add(GL_LINE_LOOP, vertbuf.size, 4, 0);
	vertbuf_reserve_extra(4);
	vertbuf_add(x1, y1);
	vertbuf_add(x2, y1);
	vertbuf_add(x2, y2);
	vertbuf_add(x1, y2);
}

RND_INLINE void direct_draw_rect(GLenum mode, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	float points[4][6];
	int i;

	for(i=0; i<4; ++i) {
		points[i][2] = red;
		points[i][3] = green;
		points[i][4] = blue;
		points[i][5] = alpha;
	}

	points[0][0] = x1; points[0][1] = y1;
	points[1][0] = x2; points[1][1] = y1;
	points[2][0] = x2; points[2][1] = y2;
	points[3][0] = x1; points[3][1] = y2;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(float) * 6, points);
	glColorPointer(4, GL_FLOAT, sizeof(float) * 6, &points[0][2]);
	glDrawArrays(mode, 0, 4);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

RND_INLINE void direct_draw_rectangle(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	direct_draw_rect(GL_LINE_LOOP, x1, y1, x2, y2);
}

static void direct_prim_add_fillrect(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	direct_draw_rect(GL_TRIANGLE_FAN, x1, y1, x2, y2);
}

static void direct_prim_reserve_triangles(int count)
{
	vertbuf_reserve_extra(count * 3);
}

/* This function will draw the specified primitive but it may also modify the state of
   the stencil buffer when MASK primitives exist. */
RND_INLINE void drawgl_draw_primitive(primitive_t *prim)
{
	if (prim->texture_id > 0) {
		glBindTexture(GL_TEXTURE_2D, prim->texture_id);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glEnable(GL_TEXTURE_2D);
		glAlphaFunc(GL_GREATER, 0.5);
		glEnable(GL_ALPHA_TEST);
	}

	glDrawArrays(prim->type, prim->first, prim->count);

	if (prim->texture_id > 0) {
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);
	}
}

RND_INLINE void direct_begin_prim_vertbuf(void)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(vertex_t), &vertbuf.data[0].x);
	glTexCoordPointer(2, GL_FLOAT, sizeof(vertex_t), &vertbuf.data[0].u);
	glColorPointer(4, GL_FLOAT, sizeof(vertex_t), &vertbuf.data[0].r);
}

RND_INLINE void direct_end_prim_vertbuf(void)
{
	/* disable the vertex buffer */
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

static void direct_prim_flush(void)
{
	int index = primbuf.dirty_index;
	int end = primbuf.size;
	primitive_t *prim = &primbuf.data[index];

	direct_begin_prim_vertbuf();

	/* draw the primitives */
	while(index < end) {
		drawgl_draw_primitive(prim);
		++prim;
		++index;
	}

	direct_end_prim_vertbuf();

	primbuf.dirty_index = end;
}

static void direct_prim_draw_all(int stencil_bits)
{
	int index = primbuf.size;
	primitive_t *prim;

	if ((index == 0) || (primbuf.data == NULL))
		return;

	--index;
	prim = &primbuf.data[index];

	direct_begin_prim_vertbuf();

	/* draw the primitives */
	while(index >= 0) {
		drawgl_draw_primitive(prim);
		--prim;
		--index;
	}

	direct_end_prim_vertbuf();
}

static void direct_flush(void)
{
	glFlush();
}

static void direct_reset(void)
{
	vertbuf_clear();
	primbuf_clear();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_STENCIL_TEST);
	hidgl_draw.xor_end();
}

static void direct_push_matrix(int projection)
{
	if (projection)
		glMatrixMode(GL_PROJECTION);
	glPushMatrix();
}

static void direct_pop_matrix(int projection)
{
	if (projection)
		glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}

static long direct_texture_import(unsigned char *pixels, int width, int height, int has_alpha)
{
	GLuint texture_id;

	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, has_alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, pixels);

	return texture_id;
}


static void direct_prim_set_marker(void)
{
	vertbuf_set_marker();
	primbuf_set_marker();
}

static void direct_prim_rewind_to_marker(void)
{
	vertbuf_rewind();
	primbuf_rewind();
}

static void direct_draw_points_pre(GLfloat *pts)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, pts);
}

static void direct_draw_points(int npts)
{
	glDrawArrays(GL_POINTS, 0, npts);
}

static void direct_draw_points_post(void)
{
	glDisableClientState(GL_VERTEX_ARRAY);
}

static void direct_draw_lines6(GLfloat *pts, int npts)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(float) * 6, pts);
	glColorPointer(4, GL_FLOAT, sizeof(float) * 6, pts+2);
	glDrawArrays(GL_LINES, 0, npts);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}


static void direct_expose_init(int w, int h, const rnd_color_t *bg_c)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, 0, 100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -HIDGL_Z_NEAR);

	glEnable(GL_STENCIL_TEST);
	glClearColor(bg_c->fr, bg_c->fg, bg_c->fb, 1.);
	glStencilMask(~0);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	stencilgl_reset_stencil_usage();

	/* Disable the stencil test until we need it - otherwise it gets dirty */
	glDisable(GL_STENCIL_TEST);
	glStencilMask(0);
	glStencilFunc(GL_ALWAYS, 0, 0);
}

static void direct_set_view(double tx, double ty, double zx, double zy, double zz)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -HIDGL_Z_NEAR);
	glScalef(zx, zy, zz);
	glTranslatef(tx, ty, 0);
}

static void direct_xor_start(void)
{
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_XOR);
}

static void direct_xor_end(void)
{
	glDisable(GL_COLOR_LOGIC_OP);
}


static void direct_uninit(void)
{
	vertbuf_destroy();
	primbuf_destroy();
}

static int direct_init(void)
{
	GLint profmask, major = 0;

	if (conf_lib_hid_gl.plugins.lib_hid_gl.backend.disable_direct) {
		rnd_message(RND_MSG_DEBUG, "opengl direct_init refuse: disabled from conf\n");
		return -1;
	}

#ifdef GL_MAJOR_VERSION
	glGetIntegerv(GL_MAJOR_VERSION, &major);
#endif

	if (major == 0)
		glGetIntegerv(GL_VERSION, &major);

	if (major == 0) {
		const GLubyte *verstr = glGetString(GL_VERSION);
		rnd_message(RND_MSG_DEBUG, "opengl direct_init accept: you have a real ancient opengl version '%s'\n", verstr == NULL ? "<unknown>" : (const char *)verstr);
		return 0;
	}

	if (major < 3) {
		rnd_message(RND_MSG_DEBUG, "opengl direct_init accept: major %d is below 3\n", major);
		return 0;
	}

#ifdef GL_CONTEXT_PROFILE_MASK
	glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profmask);
	if ((profmask != 0) && !(profmask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)) {
		rnd_message(RND_MSG_DEBUG, "opengl direct_init refuse: GL_CONTEXT_PROFILE_MASK (%d) lacks compatibility mode in major %d\n", profmask, major);
		return -1;
	}
#else
	rnd_message(RND_MSG_DEBUG, "opengl direct_init refuse: GL_CONTEXT_PROFILE_MASK missing with major %d\n", major);
	return -1;
#endif

	rnd_message(RND_MSG_DEBUG, "opengl direct_init accept\n");
	return 0;
}

static void direct_new_context(void)
{
	return 0;
}


hidgl_draw_t hidgl_draw_direct = {
	"direct",

	direct_init,
	direct_uninit,
	direct_new_context,
	direct_set_color,
	direct_flush,
	direct_reset,
	direct_expose_init,
	direct_set_view,
	direct_texture_import,
	direct_push_matrix,
	direct_pop_matrix,
	direct_xor_start,
	direct_xor_end,

	direct_prim_draw_all,
	direct_prim_set_marker,
	direct_prim_rewind_to_marker,
	direct_prim_reserve_triangles,
	direct_prim_flush,

	direct_prim_add_triangle,
	direct_prim_add_line,
	direct_prim_add_rect,
	direct_prim_add_fillrect,
	direct_prim_add_textrect,

	direct_draw_points_pre,
	direct_draw_points,
	direct_draw_points_post,
	direct_draw_lines6,
};