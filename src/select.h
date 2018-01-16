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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#ifndef	PCB_SELECT_H
#define	PCB_SELECT_H

#include "config.h"
#include "operation.h"

#define PCB_SELECT_TYPES	\
	(PCB_TYPE_VIA | PCB_TYPE_LINE | PCB_TYPE_TEXT | PCB_TYPE_POLY | PCB_TYPE_ELEMENT | PCB_TYPE_SUBC | \
	 PCB_TYPE_PIN | PCB_TYPE_PAD | PCB_TYPE_PSTK | PCB_TYPE_ELEMENT_NAME | PCB_TYPE_RATLINE | PCB_TYPE_ARC)

pcb_bool pcb_select_object(pcb_board_t *pcb);
pcb_bool pcb_select_block(pcb_board_t *pcb, pcb_box_t *, pcb_bool);
long int *pcb_list_block(pcb_board_t *pcb, pcb_box_t *Box, int *len);

pcb_bool pcb_select_connection(pcb_board_t *pcb, pcb_bool);


typedef enum {
	PCB_SM_REGEX = 0,
	PCB_SM_LIST = 1
} pcb_search_method_t;

pcb_bool pcb_select_object_by_name(pcb_board_t *pcb, int, const char *, pcb_bool, pcb_search_method_t);

/* New API */

/* Change the selection of an element or element name (these have side effects) */
void pcb_select_element(pcb_board_t *pcb, pcb_element_t *element, pcb_change_flag_t how, int redraw);
void pcb_select_element_name(pcb_element_t *element, pcb_change_flag_t how, int redraw);

#endif
