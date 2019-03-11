/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *
 */

/* This file written by Bill Wilson for the PCB Gtk port */

#include "config.h"

#include <ctype.h>
#include <stdarg.h>
#include <gdk/gdkkeysyms.h>

#include "dlg_log.h"

#include "conf_core.h"
#include "conf_hid.h"

#include "pcb-printf.h"
#include "actions.h"

#include "win_place.h"
#include "bu_text_view.h"
#include "compat.h"
#include "event.h"

#include "hid_gtk_conf.h"

const char pcb_gtk_acts_logshowonappend[] = "LogShowOnAppend(true|false)";
const char pcb_gtk_acth_logshowonappend[] = "If true, the log window will be shown whenever something is appended to it. If false, the log will still be updated, but the window won't be shown.";
fgw_error_t pcb_gtk_act_logshowonappend(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
#if 0
	const char *a = "";
	const char *pcb_acts_logshowonappend = pcb_gtk_acts_logshowonappend;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, logshowonappend, a = argv[1].val.str);

	if (tolower(*a) == 't')
		log_show_on_append = TRUE;
	else if (tolower(*a) == 'f')
		log_show_on_append = FALSE;
#endif
	PCB_ACT_IRES(0);
	return 0;
}


