/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2022 Tibor 'Igor2' Palinkas
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

#include <librnd/core/error.h>

#include "stenc.h"


static void error_clear_stencil_bits(int bits) { }
static void error_mode_write_clear(int bits) { }
static void error_mode_write_set(int bits) { }
static void error_mode_reset(void) { }
static void error_mode_positive(void) { }
static void error_mode_positive_xor(void) { }
static void error_mode_negative(void) { }
static void error_flush(int comp_stencil_bit) { }

static int error_init(int *stencil_bits_out)
{
	rnd_message(RND_MSG_ERROR, "No opengl stencil available. Rendering is broken.\n");
	return 0;
}


hidgl_stenc_t hidgl_stenc_error = {
	"error",

	error_init,
	error_clear_stencil_bits,
	error_mode_write_clear,
	error_mode_write_set,
	error_mode_reset,
	error_mode_positive,
	error_mode_positive_xor,
	error_mode_negative,
	error_flush
};
