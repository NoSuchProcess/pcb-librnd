/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
 *
 *  This module, dialogs, was written and is Copyright (C) 2017 by Tibor Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <genht/htsp.h>
#include <genht/hash.h>
#include <ctype.h>
#include "actions.h"
#include "compat_misc.h"
#include "hid_dad.h"
#include "error.h"

#include "act_dad.h"

#define MAX_ENUM 128

typedef struct tmp_str_s {
	struct tmp_str_s *next;
	char str[1];
} tmp_str_t;

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	char *name;
	int level;
	tmp_str_t *tmp_str_head;
	unsigned running:1;
} dad_t;

htsp_t dads;

static int dad_new(const char *name)
{
	dad_t *dad;

	if (htsp_get(&dads, name) != NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't create named DAD dialog %s: already exists\n", name);
		return -1;
	}

	dad = calloc(sizeof(dad_t), 1);
	dad->name = pcb_strdup(name);
	htsp_set(&dads, dad->name, dad);
	return 0;
}

static void dad_destroy(dad_t *dad)
{
	tmp_str_t *t, *tnext;
	for(t = dad->tmp_str_head; t != NULL; t = tnext) {
		tnext = t->next;
		free(t);
	}
	htsp_pop(&dads, dad->name);
	free(dad->name);
	free(dad);
}

static void dad_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	dad_t *dad = caller_data;
	PCB_DAD_FREE(dad->dlg);
	dad_destroy(dad);
}

static char *tmp_str_dup(dad_t *dad, const char *txt)
{
	size_t len = strlen(txt);
	tmp_str_t *tmp = malloc(sizeof(tmp_str_t) + len);
	tmp->next = dad->tmp_str_head;
	memcpy(tmp->str, txt, len+1);
	return tmp->str;
}


const char pcb_acts_dad[] =
	"dad(dlgname, new) - create new dialog\n"
	"dad(dlgname, label, text) - append a label widget\n"
	"dad(dlgname, button, text) - append a button widget\n"
	"dad(dlgname, enum, choices) - append an enum (combo box) widget; choices is a tab separated list\n"
	"dad(dlgname, bool, [label]) - append an checkbox widget (default off)\n"
	"dad(dlgname, integer|real|coord, min, max, [label]) - append an input field\n"
	"dad(dlgname, string) - append a single line text input field\n"
	"dad(dlgname, begin_hbox) - begin horizontal box\n"
	"dad(dlgname, begin_vbox) - begin vertical box\n"
	"dad(dlgname, begin_table, cols) - begin table layout box\n"
	"dad(dlgname, end) - end the last begin\n"
	"dad(dlgname, flags, flg1, flg2, ...) - change the flags of the last created widget\n"
	"dad(dlgname, run, longname, shortname) - present dlgname as a non-modal dialog\n"
	"dad(dlgname, run_modal, longname, shortname) - present dlgname as a modal dialog\n"
	;
