/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Central, DAD based file selection dialog implementation */

#include "config.h"

/* must be even */
#define FSD_MAX_DIRS 16

#define FSD_MAX_DIRNAME_LEN dialogs_conf.plugins.lib_hid_common.fsd.dirname_maxlen
#define FSD_RECENT_MAX_LINES dialogs_conf.plugins.lib_hid_common.fsd.recent_maxlines

#include <limits.h>
#include <genht/hash.h>
#include <genvector/vti0.h>
#include <genregex/regex_se.h>
#include <librnd/core/error.h>
#include <librnd/hid/hid_dad.h>
#include <librnd/hid/hid_dad_tree.h>
#include <librnd/core/globalconst.h>
#include <librnd/core/compat_fs.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/safe_fs_dir.h>
#include <librnd/core/event.h>
#include <librnd/core/hidlib.h>
#include <librnd/core/hidlib_conf.h>

#include "dialogs_conf.h"
#include "dlg_fileselect.h"
#include "xpm.h"
#include "lib_hid_common.h"


typedef struct {
	char *name;
	unsigned is_dir:1;
	unsigned vis:1;         /* if 0 entry is invisible due to filter */
	size_t size;
	double mtime;
} fsd_dirent_t;

#define GVT(x) vtde_ ## x
#define GVT_ELEM_TYPE fsd_dirent_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 4096
#define GVT_START_SIZE 128
#define GVT_FUNC
#define GVT_SET_NEW_BYTES_TO 0
#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
#include <genvector/genvector_impl.c>
#include <genvector/genvector_undef.h>

typedef enum fsd_sort_e     { SORT_FN, SORT_SIZE, SORT_MTIME } fsd_sort_t;
const char *sort_names[] =  { "name",  "size",    "mod. time", NULL } ;

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	int active;
	int wpath, wdir[FSD_MAX_DIRS], wdirlong, wshcut, wshdel, wfilelist;
	int wsort, wsort_rev, wsort_dirgrp, wsort_icase;
	int wflt;
	char cwd_buf[RND_PATH_MAX];
	char *cwd;
	int cwd_offs[FSD_MAX_DIRS]; /* string lengths for each dir button within ->cwd */
	vtde_t des;
	rnd_design_t *hidlib;
	rnd_hid_fsd_flags_t flags;
	const rnd_hid_fsd_filter_t *flt;
	const char *history_tag;
	char *res_path;
} fsd_ctx_t;

static fsd_ctx_t fsd_ctx;


TODO("Maybe remove this")
static void fsd_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
#if 0
	fsd_ctx_t *fsd = caller_data;
	fsd->active = 0;
#endif
}

#include "dlg_fileselect_io.c"

/* Returns 1 if ctx->res_path is acceptable; if not acceptable, free's it
   and sets it to NULL and if report is non-zero writes an error message in
   the log */
static int fsd_acceptable(fsd_ctx_t *ctx, int report)
{
	if (ctx->flags & RND_HID_FSD_READ) {
		if (!rnd_file_readable(ctx->hidlib, ctx->res_path)) {
			if (report) rnd_message(RND_MSG_ERROR, "File '%s' does not exist or is not a file or is not readable\n", ctx->res_path);
			goto err;
		}
		return 1;
	}

	return 1;

	err:;
	free(ctx->res_path);
	ctx->res_path = NULL;
	return 0;
}

/*** file listing ***/
static void fsd_clear(fsd_ctx_t *ctx)
{
	long n;
	for(n = 0; n < ctx->des.used; n++)
		free(ctx->des.array[n].name);
	ctx->des.used = 0;
}


/* Fill in the file listing */
static void fsd_list(fsd_ctx_t *ctx)
{
	DIR *dir;
	struct dirent *de;
	gds_t fullp = {0};
	long fullp_len;

	dir = rnd_opendir(ctx->hidlib, ctx->cwd);
	if (dir == NULL)
		return;

	gds_append_str(&fullp, ctx->cwd);
	gds_append(&fullp, '/');
	fullp_len = fullp.used;
	for(de = rnd_readdir(dir); de != NULL; de = rnd_readdir(dir)) {
		fsd_dirent_t *new_de;
		int is_dir;
		long size;
		double mtime;

		if ((de->d_name[0] == '.') && (de->d_name[1] == '\0'))
			continue;

		fullp.used = fullp_len;
		gds_append_str(&fullp, de->d_name);
		if (rnd_file_stat(ctx->hidlib, fullp.array, &is_dir, &size, &mtime) != 0)
			continue;

		new_de = vtde_alloc_append(&ctx->des, 1);
		new_de->name = rnd_strdup(de->d_name);
		new_de->is_dir = is_dir;
		new_de->size   = size;
		new_de->mtime  = mtime;
	}
	rnd_closedir(dir);
}


