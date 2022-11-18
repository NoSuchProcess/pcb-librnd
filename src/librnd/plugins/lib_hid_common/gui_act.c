/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
 *  Copyright (C) 2019,2020 Tibor 'Igor2' Palinkas
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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include <librnd/rnd_config.h>
#include <librnd/core/hidlib_conf.h>
#include <librnd/core/tool.h>
#include <librnd/core/grid.h>
#include <librnd/core/error.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/event.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/tool.h>
#include <librnd/core/hid_dad.h>

void rnd_hidcore_crosshair_move_to(rnd_hidlib_t *hidlib, rnd_coord_t abs_x, rnd_coord_t abs_y, int mouse_mot)
{
	if (mouse_mot)
		rnd_event(hidlib, RND_EVENT_STROKE_RECORD, "cc", abs_x, abs_y);
	if (rnd_app.crosshair_move_to != NULL)
		rnd_app.crosshair_move_to(hidlib, abs_x, abs_y, mouse_mot);
}


/* This action is provided for CLI convenience */
static const char rnd_acts_FullScreen[] = "FullScreen(on|off|toggle)\n";
static const char rnd_acth_FullScreen[] = "Hide widgets to get edit area full screen";

static fgw_error_t rnd_act_FullScreen(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd = NULL;
	RND_ACT_MAY_CONVARG(1, FGW_STR, FullScreen, cmd = argv[1].val.str);

	if ((cmd == NULL) || (rnd_strcasecmp(cmd, "Toggle") == 0))
		rnd_conf_setf(RND_CFR_DESIGN, "editor/fullscreen", -1, "%d", !rnd_conf.editor.fullscreen, RND_POL_OVERWRITE);
	else if (rnd_strcasecmp(cmd, "On") == 0)
		rnd_conf_set(RND_CFR_DESIGN, "editor/fullscreen", -1, "1", RND_POL_OVERWRITE);
	else if (rnd_strcasecmp(cmd, "Off") == 0)
		rnd_conf_set(RND_CFR_DESIGN, "editor/fullscreen", -1, "0", RND_POL_OVERWRITE);
	else
		RND_ACT_FAIL(FullScreen);

	RND_ACT_IRES(0);
	return 0;
}

static const char rnd_acts_Cursor[] = "Cursor(Type,DeltaUp,DeltaRight,Units)";
static const char rnd_acth_Cursor[] = "Move the cursor.";
/* DOC: cursor.html */
static fgw_error_t rnd_act_Cursor(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_hidlib_t *hidlib = RND_ACT_HIDLIB;
	rnd_unit_list_t extra_units_x = {
		{"grid", 0, 0},
		{"view", 0, RND_UNIT_PERCENT},
		{"board", 0, RND_UNIT_PERCENT},
		{"design", 0, RND_UNIT_PERCENT},
		{"", 0, 0}
	};
	rnd_unit_list_t extra_units_y = {
		{"grid", 0, 0},
		{"view", 0, RND_UNIT_PERCENT},
		{"board", 0, RND_UNIT_PERCENT},
		{"design", 0, RND_UNIT_PERCENT},
		{"", 0, 0}
	};
	int pan_warp = RND_SC_DO_NOTHING;
	double dx, dy;
	rnd_coord_t view_width, view_height;
	const char *a1, *a2, *a3, *op;
	rnd_box_t vbx;

	extra_units_x[0].scale = RND_ACT_HIDLIB->grid;
	extra_units_x[2].scale = rnd_dwg_get_size_x(RND_ACT_HIDLIB);
	extra_units_x[3].scale = rnd_dwg_get_size_x(RND_ACT_HIDLIB);

	extra_units_y[0].scale = RND_ACT_HIDLIB->grid;
	extra_units_y[2].scale = rnd_dwg_get_size_y(RND_ACT_HIDLIB);
	extra_units_y[3].scale = rnd_dwg_get_size_y(RND_ACT_HIDLIB);

	rnd_gui->view_get(rnd_gui, &vbx);
	view_width = vbx.X2 - vbx.X1;
	view_height = vbx.Y2 - vbx.Y1;

	extra_units_x[1].scale = view_width;
	extra_units_y[1].scale = view_height;

	RND_ACT_CONVARG(1, FGW_STR, Cursor, op = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_STR, Cursor, a1 = argv[2].val.str);
	RND_ACT_CONVARG(3, FGW_STR, Cursor, a2 = argv[3].val.str);
	RND_ACT_CONVARG(4, FGW_STR, Cursor, a3 = argv[4].val.str);

	switch(*op) {
		case 'p': case 'P': /* Pan */
			pan_warp = RND_SC_PAN_VIEWPORT;
			break;
		case 'w': case 'W': /* Warp */
			pan_warp = RND_SC_WARP_POINTER;
			break;
		default:
			RND_ACT_FAIL(Cursor);
	}

	if (rnd_strcasecmp(a3, "grid") == 0) {
		char *end;
		dx = strtod(a1, &end) * rnd_conf.editor.grid;
		dy = strtod(a2, &end) * rnd_conf.editor.grid;
	}
	else {
		dx = rnd_get_value_ex(a1, a3, NULL, extra_units_x, "", NULL);
		dy = rnd_get_value_ex(a2, a3, NULL, extra_units_y, "", NULL);
	}

	if (rnd_strcasecmp(a3, "view") == 0) {
		if ((rnd_gui != NULL) && (rnd_gui->view_get != NULL)) {
			rnd_box_t viewbox;
			rnd_gui->view_get(rnd_gui, &viewbox);
			if (rnd_conf.editor.view.flip_x)
				dx = viewbox.X2 - dx;
			else
				dx += viewbox.X1;
			if (rnd_conf.editor.view.flip_y)
				dy = viewbox.Y2 - dy;
			else
				dy += viewbox.Y1;
		}
	}

	/* Allow leaving snapped pin/pad/padstack */
	if (hidlib->tool_snapped_obj_bbox) {
		rnd_box_t *bbx = hidlib->tool_snapped_obj_bbox;
		rnd_coord_t radius = ((bbx->X2 - bbx->X1) + (bbx->Y2 - bbx->Y1))/6;
		if (dx < 0)
			dx -= radius;
		else if (dx > 0)
			dx += radius;
		if (dy < 0)
			dy -= radius;
		else if (dy > 0)
			dy += radius;
	}

	if (rnd_conf.editor.view.flip_x) dx = -dx;
	if (rnd_conf.editor.view.flip_y) dy = -dy;

	rnd_hidcore_crosshair_move_to(hidlib, hidlib->ch_x+dx, hidlib->ch_y+dy, 1);
	rnd_gui->set_crosshair(rnd_gui, hidlib->ch_x, hidlib->ch_y, pan_warp);

	RND_ACT_IRES(0);
	return 0;
}

