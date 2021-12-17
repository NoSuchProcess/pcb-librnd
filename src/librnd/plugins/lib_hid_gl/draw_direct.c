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

/* Low level gl rendering: direct access, pre-vao, works with gtk2 */

#include "config.h"
#include <stdlib.h>

/*#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>*/

#include <librnd/core/color.h>

#include "hidgl.h"
#include "stencil_gl.h"

#include "draw.h"

/* Vertex Buffer Data
   The vertex buffer is a dynamic array of vertices. Each vertex contains
   position and color information. */
typedef struct {
	GLfloat x;
	GLfloat y;
	GLfloat u;
	GLfloat v;
	GLfloat r;
	GLfloat g;
	GLfloat b;
	GLfloat a;
} vertex_t;

#include "vertbuf.c"
#include "primbuf.c"

static GLfloat red = 0.0f;
static GLfloat green = 0.0f;
static GLfloat blue = 0.0f;
static GLfloat alpha = 0.75f;

static int mask_stencil = 0;

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

void drawgl_direct_set_color(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	red = r;
	green = g;
	blue = b;
	alpha = a;
}

static void drawgl_direc_prim_add_triangle(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat x3, GLfloat y3)
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

static void drawgl_direct_prim_add_textrect(GLfloat x1, GLfloat y1, GLfloat u1, GLfloat v1,
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

static void drawgl_direct_prim_add_line(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	primbuf_add(GL_LINES, vertbuf.size, 2, 0);
	vertbuf_reserve_extra(2);
	vertbuf_add(x1, y1);
	vertbuf_add(x2, y2);
}

static void drawgl_direct_prim_add_rect(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	primbuf_add(GL_LINE_LOOP, vertbuf.size, 4, 0);
	vertbuf_reserve_extra(4);
	vertbuf_add(x1, y1);
	vertbuf_add(x2, y1);
	vertbuf_add(x2, y2);
	vertbuf_add(x1, y2);
}

RND_INLINE void drawgl_add_mask_create(void)
{
	primbuf_add(PRIM_MASK_CREATE, 0, 0, 0);
}

RND_INLINE void drawgl_add_mask_destroy(void)
{
	primbuf_add(PRIM_MASK_DESTROY, 0, 0, 0);
}

RND_INLINE void drawgl_add_mask_use(void)
{
	primbuf_add(PRIM_MASK_USE, 0, 0, 0);
}

RND_INLINE void drawgl_direct_draw_rect(GLenum mode, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	float points[4][6];
	int i;
	for(i=0;i<4;++i)
	{
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

RND_INLINE void drawgl_direct_draw_rectangle(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	drawgl_direct_draw_rect(GL_LINE_LOOP, x1, y1, x2, y2);
}

static void drawgl_direct_prim_add_fillrect(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	drawgl_direct_draw_rect(GL_TRIANGLE_FAN, x1, y1, x2, y2);
}

static void drawgl_direct_prim_reserve_triangles(int count)
{
	vertbuf_reserve_extra(count * 3);
}

/* This function will draw the specified primitive but it may also modify the state of
   the stencil buffer when MASK primtives exist. */
RND_INLINE void drawgl_draw_primtive(primitive_t *prim)
{
	switch (prim->type) {
		case PRIM_MASK_CREATE:
			if (mask_stencil)
				stencilgl_return_stencil_bit(mask_stencil);
			mask_stencil = stencilgl_allocate_clear_stencil_bit();
			if (mask_stencil != 0) {
				glPushAttrib(GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
				glStencilMask(mask_stencil);
				glStencilFunc(GL_ALWAYS, mask_stencil, mask_stencil);
				glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
				glColorMask(0, 0, 0, 0);
			}
			break;

		case PRIM_MASK_USE:
			{
				GLint ref = 0;
				glPopAttrib();
				glPushAttrib(GL_STENCIL_BUFFER_BIT);
				glGetIntegerv(GL_STENCIL_REF, &ref);
				glStencilFunc(GL_GEQUAL, ref & ~mask_stencil, mask_stencil);
			}
			break;

		case PRIM_MASK_DESTROY:
			glPopAttrib();
			stencilgl_return_stencil_bit(mask_stencil);
			mask_stencil = 0;
			break;

		default:
			if(prim->texture_id > 0) {
				glBindTexture(GL_TEXTURE_2D, prim->texture_id);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
				glEnable(GL_TEXTURE_2D);
				glAlphaFunc(GL_GREATER,0.5);
				glEnable(GL_ALPHA_TEST);
			}
			glDrawArrays(prim->type, prim->first, prim->count);
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_ALPHA_TEST);
			break;
	}
}

static void drawgl_direct_prim_flush(void)
{
	int index = primbuf.dirty_index;
	int end = primbuf.size;
	primitive_t *prim = &primbuf.data[index];

	/* Setup the vertex buffer */
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(vertex_t), &vertbuf.data[0].x);
	glTexCoordPointer(2, GL_FLOAT, sizeof(vertex_t), &vertbuf.data[0].u);
	glColorPointer(4, GL_FLOAT, sizeof(vertex_t), &vertbuf.data[0].r);

	/* draw the primitives */
	while(index < end) {
		drawgl_draw_primtive(prim);
		++prim;
		++index;
	}

	/* disable the vertex buffer */
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	primbuf.dirty_index = end;
}

static void drawgl_direct_prim_draw_all(int stencil_bits)
{
	int index = primbuf.size;
	primitive_t *prim;
	int mask = 0;

	if ((index == 0) || (primbuf.data == NULL))
		return;

	--index;
	prim = &primbuf.data[index];

	/* Setup the vertex buffer */
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(vertex_t), &vertbuf.data[0].x);
	glTexCoordPointer(2, GL_FLOAT, sizeof(vertex_t), &vertbuf.data[0].u);
	glColorPointer(4, GL_FLOAT, sizeof(vertex_t), &vertbuf.data[0].r);

	/* draw the primitives */
	while(index >= 0) {
		switch (prim->type) {
			case PRIM_MASK_DESTROY:
				/* The primitives are drawn in reverse order. The mask primitives are required
				   to be processed in forward order so we must search for the matching 'mask create'
				   primitive and then iterate through the primtives until we reach the 'mask destroy'
				   primitive. */
				{
					primitive_t *next_prim = prim - 1;;
					primitive_t *mask_prim = prim;
					int mask_index = index;
					int next_index = index - 1;

					/* Find the 'mask create' primitive. */
					while((mask_index >= 0) && (mask_prim->type != PRIM_MASK_CREATE)) {
						--mask_prim;
						--mask_index;
					}

					/* Process the primitives in forward order until we reach the 'mask destroy' primitive */
					if (mask_prim->type == PRIM_MASK_CREATE) {
						next_index = mask_index;
						next_prim = mask_prim;

						while(mask_index <= index) {
							switch (mask_prim->type) {
								case PRIM_MASK_CREATE:
									if (mask)
										stencilgl_return_stencil_bit(mask);
									mask = stencilgl_allocate_clear_stencil_bit();

									if (mask != 0) {
										glPushAttrib(GL_STENCIL_BUFFER_BIT);

										glEnable(GL_STENCIL_TEST);
										stencilgl_mode_write_set(mask);
									}
									glPushAttrib(GL_COLOR_BUFFER_BIT);
									glColorMask(0, 0, 0, 0);
									break;

								case PRIM_MASK_USE:
									glPopAttrib();
									if (mask != 0) {
										glEnable(GL_STENCIL_TEST);
										glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
										glStencilFunc(GL_EQUAL, stencil_bits, stencil_bits | mask);
										glStencilMask(stencil_bits);
									}
									break;

								case PRIM_MASK_DESTROY:
									if (mask != 0) {
										glPopAttrib();
										stencilgl_return_stencil_bit(mask);
										mask = 0;
									}
									break;

								default:
									glDrawArrays(mask_prim->type, mask_prim->first, mask_prim->count);
									break;
							}
							++mask_prim;
							++mask_index;
						}

						index = next_index;
						prim = next_prim;
					}
					else {
						index = mask_index;
						prim = mask_prim;
					}
				}
				break;

			default:
				if(prim->texture_id > 0) {
					glBindTexture(GL_TEXTURE_2D, prim->texture_id);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
					glEnable(GL_TEXTURE_2D);
					glAlphaFunc(GL_GREATER,0.5);
					glEnable(GL_ALPHA_TEST);
				}
				glDrawArrays(prim->type, prim->first, prim->count);
				glDisable(GL_TEXTURE_2D);
				glDisable(GL_ALPHA_TEST);
				break;
		}

		--prim;
		--index;
	}

	if (mask)
		stencilgl_return_stencil_bit(mask);

	/* disable the vertex buffer */
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

}

void drawgl_direct_flush(void)
{
	glFlush();
}

void drawgl_direct_reset(void)
{
	vertbuf_clear();
	primbuf_clear();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_STENCIL_TEST);
	drawgl_mode_positive_xor_end();
}

