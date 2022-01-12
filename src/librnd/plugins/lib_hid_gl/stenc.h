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
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#ifndef HID_GL_STENC_GL_H
#define HID_GL_STENC_GL_H

/*** DO NOT INCLUDE THIS HEADER from outside of lib_hid_gl; use hidgl.h instead. ***/

typedef struct hidgl_stenc_s hidgl_stenc_t;

struct hidgl_stenc_s {
	const char *name;

 /* Returns 0 if the stencil backend is compatible with host opengl and loads stencil_bits_out */
	int (*init)(int *stencil_bits_out);

	void (*clear_stencil_bits)(int bits);
	void (*mode_write_clear)(int bits);

	/*** admin ***/
	hidgl_stenc_t *next; /* linked list of all backends, except for the error backend */
};

extern hidgl_stenc_t hidgl_stenc; /* active drawing backend */

#endif
