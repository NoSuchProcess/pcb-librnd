/*
 *                            COPYRIGHT
 *
 *  librnd, modular 2D CAD framework
 *  (file imported from: pcb-rnd, interactive printed circuit board design)
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
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
 *    Project page: http://repo.hu/projects/librnd
 *    lead developer: http://repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <librnd/rnd_config.h>

#include <genlist/gendlist.h>
#include <genregex/regex_se.h>

#include <librnd/core/actions.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/compat_lrealpath.h>
#include <librnd/core/error.h>
#include <librnd/core/conf.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/safe_fs_dir.h>
#include <librnd/core/hidlib.h>
#include <librnd/core/event.h>
#include <librnd/core/paths.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/rnd_conf.h>

#include "anyload.h"

static const char anyload_cookie[] = "core/anyload";

typedef struct {
	const rnd_anyload_t *al;
	re_se_t *rx;
	const char *rx_str;
	gdl_elem_t link;
} rnd_aload_t;

static gdl_list_t anyloads;


int rnd_anyload_reg(const char *root_regex, const rnd_anyload_t *al_in)
{
	re_se_t *rx;
	rnd_aload_t *al;

	rx = re_se_comp(root_regex);
	if (rx == NULL) {
		rnd_message(RND_MSG_ERROR, "rnd_anyload_reg: failed to compile regex '%s' for '%s'\n", root_regex, al_in->cookie);
		return -1;
	}

	al = calloc(sizeof(rnd_aload_t), 1);
	al->al = al_in;
	al->rx = rx;
	al->rx_str = root_regex;

	gdl_append(&anyloads, al, link);

	return 0;
}

static void rnd_anyload_free(rnd_aload_t *al)
{
	gdl_remove(&anyloads, al, link);
	re_se_free(al->rx);
	free(al);
}

void rnd_anyload_unreg_by_cookie(const char *cookie)
{
	rnd_aload_t *al, *next;
	for(al = gdl_first(&anyloads); al != NULL; al = next) {
		next = al->link.next;
		if (al->al->cookie == cookie)
			rnd_anyload_free(al);
	}
}

void rnd_anyload_uninit(void)
{
	rnd_aload_t *al;
	while((al = gdl_first(&anyloads)) != NULL) {
		rnd_message(RND_MSG_ERROR, "rnd_anyload: '%s' left anyloader regs in\n", al->al->cookie);
		rnd_anyload_free(al);
	}
	rnd_event_unbind_allcookie(anyload_cookie);
}

static lht_doc_t *load_lht(rnd_design_t *hidlib, const char *path)
{
	lht_doc_t *doc;
	FILE *f = rnd_fopen(hidlib, path, "r");
	char *errmsg;

	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "anyload: can't open %s for read\n", path);
		return NULL;
	}

	doc = lht_dom_load_stream(f, path, &errmsg);
	fclose(f);
	if (doc == NULL) {
		rnd_message(RND_MSG_ERROR, "anyload: can't load %s: %s\n", path, errmsg);
		return NULL;
	}
	return doc;
}


/* call a loader to load a subtree; retruns non-zero on error */
int rnd_anyload_parse_subtree(rnd_design_t *hidlib, lht_node_t *subtree)
{
	rnd_aload_t *al;

	for(al = gdl_first(&anyloads); al != NULL; al = al->link.next)
		if (re_se_exec(al->rx, subtree->name))
			return al->al->load_subtree(al->al, hidlib, subtree);

	rnd_message(RND_MSG_ERROR, "anyload: root node '%s' is not recognized by any loader\n", subtree->name);
	return -1;
}

