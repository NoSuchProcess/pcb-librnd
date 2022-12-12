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

#include "config.h"

#include "opengl.h"
#include "draw.h"

#include <librnd/core/error.h>

#include "stenc.h"
#include "stenc_COMMON.c"
#include "lib_hid_gl_conf.h"
extern conf_lib_hid_gl_t conf_lib_hid_gl;

static int framebuffer_init(int *stencil_bits_out)
{
	int stencil_bits;


	if (conf_lib_hid_gl.plugins.lib_hid_gl.stencil.disable_framebuffer) {
		rnd_message(RND_MSG_DEBUG, "opengl stencil: framebuffer_init refuse: disabled from conf\n");
		return -1;
	}

	stencil_bits = 0;

	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &stencil_bits);

	if (stencil_bits != 0) {
		*stencil_bits_out = stencil_bits;
		rnd_message(RND_MSG_DEBUG, "opengl stencil: framebuffer_init accept\n");

		return 0;
	}

	rnd_message(RND_MSG_DEBUG, "opengl stencil: framebuffer_init refuse: 0 stencil bits\n");
	return -1;
}


hidgl_stenc_t hidgl_stenc_framebuffer = {
	"framebuffer",

	framebuffer_init,
	common_clear_stencil_bits,
	common_mode_write_clear,
	common_mode_write_set,
	common_mode_reset,
	common_mode_positive,
	common_mode_positive_xor,
	common_mode_negative,
	common_flush
};
