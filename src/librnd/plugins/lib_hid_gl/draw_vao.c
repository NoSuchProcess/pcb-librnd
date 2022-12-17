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
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Low level gl rendering: vao, works with gtk4.
   Required opengl API version: 3.0 */

#include "config.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <librnd/core/color.h>

#include "opengl_debug.h"

#include "hidgl.h"
#include "stencil_gl.h"

#include "draw.h"

#include "lib_hid_gl_conf.h"
extern conf_lib_hid_gl_t conf_lib_hid_gl;

/* Store color in primitives instead of vertices */
#define PRIM_COLOR

/* Vertex Buffer Data
   The vertex buffer is a dynamic array of vertices. Each vertex contains
   position and color information. */
typedef struct {
	GLfloat x, y;
	GLfloat u, v;
} vertex_t;

#include "vertbuf.c"
#include "primbuf.c"

static GLfloat red = 0.0f, green = 0.0f, blue = 0.0f, alpha = 0.75f;

static GLuint program, inputColor_location, inputTexture_location, xform_location, position_buffer;
static int vao_xor_mode;

RND_INLINE void vertbuf_add(GLfloat x, GLfloat y)
{
	vertex_t *p_vert = vertbuf_allocate(1);
	if (p_vert) {
		p_vert->x = x;
		p_vert->y = y;
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
	}
}

#include "draw_COMMON.c"

static float vertbuf_last_r = -1, vertbuf_last_g = -1, vertbuf_last_b = -1, vertbuf_last_a = -1;

/* Upload vertex buffer buf; buf_size is the byte size of the whole buffer;
   the buffer is an array of elems of size elem_size. Each elem has an x,y
   coord pair at offset x_offs. */
RND_INLINE void vao_begin_vertbuf(void *buf, int comps, long buf_size, long elem_size, size_t x_offs)
{
	/* This is the buffer that holds the vertices */
	glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
	glBufferData(GL_ARRAY_BUFFER, buf_size, buf, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	/* Use the vertices in our buffer */
	glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, comps, GL_FLOAT, GL_FALSE, elem_size, (void *)x_offs);
}

RND_INLINE void vao_end_vertbuf(void)
{
}

/* Set drawing color of the following glDrawArrays() calls */
RND_INLINE void vao_color_vertbuf(float r, float g, float b, float a)
{
	if ((r != vertbuf_last_r) || (g != vertbuf_last_g) || (b != vertbuf_last_b) || (a != vertbuf_last_a)) {
		if (vao_xor_mode) {
			r = 1.0 - r;
			g = 1.0 - g;
			b = 1.0 - b;
			a = a / 2.0;
		}
		vertbuf_last_r = r;
		vertbuf_last_g = g;
		vertbuf_last_b = b;
		vertbuf_last_a = a;
		glUniform4f(inputColor_location, r, g, b, a);
	}
}


RND_INLINE void vao_draw_rect(GLenum mode, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	float points[4][2];

	points[0][0] = x1; points[0][1] = y1;
	points[1][0] = x2; points[1][1] = y1;
	points[2][0] = x2; points[2][1] = y2;
	points[3][0] = x1; points[3][1] = y2;

	vao_begin_vertbuf(points, 2, sizeof(points), sizeof(float) * 2, 0);
	vao_color_vertbuf(red, green, blue, alpha);
	glDrawArrays(mode, 0, 4);
	vao_end_vertbuf();
}

RND_INLINE void vao_draw_rectangle(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	vao_draw_rect(GL_LINE_LOOP, x1, y1, x2, y2);
}

static void vao_prim_add_fillrect(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	vao_draw_rect(GL_TRIANGLE_FAN, x1, y1, x2, y2);
}

/* This function will draw the specified primitive but it may also modify the state of
   the stencil buffer when MASK primitives exist. */