static const char rnd_acts_MoveCursorTo[] = "MoveCursorTo(x,y)";
static const char rnd_acth_MoveCursorTo[] = "Move the cursor to absolute coords, pan the view as needed.";
static fgw_error_t rnd_act_MoveCursorTo(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_hidlib_t *hidlib = RND_ACT_HIDLIB;
	rnd_coord_t x, y;

	RND_ACT_CONVARG(1, FGW_COORD, Cursor, x = fgw_coord(&argv[1]));
	RND_ACT_CONVARG(2, FGW_COORD, Cursor, y = fgw_coord(&argv[2]));

	rnd_hidcore_crosshair_move_to(RND_ACT_HIDLIB, x, y, 0);
	rnd_gui->set_crosshair(rnd_gui, hidlib->ch_x, hidlib->ch_y, RND_SC_PAN_VIEWPORT);

	RND_ACT_IRES(0);
	return 0;
}


static void grid_ask_enter_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_dad_retovr_t **retovr = caller_data;
	rnd_hid_dad_close(hid_ctx, *retovr, 0);
}

static rnd_coord_t grid_ask(void)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {"ok", 0}, {NULL, 0}};
	int wc;
	rnd_coord_t res = -1;
	RND_DAD_DECL(dlg);

	RND_DAD_BEGIN_VBOX(dlg);
		RND_DAD_COMPFLAG(dlg, RND_HATF_EXPFILL);
		RND_DAD_BEGIN_HBOX(dlg);
			RND_DAD_LABEL(dlg, "New grid size:");
			RND_DAD_COORD(dlg);
				wc = RND_DAD_CURRENT(dlg);
				RND_DAD_ENTER_CB(dlg, grid_ask_enter_cb);
		RND_DAD_END(dlg);
		RND_DAD_BUTTON_CLOSES(dlg, clbtn);
	RND_DAD_END(dlg);

	RND_DAD_NEW("grid_ask", dlg, "Set custom grid size", &dlg_ret_override, rnd_true, NULL);
	if (RND_DAD_RUN(dlg) == 0)
		res = dlg[wc].val.crd;
	RND_DAD_FREE(dlg);

	return res;
}


static const char rnd_acts_grid[] =
	"grid(set, [name:]size[@offs][!unit])\n"
	"grid(+|up)\n" "grid(-|down)\n" "grid(#N)\n" "grid(idx, N)\n" "grid(get)\n" "grid(ask)\n";
