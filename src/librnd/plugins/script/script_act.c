/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <ctype.h>
#include <librnd/hid/hid_dad.h>
#include <librnd/hid/hid_dad_tree.h>
#include "live_script.h"

/*** dialog box ***/
typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */

	int wslist; /* list of scripts */
	int walist; /* list of actions */
} script_dlg_t;

script_dlg_t script_dlg;

static void script_dlg_s2d_act(script_dlg_t *ctx)
{
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	rnd_hid_row_t *r;
	char *cell[2];
	htsp_entry_t *e;
	script_t *sc;

	attr = &ctx->dlg[ctx->walist];
	tree = attr->wdata;

	/* remove existing items */
	for(r = gdl_first(&tree->rows); r != NULL; r = gdl_first(&tree->rows))
		rnd_dad_tree_remove(attr, r);

	r = rnd_dad_tree_get_selected(&ctx->dlg[ctx->wslist]);
	if (r == NULL)
		return;

	sc = htsp_get(&scripts, r->cell[0]);
	if (sc == NULL)
		return;

	/* add all actions */
	cell[1] = NULL;
	for(e = htsp_first(&sc->obj->func_tbl); e; e = htsp_next(&sc->obj->func_tbl, e)) {
		cell[0] = rnd_strdup(e->key);
		rnd_dad_tree_append(attr, NULL, cell);
	}
}


static void script_dlg_s2d(script_dlg_t *ctx)
{
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	rnd_hid_row_t *r;
	char *cell[4], *cursor_path = NULL;
	htsp_entry_t *e;

	attr = &ctx->dlg[ctx->wslist];
	tree = attr->wdata;

	/* remember cursor */
	r = rnd_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = rnd_strdup(r->cell[0]);

	/* remove existing items */
	for(r = gdl_first(&tree->rows); r != NULL; r = gdl_first(&tree->rows))
		rnd_dad_tree_remove(attr, r);

	/* add all items */
	cell[3] = NULL;
	for(e = htsp_first(&scripts); e; e = htsp_next(&scripts, e)) {
		script_t *s = (script_t *)e->value;
		cell[0] = rnd_strdup(s->id);
		cell[1] = rnd_strdup(s->lang);
		cell[2] = rnd_strdup(s->fn);
		rnd_dad_tree_append(attr, NULL, cell);
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		rnd_hid_attr_val_t hv;
		hv.str = cursor_path;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wslist, &hv);
		free(cursor_path);
	}
	script_dlg_s2d_act(ctx);
}

void script_dlg_update(void)
{
	if (script_dlg.active)
		script_dlg_s2d(&script_dlg);
}

static void script_dlg_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	script_dlg_t *ctx = caller_data;
	RND_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(script_dlg_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void btn_unload_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	script_dlg_t *ctx = caller_data;
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(&ctx->dlg[ctx->wslist]);
	if (row == NULL)
		return;

	rnd_script_unload(row->cell[0], "unload");
	script_dlg_s2d(ctx);
}

static void btn_reload_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	script_dlg_t *ctx = caller_data;
	rnd_design_t *hl = rnd_gui->get_dad_design(hid_ctx);
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(&ctx->dlg[ctx->wslist]);
	if (row == NULL)
		return;

	script_reload(hl, row->cell[0]);
	script_dlg_s2d(ctx);
}

static void slist_cb(rnd_hid_attribute_t *attr, void *hid_ctx, rnd_hid_row_t *row)
{
	rnd_hid_tree_t *tree = attr->wdata;
	script_dlg_s2d_act((script_dlg_t *)tree->user_ctx);
}