RND_INLINE void drawgl_draw_primitive(primitive_t *prim)
{
	if (prim->texture_id > 0) {
		glActiveTexture(GL_TEXTURE0);
		draw_common_config_texture(prim->texture_id);
		vao_color_vertbuf(-3.3, 0, 0, 0); /* enable using texture instead of color */
	}
	else
		vao_color_vertbuf(prim->r, prim->g, prim->b, prim->a);
/*printf("prim color: %f %f %f %f\n", prim->r, prim->g, prim->b, prim->a);*/

	glDrawArrays(prim->type, prim->first, prim->count);

	if (prim->texture_id > 0)
		glDisable(GL_TEXTURE_2D);
}



RND_INLINE void vao_begin_prim_vertbuf(void)
{
	vao_begin_vertbuf(vertbuf.data, 4, vertbuf.size * sizeof(vertex_t), sizeof(vertex_t),  offsetof(vertex_t, x));
}

RND_INLINE void vao_end_prim_vertbuf(void)
{
	vao_end_vertbuf();
}

static void vao_prim_flush(void)
{
	int index = primbuf.dirty_index;
	int end = primbuf.size;
	primitive_t *prim = &primbuf.data[index];

	if ((primbuf.size == 0) || (primbuf.data == NULL))
		return;

	vao_begin_prim_vertbuf();

	/* draw the primitives */
	while(index < end) {
		drawgl_draw_primitive(prim);
		++prim;
		++index;
	}

	vao_end_prim_vertbuf();

	primbuf.dirty_index = end;
}

static void vao_prim_draw_all(int stencil_bits)
{
	int index = primbuf.size;
	primitive_t *prim;

	if ((index == 0) || (primbuf.data == NULL))
		return;

	--index;
	prim = &primbuf.data[index];

	vao_begin_prim_vertbuf();

	/* draw the primitives */
	while(index >= 0) {
		drawgl_draw_primitive(prim);
		--prim;
		--index;
	}

	vao_end_prim_vertbuf();
}

static void vao_reset(void)
{
	vertbuf_clear();
	primbuf_clear();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_STENCIL_TEST);
	hidgl_draw.xor_end();
}

/* We don't have matrix or stack for matrices, transformation is done from shader */
static void vao_push_matrix(int projection) { }
static void vao_pop_matrix(int projection) { }

static GLfloat *vao_draw_pts;
static void vao_draw_points_pre(GLfloat *pts)
{
	vao_draw_pts = pts;
}

static void vao_draw_points(int npts)
{
	vao_begin_vertbuf(vao_draw_pts, 2, sizeof(float) * npts * 2, sizeof(float) * 2, 0);
	vao_color_vertbuf(red, green, blue, alpha);
	glDrawArrays(GL_POINTS, 0, npts);
}

static void vao_draw_points_post(void)
{
	vao_draw_pts = NULL;
	vao_end_vertbuf();
}

static void vao_draw_lines6(GLfloat *pts, int npts, float red, float green, float blue, float alpha)
{
	vao_begin_vertbuf(pts, 2, sizeof(float) * npts * 6, sizeof(float) * 6, 0);
	vao_color_vertbuf(red, green, blue, alpha);
	glDrawArrays(GL_LINES, 0, npts);
	vao_end_vertbuf();
}

static int vao_view_w, vao_view_h;
static void vao_expose_init(int x0, int y0, int w, int h, const rnd_color_t *bg_c)
{
	glUseProgram(program);

	glViewport(x0, y0, w, h);
	vao_view_w = w;
	vao_view_h = h;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

static void vao_set_view(double tx, double ty, double zx, double zy, double zz)
{
	zx /= (double)vao_view_w / 2.0;
	zy /= -(double)vao_view_h / 2.0; /* invert: opengl's y+ is upward, librnd's is pointing down */
	glUniform4f(xform_location, tx, ty, zx, zy);
}

static int vao_xor_start(void)
{
	vao_xor_mode = 1;
	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT, GL_MIN);
	return 0;
}

static void vao_xor_end(void)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	vao_xor_mode = 0;
}


