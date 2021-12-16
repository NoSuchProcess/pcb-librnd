/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2009-2011 PCB Contributers (See ChangeLog for details)
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

#include <stdio.h>
#include <stdlib.h>

#include "stencil_gl.h"

static GLint stencil_bits = 0;
static int dirty_bits = 0;
static int assigned_bits = 0;


int stencilgl_bit_count()
{
	return stencil_bits;
}

void stencilgl_clear_stencil_bits(int bits)
{
	glPushAttrib(GL_STENCIL_BUFFER_BIT);
	glStencilMask(bits);
	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT);
	glPopAttrib();

	dirty_bits = (dirty_bits & ~bits) | assigned_bits;
}

void stencilgl_clear_unassigned_stencil()
{
	stencilgl_clear_stencil_bits(~assigned_bits);
}

int stencilgl_allocate_clear_stencil_bit(void)
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

void stencilgl_return_stencil_bit(int bit)
{
	assigned_bits &= ~bit;
}

void stencilgl_reset_stencil_usage()
{
	assigned_bits = 0;
}

void stencilgl_init()
{
	glGetIntegerv(GL_STENCIL_BITS, &stencil_bits);

	if (stencil_bits == 0) {
		printf("No stencil bits available.\n" "Cannot mask polygon holes or subcomposite layers\n");
		/* TODO: Flag this to the HID so it can revert to the dicer? */
	}
	else if (stencil_bits == 1) {
		printf("Only one stencil bitplane avilable\n" "Cannot use stencil buffer to sub-composite layers.\n");
		/* Do we need to disable that somewhere? */
	}

	stencilgl_reset_stencil_usage();
	stencilgl_clear_unassigned_stencil();
}


/* Setup the stencil buffer so that writes will clear stencil bits */
static inline void stencilgl_mode_write_clear(int bits)
{
	glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
	glStencilMask(bits);
	glStencilFunc(GL_ALWAYS, bits, bits);
}

/* temporary */
void drawgl_draw_all(int stencil_bits);
void drawgl_set_marker();
void drawgl_direct_draw_solid_rectangle(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);


static int comp_stencil_bit = 0;

void drawgl_mode_reset(rnd_bool direct, const rnd_box_t *screen)
{
	drawgl_flush();
	drawgl_reset();
	glColorMask(0, 0, 0, 0); /* Disable colour drawing */
	stencilgl_reset_stencil_usage();
	glDisable(GL_COLOR_LOGIC_OP);
	comp_stencil_bit = 0;
}

void drawgl_mode_positive(rnd_bool direct, const rnd_box_t *screen)
{
	if (comp_stencil_bit == 0)
		comp_stencil_bit = stencilgl_allocate_clear_stencil_bit();
	else
		drawgl_flush();

	glEnable(GL_STENCIL_TEST);
	glDisable(GL_COLOR_LOGIC_OP);
	stencilgl_mode_write_set(comp_stencil_bit);
}

void drawgl_mode_positive_xor(rnd_bool direct, const rnd_box_t *screen)
{
	drawgl_flush();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_STENCIL_TEST);
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_XOR);
}

void drawgl_mode_negative(rnd_bool direct, const rnd_box_t *screen)
{
	glEnable(GL_STENCIL_TEST);
	glDisable(GL_COLOR_LOGIC_OP);
	
	if (comp_stencil_bit == 0) {
		/* The stencil isn't valid yet which means that this is the first pos/neg mode
		   set since the reset. The compositing stencil bit will be allocated. Because
		   this is a negative mode and it's the first mode to be set, the stencil buffer
		   will be set to all ones. */
		comp_stencil_bit = stencilgl_allocate_clear_stencil_bit();
		stencilgl_mode_write_set(comp_stencil_bit);
		drawgl_direct_draw_solid_rectangle(screen->X1, screen->Y1, screen->X2, screen->Y2);
	}
	else
		drawgl_flush();

	stencilgl_mode_write_clear(comp_stencil_bit);
	drawgl_set_marker();
}


void drawgl_mode_flush(rnd_bool direct, rnd_bool xor_mode, const rnd_box_t *screen)
{
	drawgl_flush();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	if (comp_stencil_bit) {
		glEnable(GL_STENCIL_TEST);

		/* Setup the stencil to allow writes to the colour buffer if the
		   comp_stencil_bit is set. After the operation, the comp_stencil_bit
		   will be cleared so that any further writes to this pixel are disabled. */
		glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
		glStencilMask(comp_stencil_bit);
		glStencilFunc(GL_EQUAL, comp_stencil_bit, comp_stencil_bit);

		/* Draw all primtives through the stencil to the colour buffer. */
		drawgl_draw_all(comp_stencil_bit);
	}

	glDisable(GL_STENCIL_TEST);
	stencilgl_reset_stencil_usage();
	comp_stencil_bit = 0;
}