static const char rnd_acth_grid[] = "Set the grid.";
static fgw_error_t rnd_act_grid(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *op, *a;

	RND_ACT_CONVARG(1, FGW_STR, grid, op = argv[1].val.str);
	RND_ACT_IRES(0);

	if (strcmp(op, "set") == 0) {
		rnd_grid_t dst;
		RND_ACT_CONVARG(2, FGW_STR, grid, a = argv[2].val.str);
		if (!rnd_grid_parse(&dst, a))
			RND_ACT_FAIL(grid);
		rnd_grid_set(RND_ACT_HIDLIB, &dst);
		rnd_grid_free(&dst);
	}
	else if ((strcmp(op, "up") == 0) || (strcmp(op, "+") == 0))
		rnd_grid_list_step(RND_ACT_HIDLIB, +1);
	else if ((strcmp(op, "down") == 0) || (strcmp(op, "-") == 0))
		rnd_grid_list_step(RND_ACT_HIDLIB, -1);
	else if (strcmp(op, "idx") == 0) {
		RND_ACT_CONVARG(2, FGW_STR, grid, a = argv[2].val.str);
		rnd_grid_list_jump(RND_ACT_HIDLIB, atoi(a));
	}
	else if (op[0] == '#') {
		rnd_grid_list_jump(RND_ACT_HIDLIB, atoi(op+1));
	}
	else if (strcmp(op, "get") == 0) {
		res->type = FGW_COORD;
		fgw_coord(res) = rnd_conf.editor.grid;
	}
	else if (strcmp(op, "ask") == 0) {
		rnd_grid_t g = {0};

		if (!RND_HAVE_GUI_ATTR_DLG)
			return -1;

		g.size = grid_ask();
		if (g.size > 0)
			rnd_grid_set(RND_ACT_HIDLIB, &g);
	}
	else
		RND_ACT_FAIL(grid);

	return 0;
}

static const char rnd_acts_GetXY[] = "GetXY([message, [x|y]])";
static const char rnd_acth_GetXY[] = "Get a coordinate. If x or y specified, the return value of the action is the x or y coordinate.";
/* DOC: getxy.html */
static fgw_error_t rnd_act_GetXY(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_coord_t x, y;
	const char *op = NULL, *msg = "Click to enter a coordinate.";

	RND_ACT_MAY_CONVARG(1, FGW_STR, GetXY, msg = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, GetXY, op = argv[2].val.str);

	rnd_hid_get_coords(msg, &x, &y, 0);

	RND_ACT_IRES(0);
	if (op != NULL) {
		if (((op[0] == 'x') || (op[0] == 'X')) && op[1] == '\0') {
			res->type = FGW_COORD;
			fgw_coord(res) = x;
		}
		else if (((op[0] == 'y') || (op[0] == 'Y')) && op[1] == '\0') {
			res->type = FGW_COORD;
			fgw_coord(res) = y;
		}
		else
			RND_ACT_FAIL(GetXY);
	}

	return 0;
}

static const char rnd_acts_Benchmark[] = "Benchmark()";
static const char rnd_acth_Benchmark[] = "Benchmark the GUI speed.";
/* DOC: benchmark.html */
static fgw_error_t rnd_act_Benchmark(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	double fps = 0;

	if ((rnd_gui != NULL) && (rnd_gui->benchmark != NULL)) {
		fps = rnd_gui->benchmark(rnd_gui);
		rnd_message(RND_MSG_INFO, "%f redraws per second\n", fps);
	}
	else
		rnd_message(RND_MSG_ERROR, "benchmark is not available in the current HID\n");

	RND_ACT_DRES(fps);
	return 0;
}

static const char rnd_acts_Redraw[] = "Redraw()";
static const char rnd_acth_Redraw[] = "Redraw the entire screen";
static fgw_error_t rnd_act_Redraw(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_gui->invalidate_all(rnd_gui);
	RND_ACT_IRES(0);
	return 0;
}

static rnd_action_t rnd_gui_action_list[] = {
	{"FullScreen", rnd_act_FullScreen, rnd_acth_FullScreen, rnd_acts_FullScreen},
	{"Cursor", rnd_act_Cursor, rnd_acth_Cursor, rnd_acts_Cursor},
	{"MoveCursorTo", rnd_act_MoveCursorTo, rnd_acth_MoveCursorTo, rnd_acts_MoveCursorTo},
	{"Grid", rnd_act_grid, rnd_acth_grid, rnd_acts_grid},
	{"GetXY", rnd_act_GetXY, rnd_acth_GetXY, rnd_acts_GetXY},
	{"Benchmark", rnd_act_Benchmark, rnd_acth_Benchmark, rnd_acts_Benchmark},
	{"Redraw", rnd_act_Redraw, rnd_acth_Redraw, rnd_acts_Redraw}
};

void rnd_gui_act_init2(void)
{
	RND_REGISTER_ACTIONS(rnd_gui_action_list, NULL);
}
