/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
#include "conf.h"
#include "data.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "plugins.h"
#include "actions.h"


static const char pcb_acts_ExpFeatTmp[] = "ExpFeatTmp(...)";
static const char pcb_acth_ExpFeatTmp[] = "Experimental Feature Template.";
/* DOC: foobar.html */
/* (change the name of the above reference to the lowercase version of
   your action name and create the document in
   doc/user/09_appendix/action_src/) */
static fgw_error_t pcb_act_ExpFeatTmp(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_message(PCB_MSG_INFO, "Hello world from expfeat!\n");
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_ClipInhibit[] = "ClipInhibit([on|off|check])";
static const char pcb_acth_ClipInhibit[] = "ClipInhibit Feature Template.";
static fgw_error_t pcb_act_ClipInhibit(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	static int is_on = 0;
	int target;
	const char *op;

	PCB_ACT_CONVARG(1, FGW_STR, ClipInhibit, op = argv[1].val.str);

	if (strcmp(op, "check") == 0) {
		PCB_ACT_IRES(is_on);
		return 0;
	}


	if (strcmp(op, "toggle") == 0) {
		target = !is_on;
	}
	else {
		if (op[0] != 'o')
			PCB_ACT_FAIL(ClipInhibit);

		switch(op[1]) {
			case 'n': target = 1; break;
			case 'f': target = 0; break;
			default: PCB_ACT_FAIL(ClipInhibit);
		}
	}

	if (is_on == target) {
		PCB_ACT_IRES(0);
		return 0;
	}

	is_on = target;
	if (is_on)
		pcb_data_clip_inhibit_inc(PCB->Data);
	else
		pcb_data_clip_inhibit_dec(PCB->Data, 1);

	PCB_ACT_IRES(0);
	return 0;
}


static const pcb_action_t expfeat_action_list[] = {
	{"ExpFeatTmp", pcb_act_ExpFeatTmp, pcb_acth_ExpFeatTmp, pcb_acts_ExpFeatTmp},
	{"ClipInhibit", pcb_act_ClipInhibit, pcb_acth_ClipInhibit, pcb_acts_ClipInhibit}
};

static const char *expfeat_cookie = "experimental features plugin";

PCB_REGISTER_ACTIONS(expfeat_action_list, expfeat_cookie)

int pplg_check_ver_expfeat(int ver_needed) { return 0; }

void pplg_uninit_expfeat(void)
{
	pcb_remove_actions_by_cookie(expfeat_cookie);
}

#include "dolists.h"
int pplg_init_expfeat(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(expfeat_action_list, expfeat_cookie)
	return 0;
}