int rnd_anyload_ext_file(rnd_design_t *hidlib, const char *path, const char *type, lht_node_t *nd, const char *real_cwd, int real_cwd_len)
{
	rnd_aload_t *al;
	char *fpath, *tmp;

	/* relative path needs to be relative to real_cwd */
	tmp = rnd_concat(real_cwd, RND_DIR_SEPARATOR_S, path, NULL);
	fpath = rnd_lrealpath(tmp);
	free(tmp);

	if (fpath == NULL) {
		rnd_message(RND_MSG_ERROR, "anyload: external file '%s' not found\n", path);
		return -1;
	}
	if ((memcmp(fpath, real_cwd, real_cwd_len) != 0) || (fpath[real_cwd_len] != '/')) {
		rnd_message(RND_MSG_ERROR, "anyload: external file '%s' (really '%s') not within directory tree '%s' (or the file does not exist)\n", path, fpath, real_cwd);
		return -1;
	}

/*	rnd_trace("ext file: '%s' '%s' PATHS: '%s' in '%s'\n", path, type, fpath, real_cwd);*/

	if (type == NULL) /* must be a lihata file */
		return rnd_anyload(hidlib, fpath);

	/* we have an explicit type, look for a plugin to handle that */
	for(al = gdl_first(&anyloads); al != NULL; al = al->link.next) {
		if (re_se_exec(al->rx, type)) {
			if (al->al->load_file != NULL)
				return al->al->load_file(al->al, hidlib, fpath, type, nd);
			if (al->al->load_subtree != NULL) {
				int res;
				lht_doc_t *doc = load_lht(hidlib, fpath);
				if (doc == NULL) {
					rnd_message(RND_MSG_ERROR, "anyload: '%s' (really '%s') with type '%s' should be a lihata document but fails to parse\n", path, fpath, type);
					return -1;
				}
				res = al->al->load_subtree(al->al, hidlib, doc->root);
				lht_dom_uninit(doc);
				return res;
			}
		}
	}

	rnd_message(RND_MSG_ERROR, "anyload: type '%s' is not recognized by any loader\n", type);
	return -1;
}

/* parse an anyload file, load all roots; retruns non-zero on any error */
int rnd_anyload_parse_anyload_v1(rnd_design_t *hidlib, lht_node_t *root, const char *cwd)
{
	lht_node_t *rsc, *n;
	int res = 0, real_cwd_len = 0;
	char *real_cwd = NULL;

	if (root->type != LHT_HASH) {
		rnd_message(RND_MSG_ERROR, "anyload: %s-anyload-v* root node must be a hash\n", rnd_app.package);
		return -1;
	}

	rsc = lht_dom_hash_get(root, "resources");
	if (rsc == NULL) {
		rnd_message(RND_MSG_WARNING, "anyload: %s-anyload-v* without li:resources node - nothing loaded\n(this is probably not what you wanted)\n", rnd_app.package);
		return -1;
	}
	if (rsc->type != LHT_LIST) {
		rnd_message(RND_MSG_WARNING, "anyload: %s-anyload-v*/resources must be a list\n", rnd_app.package);
		return -1;
	}

	for(n = rsc->data.list.first; n != NULL; n = n->next) {
		int r;
		if ((n->type == LHT_HASH) && (strcmp(n->name, "file") == 0)) {
			lht_node_t *npath, *ntype;
			const char *path = NULL, *type = NULL;

			npath = lht_dom_hash_get(n, "path");
			ntype = lht_dom_hash_get(n, "type");
			if (npath != NULL) {
				if (npath->type != LHT_TEXT) {
					rnd_message(RND_MSG_WARNING, "anyload: file path needs to be a text node\n");
					res = -1;
					goto error;
				}
				path = npath->data.text.value;
			}
			if (ntype != NULL) {
				if (ntype->type != LHT_TEXT) {
					rnd_message(RND_MSG_WARNING, "anyload: file type needs to be a text node\n");
					res = -1;
					goto error;
				}
				type = ntype->data.text.value;
			}
			if (real_cwd == NULL) {
				real_cwd = rnd_lrealpath(cwd);
				if (real_cwd == NULL) {
					rnd_message(RND_MSG_WARNING, "anyload: realpath: no such path '%s'\n", cwd);
					res = -1;
					goto error;
				}
				real_cwd_len = strlen(real_cwd);
			}
			if (path == NULL) {
					rnd_message(RND_MSG_WARNING, "anyload: file without path\n");
					res = -1;
					goto error;
			}
			r = rnd_anyload_ext_file(hidlib, path, type, n, real_cwd, real_cwd_len);
		}
		else
		 r = rnd_anyload_parse_subtree(hidlib, n);

		if (r != 0)
			res = r;
	}

	error:;
	free(real_cwd);
	return res;
}

/* parse the root of a random file, which will either be an anyload pack or
   a single root one of our backends can handle; retruns non-zero on error */
int rnd_anyload_parse_root(rnd_design_t *hidlib, lht_node_t *root, const char *cwd)
{
	static int applen = -1;
	if (applen < 0)
		applen = strlen(rnd_app.package);
	if ((strncmp(root->name, rnd_app.package, applen) == 0) && (strcmp(root->name + applen, "-anyload-v1") == 0))
		return rnd_anyload_parse_anyload_v1(hidlib, root, cwd);
	return rnd_anyload_parse_subtree(hidlib, root);
}



