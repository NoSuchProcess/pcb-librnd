/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *
 *  Specctra .dsn import HID
 *  Copyright (C) 2008, 2011 Josh Jordan, Dan McMahill, and Jared Casper
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

/*
This program imports specctra .dsn files to geda .pcb files.
By Josh Jordan and Dan McMahill, modified from bom.c
  -- Updated to use Coord and other fixes by Jared Casper 16 Sep 2011
*/

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "board.h"
#include "data.h"
#include "error.h"
#include "rats.h"
#include "buffer.h"
#include "change.h"
#include "draw.h"
#include "undo.h"
#include "pcb-printf.h"
#include "polygon.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "obj_pinvia.h"
#include "obj_rat.h"

#include "action_helper.h"
#include "hid.h"
#include "hid_draw_helpers.h"
#include "hid_nogui.h"
#include "hid_actions.h"
#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_helper.h"
#include "plugins.h"

static const char *dsn_cookie = "dsn importer";

/* dsn name */
#define FREE(x) if((x) != NULL) { free (x); (x) = NULL; }

static const char load_dsn_syntax[] = "LoadDsnFrom(filename)";

static const char load_dsn_help[] = "Loads the specified dsn resource file.";

/* %start-doc actions LoaddsnFrom

@cindex Specctra routed import
@findex LoadDsnFrom()

@table @var
@item filename
Name of the dsn resource file.  If not specified, the user will
be prompted to enter one.
@end table

%end-doc */

int pcb_act_LoadDsnFrom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname = NULL;
	static char *default_file = NULL;
	char str[200];
	FILE *fp;
	int ret;
	pcb_coord_t dim1, dim2, x0 = 0, y0 = 0, x1, y1;
	pcb_coord_t linethick = 0, lineclear, viadiam, viadrill;
	char lname[200];
	pcb_layer_t *rlayer = NULL;
	pcb_line_t *line = NULL;

	fname = argc ? argv[0] : 0;

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect(_("Load dsn routing session Resource File..."),
														_("Picks a dsn session resource file to load.\n"
															"This file could be generated by freeroute.net\n"),
														default_file, ".ses", "ses", HID_FILESELECT_READ);
		if (fname == NULL)
			PCB_AFAIL(load_dsn);
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}

		if (fname && *fname)
			default_file = pcb_strdup(fname);
	}

	lineclear = PCB->RouteStyle.array[0].Clearance * 2;
	fp = fopen(fname, "r");
	if (!fp) {
		pcb_message(PCB_MSG_ERROR, "Can't open dsn file %s for read\n", fname);
		return 1;
	}
	while (fgets(str, sizeof(str), fp) != NULL) {
		/* strip trailing '\n' if it exists */
		int len = strlen(str) - 1;
		if (str[len] == '\n')
			str[len] = 0;
		ret = sscanf(str, "          (path %s %ld", lname, &dim1);
		if (ret == 2) {
			rlayer = 0;
			LAYER_LOOP(PCB->Data, pcb_max_group) {
				if (!strcmp(layer->Name, lname))
					rlayer = layer;
			}
			PCB_END_LOOP;
			linethick = dim1;
			x0 = 0;
			y0 = 0;
		}
		ret = sscanf(str, "            %ld %ld", &dim1, &dim2);
		if (ret == 2) {
			x1 = dim1;
			y1 = dim2;
			if (x0 != 0 || y0 != 0) {
				line = pcb_line_new_merge(rlayer, x0, PCB->MaxHeight - y0,
																			x1, PCB->MaxHeight - y1, linethick, lineclear, pcb_flag_make(PCB_FLAG_AUTO | PCB_FLAG_CLEARLINE));
				pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_LINE, rlayer, line);
			}
			x0 = x1;
			y0 = y1;
		}
		ret = sscanf(str, "        (via via_%ld_%ld %ld %ld", &viadiam, &viadrill, &dim1, &dim2);
		if (ret == 4) {
			pcb_via_new(PCB->Data, dim1, PCB->MaxHeight - dim2, viadiam, lineclear, 0, viadrill, 0, pcb_flag_make(PCB_FLAG_AUTO));
		}
	}
	fclose(fp);
	return 0;
}

pcb_hid_action_t dsn_action_list[] = {
	{"LoadDsnFrom", 0, pcb_act_LoadDsnFrom, load_dsn_help, load_dsn_syntax}
};

PCB_REGISTER_ACTIONS(dsn_action_list, dsn_cookie)

static void hid_dsn_uninit()
{

}

#include "dolists.h"
pcb_uninit_t hid_import_dsn_init()
{
	PCB_REGISTER_ACTIONS(dsn_action_list, dsn_cookie)
	return hid_dsn_uninit;
}