/* This is not really reentrant but we are single threaded and in a modal dialog */
static fsd_sort_t cmp_sort;
static int cmp_sort_rev, cmp_sort_dirgrp, cmp_sort_icase;
#define rev_order(order) (cmp_sort_rev ? -(order) : (order))

static int fsd_sort_cmp(const void *a_, const void *b_)
{
	const fsd_dirent_t *a = a_, *b = b_;
	int order;

	if (cmp_sort_dirgrp) {
		if (a->is_dir && !b->is_dir) return rev_order(-1);
		if (!a->is_dir && b->is_dir) return rev_order(+1);
	}

	/* entries within the same group */
	switch(cmp_sort) {
		case SORT_SIZE:
			if (a->size == b->size) goto by_name;
			order = (a->size > b->size) ? +1 : -1;
			break;
		case SORT_MTIME:
			if (a->mtime == b->mtime) goto by_name;
			order = (a->mtime > b->mtime) ? +1 : -1;
			break;
		case SORT_FN:
			by_name:;
			order = cmp_sort_icase ? rnd_strcasecmp(a->name, b->name) : strcmp(a->name, b->name);
			break;
	}
	

	return rev_order(order);
}

static void fsd_sort(fsd_ctx_t *ctx)
{
	cmp_sort = ctx->dlg[ctx->wsort].val.lng;
	cmp_sort_rev = ctx->dlg[ctx->wsort_rev].val.lng;
	cmp_sort_dirgrp = ctx->dlg[ctx->wsort_dirgrp].val.lng;
	cmp_sort_icase = ctx->dlg[ctx->wsort_icase].val.lng;
	qsort(ctx->des.array, ctx->des.used, sizeof(fsd_dirent_t), fsd_sort_cmp);

	if (ctx->flt != NULL) {
		long n, pidx = ctx->dlg[ctx->wflt].val.lng;
		const char **p, *cs;
		gds_t tmp = {0};

		for(n = 0; n < ctx->des.used; n++)
			ctx->des.array[n].vis = ctx->des.array[n].is_dir;

		for(p = ctx->flt[pidx].pat; *p != NULL; p++) {
			re_se_t *rx;

			/* build a regex in tmp and rx */
			tmp.used = 0;
			gds_append(&tmp, '^');
			for(cs = *p; *cs != '\0'; cs++) {
				switch(*cs) {
					case '*':
						gds_append(&tmp, '.');
						gds_append(&tmp, '*');
						break;
					case '.':
						gds_append(&tmp, '[');
						gds_append(&tmp, *cs);
						gds_append(&tmp, ']');
						break;
					default:
						gds_append(&tmp, *cs);
				}
			}
			gds_append(&tmp, '$');
			rx = re_se_comp(tmp.array);

			/* check if any invisible entry becomes visible */
			/*printf("filter for: '%s' as '%s' (%p)\n", *p, tmp.array, rx);*/
			for(n = 0; n < ctx->des.used; n++)
				if (!ctx->des.array[n].vis && re_se_exec(rx, ctx->des.array[n].name))
					ctx->des.array[n].vis = 1;
			re_se_free(rx);
		}
		gds_uninit(&tmp);
	}
	else {
		long n;
		for(n = 0; n < ctx->des.used; n++)
			ctx->des.array[n].vis = 1;
	}
}

static void fsd_load(fsd_ctx_t *ctx, int reset_scroll)
{
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wfilelist];
	rnd_hid_tree_t *tree = attr->wdata;
	char *cell[4];
	const char *first_path = NULL;
	long n;

	rnd_dad_tree_clear(tree);

	for(n = 0; n < ctx->des.used; n++) {
		char ssize[64], smtime[64];
		struct tm *tm;
		time_t t;
		rnd_hid_row_t *r;

/*		printf("list: %s dir=%d %ld %f\n", ctx->des.array[n].name, ctx->des.array[n].is_dir, ctx->des.array[n].size, ctx->des.array[n].mtime);*/

		if (!ctx->des.array[n].vis)
			continue;

		if (ctx->des.array[n].is_dir)
			strcpy(ssize, "<dir>");
		else
			sprintf(ssize, "%ld", ctx->des.array[n].size);

		t = ctx->des.array[n].mtime;
		tm = localtime(&t);
		strftime(smtime, sizeof(smtime), "%Y-%m-%d", tm);

		cell[0] = rnd_strdup(ctx->des.array[n].name);
		cell[1] = rnd_strdup(ssize);
		cell[2] = rnd_strdup(smtime);
		cell[3] = NULL;
		r = rnd_dad_tree_append(attr, NULL, cell);
		if (first_path == NULL)
			first_path = r->path;
	}

	if (reset_scroll && (first_path != NULL)) {
		rnd_hid_attr_val_t hv;
		hv.str = first_path;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wfilelist, &hv);
		hv.str = NULL;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wfilelist, &hv);
	}
}

