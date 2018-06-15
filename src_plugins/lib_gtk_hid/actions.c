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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include "gui.h"

#include "board.h"
#include "data.h"
#include "search.h"
#include "change.h"
#include "action_helper.h"
#include "hid_attrib.h"
#include "hid_actions.h"
#include "hid.h"
#include "compat_nls.h"
#include "compat_misc.h"

#include "../src_plugins/lib_gtk_common/act_print.h"
#include "../src_plugins/lib_gtk_common/act_fileio.h"
#include "../src_plugins/lib_gtk_common/wt_layersel.h"
#include "../src_plugins/lib_gtk_common/dlg_route_style.h"
#include "../src_plugins/lib_gtk_common/dlg_export.h"
#include "../src_plugins/lib_gtk_common/dlg_library.h"
#include "../src_plugins/lib_gtk_common/dlg_log.h"
#include "../src_plugins/lib_gtk_common/dlg_about.h"
#include "../src_plugins/lib_gtk_common/dlg_drc.h"
#include "../src_plugins/lib_gtk_common/dlg_netlist.h"
#include "../src_plugins/lib_gtk_common/dlg_search.h"
#include "../src_plugins/lib_gtk_common/dlg_fontsel.h"
#include "../src_plugins/lib_gtk_config/lib_gtk_config.h"

#include "actions.h"

static const char *ghid_act_cookie = "gtk HID actions";

/* ------------------------------------------------------------ */

static const char pcb_acts_AdjustStyle[] = "AdjustStyle()\n";

static const char pcb_acth_AdjustStyle[] = "Open the window which allows editing of the route styles.";

/* %start-doc actions AdjustStyle

Opens the window which allows editing of the route styles.

%end-doc */

static int pcb_act_AdjustStyle(int argc, const char **argv)
{
	if (argc > 1)
		PCB_ACT_FAIL(AdjustStyle);

  pcb_gtk_route_style_edit_dialog(&ghidgui->common, GHID_ROUTE_STYLE(ghidgui->topwin.route_style_selector));
	return 0;
}

/* ------------------------------------------------------------ */

