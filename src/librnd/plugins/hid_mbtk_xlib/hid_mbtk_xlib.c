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

#include <stdlib.h>

#include <librnd/core/plugins.h>
#include <librnd/core/hid_init.h>

#include <libmbtk/backend.h>
#include <libmbtk/backend/xlib.h>

#include <librnd/plugins/lib_mbtk_common/glue_hid.h>

const char *mbtkx_cookie = "miniboxtk hid, xlib backend";

rnd_hid_t mbtkx_hid;

static int rnd_mbtk_xlib_init_backend(rnd_mbtk_t *mctx, int *argc, char **argv[])
{
	if (mbtk_xlib_backend(&mctx->be) != 0) {
		fprintf(stderr, "Unable to set up the xlib backend\n");
		return -1;
	}

	mctx->default_font = "-*-helvetica-*-*-*-*-*-*-*-*-*-*-*-*";

	return 0;
}

int mbtk_xlib_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	return rnd_mbtk_parse_arguments(hid, argc, argv);
}

int pplg_check_ver_hid_mbtk_xlib(int ver_needed) { return 0; }

void pplg_uninit_hid_mbtk_xlib(void)
{
}

int pplg_init_hid_mbtk_xlib(void)
{
	RND_API_CHK_VER;

	rnd_mbtk_glue_hid_init(&mbtkx_hid, rnd_mbtk_xlib_init_backend);

	mbtkx_hid.parse_arguments = mbtk_xlib_parse_arguments;

	mbtkx_hid.name = "mbtk_xlib";
	mbtkx_hid.description = "miniboxtk xlib - software rendering on X11";

	rnd_hid_register_hid(&mbtkx_hid);

	return 0;
}