/* Load the value and help and aux arrays of a dir button from the path array */
#define load_dir_btn(path_idx, btn_idx) \
	do { \
		char *dn = (char *)path.array[path_idx]; \
		int len = strlen(dn); \
		if ((len > FSD_MAX_DIRNAME_LEN) && (len > 4)) { \
			rnd_gui->attr_dlg_set_help(ctx->dlg_hid_ctx, ctx->wdir[btn_idx], dn); \
			strcpy(dn + FSD_MAX_DIRNAME_LEN - 3, "..."); \
		} \
		hv.str = dn; \
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wdir[btn_idx], &hv); \
		rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wdir[btn_idx], 0); \
		ctx->cwd_offs[btn_idx] = offs.array[path_idx]; \
	} while(0)


/* Change to directory, relative to ctx->cwd. If rel is NULL, just cd to
   ctx->cwd. */
static void fsd_cd(fsd_ctx_t *ctx, const char *rel)
{
	vtp0_t path = {0};
	vti0_t offs = {0};
	char *s, *next, tmp[RND_PATH_MAX];
	int n, m, end;
	rnd_hid_attr_val_t hv;

	if (rel != NULL) {
		if ((rel[0] == '.') && (rel[1] == '.') && (rel[2] == '\0')) {
			char *sep = fsd_io_rsep(ctx->cwd);
			if (sep != NULL) {
				if (sep > ctx->cwd)
					*sep = '\0';
				else
					sep[1] = '\0'; /* going back to root */
			}
			else
				return; /* already at root */
		}
		else {
			char *new_cwd, *end, *sep = "/";
			DIR *dir;

			/* append relative, with / inserted only if needed */
			end = ctx->cwd + strlen(ctx->cwd) - 1;
#ifdef __WIN32__
			if ((*end == '/') || (*end == '\\')) sep = "";
#else
			if (*end == '/') sep = "";
#endif
			new_cwd = rnd_concat(ctx->cwd, sep, rel, NULL);

			/* check if new path is a dir */
			dir = rnd_opendir(ctx->hidlib, ctx->cwd);
			if (dir != NULL) {
				free(ctx->cwd);
				ctx->cwd = new_cwd;
			}
			else {
				rnd_message(RND_MSG_ERROR, "Can't read directory '%s'\n", new_cwd);
				free(new_cwd);
				return;
			}
		}
	}

	s = tmp;
	strcpy(tmp, ctx->cwd);

/* Append root */
#ifdef __WIN32__
	if (s[1] == ':') {
		vtp0_append(&path, s);
		s[2] = '\0';
		s += 3;
		vti0_append(&offs, 3);
	}
#else
	vtp0_append(&path, "/");
	vti0_append(&offs, 1);
	if (*s == '/') s++;
#endif

	if (*s == '\0') s = NULL;

	for(; s != NULL; s = next) {
#		ifdef __WIN32__
			while((*s == '/') || (*s == '\\')) s++;
			next = strpbrk(s, "/\\");
#		else
			while(*s == '/') s++;
			next = strchr(s, '/');
#		endif
		if (next != NULL) {
			*next = '\0';
			end = next - tmp;
			next++;
			if (*next == '\0')
				next = NULL;
		}
		else
			end = (s - tmp) + strlen(s);

		vtp0_append(&path, s);
		vti0_append(&offs, end);
	}


	if (path.used > FSD_MAX_DIRS) {
		/* path too long - split the path in 2 parts and enable "..." in the middle */
		for(n = 0; n < FSD_MAX_DIRS/2; n++)
			load_dir_btn(n, n);
		m = n;
		for(n = path.used - FSD_MAX_DIRS/2; n < path.used; n++,m++)
			load_dir_btn(n, m);
		rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wdirlong, 0);
	}
	else {
		/* path short enough for hiding "..." and displaying all */
		for(n = 0; n < path.used; n++)
			load_dir_btn(n, n);
		for(; n < FSD_MAX_DIRS; n++) {
			rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wdir[n], 1);
		}
		rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wdirlong, 1);
	}

	fsd_clear(ctx);
	fsd_list(ctx);
	fsd_sort(ctx);
	fsd_load(ctx, 1);
}

#undef load_dir_btn

static void cd_button_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	fsd_ctx_t *ctx = caller_data;
	int idx, w = attr - ctx->dlg;

	for(idx = 0; idx < FSD_MAX_DIRS; idx++)
		if (w == ctx->wdir[idx])
			break;

	if (idx == FSD_MAX_DIRS)
		return; /* button not found - function called from where? */

	/* truncate cwd and cd there */
	ctx->cwd[ctx->cwd_offs[idx]] = '\0';
	fsd_cd(ctx, NULL);
}

static void resort_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	fsd_ctx_t *ctx = caller_data;
	fsd_sort(ctx);
	fsd_load(ctx, 0);
}