static void btn_load_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	script_dlg_t *ctx = caller_data;
	rnd_design_t *hl = rnd_gui->get_dad_design(hid_ctx);
	int failed;
	char *tmp, *fn = rnd_hid_fileselect(rnd_gui, "script to load", "Select a script file to load", NULL, NULL, NULL, "script", RND_HID_FSD_READ, NULL);
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {"ok", 0}, {NULL, 0}};
	typedef struct {
		RND_DAD_DECL_NOINIT(dlg)
		int wid, wlang;
	} idlang_t;
	idlang_t idlang;

	if (fn == NULL)
		return;

	memset(&idlang, 0, sizeof(idlang));
	RND_DAD_BEGIN_VBOX(idlang.dlg);
		RND_DAD_BEGIN_HBOX(idlang.dlg);
			RND_DAD_LABEL(idlang.dlg, "ID:");
			RND_DAD_STRING(idlang.dlg);
				idlang.wid = RND_DAD_CURRENT(idlang.dlg);
				tmp = strrchr(fn, RND_DIR_SEPARATOR_C);
				if (tmp != NULL) {
					tmp++;
					idlang.dlg[idlang.wid].val.str = tmp = rnd_strdup(tmp);
					tmp = strchr(tmp, '.');
					if (tmp != NULL)
						*tmp = '\0';
				}
		RND_DAD_END(idlang.dlg);
		RND_DAD_BEGIN_HBOX(idlang.dlg);
			RND_DAD_LABEL(idlang.dlg, "language:");
			RND_DAD_STRING(idlang.dlg);
				idlang.wlang = RND_DAD_CURRENT(idlang.dlg);
				idlang.dlg[idlang.wlang].val.str = rnd_strdup_null(rnd_script_guess_lang(NULL, fn, 1));
		RND_DAD_END(idlang.dlg);
		RND_DAD_BUTTON_CLOSES(idlang.dlg, clbtn);
	RND_DAD_END(idlang.dlg);


	RND_DAD_AUTORUN("script_load", idlang.dlg, "load script", NULL, failed);

	if ((!failed) && (rnd_script_load(hl, idlang.dlg[idlang.wid].val.str, fn, idlang.dlg[idlang.wlang].val.str) == 0))
		script_dlg_s2d(ctx);

	RND_DAD_FREE(idlang.dlg);
}

