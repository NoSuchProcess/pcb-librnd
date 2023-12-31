/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2023 Tibor 'Igor2' Palinkas
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
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <librnd/rnd_config.h>

#include <genht/htsp.h>
#include <genht/hash.h>
#include <genvector/vts0.h>
#include <ctype.h>
#include <librnd/core/actions.h>
#include <librnd/core/compat_misc.h>
#include <librnd/hid/hid_dad.h>
#include <librnd/hid/hid_dad_tree.h>
#include <librnd/core/error.h>

#include "act_dad.h"

#define MAX_ENUM 128

typedef union tmp_u tmp_t;

typedef struct tmp_str_s {
	tmp_t *next;
	char str[1];
} tmp_str_t;

typedef struct tmp_strlist_s {
	tmp_t *next;
	char *values[MAX_ENUM+1];
} tmp_strlist_t;

union tmp_u {
	struct {
		tmp_t *next;
	} list;
	tmp_str_t str;
	tmp_strlist_t strlist;
};

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	rnd_design_t *hidlib;
	char *name;
	const char *row_domain;
	int level;
	tmp_t *tmp_str_head;
	vts0_t change_cb;
	unsigned running:1;
} dad_t;

htsp_t dads;

static int dad_new(rnd_design_t *hl, const char *name)
{
	dad_t *dad;

	if (htsp_get(&dads, name) != NULL) {
		rnd_message(RND_MSG_ERROR, "Can't create named DAD dialog %s: already exists\n", name);
		return -1;
	}

	dad = calloc(sizeof(dad_t), 1);
	dad->name = rnd_strdup(name);
	dad->row_domain = dad->name;
	dad->hidlib = hl;
	htsp_set(&dads, dad->name, dad);
	return 0;
}

static void dad_destroy(dad_t *dad)
{
	tmp_t *t, *tnext;
	for(t = dad->tmp_str_head; t != NULL; t = tnext) {
		tnext = t->list.next;
		free(t);
	}
	htsp_pop(&dads, dad->name);
	free(dad->name);
	free(dad);
}

static void dad_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	dad_t *dad = caller_data;
	RND_DAD_FREE(dad->dlg);
	dad_destroy(dad);
}

static void dad_change_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	dad_t *dad = caller_data;
	int idx = attr - dad->dlg;
	char **act = vts0_get(&dad->change_cb, idx, 0);
	if ((act != NULL) && (*act != NULL))
		rnd_parse_command(dad->hidlib, *act, 1);
}

static void dad_row_free_cb(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_hid_row_t *row)
{
	rnd_hid_tree_t *tree = attrib->wdata;
	dad_t *dad = tree->user_ctx;
	fgw_arg_t res;
	res.type = FGW_PTR | FGW_VOID;
	res.val.ptr_void = row;
	fgw_ptr_unreg(&rnd_fgw, &res, dad->row_domain);
}

typedef struct {
	char *act_expose, *act_mouse, *act_free;
	char *udata;
	rnd_design_t *hidlib;
} dad_prv_t;

static int prv_action(rnd_design_t *hl, const char *actname, rnd_hid_gc_t gc, const char *udata)
{
	fgw_arg_t r = {0};
	fgw_arg_t args[3];
	int rv = 0;

	if ((actname == NULL) || (*actname == '\0'))
		return 0;

	if (gc == NULL) {
		args[2].type = FGW_PTR; args[2].val.ptr_void = NULL;
	}
	else
		fgw_ptr_reg(&rnd_fgw, &args[1], RND_PTR_DOMAIN_GC, FGW_PTR | FGW_STRUCT, gc);
	args[2].type = FGW_STR; args[2].val.cstr = udata;
	rnd_actionv_bin(hl, actname, &r, 3, args);
	if (fgw_arg_conv(&rnd_fgw, &r, FGW_INT) == 0)
		rv = r.val.nat_int;
	fgw_arg_free(&rnd_fgw, &r);
	if (gc != NULL)
		fgw_ptr_unreg(&rnd_fgw, &args[1], RND_PTR_DOMAIN_GC);

	return rv;
}