/* Handle new text entered in the path field */
static void edit_enter_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	fsd_ctx_t *ctx = caller_data;
	static rnd_dad_retovr_t retovr;
	const char *fn = attr->val.str;
	rnd_hid_attr_val_t hv;

	if ((fn == NULL) || (*fn == '\0'))
		return;

	if (!rnd_is_path_abs(fn)) {
		ctx->res_path = rnd_concat(ctx->cwd, "/", fn, NULL);
		if (rnd_is_dir(ctx->hidlib, ctx->res_path)) { /* relative dir */
			long len = strlen(ctx->res_path);
			if (len < RND_PATH_MAX) {
				free(ctx->res_path);
				ctx->res_path = NULL;
				fsd_cd(ctx, fn); /* do a relative cd so .. works */
				goto clear;
			}
			goto err_too_long;
		}

		/* relative file already built in ctx->res_path */
		if (fsd_acceptable(ctx, 1))
			rnd_hid_dad_close(hid_ctx, &retovr, 0);
	}
	else { /* absolute path */
		long len = strlen(fn);
		if (len >= RND_PATH_MAX)
			goto err_too_long;

		if (rnd_is_dir(ctx->hidlib, fn)) {
			/* absolute dir */
			memcpy(ctx->cwd, fn, len+1);
			fsd_cd(ctx, NULL);
			goto clear;
		}

		/* absolute file */
		ctx->res_path = rnd_strdup(fn);
		if (fsd_acceptable(ctx, 1))
			rnd_hid_dad_close(hid_ctx, &retovr, 0);
	}
	return;

	err_too_long:;
	rnd_message(RND_MSG_ERROR, "Path too long.\n");
	return;

	clear:;
	hv.str = "";
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wpath, &hv);
}

TODO("closing from within a tree table causes a gtk/glib segf")
static void timed_close_cb(rnd_hidval_t user_data)
{
	static rnd_dad_retovr_t retovr;
	rnd_hid_dad_close(user_data.ptr, &retovr, 0);
}


static void fsd_filelist_cb(rnd_hid_attribute_t *attr, void *hid_ctx, rnd_hid_row_t *row)
{
	rnd_hid_tree_t *tree = attr->wdata;
	fsd_ctx_t *ctx = tree->user_ctx;

	if (row != NULL) { /* first click on a new row */
		if (row->cell[1][0] != '<') { /* file: load the edit line with the new file name */
			rnd_hid_attr_val_t hv;
			hv.str = (char *)row->cell[0];
			rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wpath, &hv);
		}
	}
}

TODO("We shouldn't need a timer for close (fix this in DAD)")
static void fsd_filelist_enter_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	fsd_ctx_t *ctx = caller_data;
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(attr);

	if (row == NULL)
		return;

	if (row->cell[1][0] != '<') {
		rnd_hidval_t rv;
		rv.ptr = hid_ctx;
		ctx->res_path = rnd_concat(ctx->cwd, "/", row->cell[0], NULL);
		if (fsd_acceptable(ctx, 1))
			rnd_gui->add_timer(rnd_gui, timed_close_cb, 1, rv);
	}
	else
		fsd_cd(ctx, row->cell[0]);
}


static void fsd_ok_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_IGNORED)
{
	fsd_ctx_t *ctx = caller_data;
	rnd_hid_attribute_t *inp = &ctx->dlg[ctx->wpath];
	const char *fn = inp->val.str;

	if ((fn != NULL) && (*fn != '\0')) {
		static rnd_dad_retovr_t retovr;
		ctx->res_path = rnd_concat(ctx->cwd, "/", fn, NULL);
		if (fsd_acceptable(ctx, 1))
			rnd_hid_dad_close(hid_ctx, &retovr, 0);
	}
}

/*** shortcut ***/

static void fsd_shcut_load(fsd_ctx_t *ctx)
{
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wshcut];
	rnd_hid_tree_t *tree = attr->wdata;
	char *cell[2];
	rnd_hid_row_t *rparent;
	gds_t path = {0}, gpath = {0};

	rnd_dad_tree_clear(tree);

	cell[1] = NULL;

	/* filesystem */
	cell[0] = rnd_strdup("filesystem");
	rparent = rnd_dad_tree_append(attr, NULL, cell);

	cell[0] = rnd_strdup("/"); rnd_dad_tree_append_under(attr, rparent, cell);
	if (rnd_conf.rc.path.home != NULL) {
		cell[0] = rnd_strdup(rnd_conf.rc.path.home); rnd_dad_tree_append_under(attr, rparent, cell);
	}
	cell[0] = rnd_strdup("/tmp"); rnd_dad_tree_append_under(attr, rparent, cell);

	rnd_dad_tree_expcoll_(tree, rparent, 1, 0);


	if (fsd_shcut_path_setup(ctx, &path, 1, 0) != 0)
		return;
	if (fsd_shcut_path_setup(ctx, &gpath, 0, 0) != 0)
		return;


	cell[0] = rnd_strdup("favorites (global)");
	rparent = rnd_dad_tree_append(attr, NULL, cell);
	fsd_shcut_load_file(ctx, attr, rparent, &gpath, "Fav.lst");
	rnd_dad_tree_expcoll_(tree, rparent, 1, 0);

	cell[0] = rnd_strdup("favorites (local)");
	rparent = rnd_dad_tree_append(attr, NULL, cell);
	fsd_shcut_load_file(ctx, attr, rparent, &path, ".fav.lst");
	rnd_dad_tree_expcoll_(tree, rparent, 1, 0);

	cell[0] = rnd_strdup("recent");
	rparent = rnd_dad_tree_append(attr, NULL, cell);
	fsd_shcut_load_file(ctx, attr, rparent, &path, ".recent.lst");
	rnd_dad_tree_expcoll_(tree, rparent, 1, 0);

	gds_uninit(&path);
	gds_uninit(&gpath);
}