int rnd_anyload_conf_needs_update = 0;
static int al_conf_inhibit;
static void anyload_conf_update(void)
{
	if (al_conf_inhibit || !rnd_anyload_conf_needs_update)
		return;

	rnd_anyload_conf_needs_update = 0;
	rnd_conf_merge_all(NULL);
	rnd_conf_update(NULL, -1);
}

static void anyload_conf_inhibit_inc(void)
{
	al_conf_inhibit++;
}

static void anyload_conf_inhibit_dec(void)
{
	al_conf_inhibit--;
	if (al_conf_inhibit == 0)
		anyload_conf_update();
}

int rnd_anyload(rnd_design_t *hidlib, const char *path)
{
	char *path_free = NULL, *cwd_free = NULL;
	const char *cwd;
	int res = -1, req_anyload = 0;
	lht_doc_t *doc;

	if (rnd_is_dir(hidlib, path)) {
		cwd = path;
		path = path_free = rnd_concat(cwd, RND_DIR_SEPARATOR_S, "anyload.lht", NULL);
		req_anyload = 1;
	}
	else {
		const char *s, *sep = NULL;

		for(s = path; *s != '\0'; s++)
			if ((*s == RND_DIR_SEPARATOR_C) || (*s == '/'))
				sep = s;

		if (sep != NULL)
			cwd = cwd_free = rnd_strndup(path, sep-path);
		else
			cwd = ".";
	}

	doc = load_lht(hidlib, path);
	if (doc != NULL) {
		if (req_anyload)
			res = rnd_anyload_parse_anyload_v1(hidlib, doc->root, cwd);
		else
			res = rnd_anyload_parse_root(hidlib, doc->root, cwd);
		lht_dom_uninit(doc);
	}

	anyload_conf_update();

	free(path_free);
	free(cwd_free);
	return res;
}

static void anyload_persistent_load_dir(rnd_design_t *hidlib, const char *path, int silent_fail)
{
	DIR *d = rnd_opendir(hidlib, path);
	struct dirent *de;
	gds_t fullp = {0};
	long base;

	if (d == NULL) {
		if (!silent_fail)
			rnd_message(RND_MSG_ERROR, "anyload persist: unable list content of dir '%s'\n", path);
		return;
	}

	gds_append_str(&fullp, path);
	gds_append(&fullp, RND_DIR_SEPARATOR_C);
	base = fullp.used;

	while((de = rnd_readdir(d)) != NULL) {
		if (de->d_name[0] == '.')
			continue;
		fullp.used = base;
		gds_append_str(&fullp, de->d_name);
		if (rnd_anyload(hidlib, fullp.array) != 0)
			rnd_message(RND_MSG_ERROR, "anyload persist: failed to load '%s' in '%s'\n", de->d_name, path);
	}

	gds_uninit(&fullp);
	rnd_closedir(d);
}

static void anyload_persistent_init(rnd_design_t *hidlib)
{
	rnd_conf_listitem_t *ci;

	anyload_conf_inhibit_inc();

	for(ci = rnd_conflist_first((rnd_conflist_t *)&rnd_conf.rc.anyload_persist); ci != NULL; ci = rnd_conflist_next(ci)) {
		const char *p = ci->val.string[0];
		char *p_exp;
		int silent_fail = 0;

		/* make ? prefixed paths optional */
		if (*p == '?') {
			silent_fail = 1;
			p++;
		}

		p_exp = rnd_build_fn(hidlib, p);
		if (p_exp == NULL) {
			if (!silent_fail)
				rnd_message(RND_MSG_ERROR, "anyload persist: unable to resolve path '%s'\n", p);
			continue;
		}

/*rnd_trace(" path='%s' -> '%s'\n", p, p_exp);*/
		if (!rnd_is_dir(hidlib, p_exp)) {
			if (!silent_fail)
				rnd_message(RND_MSG_ERROR, "anyload persist: '%s' (really '%s') is not a directory\n", p, p_exp);
			continue;
		}

		anyload_persistent_load_dir(hidlib, p_exp, silent_fail);
	}

	anyload_conf_inhibit_dec();
}

static void anyload_mainloop_perma_ev(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if (rnd_hid_in_main_loop)
		anyload_persistent_init(hidlib);
}

void rnd_anyload_init2(void)
{
	if (rnd_hid_in_main_loop)
		anyload_persistent_init(NULL);
	else
		rnd_event_bind(RND_EVENT_MAINLOOP_CHANGE, anyload_mainloop_perma_ev, NULL, anyload_cookie);
}

