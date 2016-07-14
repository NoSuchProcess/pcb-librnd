/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 * 
 *  This module, debug, was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"
#include "global.h"
#include "hid.h"
#include "plug_footprint.h"

static const char fp_rehash_syntax[] = "fp_rehash()" ;
static const char fp_rehash_help[] = "Flush the library index; rescan all library search paths and rebuild the library index. Useful if there are changes in the library during a pcb-rnd session.";
static int Action_fp_rehash(int argc, char **argv, Coord x, Coord y)
{
	fp_rehash();
	return 0;
}


HID_Action conf_plug_footprint_list[] = {
	{"fp_rehash", 0, Action_fp_rehash,
	 fp_rehash_help, fp_rehash_syntax}
};

REGISTER_ACTIONS(conf_plug_footprint_list, NULL)
