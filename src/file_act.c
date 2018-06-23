/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include "board.h"
#include "build_run.h"
#include "conf_core.h"
#include "funchash_core.h"
#include "data.h"
#include "buffer.h"
#include "action_helper.h"

#include "plug_io.h"
#include "plug_import.h"
#include "remove.h"
#include "draw.h"
#include "find.h"
#include "search.h"
#include "actions.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "hid_init.h"
#include "layer_vis.h"
#include "safe_fs.h"
#include "tool.h"

/* --------------------------------------------------------------------------- */

static const char pcb_acts_LoadFrom[] = "LoadFrom(Layout|LayoutToBuffer|SubcToBuffer|Netlist|Revert,filename[,format])";

static const char pcb_acth_LoadFrom[] = "Load layout data from a file.";

/* %start-doc actions LoadFrom

This action assumes you know what the filename is.  The various GUIs
should have a similar @code{Load} action where the filename is
optional, and will provide their own file selection mechanism to let
you choose the file name.

@table @code

@item Layout
Loads an entire PCB layout, replacing the current one.

@item LayoutToBuffer
Loads an entire PCB layout to the paste buffer.

@item ElementToBuffer
Loads the given footprint file into the paste buffer. Footprint files
contain only a single subcircuit definition in one of the various
supported file formats.

@item Netlist
Loads a new netlist, replacing any current netlist.

@item Revert
Re-loads the current layout from its disk file, reverting any changes
you may have made.

@end table

%end-doc */

static fgw_error_t pcb_act_LoadFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *name, *format = NULL;
	int op;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, LoadFrom, op = argv[1].val.nat_keyword);
	PCB_ACT_CONVARG(2, FGW_STR, LoadFrom, name = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, LoadFrom, format = argv[3].val.str);

	switch(op) {
		case F_ElementToBuffer:
		case F_Element:
		case F_SubcToBuffer:
		case F_Subcircuit:
		case F_Footprint:
			pcb_notify_crosshair_change(pcb_false);
			if (pcb_buffer_load_footprint(PCB_PASTEBUFFER, name, format))
				pcb_tool_select_by_id(PCB_MODE_PASTE_BUFFER);
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_LayoutToBuffer:
			pcb_notify_crosshair_change(pcb_false);
			if (pcb_buffer_load_layout(PCB, PCB_PASTEBUFFER, name, format))
				pcb_tool_select_by_id(PCB_MODE_PASTE_BUFFER);
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_Layout:
			if (!PCB->Changed || pcb_gui->confirm_dialog("OK to override layout data?", 0))
				pcb_load_pcb(name, format, pcb_true, 0);
			break;

		case F_Netlist:
			if (PCB->Netlistname)
				free(PCB->Netlistname);
			PCB->Netlistname = pcb_strdup_strip_wspace(name);
			{
				int i;
				for (i = 0; i < PCB_NUM_NETLISTS; i++)
					pcb_lib_free(&(PCB->NetlistLib[i]));
			}
			if (!pcb_import_netlist(PCB->Netlistname))
				pcb_netlist_changed(1);
			break;

		case F_Revert:
			if (PCB->Filename && (!PCB->Changed || pcb_gui->confirm_dialog("OK to override changes?", 0)))
				pcb_revert_pcb();
			break;
	}

	PCB_ACT_IRES(0);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_New[] = "New([name])";

static const char pcb_acth_New[] = "Starts a new layout.";

/* %start-doc actions New

If a name is not given, one is prompted for.

%end-doc */

