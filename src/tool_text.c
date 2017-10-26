/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
#include "conf_core.h"

#include "action_helper.h"
#include "board.h"
#include "compat_nls.h"
#include "data.h"
#include "draw.h"
#include "hid_actions.h"
#include "tool.h"
#include "undo.h"

#include "obj_text_draw.h"


void pcb_tool_text_notify_mode(void)
{
	char *string;

	if ((string = pcb_gui->prompt_for(_("Enter text:"), "")) != NULL) {
		if (strlen(string) > 0) {
			pcb_text_t *text;
			int flag = PCB_FLAG_CLEARLINE;

			if (pcb_layer_flags(PCB, INDEXOFCURRENT) & PCB_LYT_BOTTOM)
				flag |= PCB_FLAG_ONSOLDER;
			if ((text = pcb_text_new(CURRENT, pcb_font(PCB, conf_core.design.text_font_id, 1), Note.X,
																Note.Y, 0, conf_core.design.text_scale, string, pcb_flag_make(flag))) != NULL) {
				pcb_undo_add_obj_to_create(PCB_TYPE_TEXT, CURRENT, text, text);
				pcb_undo_inc_serial();
				pcb_text_invalidate_draw(CURRENT, text);
				pcb_draw();
			}
		}
		free(string);
	}
}

pcb_tool_t pcb_tool_text = {
	"text", NULL, 100,
	pcb_tool_text_notify_mode,
	NULL,
	NULL
};
