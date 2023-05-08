/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2023 Tibor 'Igor2' Palinkas
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

#include <librnd/rnd_config.h>
#include <librnd/core/actions.h>
#include <librnd/core/safe_fs.h>


static const char rnd_acts_SafeFsSystem[] = "SafeFsSystem(cmdline)";
static const char rnd_acth_SafeFsSystem[] = "Runs cmdline with a shell using librnd safe_fs. Return value is the same integer as system()'s";
static fgw_error_t rnd_act_SafeFsSystem(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd;

	RND_ACT_CONVARG(1, FGW_STR, SafeFsSystem, cmd = argv[1].val.str);

	res->type = FGW_INT;
	res->val.nat_int = rnd_system(RND_ACT_DESIGN, cmd);
	return 0;
}


static rnd_action_t rnd_safe_fs_action_list[] = {
	{"SafeFsSystem", rnd_act_SafeFsSystem, rnd_acth_SafeFsSystem, rnd_acts_SafeFsSystem}
};

void rnd_safe_fs_act_init2(void)
{
	RND_REGISTER_ACTIONS(rnd_safe_fs_action_list, NULL);
}



