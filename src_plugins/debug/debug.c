/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include "global.h"
#include "data.h"
#include "action_helper.h"
#include "plugins.h"

HID_Action debug_action_list[] = {
};

static const char *debug_cookie = "debug plugin";

REGISTER_ACTIONS(debug_action_list, debug_cookie)

static void hid_debug_uninit(void)
{
	hid_remove_actions_by_cookie(debug_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_debug_init(void)
{
	REGISTER_ACTIONS(debug_action_list, debug_cookie)
	return hid_debug_uninit;
}

