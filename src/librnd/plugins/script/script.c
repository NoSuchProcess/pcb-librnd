/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2020 Tibor 'Igor2' Palinkas
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
#include <librnd/rnd_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <genht/htsp.h>
#include <genht/hash.h>
#include <puplug/puplug.h>
#include <genregex/regex_se.h>

#include <librnd/core/actions.h>
#include <librnd/core/anyload.h>
#include <librnd/core/plugins.h>
#include <librnd/core/error.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/compat_fs.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/hid/hid_menu.h>
#include <librnd/core/event.h>
#include <librnd/core/globalconst.h>
#include <librnd/hid/hid_init.h>

#include "script.h"
#include "guess_lang.c"

#ifndef RND_HAVE_SYS_FUNGW
int pplg_init_fungw_fawk(void);
int pplg_uninit_fungw_fawk(void);
#endif

typedef struct {
	char *id, *fn, *lang;
	pup_plugin_t *pup;
	fgw_obj_t *obj;
} script_t;

static htsp_t scripts; /* ID->script_t */

static pup_context_t script_pup;

#include "c_script.c"
#include <librnd/core/rnd_conf.h>
#include <librnd/core/compat_fs.h>

/* dir name under dotdir for saving script persistency data */
#define SCRIPT_PERS "script_pers"

/* allow only 1 megabyte of script persistency to be loaded */
#define PERS_MAX_SIZE 1024*1024

static int script_save_preunload(script_t *s, const char *data)
{
	FILE *f;
	gds_t fn;

	if (rnd_app.dot_dir == NULL) {
		rnd_message(RND_MSG_ERROR, "script_save_preunload: can not save script persistency: the application did not configure rnd_app.dot_dir\n");
		return -1;
	}

	gds_init(&fn);
	gds_append_str(&fn, rnd_conf.rc.path.home);
	gds_append(&fn, RND_DIR_SEPARATOR_C);
	gds_append_str(&fn, rnd_app.dot_dir);
	rnd_mkdir(NULL, fn.array, 0755);

	gds_append(&fn, RND_DIR_SEPARATOR_C);
	gds_append_str(&fn, SCRIPT_PERS);
	rnd_mkdir(NULL, fn.array, 0750);

	gds_append(&fn, RND_DIR_SEPARATOR_C);
	gds_append_str(&fn, s->obj->name);

	f = rnd_fopen(NULL, fn.array, "w");
	if (f != NULL) {
		gds_uninit(&fn);
		fputs(data, f);
		fclose(f);
		return 0;
	}

	gds_uninit(&fn);
	return -1;
}

/* auto-unregister from any central infra the script may have regitered in using
   the standard script cookie */
static void script_unreg(const char *cookie)
{
	rnd_hid_menu_unload(rnd_gui, cookie);
}

/* unload a script, free all memory and remove it from all lists/hashes.
   If preunload is not NULL, it's the unload reason the script's own
   preunload() function is called and its return is saved. */
static void script_free(script_t *s, const char *preunload, const char *cookie)
{
	if ((preunload != NULL) && (s->obj != NULL)) {
		fgw_func_t *f;
		fgw_arg_t res, argv[2];

		f = fgw_func_lookup(s->obj, "preunload");
		if (f != NULL) {
			argv[0].type = FGW_FUNC;
			argv[0].val.argv0.func = f;
			argv[0].val.argv0.user_call_ctx = NULL;
			argv[1].type = FGW_STR;
			argv[1].val.cstr = preunload;
			res.type = FGW_INVALID;
			if (f->func(&res, 2, argv) == 0) {
				if ((fgw_arg_conv(&rnd_fgw, &res, FGW_STR) == 0) && (res.val.str != NULL) && (*res.val.str != '\0'))
					script_save_preunload(s, res.val.str);
			}
			fgw_arg_free(&rnd_fgw, &res);
		}
	}

	if (cookie != NULL)
		script_unreg(cookie);

	if (s->obj != NULL)
		fgw_obj_unreg(&rnd_fgw, s->obj);
#ifdef RND_HAVE_SYS_FUNGW
	if (s->pup != NULL)
		pup_unload(&script_pup, s->pup, NULL);
#endif
	free(s->id);
	free(s->fn);
	free(s);
}

