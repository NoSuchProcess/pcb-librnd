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

#include "draw_COMMON.c"

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

/* This function will draw the specified primitive but it may also modify the state of
   the stencil buffer when MASK primitives exist. */
RND_INLINE void drawgl_draw_primitive(primitive_t *prim)
{
	if (prim->texture_id > 0) {
		draw_common_config_texture(prim->texture_id);
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

static void direct_draw_lines6(GLfloat *pts, int npts, float red, float green, float blue, float alpha)
{
	GLfloat *p;
	int i;

	for(i = 0, p = pts; i < npts; i++, p += 6) {
		p[2] = red;
		p[3] = green;
		p[4] = blue;
		p[5] = alpha;
	}

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
	GLint profmask = 0, major;

	if (conf_lib_hid_gl.plugins.lib_hid_gl.backend.disable_direct) {
		rnd_message(RND_MSG_DEBUG, "opengl direct_init refuse: disabled from conf\n");
		return -1;
	}

	major = gl_get_ver_major();
	if (major < 0) {
		rnd_message(RND_MSG_DEBUG, "opengl direct_init accept: ancient opengl is probably compatible\n");
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

static int direct_new_context(void)
{
	return 0;
}


hidgl_draw_t hidgl_draw_direct = {
	"direct",

	direct_init,
	direct_uninit,
	direct_new_context,
	common_set_color,
	common_flush,
	direct_reset,
	direct_expose_init,
	direct_set_view,
	common_texture_import,
	direct_push_matrix,
	direct_pop_matrix,
	direct_xor_start,
	direct_xor_end,

	direct_prim_draw_all,
	common_prim_set_marker,
	common_prim_rewind_to_marker,
	common_prim_reserve_triangles,
	direct_prim_flush,

	common_prim_add_triangle,
	common_prim_add_line,
	common_prim_add_rect,
	direct_prim_add_fillrect,
	common_prim_add_textrect,

	direct_draw_points_pre,
	direct_draw_points,
	direct_draw_points_post,
	direct_draw_lines6,
};
