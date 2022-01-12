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

#include "config.h"

#include "opengl.h"
#include "draw.h"

#include <librnd/core/error.h>

#include "stenc.h"


static void direct_clear_stencil_bits(int bits)
{
	glPushAttrib(GL_STENCIL_BUFFER_BIT);
	glStencilMask(bits);
	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT);
	glPopAttrib();
}

static void direct_mode_write_clear(int bits)
{
	glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
	glStencilMask(bits);
	glStencilFunc(GL_ALWAYS, bits, bits);
}

static void direct_mode_write_set(int bits)
{
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilMask(bits);
	glStencilFunc(GL_ALWAYS, bits, bits);
}

static void direct_mode_reset(void)
{
	glColorMask(0, 0, 0, 0); /* Disable color drawing */
}

static void direct_mode_positive(void)
{
	glEnable(GL_STENCIL_TEST);
}

static void direct_mode_positive_xor(void)
{
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_STENCIL_TEST);
}

static void direct_mode_negative(void)
{
	glEnable(GL_STENCIL_TEST);
}

void direct_flush(int comp_stencil_bit)
{
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	if (comp_stencil_bit) {
		glEnable(GL_STENCIL_TEST);

		/* Setup the stencil to allow writes to the color buffer if the
		   comp_stencil_bit is set. After the operation, the comp_stencil_bit
		   will be cleared so that any further writes to this pixel are disabled. */
		glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
		glStencilMask(comp_stencil_bit);
		glStencilFunc(GL_EQUAL, comp_stencil_bit, comp_stencil_bit);

		/* Draw all primitives through the stencil to the color buffer. */
		hidgl_draw.prim_draw_all(comp_stencil_bit);
	}

	glDisable(GL_STENCIL_TEST);
}

static int direct_init(int *stencil_bits_out)
{
	int stencil_bits;

	stencil_bits = 0;
	glGetIntegerv(GL_STENCIL_BITS, &stencil_bits);

	if (stencil_bits != 0) {
		*stencil_bits_out = stencil_bits;
		rnd_message(RND_MSG_DEBUG, "opengl stencil: direct_init accept\n");

		return 0;
	}

	rnd_message(RND_MSG_DEBUG, "opengl stencil: direct_init refuse: 0 stencil bits\n");
	return -1;
}


hidgl_stenc_t hidgl_stenc_direct = {
	"direct",

	direct_init,
	direct_clear_stencil_bits,
	direct_mode_write_clear,
	direct_mode_write_set,
	direct_mode_reset,
	direct_mode_positive,
	direct_mode_positive_xor,
	direct_mode_negative,
	direct_flush
};
