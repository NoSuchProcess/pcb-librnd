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
#include "conf.h"
#include "data.h"
#include "action_helper.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "plugins.h"
/* -------------------------------------------------------------------------- */

static const char dumplibrary_syntax[] = "DumpLibrary()";

static const char dumplibrary_help[] = "Display the entire contents of the libraries.";

/* %start-doc actions DumpLibrary


%end-doc */

static int ActionDumpLibrary(int argc, char **argv, Coord x, Coord y)
{
	int i, j;

	printf("**** Do not count on this format.  It will change ****\n\n");
	printf("MenuN   = %d\n", Library.MenuN);
	printf("MenuMax = %d\n", Library.MenuMax);
	for (i = 0; i < Library.MenuN; i++) {
		printf("Library #%d:\n", i);
		printf("    EntryN    = %d\n", Library.Menu[i].EntryN);
		printf("    EntryMax  = %d\n", Library.Menu[i].EntryMax);
		printf("    Name      = \"%s\"\n", UNKNOWN(Library.Menu[i].Name));
		printf("    directory = \"%s\"\n", UNKNOWN(Library.Menu[i].directory));
		printf("    Style     = \"%s\"\n", UNKNOWN(Library.Menu[i].Style));
		printf("    flag      = %d\n", Library.Menu[i].flag);

		for (j = 0; j < Library.Menu[i].EntryN; j++) {
			printf("    #%4d: ", j);
			printf("newlib: \"%s\"\n", UNKNOWN(Library.Menu[i].Entry[j].ListEntry));
		}
	}

	return 0;
}

/* ---------------------------------------------------------------------------
 * no operation, just for testing purposes
 * syntax: Bell(volume)
 */
static const char bell_syntax[] = "Bell()";

static const char bell_help[] = "Attempt to produce audible notification (e.g. beep the speaker).";

static int ActionBell(int argc, char **argv, Coord x, Coord y)
{
	gui->beep();
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char debug_syntax[] = "Debug(...)";

static const char debug_help[] = "Debug action.";

/* %start-doc actions Debug

This action exists to help debug scripts; it simply prints all its
arguments to stdout.

%end-doc */

static const char debugxy_syntax[] = "DebugXY(...)";

static const char debugxy_help[] = "Debug action, with coordinates";

/* %start-doc actions DebugXY

Like @code{Debug}, but requires a coordinate.  If the user hasn't yet
indicated a location on the board, the user will be prompted to click
on one.

%end-doc */

static int Debug(int argc, char **argv, Coord x, Coord y)
{
	int i;
	printf("Debug:");
	for (i = 0; i < argc; i++)
		printf(" [%d] `%s'", i, argv[i]);
	pcb_printf(" x,y %$mD\n", x, y);
	return 0;
}

static const char return_syntax[] = "Return(0|1)";

static const char return_help[] = "Simulate a passing or failing action.";

/* %start-doc actions Return

This is for testing.  If passed a 0, does nothing and succeeds.  If
passed a 1, does nothing but pretends to fail.

%end-doc */

static int Return(int argc, char **argv, Coord x, Coord y)
{
	return atoi(argv[0]);
}


static const char djopt_sao_syntax[] = "OptAutoOnly()";

static const char djopt_sao_help[] = "Toggles the optimize-only-autorouted flag.";

/* %start-doc actions OptAutoOnly

The original purpose of the trace optimizer was to clean up the traces
created by the various autorouters that have been used with PCB.  When
a board has a mix of autorouted and carefully hand-routed traces, you
don't normally want the optimizer to move your hand-routed traces.
But, sometimes you do.  By default, the optimizer only optimizes
autorouted traces.  This action toggles that setting, so that you can
optimize hand-routed traces also.

%end-doc */

int djopt_set_auto_only(int argc, char **argv, Coord x, Coord y)
{
	conf_set(CFR_DESIGN, "plugins/djopt/auto_only", -1, conf_djopt.plugins.djopt.auto_only ? "0" : "1", POL_OVERWRITE);
	return 0;
}

/* ************************************************************ */

static const char toggle_vendor_syntax[] = "ToggleVendor()";

static const char toggle_vendor_help[] = "Toggles the state of automatic drill size mapping.";

/* %start-doc actions ToggleVendor

@cindex vendor map 
@cindex vendor drill table
@findex ToggleVendor()

When drill mapping is enabled, new instances of pins and vias will
have their drill holes mapped to one of the allowed drill sizes
specified in the currently loaded vendor drill table.  To enable drill
mapping, a vendor lihata file containing a drill table must be
loaded first.

%end-doc */

int ActionToggleVendor(int argc, char **argv, Coord x, Coord y)
{
	conf_set(CFR_DESIGN, "plugins/vendor/enable", -1, conf_vendor.plugins.vendor.enable ? "0" : "1", POL_OVERWRITE);
	return 0;
}

/* ************************************************************ */

static const char enable_vendor_syntax[] = "EnableVendor()";

static const char enable_vendor_help[] = "Enables automatic drill size mapping.";

/* %start-doc actions EnableVendor

@cindex vendor map 
@cindex vendor drill table
@findex EnableVendor()

When drill mapping is enabled, new instances of pins and vias will
have their drill holes mapped to one of the allowed drill sizes
specified in the currently loaded vendor drill table.  To enable drill
mapping, a vendor lihata file containing a drill table must be
loaded first.

%end-doc */

int ActionEnableVendor(int argc, char **argv, Coord x, Coord y)
{
	conf_set(CFR_DESIGN, "plugins/vendor/enable", -1, "1", POL_OVERWRITE);
	return 0;
}

/* ************************************************************ */

static const char disable_vendor_syntax[] = "DisableVendor()";

static const char disable_vendor_help[] = "Disables automatic drill size mapping.";

/* %start-doc actions DisableVendor

@cindex vendor map 
@cindex vendor drill table
@findex DisableVendor()

When drill mapping is enabled, new instances of pins and vias will
have their drill holes mapped to one of the allowed drill sizes
specified in the currently loaded vendor drill table.

%end-doc */

int ActionDisableVendor(int argc, char **argv, Coord x, Coord y)
{
	conf_set(CFR_DESIGN, "plugins/vendor/enable", -1, "0", POL_OVERWRITE);
	return 0;
}


HID_Action oldactions_action_list[] = {
	{"DumpLibrary", 0, ActionDumpLibrary,
	 dumplibrary_help, dumplibrary_syntax},
	{"Bell", 0, ActionBell,
	 bell_help, bell_syntax},
	{"Debug", 0, Debug,
	 debug_help, debug_syntax},
	{"DebugXY", "Click X,Y for Debug", Debug,
	 debugxy_help, debugxy_syntax},
	{"Return", 0, Return,
	 return_help, return_syntax},
	{"OptAutoOnly", 0, djopt_set_auto_only,
	 djopt_sao_help, djopt_sao_syntax},
	{"ToggleVendor", 0, ActionToggleVendor,
	 toggle_vendor_help, toggle_vendor_syntax},
	{"EnableVendor", 0, ActionEnableVendor,
	 enable_vendor_help, enable_vendor_syntax},
	{"DisableVendor", 0, ActionDisableVendor,
	 disable_vendor_help, disable_vendor_syntax}
};

static const char *oldactions_cookie = "oldactions plugin";

REGISTER_ACTIONS(oldactions_action_list, oldactions_cookie)

static void hid_oldactions_uninit(void)
{
	hid_remove_actions_by_cookie(oldactions_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_oldactions_init(void)
{
	REGISTER_ACTIONS(oldactions_action_list, oldactions_cookie)
	return hid_oldactions_uninit;
}