void dad_prv_expose_cb(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_gc_t gc, rnd_hid_expose_ctx_t *e)
{
	dad_prv_t *ctx = prv->user_ctx;
	prv_action(ctx->hidlib, ctx->act_expose, gc, ctx->udata);
}

rnd_bool dad_prv_mouse_cb(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_mouse_ev_t kind, rnd_coord_t x, rnd_coord_t y)
{
	dad_prv_t *ctx = prv->user_ctx;
	return prv_action(ctx->hidlib, ctx->act_mouse, NULL, ctx->udata);
}

void dad_prv_free_cb(rnd_hid_attribute_t *attrib, void *user_ctx, void *hid_ctx)
{
	dad_prv_t *ctx = user_ctx;

	prv_action(ctx->hidlib, ctx->act_free, NULL, ctx->udata);

	free(ctx->act_expose);
	free(ctx->act_mouse);
	free(ctx->act_free);
	free(ctx->udata);
	free(ctx);
}


static char *tmp_str_dup(dad_t *dad, const char *txt)
{
	size_t len = strlen(txt);
	tmp_str_t *tmp = malloc(sizeof(tmp_str_t) + len);
	tmp->next = dad->tmp_str_head;
	dad->tmp_str_head = (tmp_t *)tmp;
	memcpy(tmp->str, txt, len+1);
	return tmp->str;
}

static char **tmp_new_strlist(dad_t *dad)
{
	tmp_strlist_t *tmp = malloc(sizeof(tmp_strlist_t));
	tmp->next = dad->tmp_str_head;
	dad->tmp_str_head = (tmp_t *)tmp;
	return tmp->values;
}

static int split_tablist(dad_t *dad, char **values, const char *txt, const char *cmd)
{
	char *next, *s = tmp_str_dup(dad, txt);
	int len = 0;

	while(isspace(*s)) s++;

	for(len = 0; s != NULL; s = next) {
		if (len >= MAX_ENUM) {
			rnd_message(RND_MSG_ERROR, "Too many DAD %s values\n", cmd);
			return -1;
		}
		next = strchr(s, '\t');
		if (next != NULL) {
			*next = '\0';
			next++;
			while(isspace(*next)) next++;
		}
		values[len] = s;
		len++;
	}
	values[len] = NULL;
	return 0;
}

