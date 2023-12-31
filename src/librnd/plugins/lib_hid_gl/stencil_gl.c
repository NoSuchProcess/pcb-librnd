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
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <stdio.h>
#include <stdlib.h>

#include "opengl_debug.h"

#include "draw.h"
#include "stenc.h"

#include "stencil_gl.h"

#include <librnd/core/error.h>

static GLint stencil_bits = 0;
static int dirty_bits = 0;
static int assigned_bits = 0;

static void stencilgl_clear_stencil_bits(int bits)
{
	hidgl_stenc.clear_stencil_bits(bits);

	dirty_bits = (dirty_bits & ~bits) | assigned_bits;
}

static void stencilgl_clear_unassigned_stencil(void)
{
	stencilgl_clear_stencil_bits(~assigned_bits);
}

static int stencilgl_allocate_clear_stencil_bit(void)
{
	int stencil_bitmask = (1 << stencil_bits) - 1;
	int test;
	int first_dirty = 0;

	if (assigned_bits == stencil_bitmask) {
		printf("No more stencil bits available, total of %i already assigned\n", stencil_bits);
		return 0;
	}

	/* Look for a bitplane we don't have to clear */
	for(test = 1; test & stencil_bitmask; test <<= 1) {
		if (!(test & dirty_bits)) {
			assigned_bits |= test;
			dirty_bits |= test;
			return test;
		}
		else if (!first_dirty && !(test & assigned_bits)) {
			first_dirty = test;
		}
	}

	/* Didn't find any non dirty planes. Clear those dirty ones which aren't in use */
	stencilgl_clear_unassigned_stencil();
	assigned_bits |= first_dirty;
	dirty_bits = assigned_bits;

	return first_dirty;
}

void stencilgl_reset_stencil_usage(void)
{
	assigned_bits = 0;
}

int stencilgl_init(int stencil_bits_as_inited)
{
	stencil_bits = stencil_bits_as_inited;

	if (stencil_bits == 0) {
		rnd_message(RND_MSG_ERROR, "opengl: No stencil bits available.\n");
		rnd_message(RND_MSG_ERROR, "opengl: Cannot mask polygon holes or subcomposite layers\n");
		/* TODO: Flag this to the HID so it can revert to the dicer? */
	}
	else if (stencil_bits == 1) {
		rnd_message(RND_MSG_ERROR, "opengl: Only one stencil bitplane avilable\n");
		rnd_message(RND_MSG_ERROR, "opengl: Cannot use stencil buffer to sub-composite layers.\n");
		/* Do we need to disable that somewhere? */
	}

	stencilgl_reset_stencil_usage();
	stencilgl_clear_unassigned_stencil();
	return 0;
}


/* Setup the stencil buffer so that writes will clear stencil bits */
RND_INLINE void stencilgl_mode_write_clear(int bits)
{
	hidgl_stenc.mode_write_clear(bits);
}

static int comp_stencil_bit = 0;

void drawgl_mode_reset(rnd_bool direct, const rnd_box_t *screen)
{
	hidgl_draw.prim_flush();
	hidgl_draw.reset();
	hidgl_stenc.mode_reset();
	stencilgl_reset_stencil_usage();
	hidgl_draw.xor_end();
	comp_stencil_bit = 0;
}

void drawgl_mode_positive(rnd_bool direct, const rnd_box_t *screen)
{
	if (comp_stencil_bit == 0)
		comp_stencil_bit = stencilgl_allocate_clear_stencil_bit();
	else
		hidgl_draw.prim_flush();

	hidgl_stenc.mode_positive();
	hidgl_draw.xor_end();
	hidgl_stenc.mode_write_set(comp_stencil_bit);
}

void drawgl_mode_positive_xor(rnd_bool direct, const rnd_box_t *screen)
{
	hidgl_draw.prim_flush();
	hidgl_stenc.mode_positive_xor();
	hidgl_draw.xor_start();
}

void drawgl_mode_negative(rnd_bool direct, const rnd_box_t *screen)
{
	hidgl_stenc.mode_negative();
	hidgl_draw.xor_end();
	
	if (comp_stencil_bit == 0) {
		/* The stencil isn't valid yet which means that this is the first pos/neg mode
		   set since the reset. The compositing stencil bit will be allocated. Because
		   this is a negative mode and it's the first mode to be set, the stencil buffer
		   will be set to all ones. */
		comp_stencil_bit = stencilgl_allocate_clear_stencil_bit();
		hidgl_stenc.mode_write_set(comp_stencil_bit);
		hidgl_draw.prim_add_fillrect(screen->X1, screen->Y1, screen->X2, screen->Y2);
	}
	else
		hidgl_draw.prim_flush();

	stencilgl_mode_write_clear(comp_stencil_bit);
	hidgl_draw.prim_set_marker();
}

void drawgl_mode_flush(rnd_bool direct, rnd_bool xor_mode, const rnd_box_t *screen)
{
	hidgl_draw.prim_flush();
	hidgl_stenc.flush(comp_stencil_bit);
	stencilgl_reset_stencil_usage();
	comp_stencil_bit = 0;
}
