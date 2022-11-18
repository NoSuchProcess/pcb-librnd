/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2020,2021 Tibor 'Igor2' Palinkas
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
 */

#include <librnd/rnd_config.h>
#include <librnd/core/actions.h>
#include <librnd/core/hidlib_conf.h>
#include <librnd/core/funchash_core.h>
#include <librnd/core/error.h>
#include <librnd/core/hidlib.h>
#include <librnd/core/grid.h>
#include <librnd/core/tool.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/misc_util.h>


static const char rnd_acts_ChkMode[] = "ChkMode(expected_mode)" ;
static const char rnd_acth_ChkMode[] = "Return 1 if the currently selected mode is the expected_mode";
static fgw_error_t rnd_act_ChkMode(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *dst;
	rnd_toolid_t id;

	RND_ACT_CONVARG(1, FGW_STR, ChkMode, dst = argv[1].val.str);

	id = rnd_tool_lookup(dst);
	if (id >= 0) {
		RND_ACT_IRES(rnd_conf.editor.mode == id);
		return 0;
	}
	RND_ACT_IRES(-1);
	return 0;
}


static const char rnd_acts_ChkGridSize[] =
	"ChkGridSize(expected_size)\n"
	"ChkGridSize(none)\n"
	;
static const char rnd_acth_ChkGridSize[] = "Return 1 if the currently selected grid matches the expected_size. If argument is \"none\" return 1 if there is no grid.";
static fgw_error_t rnd_act_ChkGridSize(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *dst;

	RND_ACT_CONVARG(1, FGW_STR, ChkGridSize, dst = argv[1].val.str);

	if (strcmp(dst, "none") == 0) {
		RND_ACT_IRES(RND_ACT_HIDLIB->grid <= 300);
		return 0;
	}

	RND_ACT_IRES(RND_ACT_HIDLIB->grid == rnd_get_value_ex(dst, NULL, NULL, NULL, NULL, NULL));
	return 0;
}


static const char rnd_acts_ChkGridUnits[] = "ChkGridUnits(expected)";
static const char rnd_acth_ChkGridUnits[] = "Return 1 if currently selected grid unit matches the expected (normally mm or mil)";
static fgw_error_t rnd_act_ChkGridUnits(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *expected;
	RND_ACT_CONVARG(1, FGW_STR, ChkGridUnits, expected = argv[1].val.str);
	RND_ACT_IRES(strcmp(rnd_conf.editor.grid_unit->suffix, expected) == 0);
	return 0;
}

static const char rnd_acts_SetGrid[] = "SetGrid(delta|*mult|/div, [unit])";
static const char rnd_acth_SetGrid[] = "Change grid size.";
/* for doc: copy from SetValue(grid,...) */
static fgw_error_t rnd_act_SetGrid(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *val, *units = NULL;
	rnd_bool absolute;
	double value;

	RND_ACT_CONVARG(1, FGW_STR, SetGrid, val = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, SetGrid, units = argv[2].val.str);

	RND_ACT_IRES(0);

	/* special case: can't convert with rnd_get_value() */
	if ((val[0] == '*') || (val[0] == '/')) {
		double d;
		char *end;

		d = strtod(val+1, &end);
		if ((*end != '\0') || (d <= 0)) {
			rnd_message(RND_MSG_ERROR, "SetGrid: Invalid multiplier/divider for grid set: needs to be a positive number\n");
			return 1;
		}
		rnd_grid_inval();
		if (val[0] == '*')
			rnd_hidlib_set_grid(RND_ACT_HIDLIB, rnd_round(RND_ACT_HIDLIB->grid * d), rnd_false, 0, 0);
		else
			rnd_hidlib_set_grid(RND_ACT_HIDLIB, rnd_round(RND_ACT_HIDLIB->grid / d), rnd_false, 0, 0);
		return 0;
	}

	value = rnd_get_value(val, units, &absolute, NULL);

	rnd_grid_inval();
	if (absolute)
		rnd_hidlib_set_grid(RND_ACT_HIDLIB, value, rnd_false, 0, 0);
	else {
		/* On the way down, until the minimum unit (1) */
		if ((value + RND_ACT_HIDLIB->grid) < 1)
			rnd_hidlib_set_grid(RND_ACT_HIDLIB, 1, rnd_false, 0, 0);
		else if (RND_ACT_HIDLIB->grid == 1)
			rnd_hidlib_set_grid(RND_ACT_HIDLIB, value, rnd_false, 0, 0);
		else
			rnd_hidlib_set_grid(RND_ACT_HIDLIB, value + RND_ACT_HIDLIB->grid, rnd_false, 0, 0);
	}
	return 0;
}

static const char rnd_acts_SetGridOffs[] = "SetGridOffs(x_offs, y_offs)";
static const char rnd_acth_SetGridOffs[] = "Change grid offset (alignment) to x_offs and y_offs. Offsets should be specified with units.";
/* DOC: setgridoffs.html */
static fgw_error_t rnd_act_SetGridOffs(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *xs, *ys;
	rnd_bool xa, ya, xv, yv;
	rnd_coord_t x, y;

	RND_ACT_CONVARG(1, FGW_STR, SetGridOffs, xs = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_STR, SetGridOffs, ys = argv[2].val.str);

	RND_ACT_IRES(0);

	x = rnd_get_value(xs, NULL, &xa, &xv);
	y = rnd_get_value(ys, NULL, &ya, &yv);

	if (!xv || !yv) {
		rnd_message(RND_MSG_ERROR, "SetGrid: Invalid%s%s offset value\n", (xv ? "" : " x"), (yv ? "" : " x"));
		return 1;
	}

	if (!xa) x += RND_ACT_HIDLIB->grid_ox;
	if (!ya) y += RND_ACT_HIDLIB->grid_oy;


	rnd_grid_inval();
	rnd_hidlib_set_grid(RND_ACT_HIDLIB, RND_ACT_HIDLIB->grid, rnd_true, x, y);

	return 0;
}

static rnd_action_t rnd_hid_action_list[] = {
	{"ChkMode", rnd_act_ChkMode, rnd_acth_ChkMode, rnd_acts_ChkMode},
	{"ChkGridSize", rnd_act_ChkGridSize, rnd_acth_ChkGridSize, rnd_acts_ChkGridSize},
	{"ChkGridUnits", rnd_act_ChkGridUnits, rnd_acth_ChkGridUnits, rnd_acts_ChkGridUnits},
	{"SetGrid", rnd_act_SetGrid, rnd_acth_SetGrid, rnd_acts_SetGrid},
	{"SetGridOffs", rnd_act_SetGridOffs, rnd_acth_SetGridOffs, rnd_acts_SetGridOffs},
};

void rnd_hid_act_init2(void)
{
	RND_REGISTER_ACTIONS(rnd_hid_action_list, NULL);
}