static const char *fsd_shc_sparent(fsd_ctx_t *ctx)
{
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wshcut];
	rnd_hid_tree_t *tree = attr->wdata;
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(attr), *rparent;

	if (row == NULL)
		return NULL;

	rparent = rnd_dad_tree_parent_row(tree, row);
	return (rparent == NULL) ? row->cell[0] : rparent->cell[0];
}

static void fsd_shc_add_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	fsd_ctx_t *ctx = caller_data;
	int app_succ;
	const char *sparent = fsd_shc_sparent(ctx);

	if ((sparent != NULL) && (strcmp(sparent, "favorites (local)") == 0))
		app_succ = fsd_shcut_append_to_file(ctx, 1, ".fav.lst", ctx->cwd, 0);
	else
		app_succ = fsd_shcut_append_to_file(ctx, 0, "Fav.lst", ctx->cwd, 0);

	if (app_succ)
		fsd_shcut_load(ctx);
}

static void fsd_shc_del_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_IGNORE)
{
	fsd_ctx_t *ctx = caller_data;
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wshcut];
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(attr);
	const char *sparent;
	int del_succ = 0;

	if (row == NULL)
		return;

	sparent = fsd_shc_sparent(ctx);
	if (sparent == NULL)
		return;

	if (strcmp(sparent, "favorites (local)") == 0)
		del_succ = fsd_shcut_del_from_file(ctx, 1, ".fav.lst", row->cell[0]);
	else if (strcmp(sparent, "favorites (global)") == 0)
		del_succ = fsd_shcut_del_from_file(ctx, 0, "Fav.lst", row->cell[0]);
	else if (strcmp(sparent, "recent") == 0)
		del_succ = fsd_shcut_del_from_file(ctx, 1, ".recent.lst", row->cell[0]);
	else
		rnd_message(RND_MSG_ERROR, "Can not delete from subtree %s\n", sparent);

	if (del_succ)
		fsd_shcut_load(ctx);

}

static void fsd_shcut_enter_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_hid_tree_t *tree = attr->wdata;
	fsd_ctx_t *ctx = caller_data;
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(attr);

	/* deal with double clicks */
	if (row != NULL) {
		rnd_hid_row_t *rparent = rnd_dad_tree_parent_row(tree, row);
		if (rparent != NULL) {
			if (rnd_is_dir(ctx->hidlib, row->cell[0])) {
				free(ctx->cwd);
				ctx->cwd = rnd_strdup(row->cell[0]);
				fsd_cd(ctx, NULL);
			}
			else {
				rnd_hidval_t rv;
				rv.ptr = hid_ctx;
				ctx->res_path = rnd_strdup(row->cell[0]);
				if (fsd_acceptable(ctx, 1))
					rnd_gui->add_timer(rnd_gui, timed_close_cb, 1, rv);
			}
		}
	}
}

static int rnd_dlg_fsd_poke(rnd_hid_dad_subdialog_t *sub, const char *cmd, rnd_event_arg_t *res, int argc, rnd_event_arg_t *argv)
{
	fsd_ctx_t *ctx = sub->parent_ctx;

	if (strcmp(cmd, "close") == 0) {
		if (ctx->active) {
			static rnd_dad_retovr_t retovr;
			rnd_hid_dad_close(ctx->dlg_hid_ctx, &retovr, -1);
		}
		return 0;
	}

	if (strcmp(cmd, "get_path") == 0) {
		rnd_hid_attribute_t *inp = &ctx->dlg[ctx->wpath];
		const char *fn = inp->val.str;

		if ((fn != NULL) && (*fn != '\0')) {
			res->d.s = rnd_concat(ctx->cwd, "/", fn, NULL);
			return 0;
		}
	}

	if ((strcmp(cmd, "set_file_name") == 0) && (argc == 1) && (argv[0].type == RND_EVARG_STR)) {
		rnd_hid_attr_val_t hv;
		hv.str = argv[0].d.s;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wpath, &hv);
		free((char *)argv[0].d.s);
		return 0;
	}

	return -1;
}