static void vao_uninit(void)
{
	glDeleteProgram(program);
	glDeleteBuffers(1, &position_buffer);
	vertbuf_destroy();
	primbuf_destroy();
}

static int vao_init_checkver(void)
{
	GLint profmask = 0, major = gl_get_ver_major();

	if (major < 0) {
		rnd_message(RND_MSG_DEBUG, "opengl draw: vao_init refuse: failed to determine opengl version\n");
		return -1;
	}

	if (gl_is_es()) {
		if (major >= 1) {
			rnd_message(RND_MSG_DEBUG, "opengl draw: vao_init accept (ES with major %d)\n", major);
			return 0;
		}
	}

	if (major < 3) {
		rnd_message(RND_MSG_DEBUG, "opengl draw: vao_init refuse: major %d is below 3\n", major);
		return -1;
	}

#ifdef GL_CONTEXT_PROFILE_MASK
	glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profmask);
	if ((profmask != 0) && !(profmask & GL_CONTEXT_CORE_PROFILE_BIT)) {
		rnd_message(RND_MSG_DEBUG, "opengl draw: vao_init refuse: GL_CONTEXT_PROFILE_MASK (%d) lacks core mode in major %d\n", profmask, major);
		return -1;
	}
#else
	rnd_message(RND_MSG_DEBUG, "opengl draw: vao_init refuse: GL_CONTEXT_PROFILE_MASK missing with major %d\n", major);
	return -1;
#endif

	rnd_message(RND_MSG_DEBUG, "opengl draw: vao_init accept\n");
	return 0;
}


/* Create and compile a shader */
RND_INLINE GLuint vao_create_shader(int type, const char *src)
{
	int status;
	GLuint shader = glCreateShader(type);

	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		int log_len;
		char *buffer;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);

		buffer = malloc(log_len + 1);
		glGetShaderInfoLog(shader, log_len, NULL, buffer);
		rnd_message(RND_MSG_ERROR, "opengl draw: vao_init: Compile failure in %s shader:\n%s\n", type == GL_VERTEX_SHADER ? "vertex" : "fragment", buffer);
		free(buffer);

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

/* Initialize the shaders and link them into a program */
RND_INLINE int vao_init_shaders_(const char *vertex_sh, const char *fragment_sh, GLuint *program_out, GLuint *inputColor_out, GLuint *inputTexture_out, GLuint *xform_out)
{
	GLuint vertex, fragment;
	GLuint program = 0;
	GLuint inputColor = 0;
	GLuint inputTexture = 0;
	GLuint xform = 0;
	int status, res = -1;

	vertex = vao_create_shader(GL_VERTEX_SHADER, vertex_sh);
	if (vertex == 0)
		return -1;

	fragment = vao_create_shader(GL_FRAGMENT_SHADER, fragment_sh);
	if (fragment == 0) {
		glDeleteShader(vertex);
		return -1;
	}

	program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		int log_len;
		char *buffer;

		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);

		buffer = malloc(log_len + 1);
		glGetProgramInfoLog(program, log_len, NULL, buffer);
		rnd_message(RND_MSG_ERROR, "opengl draw: vao_init: Linking failure:\n%s\n", buffer);
		free(buffer);

		glDeleteProgram(program);
		program = 0;

		goto out;
	}

	inputColor = glGetUniformLocation(program, "inputColor");
	inputTexture = glGetUniformLocation(program, "inputTexture");
	xform = glGetUniformLocation(program, "xform");

	glDetachShader(program, vertex);
	glDetachShader(program, fragment);
	res = 0;

out:
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	*program_out = program;
	*inputColor_out = inputColor;
	*inputTexture_out = inputTexture;
	*xform_out = xform;

	return res;
}

