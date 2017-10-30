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
#include "change.h"
#include "data.h"
#include "hid_actions.h"
#include "obj_padstack.h"
#include "search.h"
#include "thermal.h"
#include "tool.h"

static void tool_thermal_on_pinvia(int type, void *ptr1, void *ptr2, void *ptr3)
{
	if (pcb_gui->shift_is_pressed()) {
		int tstyle = PCB_FLAG_THERM_GET(INDEXOFCURRENT, (pcb_pin_t *) ptr3);
		tstyle++;
		if (tstyle > 5)
			tstyle = 1;
		pcb_chg_obj_thermal(type, ptr1, ptr2, ptr3, tstyle, INDEXOFCURRENT);
	}
	else if (PCB_FLAG_THERM_GET(INDEXOFCURRENT, (pcb_pin_t *) ptr3))
		pcb_chg_obj_thermal(type, ptr1, ptr2, ptr3, 0, INDEXOFCURRENT);
	else
		pcb_chg_obj_thermal(type, ptr1, ptr2, ptr3, PCB->ThermStyle, INDEXOFCURRENT);
}

static void tool_thermal_on_padstack(pcb_padstack_t *ps, unsigned long lid)
{
	unsigned char *th;
	unsigned char cycle[] = {
		PCB_THERMAL_ON | PCB_THERMAL_ROUND | PCB_THERMAL_DIAGONAL,
		PCB_THERMAL_ON | PCB_THERMAL_ROUND,
		PCB_THERMAL_ON | PCB_THERMAL_SHARP | PCB_THERMAL_DIAGONAL,
		PCB_THERMAL_ON | PCB_THERMAL_SHARP,
		PCB_THERMAL_ON | PCB_THERMAL_SOLID,
		PCB_THERMAL_ON | PCB_THERMAL_NOSHAPE
	};
	int cycles = sizeof(cycle) / sizeof(cycle[0]);

	th = pcb_padstack_get_thermal(ps, lid, 1);
	if (pcb_gui->shift_is_pressed()) {
		int n, curr = -1;
		/* cycle through the variants to find the current one */
		for(n = 0; n < cycles; n++) {
			if (*th == cycle[n]) {
				curr = n;
				break;
			}
		}
		
		/* bump current pattern to the next or set it up */
		if (curr >= 0) {
			curr++;
			if (curr >= cycles)
				curr = 0;
		}
		else
			curr = 0;

#warning thermal TODO: add undo
		*th = cycle[curr];
	}
	else {
#warning thermal TODO: add undo
		*th ^= PCB_THERMAL_ON;
	}
}


void pcb_tool_thermal_notify_mode(void)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	if (((type = pcb_search_screen(Note.X, Note.Y, PCB_TYPEMASK_PIN, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE)
			&& !PCB_FLAG_TEST(PCB_FLAG_HOLE, (pcb_pin_t *) ptr3)) {
		if (type == PCB_TYPE_PADSTACK)
			tool_thermal_on_padstack((pcb_padstack_t *)ptr2, INDEXOFCURRENT);
		else
			tool_thermal_on_pinvia(type, ptr1, ptr2, ptr3);
	}
}

pcb_tool_t pcb_tool_thermal = {
	"thermal", NULL, 100,
	pcb_tool_thermal_notify_mode,
	NULL,
	NULL
};
