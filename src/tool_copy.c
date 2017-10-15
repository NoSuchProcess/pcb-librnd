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

#include "action_helper.h"
#include "board.h"
#include "copy.h"
#include "crosshair.h"
#include "search.h"
#include "tool.h"


void pcb_tool_copy_notify_mode(void)
{
	switch (pcb_crosshair.AttachedObject.State) {
		/* first notify, lookup object */
	case PCB_CH_STATE_FIRST:
		{
			int types = PCB_COPY_TYPES;

			pcb_crosshair.AttachedObject.Type =
				pcb_search_screen(Note.X, Note.Y, types,
										 &pcb_crosshair.AttachedObject.Ptr1, &pcb_crosshair.AttachedObject.Ptr2, &pcb_crosshair.AttachedObject.Ptr3);
			if (pcb_crosshair.AttachedObject.Type != PCB_TYPE_NONE) {
				pcb_attach_for_copy(Note.X, Note.Y);
			}
			break;
		}

		/* second notify, move or copy object */
	case PCB_CH_STATE_SECOND:
		pcb_copy_obj(pcb_crosshair.AttachedObject.Type,
							 pcb_crosshair.AttachedObject.Ptr1,
							 pcb_crosshair.AttachedObject.Ptr2,
							 pcb_crosshair.AttachedObject.Ptr3, Note.X - pcb_crosshair.AttachedObject.X, Note.Y - pcb_crosshair.AttachedObject.Y);
		pcb_board_set_changed_flag(pcb_true);

		/* reset identifiers */
		pcb_crosshair.AttachedObject.Type = PCB_TYPE_NONE;
		pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
		break;
	}
}