static void script_unload_entry(htsp_entry_t *e, const char *preunload, const char *cookie)
{
	script_t *s = (script_t *)e->value;
	script_free(s, preunload, cookie);
	e->key = NULL;
	htsp_delentry(&scripts, e);
}

static const char *script_persistency_id = NULL;
static int script_persistency(fgw_arg_t *res, const char *cmd)
{
	char *fn;

	if (script_persistency_id == NULL) {
		rnd_message(RND_MSG_ERROR, "ScriptPersistency may be called only from the init part of a script\n");
		goto err;
	}

	if (rnd_app.dot_dir == NULL) {
		rnd_message(RND_MSG_ERROR, "ScriptPersistency: can not load script persistency: the application did not configure rnd_app.dot_dir\n");
		goto err;
	}

	fn = rnd_concat(rnd_conf.rc.path.home, RND_DIR_SEPARATOR_S, rnd_app.dot_dir, RND_DIR_SEPARATOR_S, SCRIPT_PERS, RND_DIR_SEPARATOR_S, script_persistency_id, NULL);

	if (strcmp(cmd, "remove") == 0) {
		RND_ACT_IRES(rnd_remove(NULL, fn));
		goto succ;
	}

	if (strcmp(cmd, "read") == 0) {
		FILE *f;
		long fsize = rnd_file_size(NULL, fn);
		char *data;

		if ((fsize < 0) || (fsize > PERS_MAX_SIZE))
			goto err;

		data = malloc(fsize+1);
		if (data == NULL)
			goto err;

		f = rnd_fopen(NULL, fn, "r");
		if (f == NULL) {
			free(data);
			goto err;
		}

		if (fread(data, 1, fsize, f) != fsize) {
			free(data);
			fclose(f);
			goto err;
		}

		fclose(f);
		data[fsize] = '\0';
		res->type = FGW_STR | FGW_DYN;
		res->val.str = data;
		goto succ;
	}

	rnd_message(RND_MSG_ERROR, "Unknown command for ScriptPersistency\n");

	err:;
	RND_ACT_IRES(-1);
	return 0;

	succ:
	free(fn);
	return 0; /* assume IRES is set already */
}

static char *script_gen_cookie(const char *force_id)
{
	if (force_id == NULL) {
		if (script_persistency_id == NULL) {
			rnd_message(RND_MSG_ERROR, "ScriptCookie called from outside of script init, can not generate the cookie\n");
			return NULL;
		}
		force_id = script_persistency_id;
	}
	return rnd_concat("script::fungw::", force_id, NULL);
}

int rnd_script_unload(const char *id, const char *preunload)
{
	char *cookie;
	htsp_entry_t *e = htsp_getentry(&scripts, id);
	if (e == NULL)
		return -1;

	cookie = script_gen_cookie(id);
	script_unload_entry(e, preunload, cookie);
	free(cookie);
	return 0;
}

static char *script_fn(const char *fn)
{
	if (*fn != '~')
		return rnd_strdup(fn);
	return rnd_strdup_printf("%s%c%s", rnd_conf.rc.path.home, RND_DIR_SEPARATOR_C, fn+1);
}

