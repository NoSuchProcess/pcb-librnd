/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,2006 Thomas Nau
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

/* thermal on pins/vias
 *
 * Thermals are normal lines on the layout. The only thing unique
 * about them is that they have the PCB_FLAG_USETHERMAL set so that they
 * can be identified as thermals. It is handy for pcb to automatically
 * make adjustments to the thermals when the user performs certain
 * operations, and the functions in thermal.h help implement that.
 */

#ifndef	PCB_PINVIA_THERMAL_H
#define	PCB_PINVIA_THERMAL_H

#include <stdlib.h>
#include "config.h"

pcb_polyarea_t *ThermPoly_(pcb_board_t *p, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t thickness, pcb_coord_t clearance, pcb_cardinal_t style);

#endif