static fgw_error_t pcb_act_New(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *argument_name = NULL;
	char *name = NULL;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, New, argument_name = argv[1].val.str);

	if (!PCB->Changed || pcb_gui->confirm_dialog("OK to clear layout data?", 0)) {
		if (argument_name)
			name = pcb_strdup(argument_name);
		else
			name = pcb_gui->prompt_for("Enter the layout name:", "");

		if (!name)
			return 1;

		pcb_notify_crosshair_change(pcb_false);
		/* do emergency saving
		 * clear the old struct and allocate memory for the new one
		 */
		if (PCB->Changed && conf_core.editor.save_in_tmp)
			pcb_save_in_tmp();
		pcb_board_remove(PCB);
		PCB = pcb_board_new(1);
		pcb_board_new_postproc(PCB, 1);
		pcb_set_design_dir(NULL);

		/* setup the new name and reset some values to default */
		free(PCB->Name);
		PCB->Name = name;

		pcb_layervis_reset_stack();
		pcb_crosshair_set_range(0, 0, PCB->MaxWidth, PCB->MaxHeight);
		pcb_center_display(PCB->MaxWidth / 2, PCB->MaxHeight / 2);
		pcb_redraw();
		pcb_board_changed(0);
		pcb_notify_crosshair_change(pcb_true);
		PCB_ACT_IRES(0);
		return 0;
	}
	PCB_ACT_IRES(-1);
	return 0;
}