/*** dialog box ***/
char *rnd_dlg_fileselect(rnd_hid_t *hid, const char *title, const char *descr, const char *default_file, const char *default_ext, const rnd_hid_fsd_filter_t *flt, const char *history_tag, rnd_hid_fsd_flags_t flags, rnd_hid_dad_subdialog_t *sub)
{
	fsd_ctx_t *ctx = &fsd_ctx;
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", 1}, {NULL, 0}};
	const char *shc_hdr[] = { "Shortcuts", NULL };
	const char *filelist_hdr[] = { "Name", "Size", "Modified", NULL };
	const char *help_sort = "Sort entries by this column";
	const char *help_rev = "Sort in reverse (descending) order";
	const char *help_dir_grp = "Group and sort directories separately from files";
	const char *help_icase = "Case insensitive sort on names";
	char *res_path;
	int n, free_flt = 0;
	const char **filter_names = NULL;
	const rnd_hid_fsd_filter_t *fl;
	rnd_hid_fsd_filter_t flt_local[3];

	if (ctx->active) {
		rnd_message(RND_MSG_ERROR, "Recursive call of rnd_dlg_fileselect\n");
		return NULL;
	}

	memset(ctx, 0, sizeof(fsd_ctx_t));
	ctx->flags = flags;
	ctx->active = 1;
	ctx->history_tag = history_tag;

	/* set up the filter and the filter names for the combo box */
	if ((default_ext != NULL) && (flt == NULL)) {
		memset(&flt_local, 0, sizeof(flt_local));
		flt_local[0].name = default_ext;
		flt_local[0].pat = malloc(sizeof(char *) * 2);
		flt_local[0].pat[0] = rnd_concat("*", default_ext, NULL);
		flt_local[0].pat[1] = NULL;
		flt_local[1] = rnd_hid_fsd_filter_any[0];
		flt = flt_local;
		free_flt = 1;
	}

	if (flt != NULL) {
		for(n = 0, fl = flt; fl->name != NULL; fl++, n++) ;
		if (n > 0)
			filter_names = malloc(sizeof(char *) * (n+1));
		else
			flt = NULL;
	}

	if (flt != NULL) {
		for(n = 0, fl = flt; fl->name != NULL; fl++, n++)
			filter_names[n] = fl->name;
		filter_names[n] = NULL;
	TODO("set up filter here");
	}

	ctx->flt = flt;

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);

		/* manual edit */
		RND_DAD_BEGIN_HBOX(ctx->dlg);
			RND_DAD_STRING(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				ctx->wpath = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_ENTER_CB(ctx->dlg, edit_enter_cb);
		RND_DAD_END(ctx->dlg);

		/* directory helper */
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_TIGHT);
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_FRAME | RND_HATF_TIGHT);
				for(n = 0; n < FSD_MAX_DIRS/2; n++) {
					RND_DAD_BUTTON(ctx->dlg, "");
						RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_TIGHT);
						ctx->wdir[n] = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_CHANGE_CB(ctx->dlg, cd_button_cb);
				}
				RND_DAD_LABEL(ctx->dlg, "...");
					ctx->wdirlong = RND_DAD_CURRENT(ctx->dlg);
				for(n = 0; n < FSD_MAX_DIRS/2; n++) {
					RND_DAD_BUTTON(ctx->dlg, "");
						RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_TIGHT);
						ctx->wdir[n + FSD_MAX_DIRS/2] = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_CHANGE_CB(ctx->dlg, cd_button_cb);
				}
			RND_DAD_END(ctx->dlg);
		RND_DAD_END(ctx->dlg);

		/* lists */
		RND_DAD_BEGIN_HPANE(ctx->dlg, "left-right");
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);

			RND_DAD_BEGIN_VBOX(ctx->dlg); /* shortcuts */
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_TREE(ctx->dlg, 1, 0, shc_hdr);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_FRAME | RND_HATF_TREE_COL | RND_HATF_SCROLL);
					ctx->wshcut = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_CHANGE_CB(ctx->dlg, fsd_shcut_enter_cb);
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_PICBUTTON(ctx->dlg, rnd_dlg_xpm_by_name("plus"));
						RND_DAD_HELP(ctx->dlg, "add current directory to global favorites\n(Select local favorites tree node to\nadd it to the local favorites)");
						RND_DAD_CHANGE_CB(ctx->dlg, fsd_shc_add_cb);
					RND_DAD_PICBUTTON(ctx->dlg, rnd_dlg_xpm_by_name("minus"));
						RND_DAD_HELP(ctx->dlg, "remove favorite or recent");
						RND_DAD_CHANGE_CB(ctx->dlg, fsd_shc_del_cb);
						ctx->wshdel = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_END(ctx->dlg);
			RND_DAD_END(ctx->dlg);

			RND_DAD_BEGIN_VBOX(ctx->dlg); /* file list */
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_TREE(ctx->dlg, 3, 0, filelist_hdr);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_FRAME | RND_HATF_SCROLL);
					ctx->wfilelist = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_TREE_SET_CB(ctx->dlg, selected_cb, fsd_filelist_cb);
					RND_DAD_TREE_SET_CB(ctx->dlg, ctx, ctx);
					RND_DAD_CHANGE_CB(ctx->dlg, fsd_filelist_enter_cb);
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_LABEL(ctx->dlg, "Sort:");
						RND_DAD_HELP(ctx->dlg, help_sort);
					RND_DAD_ENUM(ctx->dlg, sort_names);
						RND_DAD_HELP(ctx->dlg, help_sort);
						ctx->wsort = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_CHANGE_CB(ctx->dlg, resort_cb);
					RND_DAD_LABEL(ctx->dlg, "rev.:");
						RND_DAD_HELP(ctx->dlg, help_rev);
					RND_DAD_BOOL(ctx->dlg);
						RND_DAD_HELP(ctx->dlg, help_rev);
						ctx->wsort_rev = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_CHANGE_CB(ctx->dlg, resort_cb);
					RND_DAD_LABEL(ctx->dlg, "dir. grp.:");
						RND_DAD_HELP(ctx->dlg, help_dir_grp);
					RND_DAD_BOOL(ctx->dlg);
						RND_DAD_HELP(ctx->dlg, help_dir_grp);
						ctx->wsort_dirgrp = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_CHANGE_CB(ctx->dlg, resort_cb);
						RND_DAD_DEFAULT_NUM(ctx->dlg, dialogs_conf.plugins.lib_hid_common.fsd.dir_grp);
					RND_DAD_LABEL(ctx->dlg, "icase:");
						RND_DAD_HELP(ctx->dlg, help_icase);
					RND_DAD_BOOL(ctx->dlg);
						RND_DAD_HELP(ctx->dlg, help_icase);
						ctx->wsort_icase = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_CHANGE_CB(ctx->dlg, resort_cb);
						RND_DAD_DEFAULT_NUM(ctx->dlg, dialogs_conf.plugins.lib_hid_common.fsd.icase);
				RND_DAD_END(ctx->dlg);
			RND_DAD_END(ctx->dlg);
		RND_DAD_END(ctx->dlg);

		/* custom widgets and standard buttons */
		RND_DAD_BEGIN_HBOX(ctx->dlg);
			if (sub != NULL) {
				RND_DAD_SUBDIALOG(ctx->dlg, sub);
					sub->parent_poke = rnd_dlg_fsd_poke;
					sub->parent_ctx = ctx;
			}

			/* spring */
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
			RND_DAD_END(ctx->dlg);

			/* close button */
			RND_DAD_BEGIN_VBOX(ctx->dlg);
				/* spring */
				RND_DAD_BEGIN_VBOX(ctx->dlg);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_END(ctx->dlg);

				RND_DAD_BEGIN_VBOX(ctx->dlg);
					if (filter_names != NULL) { /* format selection */
						RND_DAD_BEGIN_HBOX(ctx->dlg);
							/* spring */
							RND_DAD_BEGIN_VBOX(ctx->dlg);
								RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
							RND_DAD_END(ctx->dlg);
							RND_DAD_ENUM(ctx->dlg, filter_names);
								RND_DAD_CHANGE_CB(ctx->dlg, resort_cb);
								ctx->wflt = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_END(ctx->dlg);
					}
					RND_DAD_BEGIN_HBOX(ctx->dlg); /* buttons */
						RND_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
						RND_DAD_BUTTON(ctx->dlg, "ok");
							RND_DAD_CHANGE_CB(ctx->dlg, fsd_ok_cb);
					RND_DAD_END(ctx->dlg);
				RND_DAD_END(ctx->dlg);
			RND_DAD_END(ctx->dlg);
		RND_DAD_END(ctx->dlg);

	RND_DAD_END(ctx->dlg);

	RND_DAD_DEFSIZE(ctx->dlg, 500, 400);
	RND_DAD_NEW("file_selection_dialog", ctx->dlg, title, ctx, rnd_true, fsd_close_cb);

	ctx->cwd = NULL;

	/* If default file is specified (dir or file), navigate there */
	if ((default_file != NULL) && (*default_file != '\0')) {
		const char *rsep = fsd_io_rsep((char *)default_file);
		if (rsep != NULL) { /* full path */
			ctx->cwd = rnd_strdup(default_file);
			if (!rnd_is_dir(ctx->hidlib, default_file)) {
				char *sep = ctx->cwd + (rsep - default_file);
				if (sep != NULL) {
					*sep = '\0';
					default_file = sep+1;
				}
			}
			else
				default_file = NULL; /* in case of dir: leave file name empty */
		}
		else { /* file name only */
			/* do nothing, so default_file is kept and will be set */
		}
	}
	
	
	if (ctx->cwd == NULL) /* fallback: no default_file, start from cwd */
		ctx->cwd = rnd_strdup(rnd_get_wd(ctx->cwd_buf));

	if ((default_file != NULL) && (*default_file != '\0')) {
		rnd_hid_attr_val_t hv;
		char *sep;
		int len;

		hv.str = default_file;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wpath, &hv);
		sep = strrchr(default_file, '.');
		len = (sep != NULL) ? sep - default_file : strlen(default_file);
		RND_DAD_STRING_SELECT_REGION(ctx->dlg_hid_ctx, ctx->wpath, 0, len);
	}

	fsd_cd(ctx, NULL);
	fsd_shcut_load(ctx);

	RND_DAD_RUN(ctx->dlg);

	if ((sub != NULL) && (sub->on_close != NULL))
		sub->on_close(sub, (ctx->res_path != NULL));

	RND_DAD_FREE(ctx->dlg);

	/* calculate output */
	ctx->active = 0;
	res_path = ctx->res_path;
	ctx->res_path = NULL;

	if (res_path != NULL)
		fsd_shcut_append_to_file(ctx, 1, ".recent.lst", res_path, FSD_RECENT_MAX_LINES);


	/* free temp storage */

	if (free_flt) {
		free((char *)flt_local[0].pat[0]);
		free(flt_local[0].pat);
	}

	if (filter_names != NULL)
		free(filter_names);

	return res_path;
}