/* We need to set up our state when we realize the GtkGLArea widget */
RND_INLINE int vao_init_shaders(void)
{
	const char *vertex_sh, *fragment_sh;

#define NL "\n"

	if (gl_is_es()) {
		rnd_message(RND_MSG_DEBUG, "opengl draw: vao_init_shaders: opengl ES\n");
		vertex_sh = 
			NL "attribute vec4 position;"
			NL "varying vec2 TexCoord;"
			NL "uniform vec4 xform;"
			NL "void main() {"
			NL "  gl_Position = vec4((position[0] + xform[0]) * xform[2] - 1.0, (position[1] + xform[1]) * xform[3] + 1.0, 0, 1);"
			NL "  TexCoord = vec2(position[2], position[3]);"
			NL "}"
			NL;

		fragment_sh =
			NL "precision highp float;"
			NL "varying vec2 TexCoord;"
			NL "uniform vec4 inputColor;"
			NL "uniform sampler2D inputTexture;" /* we do not have to load this one, as 0 is default */
			NL "void main() {"
			NL "  if ((inputColor[0] < -3.0)) {"
			NL "    gl_FragColor = texture2D(inputTexture, TexCoord);"
			NL "  } else {"
			NL "    gl_FragColor = inputColor;"
			NL "  }"
			NL "}"
			NL;
	}
	else {
		rnd_message(RND_MSG_DEBUG, "opengl draw: vao_init_shaders: opengl desktop\n");
		vertex_sh = 
			NL "#version 330"
			NL "attribute vec4 position;"
			NL "out vec2 TexCoord;"
			NL "uniform vec4 xform;"
			NL "void main() {"
			NL "  gl_Position = vec4((position[0] + xform[0]) * xform[2] - 1.0, (position[1] + xform[1]) * xform[3] + 1.0, 0, 1);"
			NL "  TexCoord = vec2(position[2], position[3]);"
			NL "}"
			NL ";"
			NL;

		fragment_sh =
			NL "#version 330"
			NL "out vec4 outputColor;"
			NL "in vec2 TexCoord;"
			NL "uniform vec4 inputColor;"
			NL "uniform sampler2D inputTexture;" /* we do not have to load this one, as 0 is default */
			NL "void main() {"
			NL "  if (inputColor[0] < -3) {"
			NL "    outputColor = texture(inputTexture, TexCoord);"
			NL "  } else {"
			NL "    outputColor = inputColor;"
			NL "  }"
			NL "}"
			NL;
	}


	return vao_init_shaders_(vertex_sh, fragment_sh, &program, &inputColor_location, &inputTexture_location, &xform_location);
}


static int vao_init(void)
{
	if (conf_lib_hid_gl.plugins.lib_hid_gl.backend.disable_vao) {
		rnd_message(RND_MSG_DEBUG, "opengl draw: vao_init refuse: disabled from conf\n");
		return -1;
	}

	return vao_init_checkver();
}

static int vao_new_context(void)
{
	GLuint vao;

	if (vao_init_shaders() != 0) {
		rnd_message(RND_MSG_ERROR, "opengl draw: vao_init: failed to init shaders, no rendering is possible\n");
		return -1;
	}

	/* We only use one VAO, so we always keep it bound */
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &position_buffer);
	return 0;
}


hidgl_draw_t hidgl_draw_vao = {
	"vao", 1,

	vao_init,
	vao_uninit,
	vao_new_context,
	common_set_color,
	common_flush,
	vao_reset,
	vao_expose_init,
	vao_set_view,
	common_texture_import,
	common_texture_free,
	vao_push_matrix,
	vao_pop_matrix,
	vao_xor_start,
	vao_xor_end,

	vao_prim_draw_all,
	common_prim_set_marker,
	common_prim_rewind_to_marker,
	common_prim_reserve_triangles,
	vao_prim_flush,

	common_prim_add_triangle,
	common_prim_add_line,
	common_prim_add_rect,
	vao_prim_add_fillrect,
	common_prim_add_textrect,

	vao_draw_points_pre,
	vao_draw_points,
	vao_draw_points_post,
	vao_draw_lines6
};