static const char pcb_acts_fontsel[] = "FontSel()\n";
static const char pcb_acth_fontsel[] = "Select the font to draw new text with.";
static int pcb_act_fontsel(int argc, const char **argv)
{
	if (argc > 1)
		PCB_ACT_FAIL(fontsel);

	if (argc > 0) {
		if (pcb_strcasecmp(argv[0], "Object") == 0) {
			pcb_coord_t x, y;
			int type;
			void *ptr1, *ptr2, *ptr3;
			pcb_hid_get_coords(_("Select an Object"), &x, &y);
			if ((type = pcb_search_screen(x, y, PCB_CHANGENAME_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID) {
/*				pcb_undo_save_serial();*/
				pcb_gtk_dlg_fontsel(&ghidgui->common, ptr1, ptr2, type, 1);
			}
		}
		else
			PCB_ACT_FAIL(fontsel);
	}
	else
		pcb_gtk_dlg_fontsel(&ghidgui->common, NULL, NULL, 0, 0);
	return 0;
}

/* ---------------------------------------------------------------------- */

static int pcb_act_LayerGroupsChanged(int argc, const char **argv)
{
	printf(_("LayerGroupsChanged -- not implemented\n"));
	return 0;
}

/* ---------------------------------------------------------------------- */

static int pcb_act_Command(int argc, const char **argv)
{
	ghid_handle_user_command(&ghidgui->topwin.cmd, TRUE);
	return 0;
}

/* ------------------------------------------------------------ */

int pcb_gtk_act_print_(int argc, const char **argv)
{
	return pcb_gtk_act_print(gport->top_window, argc, argv);
}

/* ------------------------------------------------------------ */

static int pcb_act_ExportGUI(int argc, const char **argv)
{

	/* check if layout is empty */
	if (!pcb_data_is_empty(PCB->Data)) {
		ghid_dialog_export(ghid_port.top_window);
	}
	else
		pcb_gui->log(_("Can't export empty layout"));

	return 0;
}

/* ------------------------------------------------------------ */

static int pcb_act_Benchmark(int argc, const char **argv)
{
	int i = 0;
	time_t start, end;
	GdkDisplay *display;
	GdkWindow *window;

	window = gtk_widget_get_window(gport->drawing_area);
	display = gtk_widget_get_display(gport->drawing_area);

	gdk_display_sync(display);
	time(&start);
	do {
		ghid_invalidate_all();
		gdk_window_process_updates(window, FALSE);
		time(&end);
		i++;
	}
	while (end - start < 10);

	printf(_("%g redraws per second\n"), i / 10.0);

	return 0;
}

/* ------------------------------------------------------------ */

static const char pcb_acts_DoWindows[] =
	"DoWindows(1|2|3|4|5|6|7 [,false])\n"
	"DoWindows(Layout|Library|Log|Netlist|Preferences|DRC|Search [,false])";

static const char pcb_acth_DoWindows[] = N_("Open various GUI windows. With false, do not raise the window (no focus stealing).");

/* %start-doc actions DoWindows

@table @code

@item 1
@itemx Layout
Open the layout window.  Since the layout window is always shown
anyway, this has no effect.

@item 2
@itemx Library
Open the library window.

@item 3
@itemx Log
Open the log window.

@item 4
@itemx Netlist
Open the netlist window.

@item 5
@itemx Preferences
Open the preferences window.

@item 6
@itemx DRC
Open the DRC violations window.

@item 7
@itemx Search
Open the advanced search window.

@end table

%end-doc */

static int pcb_act_DoWindows(int argc, const char **argv)
{
	const char *a = argc >= 1 ? argv[0] : "";
	gboolean raise = TRUE;

	if (argc >= 2) {
		char c = tolower((argv[1])[0]);
		if ((c == 'n') || (c == 'f') || (c == '0'))
			raise = FALSE;
	}

	if (strcmp(a, "1") == 0 || pcb_strcasecmp(a, "Layout") == 0) {
	}
	else if (strcmp(a, "2") == 0 || pcb_strcasecmp(a, "Library") == 0) {
		pcb_gtk_library_show(&ghidgui->common, raise);
	}
	else if (strcmp(a, "3") == 0 || pcb_strcasecmp(a, "Log") == 0) {
		pcb_gtk_dlg_log_show(raise);
	}
	else if (strcmp(a, "4") == 0 || pcb_strcasecmp(a, "Netlist") == 0) {
		pcb_gtk_dlg_netlist_show(&ghidgui->common, raise);
	}
	else if (strcmp(a, "5") == 0 || pcb_strcasecmp(a, "Preferences") == 0) {
		pcb_gtk_config_window_show(&ghidgui->common, raise);
		/* The 3rd argument will be the path (as a text string, not numbers) to select, once dialog is opened */
		if (argc >= 3) {
			pcb_gtk_config_set_cursor(argv[2]);
		}
	}
	else if (strcmp(a, "6") == 0 || pcb_strcasecmp(a, "DRC") == 0) {
		ghid_drc_window_show(&ghidgui->drcwin, raise);
	}
	else if (strcmp(a, "7") == 0 || pcb_strcasecmp(a, "Search") == 0) {
		ghid_search_window_show(gport->top_window, raise);
	}
	else {
		PCB_ACT_FAIL(DoWindows);
	}

	return 0;
}

/* ------------------------------------------------------------ */
static const char pcb_acts_Popup[] = "Popup(MenuName, [Button])";

static const char pcb_acth_Popup[] =
N_("Bring up the popup menu specified by @code{MenuName}.\n"
	 "If called by a mouse event then the mouse button number\n" "must be specified as the optional second argument.");

/* %start-doc actions Popup

This just pops up the specified menu.  The menu must have been defined
in the popups subtree in the menu lht file.

%end-doc */
static int pcb_act_Popup(int argc, const char **argv)
{
	GtkWidget *menu = NULL;
	char name[256];
	const char *tn = NULL;

	enum {
		CTX_NONE,
		CTX_OBJ_TYPE
	} ctx_sens = CTX_NONE;

	if (argc != 1 && argc != 2)
		PCB_ACT_FAIL(Popup);

	if (argc == 2) {
		if (strcmp(argv[1], "obj-type") == 0) ctx_sens = CTX_OBJ_TYPE;
	}

	if (strlen(argv[0]) < sizeof(name) - 32) {
		lht_node_t *menu_node;
		switch(ctx_sens) {
			case CTX_OBJ_TYPE:
				{
					pcb_coord_t x, y;;
					pcb_objtype_t type;
					void *o1, *o2, *o3;
					pcb_hid_get_coords("context sensitive popup: select object", &x, &y);
					type = pcb_search_screen(x, y, PCB_OBJ_CLASS_REAL, &o1, &o2, &o3);

					if (type == 0)
						tn = "none";
					else
						tn = pcb_obj_type_name(type);

					sprintf(name, "/popups/%s-%s", argv[0], tn);
					menu_node = pcb_hid_cfg_get_menu(ghidgui->topwin.ghid_cfg, name);

					if (menu_node == NULL) {
						sprintf(name, "/popups/%s-misc", argv[0]);
						menu_node = pcb_hid_cfg_get_menu(ghidgui->topwin.ghid_cfg, name);
					}
				}
				break;
			case CTX_NONE:
				sprintf(name, "/popups/%s", argv[0]);
				menu_node = pcb_hid_cfg_get_menu(ghidgui->topwin.ghid_cfg, name);
				break;
				
		}
		if (menu_node != NULL)
			menu = pcb_gtk_menu_widget(menu_node);
	}

	if (!GTK_IS_MENU(menu)) {
		pcb_message(PCB_MSG_ERROR, _("The specified popup menu \"%s\" (context: '%s') has not been defined.\n"), argv[0], (tn == NULL ? "" : tn));
		return 1;
	}
	else {
		ghidgui->topwin.in_popup = TRUE;
		gtk_widget_grab_focus(ghid_port.drawing_area);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
	}
	return 0;
}

/* ------------------------------------------------------------ */
static const char pcb_acts_SaveWinGeo[] = "SaveWindowGeometry()";

static const char pcb_acth_SaveWinGeo[] = N_("Saves window geometry in the config.\n");

static int pcb_act_SaveWinGeo(int argc, const char **argv)
{
	ghid_wgeo_save(1, 0);
	return 0;
}


/* ------------------------------------------------------------ */
static int pcb_act_Zoom(int argc, const char **argv)
{
	return pcb_gtk_zoom(&gport->view, argc, argv);
}

static int pcb_act_Center(int argc, const char **argv)
{
	pcb_coord_t x, y;
	int offset_x, offset_y, pointer_x, pointer_y;
	GdkDisplay *display = gdk_display_get_default();
	GdkScreen *screen = gdk_display_get_default_screen(display);

	pcb_hid_get_coords("Click to center", &x, &y);

	gdk_window_get_origin(gtk_widget_get_window(gport->drawing_area), &offset_x, &offset_y);
	pcb_gtk_act_center(&gport->view, argc, argv, x, y, offset_x, offset_y, &pointer_x, &pointer_y);
	gdk_display_warp_pointer(display, screen, pointer_x, pointer_y);
	return 0;
}

static int pcb_act_SwapSides(int argc, const char **argv)
{
	int res, oa;

	/* ugly workaround: turn off gui refreshes until the swap finishes to avoid triggering more events that may lead to infinite loop */
	oa = ghidgui->hid_active;
	ghidgui->hid_active = 0;

	res = pcb_gtk_swap_sides(&gport->view, argc, argv);

	ghidgui->hid_active = oa;
	return res;
}

static int pcb_act_Scroll(int argc, const char **argv)
{
	if (ghidgui == NULL)
		return 0;

	return pcb_gtk_act_scroll(&gport->view, argc, argv);
}

static int pcb_act_Pan(int argc, const char **argv)
{
	if (ghidgui == NULL)
		return 0;

	return pcb_gtk_act_pan(&gport->view, argc, argv);
}

static int pcb_act_NetlistShow(int argc, const char **argv)
{
	return pcb_gtk_act_netlistshow(&ghidgui->common, argc, argv);
}

static int pcb_act_NetlistPresent(int argc, const char **argv)
{
	return pcb_gtk_act_netlistpresent(&ghidgui->common, argc, argv);
}


static int pcb_act_load(int argc, const char **argv)
{
	return pcb_gtk_act_load(ghid_port.top_window, argc, argv);
}

static int pcb_act_save(int argc, const char **argv)
{
	return pcb_gtk_act_save(ghid_port.top_window, argc, argv);
}

static int pcb_act_importgui(int argc, const char **argv)
{
	return pcb_gtk_act_importgui(ghid_port.top_window, argc, argv);
}

/* ------------------------------------------------------------
 *
 * Actions specific to the GTK HID follow from here
 *
 */

/* ------------------------------------------------------------ */
static const char pcb_acts_About[] = "About()";

static const char pcb_acth_About[] = N_("Tell the user about this version of PCB.");

/* %start-doc actions About

This just pops up a dialog telling the user which version of
@code{pcb} they're running.

%end-doc */


static int pcb_act_About(int argc, const char **argv)
{
	pcb_gtk_dlg_about(gport->top_window);
	return 0;
}

/* ------------------------------------------------------------ */
static const char pcb_acts_EditLayerGroups[] = "EditLayerGroups()";
static const char pcb_acth_EditLayerGroups[] = N_("Pop up the gui that edits the layer groups (layer stackup).");

/* %start-doc actions About

Pop up the gui that edits the layer groups (layer stackup).

%end-doc */

static int pcb_act_EditLayerGroups(int argc, const char **argv)
{
	const char *args[] = {"Preferences", "1", "User PoV/Layers"};
	return pcb_act_DoWindows(sizeof(args)/sizeof(args[0]), args);
}

/* ------------------------------------------------------------ */

pcb_hid_action_t ghid_menu_action_list[] = {
	{"AdjustStyle", pcb_act_AdjustStyle, pcb_acth_AdjustStyle, pcb_acts_AdjustStyle},
	{"fontsel", pcb_act_fontsel, pcb_acth_fontsel, pcb_acts_fontsel}
};

PCB_REGISTER_ACTIONS(ghid_menu_action_list, ghid_act_cookie)

pcb_hid_action_t ghid_main_action_list[] = {
	{"About", pcb_act_About, pcb_acth_About, pcb_acts_About},
	{"Benchmark", pcb_act_Benchmark},
	{"Center", pcb_act_Center, pcb_acth_center, pcb_acts_center},
	{"Command", pcb_act_Command},
	{"DoWindows", pcb_act_DoWindows, pcb_acth_DoWindows, pcb_acts_DoWindows},
	{"EditLayerGroups", pcb_act_EditLayerGroups, pcb_acth_EditLayerGroups, pcb_acts_EditLayerGroups},
	{"ExportGUI", pcb_act_ExportGUI},
	{"ImportGUI", pcb_act_importgui, pcb_gtk_acth_importgui, pcb_gtk_acts_importgui},
	{"LayerGroupsChanged", pcb_act_LayerGroupsChanged},
	{"Load", pcb_act_load },
	{"LogShowOnAppend", pcb_gtk_act_logshowonappend, pcb_gtk_acth_logshowonappend, pcb_gtk_acts_logshowonappend},
	{"NetlistShow", pcb_act_NetlistShow, pcb_gtk_acth_netlistshow, pcb_gtk_acts_netlistshow},
	{"NetlistPresent", pcb_act_NetlistPresent, pcb_gtk_acth_netlistpresent, pcb_gtk_acts_netlistpresent},
	{"Pan", pcb_act_Pan, pcb_acth_pan, pcb_acts_pan},
	{"Popup", pcb_act_Popup, pcb_acth_Popup, pcb_acts_Popup},
	{"Print", pcb_gtk_act_print_, pcb_gtk_acth_print, pcb_gtk_acts_print},
	{"PrintCalibrate", pcb_gtk_act_printcalibrate, pcb_gtk_acth_printcalibrate, pcb_gtk_acts_printcalibrate},
	{"Save", pcb_act_save, pcb_gtk_acth_save, pcb_gtk_acts_save},
	{"SaveWindowGeometry", pcb_act_SaveWinGeo, pcb_acth_SaveWinGeo, pcb_acts_SaveWinGeo},
	{"Scroll", pcb_act_Scroll, pcb_acth_scroll, pcb_acts_scroll},
	{"SwapSides", pcb_act_SwapSides, pcb_acth_swapsides, pcb_acts_swapsides},
	{"Zoom", pcb_act_Zoom, pcb_acth_zoom, pcb_acts_zoom},
	{"ZoomTo", pcb_act_Zoom, pcb_acth_zoom, pcb_acts_zoom}
};

PCB_REGISTER_ACTIONS(ghid_main_action_list, ghid_act_cookie)
#include "dolists.h"

void pcb_gtk_action_reg(void)
{
	PCB_REGISTER_ACTIONS(ghid_main_action_list, ghid_act_cookie)
	PCB_REGISTER_ACTIONS(ghid_menu_action_list, ghid_act_cookie)
}

void pcb_gtk_action_unreg(void)
{
	pcb_hid_remove_actions_by_cookie(ghid_act_cookie);
	pcb_hid_remove_attributes_by_cookie(ghid_act_cookie);
}