int rnd_script_load(rnd_design_t *hl, const char *id, const char *fn, const char *lang)
{
	pup_plugin_t *pup = NULL;
	script_t *s;
	const char *old_id;

	if (htsp_has(&scripts, id)) {
		rnd_message(RND_MSG_ERROR, "Can not load script %s from file %s: ID already in use\n", id, fn);
		return -1;
	}

	if (lang == NULL)
		lang = rnd_script_guess_lang(hl, fn, 1);
	if (lang == NULL) {
		rnd_message(RND_MSG_ERROR, "Can not load script %s from file %s: failed to guess language from file name\n", id, fn);
		return -1;
	}

	if (strcmp(lang, "c") != 0) {
#ifdef RND_HAVE_SYS_FUNGW
		const char *paths[2], *pupname;
		int st;

		rnd_script_guess_lang_init();
		pupname = rnd_script_lang2eng(&lang);

		if (pupname == NULL) {
			rnd_message(RND_MSG_ERROR, "No script engine found for language %s\n", lang);
			return -1;
		}

		old_id = script_persistency_id;
		script_persistency_id = id;
		paths[0] = FGW_CFG_PUPDIR;
		paths[1] = NULL;
		pup = pup_load(&script_pup, paths, pupname, 0, &st);
		script_persistency_id = old_id;
		if (pup == NULL) {
			rnd_message(RND_MSG_ERROR, "Can not load script engine %s for language %s\n", pupname, lang);
			return -1;
		}
#endif
	}
	else {
		lang = "rnd_cscript";
		pup = NULL;
	}

	s = calloc(1, sizeof(script_t));
	s->pup = pup;
	s->id = rnd_strdup(id);
	s->fn = script_fn(fn);
	s->lang = rnd_strdup(lang);

	old_id = script_persistency_id;
	script_persistency_id = id;
	s->obj = fgw_obj_new2(&rnd_fgw, s->id, s->lang, s->fn, NULL, hl);
	script_persistency_id = old_id;

	if (s->obj == NULL) {
		rnd_message(RND_MSG_ERROR, "Failed to parse/execute %s script from file %s (using %s)\n", id, fn, s->pup == NULL ? "unknown" : s->pup->name);
		script_free(s, NULL, NULL);
		return -1;
	}

	htsp_set(&scripts, s->id, s);
	return 0;
}

static int script_reload(rnd_design_t *hl, const char *id)
{
	int ret;
	char *fn, *lang, *cookie;
	script_t *s;
	htsp_entry_t *e = htsp_getentry(&scripts, id);

	if (e == NULL)
		return -1;

	s = e->value;
	fn = rnd_strdup(s->fn);
	lang = rnd_strdup(s->lang);

	cookie = script_gen_cookie(id);
	script_unload_entry(e, "reload", cookie);
	free(cookie);

	ret = rnd_script_load(hl, id, fn, lang);
	free(fn);
	free(lang);
	return ret;
}

void script_list(const char *pat)
{
	htsp_entry_t *e;
	re_se_t *r = NULL;

	if ((pat != NULL) && (*pat != '\0'))
		r = re_se_comp(pat);

	for(e = htsp_first(&scripts); e; e = htsp_next(&scripts, e)) {
		script_t *s = (script_t *)e->value;
		if ((r == NULL) || (re_se_exec(r, s->id)) || (re_se_exec(r, s->fn)) || (re_se_exec(r, s->lang)))
			rnd_message(RND_MSG_INFO, "id=%s fn=%s lang=%s\n", s->id, s->fn, s->lang);
	}
	
	if (r != NULL)
		re_se_free(r);
}

static void oneliner_boilerplate(FILE *f, const char *lang, int pre)
{
	if (strcmp(lang, "mawk") == 0) {
		if (pre)
			fputs("BEGIN {\n", f);
		else
			fputs("}\n", f);
	}
	else if (strcmp(lang, "fawk") == 0) {
		if (pre)
			fputs("function main(ARGS) {\n", f);
		else
			fputs("}\n", f);
	}
	else if (strcmp(lang, "fpas") == 0) {
		if (pre)
			fputs("function main(ARGS);\nbegin\n", f);
		else
			fputs("end;\n", f);
	}
	else if (strcmp(lang, "fbas") == 0) {
		if (!pre)
			fputs("\n", f);
	}
}