const char pcb_acth_dad[] = "Manipulate Dynamic Attribute Dialogs";
fgw_error_t pcb_act_dad(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *cmd, *dlgname, *txt;
	dad_t *dad;
	int rv = 0;

	PCB_ACT_CONVARG(1, FGW_STR, dad, dlgname = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, dad, cmd = argv[2].val.str);

	if (pcb_strcasecmp(cmd, "new") == 0) {
		PCB_ACT_IRES(dad_new(dlgname));
		return 0;
	}

	dad = htsp_get(&dads, dlgname);
	if (dad == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't find named DAD dialog %s\n", dlgname);
		PCB_ACT_IRES(-1);
		return 0;
	}


	if (pcb_strcasecmp(cmd, "label") == 0) {
		if (dad->running) goto cant_chg;
		PCB_ACT_CONVARG(3, FGW_STR, dad, txt = argv[3].val.str);
		PCB_DAD_LABEL(dad->dlg, txt);
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "button") == 0) {
		if (dad->running) goto cant_chg;
		PCB_ACT_CONVARG(3, FGW_STR, dad, txt = argv[3].val.str);
		PCB_DAD_BUTTON(dad->dlg, tmp_str_dup(dad, txt));
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "bool") == 0) {
		if (dad->running) goto cant_chg;
		txt = "";
		PCB_ACT_MAY_CONVARG(3, FGW_STR, dad, txt = argv[3].val.str);
		PCB_DAD_BOOL(dad->dlg, txt);
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "integer") == 0) {
		long vmin, vmax;
		if (dad->running) goto cant_chg;
		txt = "";
		PCB_ACT_CONVARG(3, FGW_LONG, dad, vmin = argv[3].val.nat_long);
		PCB_ACT_CONVARG(4, FGW_LONG, dad, vmax = argv[4].val.nat_long);
		PCB_ACT_MAY_CONVARG(5, FGW_STR, dad, txt = argv[5].val.str);
		PCB_DAD_INTEGER(dad->dlg, txt);
		PCB_DAD_MINMAX(dad->dlg, vmin, vmax);
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "real") == 0) {
		double vmin, vmax;
		if (dad->running) goto cant_chg;
		txt = "";
		PCB_ACT_CONVARG(3, FGW_DOUBLE, dad, vmin = argv[3].val.nat_double);
		PCB_ACT_CONVARG(4, FGW_DOUBLE, dad, vmax = argv[4].val.nat_double);
		PCB_ACT_MAY_CONVARG(5, FGW_STR, dad, txt = argv[5].val.str);
		PCB_DAD_REAL(dad->dlg, txt);
		PCB_DAD_MINMAX(dad->dlg, vmin, vmax);
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "coord") == 0) {
		pcb_coord_t vmin, vmax;
		if (dad->running) goto cant_chg;
		txt = "";
		PCB_ACT_CONVARG(3, FGW_COORD_, dad, vmin = fgw_coord(&argv[3]));
		PCB_ACT_CONVARG(4, FGW_COORD_, dad, vmax = fgw_coord(&argv[4]));
		PCB_ACT_MAY_CONVARG(5, FGW_STR, dad, txt = argv[5].val.str);
		PCB_DAD_COORD(dad->dlg, txt);
		PCB_DAD_MINMAX(dad->dlg, vmin, vmax);
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "string") == 0) {
		if (dad->running) goto cant_chg;
		PCB_DAD_STRING(dad->dlg);
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "enum") == 0) {
		char *s, *next;
		const char *values[MAX_ENUM+1];
		int len = 0;

		if (dad->running) goto cant_chg;

		PCB_ACT_CONVARG(3, FGW_STR, dad, txt = argv[3].val.str);

		s = tmp_str_dup(dad, txt);
		while(isspace(*s)) s++;
		for(len = 0; s != NULL; s = next) {
			if (len >= MAX_ENUM) {
				pcb_message(PCB_MSG_ERROR, "Too many DAD enum values\n");
				rv = -1;
				break;
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
		PCB_DAD_ENUM(dad->dlg, values);
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "begin_hbox") == 0) {
		if (dad->running) goto cant_chg;
		PCB_DAD_BEGIN_HBOX(dad->dlg);
		dad->level++;
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "begin_vbox") == 0) {
		if (dad->running) goto cant_chg;
		PCB_DAD_BEGIN_VBOX(dad->dlg);
		dad->level++;
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "begin_table") == 0) {
		int cols;

		if (dad->running) goto cant_chg;

		PCB_ACT_CONVARG(3, FGW_INT, dad, cols = argv[3].val.nat_int);
		PCB_DAD_BEGIN_TABLE(dad->dlg, cols);
		dad->level++;
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "end") == 0) {
		if (dad->running) goto cant_chg;

		PCB_DAD_END(dad->dlg);
		dad->level--;
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if (pcb_strcasecmp(cmd, "flags") == 0) {
		int n;
		pcb_hatt_compflags_t tmp, flg = 0;

		if (dad->running) goto cant_chg;

		for(n = 3; n < argc; n++) {
			PCB_ACT_CONVARG(n, FGW_STR, dad, txt = argv[n].val.str);
			if ((*txt == '\0') || (*txt == '0'))
				continue;
			tmp = pcb_hid_compflag_name2bit(txt);
			if (tmp == 0)
				pcb_message(PCB_MSG_ERROR, "Invalid DAD flag: %s (ignored)\n", txt);
			flg |= tmp;
		}
		PCB_DAD_COMPFLAG(dad->dlg, flg);
		rv = PCB_DAD_CURRENT(dad->dlg);
	}
	else if ((pcb_strcasecmp(cmd, "run") == 0) || (pcb_strcasecmp(cmd, "run_modal") == 0)) {
		char *sh;

		if (dad->running) goto cant_chg;

		PCB_ACT_CONVARG(3, FGW_STR, dad, txt = argv[3].val.str);
		PCB_ACT_CONVARG(4, FGW_STR, dad, sh = argv[4].val.str);

		if (dad->level != 0) {
			pcb_message(PCB_MSG_ERROR, "Invalid DAD dialog structure: %d levels not closed (missing 'end' calls)\n", dad->level);
			rv = -1;
		}
		else {
			PCB_DAD_NEW(dad->dlg, txt, sh, dad, (cmd[3] == '_'), dad_close_cb);
			rv = PCB_DAD_CURRENT(dad->dlg);
		}
	}
	else {
		pcb_message(PCB_MSG_ERROR, "Invalid DAD dialog command: '%s'\n", cmd);
		rv = -1;
	}

	PCB_ACT_IRES(rv);
	return 0;

	cant_chg:;
	pcb_message(PCB_MSG_ERROR, "Can't find named DAD dialog %s\n", dlgname);
	PCB_ACT_IRES(-1);
	return 0;
}

void pcb_act_dad_init(void)
{
	htsp_init(&dads, strhash, strkeyeq);
}

void pcb_act_dad_uninit(void)
{
	htsp_entry_t *e;
	for(e = htsp_first(&dads); e != NULL; e = htsp_next(&dads, e))
		dad_destroy(e->value);
	htsp_uninit(&dads);
}