const char rnd_acts_dad[] =
	"dad(dlgname, new) - create new dialog\n"
	"dad(dlgname, label, text) - append a label widget\n"
	"dad(dlgname, button, text) - append a button widget\n"
	"dad(dlgname, button_closes, label, retval, ...) - standard close buttons\n"
	"dad(dlgname, enum, choices) - append an enum (combo box) widget; choices is a tab separated list\n"
	"dad(dlgname, bool) - append an checkbox widget (default off)\n"
	"dad(dlgname, integer|real|coord, min, max) - append an input field\n"
	"dad(dlgname, string) - append a single line text input field\n"
	"dad(dlgname, default, val) - set the default value of a widet while creating the dialog\n"
	"dad(dlgname, help, tooltip) - set the help (tooltip) text for the current widget\n"
	"dad(dlgname, progress) - append a progress bar (set to 0)\n"
	"dad(dlgname, preview, cb_act_prefix, minsize_x, minsize_y, [ctx]) - append a preview with a viewbox of 10*10mm, minsize in pixels\n"
	"dad(dlgname, tree, cols, istree, [header]) - append tree-table widget; header is like enum values\n"
	"dad(dlgname, tree_append, row, cells) - append after row (0 means last item of the root); cells is like enum values; returns a row pointer\n"
	"dad(dlgname, tree_append_under, row, cells) - append at the end of the list under row (0 means last item of the root); cells is like enum values; returns a row pointer\n"
	"dad(dlgname, tree_insert, row, cells) - insert before row (0 means first item of the root); cells is like enum values; returns a row pointer\n"
	"dad(dlgname, begin_hbox) - begin horizontal box\n"
	"dad(dlgname, begin_vbox) - begin vertical box\n"
	"dad(dlgname, begin_hpane) - begin horizontal paned box\n"
	"dad(dlgname, begin_vpane) - begin vertical paned box\n"
	"dad(dlgname, begin_table, cols) - begin table layout box\n"
	"dad(dlgname, begin_tabbed, tabnames) - begin a view with tabs; tabnames are like choices in an enum; must have as many children widgets as many names it has\n"
	"dad(dlgname, end) - end the last begin\n"
	"dad(dlgname, flags, flg1, flg2, ...) - change the flags of the last created widget\n"
	"dad(dlgname, onchange, action) - set the action to be called on widget change\n"
	"dad(dlgname, run, title) - present dlgname as a non-modal dialog\n"
	"dad(dlgname, run_modal, title) - present dlgname as a modal dialog\n"
	"dad(dlgname, exists) - returns wheter the named dialog exists (0 or 1)\n"
	"dad(dlgname, set, widgetID, val) - changes the value of a widget in a running dialog \n"
	"dad(dlgname, get, widgetID, [unit]) - return the current value of a widget\n"
	"dad(dlgname, iterate) - runs a global GUI iteration (event dispatch, redraw)\n"
	"dad(dlgname, raise) - pops up window in front\n"
	"dad(dlgname, close) - close the dialog (and return 0 from modal run)\n"
	;
