/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2009-2017 PCB Contributers (See ChangeLog for details)
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

#ifndef STENCIL_GL_H
#define STENCIL_GL_H

/*** DO NOT INCLUDE THIS HEADER from outside of lib_hid_gl; use hidgl.h instead. ***/

#include <librnd/core/global_typedefs.h>
#include "opengl.h"

int stencilgl_init(int stencil_bits_as_inited);
void stencilgl_reset_stencil_usage(void);

#include "opengl_debug.h"

void drawgl_mode_reset(rnd_bool direct, const rnd_box_t *screen);
void drawgl_mode_positive(rnd_bool direct, const rnd_box_t *screen);
void drawgl_mode_positive_xor(rnd_bool direct, const rnd_box_t *screen);
void drawgl_mode_negative(rnd_bool direct, const rnd_box_t *screen);
void drawgl_mode_flush(rnd_bool direct, rnd_bool xor_mode, const rnd_box_t *screen);

#endif /* !defined STENCIL_GL_H */
