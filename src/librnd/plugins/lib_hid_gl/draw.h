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

#include "config.h"
#include "opengl.h"

typedef struct hidgl_draw_s {
 /* Returns 0 if the drawing backend is compatible with host opengl */
	int (*init)(void);

	void (*uninit)(void);
	void (*flush)(void);
	void (*set_color)(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
	void (*reset)(void);

	/*** Buffer of primitives to be drawn */
	/* Draw all buffered primitives. The dirty index is ignored and will
	   remain unchanged. This function accepts stencil bits that can be
	   used to mask the drawing. */
	void (*prim_draw_all)(int stencil_bits);

	void (*prim_set_marker)(void);            /* set marker for rewind */
	void (*prim_rewind_to_marker)(void);      /* rewind to last set marker; useful for discarding primitives after the marker */

	void (*prim_reserve_triangles)(int count);


	/*** Add primitives to the buffer ***/
	void (*prim_add_triangle)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat x3, GLfloat y3);
	void (*prim_add_line)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
	void (*prim_add_rect)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
	void (*prim_add_fillrect)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
	void (*prim_add_texture_quad)(GLfloat x1, GLfloat y1, GLfloat u1, GLfloat v1, GLfloat x2, GLfloat y2, GLfloat u2, GLfloat v2, GLfloat x3, GLfloat y3, GLfloat u3, GLfloat v3, GLfloat x4, GLfloat y4, GLfloat u4, GLfloat v4, GLuint texture_id);


} hidgl_draw_t;


extern hidgl_draw_t hidgl_draw; /* active drawing backend */

/* available implementations */
extern hidgl_draw_t hidgl_draw_direct;

#endif /* ! defined HID_GL_DRAW_GL_H */