const char rnd_acth_dad[] = "Manipulate Dynamic Attribute Dialogs";
fgw_error_t rnd_act_dad(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *cmd, *dlgname, *txt;
	dad_t *dad;
	int rv = 0;

	RND_ACT_CONVARG(1, FGW_STR, dad, dlgname = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_STR, dad, cmd = argv[2].val.str);

	if (rnd_strcasecmp(cmd, "new") == 0) {
		RND_ACT_IRES(dad_new(RND_ACT_DESIGN, dlgname));
		return 0;
	}

	dad = htsp_get(&dads, dlgname);
	if (rnd_strcasecmp(cmd, "exists") == 0) {
		RND_ACT_IRES(dad != NULL);
		return 0;
	}

	if (dad == NULL) {
		rnd_message(RND_MSG_ERROR, "Can't find named DAD dialog %s\n", dlgname);
		RND_ACT_IRES(-1);
		return 0;
	}


	if (rnd_strcasecmp(cmd, "label") == 0) {
		if (dad->running) goto cant_chg;
		RND_ACT_CONVARG(3, FGW_STR, dad, txt = argv[3].val.str);
		RND_DAD_LABEL(dad->dlg, txt);
		rv = RND_DAD_CURRENT(dad->dlg);
	}
	else if (rnd_strcasecmp(cmd, "button") == 0) {
		if (dad->running) goto cant_chg;
		RND_ACT_CONVARG(3, FGW_STR, dad, txt = argv[3].val.str);
		RND_DAD_BUTTON(dad->dlg, tmp_str_dup(dad, txt));
		rv = RND_DAD_CURRENT(dad->dlg);
	}
	else if (rnd_strcasecmp(cmd, "button_closes") == 0) {
		int n, ret;

		if (dad->running) goto cant_chg;

		RND_DAD_BEGIN_HBOX(dad->dlg);
		RND_DAD_BEGIN_HBOX(dad->dlg);
		RND_DAD_COMPFLAG(dad->dlg, RND_HATF_EXPFILL);
		RND_DAD_END(dad->dlg);
		for(n = 3; n < argc; n+=2) {
			RND_ACT_CONVARG(n+0, FGW_STR, dad, txt = argv[n+0].val.str);
			RND_ACT_CONVARG(n+1, FGW_INT, dad, ret = argv[n+1].val.nat_int);
			
			RND_DAD_BUTTON_CLOSE(dad->dlg, tmp_str_dup(dad, txt), ret);
				rv = RND_DAD_CURRENT(dad->dlg);
		}
		RND_DAD_END(dad->dlg);
	}
	else if (rnd_strcasecmp(cmd, "bool") == 0) {
		if (dad->running) goto cant_chg;
		RND_DAD_BOOL(dad->dlg);
		rv = RND_DAD_CURRENT(dad->dlg);
	}
	else if (rnd_strcasecmp(cmd, "integer") == 0) {
		long vmin, vmax;
		if (dad->running) goto cant_chg;
		RND_ACT_CONVARG(3, FGW_LONG, dad, vmin = argv[3].val.nat_long);
		RND_ACT_CONVARG(4, FGW_LONG, dad, vmax = argv[4].val.nat_long);
		RND_DAD_INTEGER(dad->dlg);
		RND_DAD_MINMAX(dad->dlg, vmin, vmax);
		rv = RND_DAD_CURRENT(dad->dlg);
	}
	else if (rnd_strcasecmp(cmd, "real") == 0) {
		double vmin, vmax;
		if (dad->running) goto cant_chg;
		RND_ACT_CONVARG(3, FGW_DOUBLE, dad, vmin = argv[3].val.nat_double);
		RND_ACT_CONVARG(4, FGW_DOUBLE, dad, vmax = argv[4].val.nat_double);
		RND_DAD_REAL(dad->dlg);
		RND_DAD_MINMAX(dad->dlg, vmin, vmax);
		rv = RND_DAD_CURRENT(dad->dlg);
	}
	else if (rnd_strcasecmp(cmd, "coord") == 0) {
		rnd_coord_t vmin, vmax;
		if (dad->running) goto cant_chg;
		RND_ACT_CONVARG(3, FGW_COORD_, dad, vmin = fgw_coord(&argv[3]));
		RND_ACT_CONVARG(4, FGW_COORD_, dad, vmax = fgw_coord(&argv[4]));
		RND_DAD_COORD(dad->dlg);
		RND_DAD_MINMAX(dad->dlg, vmin, vmax);
		rv = RND_DAD_CURRENT(dad->dlg);
	}
	else if (rnd_strcasecmp(cmd, "string") == 0) {
		if (dad->running) goto cant_chg;
		RND_DAD_STRING(dad->dlg);
		rv = RND_DAD_CURRENT(dad->dlg);
	}
	else if (rnd_strcasecmp(cmd, "default") == 0) {
		int i, ty = dad->dlg[RND_DAD_CURRENT(dad->dlg)].type;
		double d;
		rnd_coord_t c;

		if (ty == RND_HATT_END) {
			rnd_hid_dad_spin_t *spin = dad->dlg[RND_DAD_CURRENT(dad->dlg)].wdata;
			switch(spin->type) {
				case RND_DAD_SPIN_INT:     ty = RND_HATT_INTEGER; break;
				case RND_DAD_SPIN_DOUBLE:  ty = RND_HATT_REAL; break;
				case RND_DAD_SPIN_FREQ:    ty = RND_HATT_REAL; break;
				case RND_DAD_SPIN_COORD:   ty = RND_HATT_COORD; break;
				case RND_DAD_SPIN_UNIT_CRD: ty = RND_HATT_UNIT; break;
			}
		}

		switch(ty) {
			case RND_HATT_COORD:
				RND_ACT_CONVARG(3, FGW_COORD, dad, c = fgw_coord(&argv[3]));
				RND_DAD_DEFAULT_NUM(dad->dlg, c);
				break;
			case RND_HATT_REAL:
			case RND_HATT_PROGRESS:
				RND_ACT_CONVARG(3, FGW_DOUBLE, dad, d = argv[3].val.nat_double);
				RND_DAD_DEFAULT_NUM(dad->dlg, d);
				break;
			case RND_HATT_INTEGER:
			case RND_HATT_BOOL:
			case RND_HATT_ENUM:
				RND_ACT_CONVARG(3, FGW_INT, dad, i = argv[3].val.nat_int);
				RND_DAD_DEFAULT_NUM(dad->dlg, i);
				break;
			default:
				rnd_message(RND_MSG_ERROR, "dad(): Invalid widget type - can not change default value (set ignored)\n");
				RND_ACT_IRES(-1);
				return 0;
		}
	}
	else if (rnd_strcasecmp(cmd, "help") == 0) {
		const char *tip;
		RND_ACT_CONVARG(3, FGW_STR, dad, tip = argv[3].val.cstr);
		RND_DAD_HELP(dad->dlg, tmp_str_dup(dad, tip));
	}
	else if (rnd_strcasecmp(cmd, "progress") == 0) {
		if (dad->running) goto cant_chg;
		RND_DAD_PROGRESS(dad->dlg);
		rv = RND_DAD_CURRENT(dad->dlg);
	}
	else if (rnd_strcasecmp(cmd, "preview") == 0) {
		char *prefix, *suctx = "";
		int sx, sy;
		rnd_box_t vb;
		dad_prv_t *uctx;

		/* dad(dlgname, preview, cb_act_prefix, minsize_x, minsize_y, ctx) */
		RND_ACT_CONVARG(3, FGW_STR, dad, prefix = argv[3].val.str);
		RND_ACT_CONVARG(4, FGW_INT, dad, sx = argv[4].val.nat_int);
		RND_ACT_CONVARG(5, FGW_INT, dad, sy = argv[5].val.nat_int);
		RND_ACT_MAY_CONVARG(6, FGW_STR, dad, suctx = argv[6].val.str);
		vb.X1 = vb.Y1 = 0;
		vb.X2 = vb.Y2 = RND_MM_TO_COORD(10);

		uctx = malloc(sizeof(dad_prv_t));
		uctx->act_expose = rnd_concat(prefix, "expose", NULL);
		uctx->act_mouse = rnd_concat(prefix, "mouse", NULL);
		uctx->act_free = rnd_concat(prefix, "free", NULL);
		uctx->udata = rnd_strdup(suctx);
		uctx->hidlib = RND_ACT_DESIGN;
		RND_DAD_PREVIEW(dad->dlg, dad_prv_expose_cb, dad_prv_mouse_cb, NULL, dad_prv_free_cb, &vb, sx, sy, uctx);
	}
	else if ((rnd_strcasecmp(cmd, "enum") == 0) || (rnd_strcasecmp(cmd, "begin_tabbed") == 0)) {
		char **values = tmp_new_strlist(dad);

		if (dad->running) goto cant_chg;

		RND_ACT_CONVARG(3, FGW_STR, dad, txt = argv[3].val.str);

		if (split_tablist(dad, values, txt, cmd) == 0) {
			if (*cmd == 'b') {
				RND_DAD_BEGIN_TABBED(dad->dlg, (const char **)values);
				dad->level++;
			}
			else
				RND_DAD_ENUM(dad->dlg, (const char **)values);
			rv = RND_DAD_CURRENT(dad->dlg);
		}
		else
			rv = -1;
	}
	else if (rnd_strcasecmp(cmd, "tree") == 0) {
		int cols, istree;
		char **values = tmp_new_strlist(dad);

		if (dad->running) goto cant_chg;

		txt = NULL;
		RND_ACT_CONVARG(3, FGW_INT, dad, cols = argv[3].val.nat_int);
		RND_ACT_CONVARG(4, FGW_INT, dad, istree = argv[4].val.nat_int);
		RND_ACT_MAY_CONVARG(5, FGW_STR, dad, txt = argv[5].val.str);

		if ((txt == NULL) || (split_tablist(dad, values, txt, cmd) == 0)) {
			RND_DAD_TREE(dad->dlg, cols, istree, (const char **)values);
			RND_DAD_TREE_SET_CB(dad->dlg, free_cb, dad_row_free_cb);
			RND_DAD_TREE_SET_CB(dad->dlg, ctx, dad);
			rv = RND_DAD_CURRENT(dad->dlg);
		}
		else
			rv = -1;
	}
	else if ((rnd_strcasecmp(cmd, "tree_append") == 0) || (rnd_strcasecmp(cmd, "tree_append_under") == 0) || (rnd_strcasecmp(cmd, "tree_insert") == 0)) {
		void *row, *nrow = NULL;
		char **values = tmp_new_strlist(dad);

		if (dad->running) goto cant_chg;

		RND_ACT_CONVARG(3, FGW_PTR, dad, row = argv[3].val.ptr_void);
		RND_ACT_CONVARG(4, FGW_STR, dad, txt = argv[4].val.str);

		if (row != NULL) {
			if (!fgw_ptr_in_domain(&rnd_fgw, &argv[3], dad->row_domain)) {
				rnd_message(RND_MSG_ERROR, "Invalid DAD row pointer\n");
				RND_ACT_IRES(-1);
				return 0;
			}
		}

		if ((txt == NULL) || (split_tablist(dad, values, txt, cmd) == 0)) {
			if (cmd[5] == 'i')
				nrow = RND_DAD_TREE_INSERT(dad->dlg, row, values);
			else if (cmd[11] == '_')
				nrow = RND_DAD_TREE_APPEND_UNDER(dad->dlg, row, values);
			else
				nrow = RND_DAD_TREE_APPEND(dad->dlg, row, values);
		}
		else
			nrow = NULL;
		fgw_ptr_reg(&rnd_fgw, res, dad->row_domain, FGW_PTR, nrow);
		return 0;
	}
	else if (rnd_strcasecmp(cmd, "begin_hbox") == 0) {
		if (dad->running) goto cant_chg;
		RND_DAD_BEGIN_HBOX(dad->dlg);
		dad->level++;
		rv = RND_DAD_CURRENT(dad->dlg);
	}
	else if (rnd_strcasecmp(cmd, "begin_vbox") == 0) {
		if (dad->running) goto cant_chg;
		RND_DAD_BEGIN_VBOX(dad->dlg);
		dad->level++;
		rv = RND_DAD_CURRENT(dad->dlg);
	}
	else if (rnd_strcasecmp(cmd, "begin_hpane") == 0) {
		if (dad->running) goto cant_chg;
		RND_DAD_BEGIN_HPANE(dad->dlg, "anon_scripted");
		dad->level++;
		rv = RND_DAD_CURRENT(dad->dlg);
	}
	else if (rnd_strcasecmp(cmd, "begin_vpane") == 0) {
		if (dad->running) goto cant_chg;
		RND_DAD_BEGIN_VPANE(dad->dlg, "anon_scripted");
		dad->level++;
		rv = RND_DAD_CURRENT(dad->dlg);
	}
	else if (rnd_strcasecmp(cmd, "begin_table") == 0) {
		int cols;

		if (dad->running) goto cant_chg;

		RND_ACT_CONVARG(3, FGW_INT, dad, cols = argv[3].val.nat_int);
		RND_DAD_BEGIN_TABLE(dad->dlg, cols);
		dad->level++;
		rv = RND_DAD_CURRENT(dad->dlg);
	}
	else if (rnd_strcasecmp(cmd, "end") == 0) {
		if (dad->running) goto cant_chg;

		RND_DAD_END(dad->dlg);
		dad->level--;
		rv = RND_DAD_CURRENT(dad->dlg);
	}
	else if (rnd_strcasecmp(cmd, "flags") == 0) {
		int n;
		rnd_hatt_compflags_t tmp, flg = 0;

		if (dad->running) goto cant_chg;

		for(n = 3; n < argc; n++) {
			RND_ACT_CONVARG(n, FGW_STR, dad, txt = argv[n].val.str);
			if ((*txt == '\0') || (*txt == '0'))
				continue;
			tmp = rnd_hid_compflag_name2bit(txt);
			if (tmp == 0)
				rnd_message(RND_MSG_ERROR, "Invalid DAD flag: %s (ignored)\n", txt);
			flg |= tmp;
		}
		RND_DAD_COMPFLAG(dad->dlg, flg);
		rv = RND_DAD_CURRENT(dad->dlg);
	}
	else if (rnd_strcasecmp(cmd, "onchange") == 0) {
		RND_ACT_CONVARG(3, FGW_STR, dad, txt = argv[3].val.str);
		RND_DAD_CHANGE_CB(dad->dlg, dad_change_cb);
		vts0_set(&dad->change_cb, RND_DAD_CURRENT(dad->dlg), tmp_str_dup(dad, txt));
		rv = 0;
	}
	else if (rnd_strcasecmp(cmd, "set") == 0) {
		int wid, i;
		double d;
		rnd_coord_t c;
		rnd_hid_attr_type_t wtype;

		RND_ACT_CONVARG(3, FGW_INT, dad, wid = argv[3].val.nat_int);
		if ((wid < 0) || (wid >= dad->dlg_len)) {
			rnd_message(RND_MSG_ERROR, "Invalid widget ID %d (set ignored)\n", wid);
			RND_ACT_IRES(-1);
			return 0;
		}

		wtype = dad->dlg[wid].type;
		if (dad->dlg[wid].type == RND_HATT_END) /* composite widget's end or real end - the spin macro handles both */
			wtype = RND_DAD_SPIN_GET_TYPE(&dad->dlg[wid]);

		if (dad->dlg_hid_ctx == NULL) {
			rnd_message(RND_MSG_ERROR, "dad(): Dialog is not yet running, can't set\n");
			RND_ACT_IRES(-1);
			return 0;
		}

		switch(wtype) {
			case RND_HATT_COORD:
				RND_ACT_CONVARG(4, FGW_COORD, dad, c = fgw_coord(&argv[4]));
				RND_DAD_SET_VALUE(dad->dlg_hid_ctx, wid, crd, c);
				break;
			case RND_HATT_REAL:
			case RND_HATT_PROGRESS:
				RND_ACT_CONVARG(4, FGW_DOUBLE, dad, d = argv[4].val.nat_double);
				RND_DAD_SET_VALUE(dad->dlg_hid_ctx, wid, dbl, d);
				break;
			case RND_HATT_INTEGER:
			case RND_HATT_BOOL:
			case RND_HATT_ENUM:
			case RND_HATT_BEGIN_TABBED:
				RND_ACT_CONVARG(4, FGW_INT, dad, i = argv[4].val.nat_int);
				RND_DAD_SET_VALUE(dad->dlg_hid_ctx, wid, lng, i);
				break;
			case RND_HATT_STRING:
			case RND_HATT_LABEL:
			case RND_HATT_BUTTON:
				RND_ACT_CONVARG(4, FGW_STR, dad, txt = argv[4].val.str);
				RND_DAD_SET_VALUE(dad->dlg_hid_ctx, wid, str, txt);
				break;
			default:
				rnd_message(RND_MSG_ERROR, "Invalid widget type %d - can not change value (set ignored)\n", wid);
				RND_ACT_IRES(-1);
				return 0;
		}
		rv = 0;
	}
	else if (rnd_strcasecmp(cmd, "get") == 0) {
		int wid;
		rnd_hid_attr_type_t wtype;

		RND_ACT_CONVARG(3, FGW_INT, dad, wid = argv[3].val.nat_int);
		if ((wid < 0) || (wid >= dad->dlg_len)) {
			rnd_message(RND_MSG_ERROR, "Invalid widget ID %d (get ignored)\n", wid);
			return FGW_ERR_NOT_FOUND;
		}

		wtype = dad->dlg[wid].type;
		if (dad->dlg[wid].type == RND_HATT_END) /* composite widget's end or real end - the spin macro handles both */
			wtype = RND_DAD_SPIN_GET_TYPE(&dad->dlg[wid]);

		switch(wtype) {
			case RND_HATT_COORD:
				txt = NULL;
				RND_ACT_MAY_CONVARG(4, FGW_STR, dad, txt = argv[4].val.str);
				if (txt != NULL) {
					const rnd_unit_t *u = rnd_get_unit_struct(txt);
					if (u == NULL) {
						rnd_message(RND_MSG_ERROR, "Invalid unit %s (get ignored)\n", txt);
						return FGW_ERR_NOT_FOUND;
					}
					res->type = FGW_DOUBLE;
					res->val.nat_double = rnd_coord_to_unit(u, dad->dlg[wid].val.crd);
				}
				else {
					res->type = FGW_COORD;
					fgw_coord(res) = dad->dlg[wid].val.crd;
				}
				break;
			case RND_HATT_INTEGER:
			case RND_HATT_BOOL:
			case RND_HATT_ENUM:
			case RND_HATT_BEGIN_TABBED:
				res->type = FGW_INT;
				res->val.nat_int = dad->dlg[wid].val.lng;
				break;
			case RND_HATT_REAL:
				res->type = FGW_DOUBLE;
				res->val.nat_double = dad->dlg[wid].val.dbl;
				break;
			case RND_HATT_STRING:
			case RND_HATT_LABEL:
			case RND_HATT_BUTTON:
				res->type = FGW_STR;
				res->val.str = (char *)dad->dlg[wid].val.str;
				break;
			default:
				rnd_message(RND_MSG_ERROR, "Invalid widget type %d - can not retrieve value (get ignored)\n", wid);
				return FGW_ERR_NOT_FOUND;
		}
		return 0;
	}
	else if (rnd_strcasecmp(cmd, "iterate") == 0) {
		rnd_gui->iterate(rnd_gui);
	}
	else if (rnd_strcasecmp(cmd, "raise") == 0) {
		if (rnd_gui->attr_dlg_raise != NULL)
			rnd_gui->attr_dlg_raise(dad->dlg_hid_ctx);
	}
	else if (rnd_strcasecmp(cmd, "close") == 0) {
		rnd_dad_retovr_t retovr = {0};
		rnd_hid_dad_close(dad->dlg_hid_ctx, &retovr, 0);
	}
	else if ((rnd_strcasecmp(cmd, "run") == 0) || (rnd_strcasecmp(cmd, "run_modal") == 0)) {
		if (dad->running) goto cant_chg;

		RND_ACT_CONVARG(3, FGW_STR, dad, txt = argv[3].val.str);

		if (dad->level != 0) {
			rnd_message(RND_MSG_ERROR, "Invalid DAD dialog structure: %d levels not closed (missing 'end' calls)\n", dad->level);
			rv = -1;
		}
		else {
			int modal = (cmd[3] == '_');
			RND_DAD_NEW(dlgname, dad->dlg, txt, dad, modal, dad_close_cb);
			if (modal) {
				rv = RND_DAD_RUN(dad->dlg);
			}
			else
				rv = RND_DAD_CURRENT(dad->dlg);
		}
	}
	else {
		rnd_message(RND_MSG_ERROR, "Invalid DAD dialog command: '%s'\n", cmd);
		rv = -1;
	}

	RND_ACT_IRES(rv);
	return 0;

	cant_chg:;
	rnd_message(RND_MSG_ERROR, "Can't find named DAD dialog %s\n", dlgname);
	RND_ACT_IRES(-1);
	return 0;
}

void rnd_act_dad_init(void)
{
	htsp_init(&dads, strhash, strkeyeq);
}

void rnd_act_dad_uninit(void)
{
	htsp_entry_t *e;
	for(e = htsp_first(&dads); e != NULL; e = htsp_next(&dads, e))
		dad_destroy(e->value);
	htsp_uninit(&dads);
}