int script_oneliner(rnd_design_t *hl, const char *lang, const char *src)
{
	FILE *f;
	char *fn;
	int res = 0;

	fn = rnd_tempfile_name_new("oneliner");
	f = rnd_fopen(NULL, fn, "w");
	if (f == NULL) {
		rnd_tempfile_unlink(fn);
		rnd_message(RND_MSG_ERROR, "script oneliner: can't open temp file for write\n");
		return -1;
	}
	oneliner_boilerplate(f, lang, 1);
	fputs(src, f);
	fputs("\n", f);
	oneliner_boilerplate(f, lang, 0);
	fclose(f);

	if (rnd_script_load(hl, "__oneliner", fn, lang) != 0) {
		rnd_message(RND_MSG_ERROR, "script oneliner: can't load/parse the script\n");
		res = -1;
	}
	rnd_script_unload("__oneliner", NULL);

	rnd_tempfile_unlink(fn);
	return res;
}

#include "timer.c"
#include "perma.c"
#include "script_act.c"

char *script_cookie = "script plugin";

static rnd_anyload_t script_anyload = {0};


static int script_anyload_file(const rnd_anyload_t *al, rnd_design_t *hl, const char *filename, const char *type, lht_node_t *nd)
{
	lht_node_t *nlang, *nid;
	const char *lang, *id;

	rnd_trace("script anyload!\n");
	assert(nd->type == LHT_HASH);
	nlang = lht_dom_hash_get(nd, "lang");
	if ((nlang == NULL) || (nlang->type != LHT_TEXT)) {
		rnd_message(RND_MSG_ERROR, "anyload user_script: missing or non-text lang node\n");
		return -1;
	}
	lang = nlang->data.text.value;

	nid = lht_dom_hash_get(nd, "id");
	if ((nid == NULL) || (nid->type != LHT_TEXT)) {
		rnd_message(RND_MSG_ERROR, "anyload user_script: missing or non-text id node\n");
		return -1;
	}
	id = nid->data.text.value;

	return rnd_script_load(hl, id, filename, lang);
}

int pplg_check_ver_script(int ver_needed) { return 0; }

void pplg_uninit_script(void)
{
	htsp_entry_t *e;

	rnd_anyload_unreg_by_cookie(script_cookie);
	rnd_live_script_uninit();
	rnd_remove_actions_by_cookie(script_cookie);
	for(e = htsp_first(&scripts); e; e = htsp_next(&scripts, e)) {
		script_t *script = e->value;
		char *cookie = script_gen_cookie(script->id);
		script_unload_entry(e, "unload", cookie);
		free(cookie);
	}

	htsp_uninit(&scripts);
	pup_uninit(&script_pup);

#ifndef RND_HAVE_SYS_FUNGW
	pplg_uninit_fungw_fawk();
#endif

	rnd_event_unbind_allcookie(script_cookie);

	rnd_script_guess_lang_uninit();
}

int pplg_init_script(void)
{
	RND_API_CHK_VER;
	RND_REGISTER_ACTIONS(script_action_list, script_cookie);

#ifndef RND_HAVE_SYS_FUNGW
	pplg_init_fungw_fawk();
#endif

	rnd_c_script_init();
	htsp_init(&scripts, strhash, strkeyeq);
	pup_init(&script_pup);
	rnd_live_script_init();
	if (rnd_hid_in_main_loop)
		perma_script_init(NULL); /* warning: no hidlib available */
	else
		rnd_event_bind(RND_EVENT_MAINLOOP_CHANGE, script_mainloop_perma_ev, NULL, script_cookie);
	rnd_event_bind(RND_EVENT_GUI_INIT, script_timer_gui_init_ev, NULL, script_cookie);


	script_anyload.load_file = script_anyload_file;
	script_anyload.cookie = script_cookie;
	rnd_anyload_reg("^user_script$", &script_anyload);

	return 0;
}