static rnd_hid_dad_subdialog_t *sub = NULL, sub_tmp;
static void fsdtest_poke_get_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_event_arg_t res = {0};

	printf("poke_get: %d\n", sub->parent_poke(sub, "get_path", &res, 0, NULL));
	printf(" '%s'\n", res.d.s);
	free((char *)res.d.s);
}

static void fsdtest_poke_set_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_event_arg_t res = {0}, argv[2];

	if (sub->parent_poke(sub, "get_path", &res, 0, NULL) == 0) {
		char *dot = strrchr(res.d.s, '.'), *slash = strrchr(res.d.s, '/');
		if ((slash != NULL) && (dot != NULL) && (strlen(dot) > 1)) {
			dot[1] = 'A';
			argv[0].type = RND_EVARG_STR;
			argv[0].d.s = rnd_strdup(slash+1);
			sub->parent_poke(sub, "set_file_name", &res, 1, argv);
		}
		free((char *)res.d.s);
	}
}

static void fsdtest_poke_close_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_event_arg_t res = {0};
	sub->parent_poke(sub, "close", &res, 0, NULL);
}


const char rnd_acts_FsdTest[] = "FsdTest()";
const char rnd_acth_FsdTest[] = "Central, DAD based File Selection Dialog demo";
fgw_error_t rnd_act_FsdTest(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *fn;
	rnd_hid_fsd_filter_t flt_local[6], *flt = NULL;
	rnd_hid_fsd_flags_t flags = 0;
	int test_subd = 1, test_filter = 1;

	if (test_subd) {
		sub = &sub_tmp;
		memset(sub, 0, sizeof(rnd_hid_dad_subdialog_t));
		RND_DAD_BEGIN_VBOX(sub->dlg);
			RND_DAD_BUTTON(sub->dlg, "poke-get");
				RND_DAD_CHANGE_CB(sub->dlg, fsdtest_poke_get_cb);
			RND_DAD_BUTTON(sub->dlg, "poke-set");
				RND_DAD_CHANGE_CB(sub->dlg, fsdtest_poke_set_cb);
		RND_DAD_END(sub->dlg);
		RND_DAD_BUTTON(sub->dlg, "poke-close");
			RND_DAD_CHANGE_CB(sub->dlg, fsdtest_poke_close_cb);
	}

	if (test_filter) {
		memset(&flt_local, 0, sizeof(flt_local));
		flt_local[0].name = "*.pcb";
		flt_local[0].pat = malloc(sizeof(char *) * 3);
		flt_local[0].pat[0] = "*.pcb";
		flt_local[0].pat[1] = "*.PCB";
		flt_local[0].pat[2] = NULL;
		flt_local[1].name = "*.lht";
		flt_local[1].pat = malloc(sizeof(char *) * 2);
		flt_local[1].pat[0] = "*.lht";
		flt_local[1].pat[1] = NULL;
		flt_local[2].name = "*.*";
		flt_local[2].pat = malloc(sizeof(char *) * 2);
		flt_local[2].pat[0] = "*.*";
		flt_local[2].pat[1] = NULL;
		flt = flt_local;
	}

	fn = rnd_dlg_fileselect(rnd_gui, "FsdTest", "DAD File Selection Dialog demo", "fsd.txt", ".txt", flt, "fsdtest", flags, sub);


	if (fn != NULL)
		rnd_message(RND_MSG_INFO, "FSD: fn='%s'\n", fn);
	else
		rnd_message(RND_MSG_INFO, "FSD: no file\n");

	return -1;
}

