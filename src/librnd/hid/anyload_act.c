/*
 *                            COPYRIGHT
 *
 *  librnd, modular 2D CAD framework
 *  (file imported from: pcb-rnd, interactive printed circuit board design)
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
 *    Project page: http://repo.hu/projects/librnd
 *    lead developer: http://repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <librnd/rnd_config.h>

#include <librnd/core/actions.h>
#include <librnd/hid/hid.h>
#include <librnd/core/anyload.h>

static const char rnd_acts_AnyLoad[] = "AnyLoad([path])";
static const char rnd_acth_AnyLoad[] = "Load \"anything\" from path (or offer a file selectio dialog if no path specified)\n";
/* DOC: anyload.html */
fgw_error_t rnd_act_AnyLoad(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *path = NULL;
	char *path_free = NULL;

	RND_ACT_MAY_CONVARG(1, FGW_STR, AnyLoad, path = argv[1].val.str);

	if (path == NULL)
		path = path_free = rnd_hid_fileselect(rnd_gui, "Import an anyload", NULL, "anyload.lht", NULL, NULL, "anyload", RND_HID_FSD_READ, NULL);

	if (path != NULL)
		RND_ACT_IRES(rnd_anyload(RND_ACT_HIDLIB, path));
	else
		RND_ACT_IRES(-1);

	free(path_free);

	return 0;
}


static rnd_action_t anyload_action_list[] = {
	{"AnyLoad", rnd_act_AnyLoad, rnd_acth_AnyLoad, rnd_acts_AnyLoad}
};

void rnd_anyload_act_init2(void)
{
	RND_REGISTER_ACTIONS(anyload_action_list, NULL);
}