static void drawgl_direct_push_matrix(int projection)
{
	if (projection)
		glMatrixMode(GL_PROJECTION);
	glPushMatrix();
}

static void drawgl_direct_pop_matrix(int projection)
{
	if (projection)
		glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}

static long drawgl_direct_texture_import(unsigned char *pixels, int width, int height, int has_alpha)
{
	GLuint texture_id;

	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, has_alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, pixels);

	return texture_id;
}


static void drawgl_direct_prim_set_marker(void)
{
	vertbuf_set_marker();
	primbuf_set_marker();
}

static void drawgl_direct_prim_rewind_to_marker(void)
{
	vertbuf_rewind();
	primbuf_rewind();
}

static void drawgl_direct_draw_points_pre(GLfloat *pts)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, pts);
}

static void drawgl_direct_draw_points(int npts)
{
	glDrawArrays(GL_POINTS, 0, npts);
}

static void drawgl_direct_draw_points_post(void)
{
	glDisableClientState(GL_VERTEX_ARRAY);
}

static void drawgl_direct_draw_lines6(GLfloat *pts, int npts)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(float) * 6, pts);
	glColorPointer(4, GL_FLOAT, sizeof(float) * 6, pts+2);
	glDrawArrays(GL_LINES, 0, npts);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}


static void drawgl_direct_expose_init(int w, int h, const rnd_color_t *bg_c)
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