static void script_dlg_open(void)
{
	static const char *hdr[] = {"ID", "language", "file", NULL};
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	if (script_dlg.active)
		return; /* do not open another */

	RND_DAD_BEGIN_VBOX(script_dlg.dlg);
	RND_DAD_COMPFLAG(script_dlg.dlg, RND_HATF_EXPFILL);
	RND_DAD_BEGIN_HPANE(script_dlg.dlg, "left-right");
		RND_DAD_COMPFLAG(script_dlg.dlg, RND_HATF_EXPFILL);
		/* left side */
		RND_DAD_BEGIN_VBOX(script_dlg.dlg);
			RND_DAD_COMPFLAG(script_dlg.dlg, RND_HATF_EXPFILL);
			RND_DAD_TREE(script_dlg.dlg, 3, 0, hdr);
				RND_DAD_COMPFLAG(script_dlg.dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
				script_dlg.wslist = RND_DAD_CURRENT(script_dlg.dlg);
				RND_DAD_TREE_SET_CB(script_dlg.dlg, selected_cb, slist_cb);
				RND_DAD_TREE_SET_CB(script_dlg.dlg, ctx, &script_dlg);
			RND_DAD_BEGIN_HBOX(script_dlg.dlg);
				RND_DAD_BUTTON(script_dlg.dlg, "Unload");
					RND_DAD_HELP(script_dlg.dlg, "Unload the currently selected script");
					RND_DAD_CHANGE_CB(script_dlg.dlg, btn_unload_cb);
				RND_DAD_BUTTON(script_dlg.dlg, "Reload");
					RND_DAD_HELP(script_dlg.dlg, "Reload the currently selected script\n(useful if the script has changed)");
					RND_DAD_CHANGE_CB(script_dlg.dlg, btn_reload_cb);
				RND_DAD_BUTTON(script_dlg.dlg, "Load...");
					RND_DAD_HELP(script_dlg.dlg, "Load a new script from disk");
					RND_DAD_CHANGE_CB(script_dlg.dlg, btn_load_cb);
			RND_DAD_END(script_dlg.dlg);
		RND_DAD_END(script_dlg.dlg);

		/* right side */
		RND_DAD_BEGIN_VBOX(script_dlg.dlg);
			RND_DAD_COMPFLAG(script_dlg.dlg, RND_HATF_EXPFILL);
			RND_DAD_LABEL(script_dlg.dlg, "Actions:");
			RND_DAD_TREE(script_dlg.dlg, 1, 0, NULL);
				RND_DAD_COMPFLAG(script_dlg.dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
				script_dlg.walist = RND_DAD_CURRENT(script_dlg.dlg);
		RND_DAD_END(script_dlg.dlg);
	RND_DAD_END(script_dlg.dlg);
	RND_DAD_BUTTON_CLOSES(script_dlg.dlg, clbtn);
	RND_DAD_END(script_dlg.dlg);

	/* set up the context */
	script_dlg.active = 1;

	RND_DAD_NEW("scripts", script_dlg.dlg, "librnd Scripts", &script_dlg, rnd_false, script_dlg_close_cb);
	script_dlg_s2d(&script_dlg);
}

/*** actions ***/

static int script_id_invalid(const char *id)
{
	for(; *id != '\0'; id++)
		if (!isalnum(*id) && (*id != '_'))
			return 1;
	return 0;
}

#define ID_VALIDATE(id, act) \
do { \
	if (script_id_invalid(id)) { \
		rnd_message(RND_MSG_ERROR, #act ": Invalid script ID '%s' (must contain only alphanumeric characters and underscores)\n", id); \
		return FGW_ERR_ARG_CONV; \
	} \
} while(0)

static const char rnd_acth_LoadScript[] = "Load a fungw script";
static const char rnd_acts_LoadScript[] = "LoadScript(id, filename, [language])";
/* DOC: loadscript.html */
static fgw_error_t rnd_act_LoadScript(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *id, *fn, *lang = NULL;
	RND_ACT_CONVARG(1, FGW_STR, LoadScript, id = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_STR, LoadScript, fn = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, LoadScript, lang = argv[3].val.str);

	ID_VALIDATE(id, LoadScript);

	RND_ACT_IRES(rnd_script_load(RND_ACT_DESIGN, id, fn, lang));
	script_dlg_update();
	return 0;
}

static const char rnd_acth_UnloadScript[] = "Unload a fungw script";
static const char rnd_acts_UnloadScript[] = "UnloadScript(id)";
/* DOC: unloadscript.html */
static fgw_error_t rnd_act_UnloadScript(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *id = NULL;
	RND_ACT_CONVARG(1, FGW_STR, UnloadScript, id = argv[1].val.str);

	ID_VALIDATE(id, UnloadScript);

	RND_ACT_IRES(rnd_script_unload(id, "unload"));
	return 0;
}

static const char rnd_acth_ReloadScript[] = "Reload a fungw script";
static const char rnd_acts_ReloadScript[] = "ReloadScript(id)";
/* DOC: reloadscript.html */
static fgw_error_t rnd_act_ReloadScript(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *id = NULL;
	RND_ACT_CONVARG(1, FGW_STR, UnloadScript, id = argv[1].val.str);

	ID_VALIDATE(id, ReloadScript);

	RND_ACT_IRES(script_reload(RND_ACT_DESIGN, id));
	script_dlg_update();
	return 0;
}

static const char rnd_acth_ScriptPersistency[] = "Read or remove script persistency data savd on preunload";
static const char rnd_acts_ScriptPersistency[] = "ScriptPersistency(read|remove)";
/* DOC: scriptpersistency.html */
static fgw_error_t rnd_act_ScriptPersistency(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd = NULL;
	RND_ACT_CONVARG(1, FGW_STR, ScriptPersistency, cmd = argv[1].val.str);
	return script_persistency(res, cmd);
}

static const char rnd_acth_ListScripts[] = "List fungw scripts, optionally filtered wiht regex pat.";
static const char rnd_acts_ListScripts[] = "ListScripts([pat])";
/* DOC: listscripts.html */
static fgw_error_t rnd_act_ListScripts(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *pat = NULL;
	RND_ACT_MAY_CONVARG(1, FGW_STR, ListScripts, pat = argv[1].val.str);

	script_list(pat);

	RND_ACT_IRES(0);
	return 0;
}

static const char rnd_acth_BrowseScripts[] = "Present a dialog box for browsing scripts";
static const char rnd_acts_BrowseScripts[] = "BrowseScripts()";
static fgw_error_t rnd_act_BrowseScripts(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	script_dlg_open();
	RND_ACT_IRES(0);
	return 0;
}

static const char rnd_acth_ScriptCookie[] = "Return a cookie specific to the current script instance during script initialization";
static const char rnd_acts_ScriptCookie[] = "ScriptCookie()";
/* DOC: scriptcookie.html */
static fgw_error_t rnd_act_ScriptCookie(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	res->type = FGW_STR | FGW_DYN;
	res->val.str = script_gen_cookie(NULL);
	if (res->val.str == NULL)
		return -1;
	return 0;
}

static const char rnd_acth_Oneliner[] = "Execute a script one-liner using a specific language";
static const char rnd_acts_Oneliner[] = "Oneliner(lang, script)";
/* DOC: onliner.html */
static fgw_error_t rnd_act_Oneliner(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *first = NULL, *lang = argv[0].val.func->name, *olang, *scr = NULL;

	if (strcmp(lang, "oneliner") == 0) {
		/* call to oneliner(lang, script) */
		RND_ACT_CONVARG(1, FGW_STR, Oneliner, lang = argv[1].val.str);
		RND_ACT_CONVARG(2, FGW_STR, Oneliner, scr = argv[2].val.str);
	}
	else if (strcmp(lang, "/exit") == 0) {
		RND_ACT_IRES(rnd_cli_leave());
		return 0;
	}
	else {
		/* call to lang(script) */
		RND_ACT_MAY_CONVARG(1, FGW_STR, Oneliner, scr = argv[1].val.str);
	}

	RND_ACT_MAY_CONVARG(1, FGW_STR, Oneliner, first = argv[1].val.str);
	if (first != NULL) {
		if (*first == '/') {
			if (rnd_strcasecmp(scr, "/exit") == 0) {
				RND_ACT_IRES(rnd_cli_leave());
				return 0;
			}
			RND_ACT_IRES(-1); /* ignore /click, /tab and others for now */
			return 0;
		}
	}

	olang = lang;
	lang = rnd_script_guess_lang(NULL, lang, 0);

	if (lang == NULL) {
		rnd_message(RND_MSG_ERROR, "Failed to load scripting engine for %s\n", olang);
		RND_ACT_IRES(-1);
		return 0;
	}

	if (scr == NULL) {
		RND_ACT_IRES(rnd_cli_enter(lang, lang));
		return 0;
	}

	if (rnd_strcasecmp(scr, "/exit") == 0) {
		RND_ACT_IRES(rnd_cli_leave());
		return 0;
	}

	RND_ACT_IRES(script_oneliner(RND_ACT_DESIGN, lang, scr));
	return 0;
}

static const char rnd_acth_ActionString[] = "Execute a pcb-rnd action parsing a string; syntac: \"action(arg,arg,arg)\"";
static const char rnd_acts_ActionString[] = "ActionString(action)";
static fgw_error_t rnd_act_ActionString(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *act;
	RND_ACT_CONVARG(1, FGW_STR, ActionString, act = argv[1].val.str);
	return rnd_parse_command_res(RND_ACT_DESIGN, res, act, 1);
}

static const char rnd_acth_rnd_math0[] = "No-argument math functions";
static const char rnd_acts_rnd_math0[] = "rnd_MATHFUNC()";
static fgw_error_t rnd_act_rnd_math0(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *actname = argv[0].val.func->name;

	switch(actname[4]) {
		case 'r':
			res->type = FGW_INT;
			res->val.nat_int = rand();
			return 0;
	}
	return FGW_ERR_ARG_CONV;
}

static const char rnd_acth_rnd_math1[] = "Single-argument math functions";
static const char rnd_acts_rnd_math1[] = "rnd_MATHFUNC(val)";
static fgw_error_t rnd_act_rnd_math1(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *actname = argv[0].val.func->name;
	double a;
	
	RND_ACT_CONVARG(1, FGW_DOUBLE, rnd_math1, a = argv[1].val.nat_double);
	res->type = FGW_DOUBLE;
	switch(actname[4]) {
		case 'a':
			switch(actname[5]) {
				case 's': res->val.nat_double = asin(a); return 0;
				case 'c': res->val.nat_double = acos(a); return 0;
				case 't': res->val.nat_double = atan(a); return 0;
			}
			break;
		case 's':
			switch(actname[5]) {
				case 'i': res->val.nat_double = sin(a); return 0;
				case 'q': res->val.nat_double = sqrt(a); return 0;
				case 'r': res->val.nat_double = 0; srand(a); return 0;
			}
			break;
		case 'c': res->val.nat_double = cos(a); return 0;
		case 't': res->val.nat_double = tan(a); return 0;
	}
	return FGW_ERR_ARG_CONV;
}

static const char rnd_acth_rnd_math2[] = "Two-argument math functions";
static const char rnd_acts_rnd_math2[] = "rnd_MATHFUNC(a,b)";
static fgw_error_t rnd_act_rnd_math2(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *actname = argv[0].val.func->name;
	double a, b;
	
	RND_ACT_CONVARG(1, FGW_DOUBLE, rnd_math2, a = argv[1].val.nat_double);
	RND_ACT_CONVARG(2, FGW_DOUBLE, rnd_math2, b = argv[2].val.nat_double);
	res->type = FGW_DOUBLE;
	switch(actname[4]) {
		case 'a': res->val.nat_double = atan2(a, b); return 0;
	}
	return FGW_ERR_ARG_CONV;
}


static rnd_action_t script_action_list[] = {
	{"LoadScript", rnd_act_LoadScript, rnd_acth_LoadScript, rnd_acts_LoadScript},
	{"UnloadScript", rnd_act_UnloadScript, rnd_acth_UnloadScript, rnd_acts_UnloadScript},
	{"ReloadScript", rnd_act_ReloadScript, rnd_acth_ReloadScript, rnd_acts_ReloadScript},
	{"ScriptPersistency", rnd_act_ScriptPersistency, rnd_acth_ScriptPersistency, rnd_acts_ScriptPersistency},
	{"ListScripts", rnd_act_ListScripts, rnd_acth_ListScripts, rnd_acts_ListScripts},
	{"BrowseScripts", rnd_act_BrowseScripts, rnd_acth_BrowseScripts, rnd_acts_BrowseScripts},
	{"AddTimer", rnd_act_AddTimer, rnd_acth_AddTimer, rnd_acts_AddTimer},
	{"ScriptCookie", rnd_act_ScriptCookie, rnd_acth_ScriptCookie, rnd_acts_ScriptCookie},
	{"LiveScript", rnd_act_LiveScript, rnd_acth_LiveScript, rnd_acts_LiveScript},
	
	/* script shorthands */
	{"fawk",        rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
	{"fpas",        rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
	{"pas",         rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
	{"fbas",        rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
	{"bas",         rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
#ifdef RND_HAVE_SYS_FUNGW
	{"awk",         rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
	{"mawk",        rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
	{"lua",         rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
	{"tcl",         rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
	{"javascript",  rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
	{"duktape",     rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
	{"js",          rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
	{"stt",         rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
	{"estutter",    rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
	{"perl",        rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
	{"ruby",        rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
	{"mruby",       rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
	{"py",          rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
	{"python",      rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
#endif
	{"Oneliner", rnd_act_Oneliner, rnd_acth_Oneliner, rnd_acts_Oneliner},
	{"ActionString", rnd_act_ActionString, rnd_acth_ActionString, rnd_acts_ActionString},

	/* math */
	{"rnd_rand",    rnd_act_rnd_math0, NULL, NULL},

	{"rnd_sin",     rnd_act_rnd_math1, NULL, NULL},
	{"rnd_cos",     rnd_act_rnd_math1, NULL, NULL},
	{"rnd_asin",    rnd_act_rnd_math1, NULL, NULL},
	{"rnd_acos",    rnd_act_rnd_math1, NULL, NULL},
	{"rnd_atan",    rnd_act_rnd_math1, NULL, NULL},
	{"rnd_tan",     rnd_act_rnd_math1, NULL, NULL},
	{"rnd_sqrt",    rnd_act_rnd_math1, NULL, NULL},
	{"rnd_srand",   rnd_act_rnd_math1, NULL, NULL},

	{"rnd_atan2",   rnd_act_rnd_math2, NULL, NULL},

	/* math (compatibility with pre-3.0.0) */
	{"pcb_rand",    rnd_act_rnd_math0, NULL, NULL},

	{"pcb_sin",     rnd_act_rnd_math1, NULL, NULL},
	{"pcb_cos",     rnd_act_rnd_math1, NULL, NULL},
	{"pcb_asin",    rnd_act_rnd_math1, NULL, NULL},
	{"pcb_acos",    rnd_act_rnd_math1, NULL, NULL},
	{"pcb_atan",    rnd_act_rnd_math1, NULL, NULL},
	{"pcb_tan",     rnd_act_rnd_math1, NULL, NULL},
	{"pcb_sqrt",    rnd_act_rnd_math1, NULL, NULL},
	{"pcb_srand",   rnd_act_rnd_math1, NULL, NULL},

	{"pcb_atan2",   rnd_act_rnd_math2, NULL, NULL}
};
