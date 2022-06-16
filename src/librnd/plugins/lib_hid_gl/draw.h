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
#ifndef HID_GL_DRAW_GL_H
#define HID_GL_DRAW_GL_H

/*** DO NOT INCLUDE THIS HEADER from outside of lib_hid_gl; use hidgl.h instead. ***/

#include "config.h"

#include <librnd/core/hidlib.h>
#include "opengl.h"

typedef struct hidgl_draw_s hidgl_draw_t;

struct hidgl_draw_s {
	const char *name;

	unsigned xor_inverts_clr:1;  /* if color in xor is 1-r;1-g;1-b */

 /* Returns 0 if the drawing backend is compatible with host opengl */
	int (*init)(void);

	void (*uninit)(void);

	/* Call this first but only once after a new GL context has been created;
	   returns 0 on success. */
	int (*new_context)(void);

	void (*set_color)(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
	void (*flush)(void);
	void (*reset)(void);
	void (*expose_init)(int w, int h, const rnd_color_t *bg_c);

	/* set up transformation to translate tx,ty and zoom zx,zy,zz */
	void (*set_view)(double tx, double ty, double zx, double zy, double zz);

	/* Allocate a texture, load it from pixels and return texture gl-ID */
	long (*texture_import)(unsigned char *pixels, int width, int height, int has_alpha);
	void (*texture_free)(long id);

	void (*push_matrix)(int projection);
	void (*pop_matrix)(int projection);

	void (*xor_start)(void);
	void (*xor_end)(void);


	/*** Buffer of primitives to be drawn */
	/* Draw all buffered primitives. The dirty index is ignored and will
	   remain unchanged. This function accepts stencil bits that can be
	   used to mask the drawing. */
	void (*prim_draw_all)(int stencil_bits);

	void (*prim_set_marker)(void);            /* set marker for rewind */
	void (*prim_rewind_to_marker)(void);      /* rewind to last set marker; useful for discarding primitives after the marker */

	void (*prim_reserve_triangles)(int count);

	void (*prim_flush)(void);                 /* draw the trailing block of dirty primitives, in order of primitive creation */


	/*** Add primitives to the buffer ***/
	void (*prim_add_triangle)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat x3, GLfloat y3);
	void (*prim_add_line)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
	void (*prim_add_rect)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
	void (*prim_add_fillrect)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
	void (*prim_add_textrect)(GLfloat x1, GLfloat y1, GLfloat u1, GLfloat v1, GLfloat x2, GLfloat y2, GLfloat u2, GLfloat v2, GLfloat x3, GLfloat y3, GLfloat u3, GLfloat v3, GLfloat x4, GLfloat y4, GLfloat u4, GLfloat v4, GLuint texture_id);


	/*** Immediate draw (bypasses vertex buffer and primitive buffer) ***/
	void (*draw_points_pre)(GLfloat *pts);   /* prepare for drawing points from x;y coord array pts */
	void (*draw_points)(int npts);           /* draw the first npts points from pts set above; can be called multiple times */
	void (*draw_points_post)(void);          /* stop drawing points */
	void (*draw_lines6)(GLfloat *pts, int npts, float red, float green,
		float blue, float alpha);              /* draw lines from an array of x,y,r,g,b,a; only x;y needs to be filled in but r,g,b,a needs to be reserved in the array */

	/*** admin ***/
	hidgl_draw_t *next; /* linked list of all backends, except for the error backend */
};


extern hidgl_draw_t hidgl_draw; /* active drawing backend */

#endif /* ! defined HID_GL_DRAW_GL_H */
