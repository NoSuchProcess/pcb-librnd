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

/* Find galvanic/geometrical connections */

#ifndef	PCB_FIND_H
#define	PCB_FIND_H

#include "config.h"
#include "obj_common.h"

#define PCB_SILK_TYPE	\
	(PCB_OBJ_LINE | PCB_OBJ_ARC | PCB_OBJ_POLY)

void pcb_lookup_conn(pcb_coord_t, pcb_coord_t, pcb_bool, pcb_coord_t, int);
pcb_bool pcb_reset_conns(pcb_bool);
void pcb_conn_lookup_init(void);
void pcb_conn_lookup_uninit(void);
void pcb_rat_find_hook(pcb_any_obj_t *obj, pcb_bool undo, pcb_bool AndRats);
void pcb_save_find_flag(int);
void pcb_restore_find_flag(void);

#include "find2.h"

#endif