/* --------------------------------------------------------------------------- */
static const char pcb_acts_normalize[] = "Normalize()";
static const char pcb_acth_normalize[] = "Move all objects within the drawing area, align the drawing to 0;0";
static fgw_error_t pcb_act_normalize(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	PCB_ACT_IRES(pcb_board_normalize(PCB));
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_SaveTo[] =
	"SaveTo(Layout|LayoutAs,filename,[fmt])\n"
	"SaveTo(AllConnections|AllUnusedPins|ElementConnections,filename)\n" "SaveTo(PasteBuffer,filename,[fmt])";

static const char pcb_acth_SaveTo[] = "Saves data to a file.";

/* %start-doc actions SaveTo

@table @code

@item Layout
Saves the current layout.

@item LayoutAs
Saves the current layout, and remembers the filename used.

@item AllConnections
Save all connections to a file.

@item AllUnusedPins
List all unused pins to a file.

@item ElementConnections
Save connections to the subcircuit at the cursor to a file.

@item PasteBuffer
Save the content of the active Buffer to a file. This is the graphical way to create a footprint.

@end table

%end-doc */

static fgw_error_t pcb_act_SaveTo(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function;
	const char *name;
	const char *fmt = NULL;

	if (argc < 1)
		PCB_ACT_FAIL(SaveTo);

	function = argv[0];
	name = argv[1];

	if (pcb_strcasecmp(function, "Layout") == 0) {
		if (argc != 1) {
			pcb_message(PCB_MSG_ERROR, "SaveTo(Layout) doesn't take file name or format - did you mean SaveTo(LayoutAs)?\n");
			return -1;
		}
		if (pcb_save_pcb(PCB->Filename, NULL) == 0)
			pcb_board_set_changed_flag(pcb_false);
		if (pcb_gui->notify_filename_changed != NULL)
			pcb_gui->notify_filename_changed();
		return 0;
	}

	if ((argc != 2) && (argc != 3))
		PCB_ACT_FAIL(SaveTo);

	if (argc >= 3)
		fmt = argv[2];

	if (pcb_strcasecmp(function, "LayoutAs") == 0) {
		if (pcb_save_pcb(name, fmt) == 0) {
			pcb_board_set_changed_flag(pcb_false);
			free(PCB->Filename);
			PCB->Filename = pcb_strdup(name);
			if (pcb_gui->notify_filename_changed != NULL)
				pcb_gui->notify_filename_changed();
		}
		return 0;
	}

	if (pcb_strcasecmp(function, "AllConnections") == 0) {
		FILE *fp;
		pcb_bool result;
		if ((fp = pcb_check_and_open_file(name, pcb_true, pcb_false, &result, NULL)) != NULL) {
			pcb_lookup_conns_to_all_elements(fp);
			fclose(fp);
			pcb_board_set_changed_flag(pcb_true);
		}
		return 0;
	}

	if (pcb_strcasecmp(function, "AllUnusedPins") == 0) {
		FILE *fp;
		pcb_bool result;
		if ((fp = pcb_check_and_open_file(name, pcb_true, pcb_false, &result, NULL)) != NULL) {
			pcb_lookup_unused_pins(fp);
			fclose(fp);
			pcb_board_set_changed_flag(pcb_true);
		}
		return 0;
	}

	if (pcb_strcasecmp(function, "ElementConnections") == 0) {
		void *ptrtmp;
		FILE *fp;
		pcb_bool result;
		pcb_coord_t x, y;

		pcb_hid_get_coords("Click on an element", &x, &y);
		if ((pcb_search_screen(x, y, PCB_OBJ_SUBC, &ptrtmp, &ptrtmp, &ptrtmp)) != PCB_OBJ_VOID) {
			pcb_subc_t *subc = (pcb_subc_t *) ptrtmp;
			if ((fp = pcb_check_and_open_file(name, pcb_true, pcb_false, &result, NULL)) != NULL) {
				pcb_lookup_subc_conns(subc, fp);
				fclose(fp);
				pcb_board_set_changed_flag(pcb_true);
			}
		}
		return 0;
	}

	if (pcb_strcasecmp(function, "PasteBuffer") == 0) {
		return pcb_save_buffer_elements(name, fmt);
	}

	PCB_ACT_FAIL(SaveTo);
	PCB_OLD_ACT_END;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_Quit[] = "Quit()";

static const char pcb_acth_Quit[] = "Quits the application after confirming.";

/* %start-doc actions Quit

If you have unsaved changes, you will be prompted to confirm (or
save) before quitting.

%end-doc */

static fgw_error_t pcb_act_Quit(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *force = PCB_ACTION_ARG(0);
	if (force && pcb_strcasecmp(force, "force") == 0) {
		PCB->Changed = 0;
		exit(0);
	}
	if (!PCB->Changed || pcb_gui->close_confirm_dialog() == HID_CLOSE_CONFIRM_OK)
		pcb_quit_app();
	return 1;
	PCB_OLD_ACT_END;
}


/* --------------------------------------------------------------------------- */
static const char pcb_acts_Export[] = "Export(exporter, [exporter-args])";
static const char pcb_acth_Export[] = "Export the current layout, e.g. Export(png, --dpi, 600)";
static fgw_error_t pcb_act_Export(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	if (argc < 1) {
		pcb_message(PCB_MSG_ERROR, "Export() needs at least one argument, the name of the export plugin\n");
		return 1;
	}

	pcb_exporter = pcb_hid_find_exporter(argv[0]);
	if (pcb_exporter == NULL) {
		pcb_message(PCB_MSG_ERROR, "Export plugin %s not found. Was it enabled in ./configure?\n", argv[0]);
		return 1;
	}

	/* remove the name of the exporter */
	argc--;
	argv++;

	/* call the exporter */
	pcb_exporter->parse_arguments(&argc, (char ***)&argv);
	pcb_exporter->do_export(NULL);

	pcb_exporter = NULL;
	return 0;
	PCB_OLD_ACT_END;
}

static const char pcb_acts_Backup[] = "Backup()";
static const char pcb_acth_Backup[] = "Backup the current layout - save using the same method that the timed backup function uses";
static fgw_error_t pcb_act_Backup(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_backup();
	return 0;
	PCB_OLD_ACT_END;
}

pcb_action_t file_action_list[] = {
	{"Backup", pcb_act_Backup, pcb_acth_Backup, pcb_acts_Backup},
	{"Export", pcb_act_Export, pcb_acth_Export, pcb_acts_Export},
	{"LoadFrom", pcb_act_LoadFrom, pcb_acth_LoadFrom, pcb_acts_LoadFrom},
	{"New", pcb_act_New, pcb_acth_New, pcb_acts_New},
	{"Normalize", pcb_act_normalize, pcb_acth_normalize, pcb_acts_normalize},
	{"SaveTo", pcb_act_SaveTo, pcb_acth_SaveTo, pcb_acts_SaveTo},
	{"Quit", pcb_act_Quit, pcb_acth_Quit, pcb_acts_Quit}
};

PCB_REGISTER_ACTIONS(file_action_list, NULL)