static void drawgl_direct_set_view(double tx, double ty, double zx, double zy, double zz)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -HIDGL_Z_NEAR);
	glScalef(zx, zy, zz);
	glTranslatef(tx, ty, 0);
}


static void drawgl_direct_uninit(void)
{
	vertbuf_destroy();
	primbuf_destroy();
}

int drawgl_direct_init(void)
{
	return 0;
}

hidgl_draw_t hidgl_draw_direct = {
	drawgl_direct_init,
	drawgl_direct_uninit,
	drawgl_direct_set_color,
	drawgl_direct_flush,
	drawgl_direct_reset,
	drawgl_direct_expose_init,
	drawgl_direct_set_view,
	drawgl_direct_texture_import,
	drawgl_direct_push_matrix,
	drawgl_direct_pop_matrix,

	drawgl_direct_prim_draw_all,
	drawgl_direct_prim_set_marker,
	drawgl_direct_prim_rewind_to_marker,
	drawgl_direct_prim_reserve_triangles,
	drawgl_direct_prim_flush,

	drawgl_direc_prim_add_triangle,
	drawgl_direct_prim_add_line,
	drawgl_direct_prim_add_rect,
	drawgl_direct_prim_add_fillrect,
	drawgl_direct_prim_add_textrect,

	drawgl_direct_draw_points_pre,
	drawgl_direct_draw_points,
	drawgl_direct_draw_points_post,
	drawgl_direct_draw_lines6,
};
