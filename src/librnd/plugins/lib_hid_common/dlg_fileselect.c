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

#define FSD_MAX_DIRNAME_LEN 16

#include <limits.h>
#include <genht/hash.h>
#include <genvector/vti0.h>
#include <librnd/core/error.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/hid_dad_tree.h>
#include <librnd/core/globalconst.h>
#include <librnd/core/compat_fs.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/safe_fs_dir.h>
#include <librnd/core/hidlib.h>
#include <librnd/core/hidlib_conf.h>

#include "dlg_fileselect.h"

typedef struct {
	char *name;
	unsigned is_dir:1;
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
	int wpath, wdir[FSD_MAX_DIRS], wdirlong, wshand, wfilelist;
	int wsort, wsort_rev, wsort_dirgrp;
	char cwd_buf[RND_PATH_MAX];
	char *cwd;
	int cwd_offs[FSD_MAX_DIRS]; /* string lengths for each dir button within ->cwd */
	vtde_t des;
	rnd_hidlib_t *hidlib;
	const char *history_tag;
	void *last_row;
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
	printf("close cb\n");
}


/*** file listing ***/
TODO("move this to compat_fs  and add in safe_fs")
int rnd_file_stat_(const char *path, int *is_dir, long *size, double *mtime)
{
	struct stat st;
	if (stat(path, &st) != 0)
		return -1;

	*is_dir = S_ISDIR(st.st_mode);

	if (st.st_size > LONG_MAX)
		*size = LONG_MAX;
	else
		*size = st.st_size;

	*mtime = st.st_mtime;

	return 0;
}

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
		if (rnd_file_stat_(fullp.array, &is_dir, &size, &mtime) != 0)
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
static int cmp_sort_rev, cmp_sort_dirgrp;
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
			order = strcmp(a->name, b->name);
			break;
	}
	

	return rev_order(order);
}

static void fsd_sort(fsd_ctx_t *ctx)
{
	cmp_sort = ctx->dlg[ctx->wsort].val.lng;
	cmp_sort_rev = ctx->dlg[ctx->wsort_rev].val.lng;
	cmp_sort_dirgrp = ctx->dlg[ctx->wsort_dirgrp].val.lng;
	qsort(ctx->des.array, ctx->des.used, sizeof(fsd_dirent_t), fsd_sort_cmp);
}

static void fsd_load(fsd_ctx_t *ctx)
{
	rnd_hid_attribute_t *attr= &ctx->dlg[ctx->wfilelist];
	rnd_hid_tree_t *tree = attr->wdata;
	char *cell[4];
	long n;

	rnd_dad_tree_clear(tree);

	for(n = 0; n < ctx->des.used; n++) {
		rnd_hid_row_t *row;
		char ssize[64], smtime[64];
		struct tm *tm;
		time_t t;

/*		printf("list: %s dir=%d %ld %f\n", ctx->des.array[n].name, ctx->des.array[n].is_dir, ctx->des.array[n].size, ctx->des.array[n].mtime);*/

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
		row = rnd_dad_tree_append(attr, NULL, cell);
	}
}

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
			char *sep;
TODO("on windows also check for \\");
			sep = strrchr(ctx->cwd, '/');
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
			char *new_cwd = rnd_concat(ctx->cwd, "/", rel, NULL);
			DIR *dir = rnd_opendir(ctx->hidlib, ctx->cwd);
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
			while((*s == '/') || (*s == '\\'))) s++;
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

		if (next - s > FSD_MAX_DIRNAME_LEN)
			strcpy(s + FSD_MAX_DIRNAME_LEN - 3, "...");
		vtp0_append(&path, s);
		vti0_append(&offs, end);
	}


	if (path.used > FSD_MAX_DIRS) {
		/* path too long - split the path in 2 parts and enable "..." in the middle */
		for(n = 0; n < FSD_MAX_DIRS/2; n++) {
			hv.str = (char *)path.array[n];
			rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wdir[n], &hv);
			rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wdir[n], 0);
			ctx->cwd_offs[n] = offs.array[n];
		}
		m = n;
		for(n = path.used - FSD_MAX_DIRS/2; n < path.used; n++,m++) {
			hv.str = (char *)path.array[n];
			rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wdir[m], &hv);
			rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wdir[m], 0);
			ctx->cwd_offs[m] = offs.array[n];
		}
		rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wdirlong, 0);
	}
	else {
		/* path short enough for hiding "..." and displaying all */
		for(n = 0; n < path.used; n++) {
			hv.str = (char *)path.array[n];
			rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wdir[n], &hv);
			rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wdir[n], 0);
			ctx->cwd_offs[n] = offs.array[n];
		}
		for(; n < FSD_MAX_DIRS; n++) {
			rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wdir[n], 1);
		}
		rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wdirlong, 1);
	}

	fsd_clear(ctx);
	fsd_list(ctx);
	fsd_sort(ctx);
	fsd_load(ctx);
}

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
	fsd_load(ctx);
}

/* Handle new text entered in the path field */
static void edit_enter_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	fsd_ctx_t *ctx = caller_data;
	static rnd_dad_retovr_t retovr;
	const char *fn = attr->val.str;
	rnd_hid_attr_val_t hv;

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

TODO("closing from within a tree table causes a gtk/glib segf");
static void timed_close_cb(rnd_hidval_t user_data)
{
	static rnd_dad_retovr_t retovr;
	rnd_hid_dad_close(user_data.ptr, &retovr, 0);
}


TODO("This should be done by the tree table widget; should also work for enter")
static void fsd_enter_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	fsd_ctx_t *ctx = caller_data;
	rnd_hid_tree_t *tree = attr->wdata;
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(attr);

	if ((row == ctx->last_row) && (row != NULL)) {
		if (row->cell[1][0] != '<') {
			rnd_hidval_t rv;
			rv.ptr = hid_ctx;
			ctx->res_path = rnd_concat(ctx->cwd, "/", row->cell[0], NULL);
			rnd_gui->add_timer(rnd_gui, timed_close_cb, 1, rv);
		}
		else {
			fsd_cd(ctx, row->cell[0]);
			ctx->last_row = NULL;
			return; /* so that last_row remains NULL */
		}
	}

	ctx->last_row = row;
}

/*** shorthand ***/
static int fsd_shand_path_setup(fsd_ctx_t *ctx, gds_t *path, int do_mkdir)
{
	if (rnd_conf.rc.path.home == NULL)
		return -1;

	gds_append_str(path, rnd_conf.rc.path.home);
	gds_append(path, '/');

	gds_append_str(path, rnd_app.dot_dir);
	gds_append_str(path, "/fsd");
	if (do_mkdir && !rnd_is_dir(ctx->hidlib, path->array))
		rnd_mkdir(ctx->hidlib, path->array, 0750);
	gds_append(path, '/');
	gds_append_str(path, ctx->history_tag);

	return 0;
}

static void fsd_shand_load_file(fsd_ctx_t *ctx, rnd_hid_attribute_t *attr, rnd_hid_row_t *rparent, gds_t *path, const char *suffix)
{
	int saved = path->used;
	FILE *f;

	gds_append_str(path, suffix);
printf("OPEN: '%s'\n", path->array);
	f = rnd_fopen(ctx->hidlib, path->array, "r");
	if (f != NULL) {
		char line[RND_PATH_MAX+8], *cell[1];
		while(fgets(line, sizeof(line), f) != NULL) {
			char *end = line + strlen(line) - 1;
			while((end >= line) && ((*end == '\n') || (*end == '\r'))) { *end = '\0'; end--; }
			if (*line == '\0')
				continue;
			cell[0] = rnd_strdup(line);
			rnd_dad_tree_append_under(attr, rparent, cell);
		}
		fclose(f);
	}

	path->used = saved;
}

static void fsd_shand_load(fsd_ctx_t *ctx)
{
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wshand];
	rnd_hid_tree_t *tree = attr->wdata;
	char *cell[1];
	long n;
	rnd_hid_row_t *rparent;
	gds_t path = {0};

	rnd_dad_tree_clear(tree);

	/* filesystem */
	cell[0] = rnd_strdup("filesystem");
	rparent = rnd_dad_tree_append(attr, NULL, cell);

	cell[0] = rnd_strdup("/"); rnd_dad_tree_append_under(attr, rparent, cell);
	if (rnd_conf.rc.path.home != NULL) {
		cell[0] = rnd_strdup(rnd_conf.rc.path.home); rnd_dad_tree_append_under(attr, rparent, cell);
	}
	cell[0] = rnd_strdup("/tmp"); rnd_dad_tree_append_under(attr, rparent, cell);

	rnd_dad_tree_expcoll_(tree, rparent, 1, 0);


	if (fsd_shand_path_setup(ctx, &path, 0) != 0)
		return;


	cell[0] = rnd_strdup("favorites");
	rparent = rnd_dad_tree_append(attr, NULL, cell);
	fsd_shand_load_file(ctx, attr, rparent, &path, ".fav.lst");
	rnd_dad_tree_expcoll_(tree, rparent, 1, 0);

	cell[0] = rnd_strdup("recent");
	rparent = rnd_dad_tree_append(attr, NULL, cell);
	fsd_shand_load_file(ctx, attr, rparent, &path, ".recent.lst");
	rnd_dad_tree_expcoll_(tree, rparent, 1, 0);
}


/*** dialog box ***/
char *rnd_dlg_fileselect(rnd_hid_t *hid, const char *title, const char *descr, const char *default_file, const char *default_ext, const rnd_hid_fsd_filter_t *flt, const char *history_tag, rnd_hid_fsd_flags_t flags, rnd_hid_dad_subdialog_t *sub)
{
	fsd_ctx_t *ctx = &fsd_ctx;
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", 1}, {"ok", 0}, {NULL, 0}};
	const char *shc_hdr[] = { "Shortcuts", NULL };
	const char *filelist_hdr[] = { "Name", "Size", "Modified", NULL };
	const char *help_sort = "Sort entries by this column";
	const char *help_rev = "Sort in reverse (descending) order";
	const char *help_dir_grp = "Group and sort directories separately from files";
	char *res_path;
	int n;

	if (ctx->active) {
		rnd_message(RND_MSG_ERROR, "Recursive call of rnd_dlg_fileselect\n");
		return NULL;
	}

	memset(ctx, 0, sizeof(fsd_ctx_t));
	ctx->active = 1;
	ctx->history_tag = history_tag;

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
		RND_DAD_BEGIN_HPANE(ctx->dlg);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
			RND_DAD_TREE(ctx->dlg, 1, 0, shc_hdr); /* shorthands */
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_FRAME | RND_HATF_TREE_COL | RND_HATF_SCROLL);
				ctx->wshand = RND_DAD_CURRENT(ctx->dlg);

			RND_DAD_BEGIN_VBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_TREE(ctx->dlg, 3, 0, filelist_hdr); /* file list */
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_FRAME | RND_HATF_SCROLL);
					ctx->wfilelist = RND_DAD_CURRENT(ctx->dlg);
/*					RND_DAD_TREE_SET_CB(ctx->dlg, enter_cb, fsd_enter_cb);*/
					RND_DAD_CHANGE_CB(ctx->dlg, fsd_enter_cb);
					RND_DAD_TREE_SET_CB(ctx->dlg, ctx, &ctx);
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
				RND_DAD_END(ctx->dlg);
			RND_DAD_END(ctx->dlg);
		RND_DAD_END(ctx->dlg);

		/* custom widgets and standard buttons */
		RND_DAD_BEGIN_HBOX(ctx->dlg);

			TODO("subdialog");

			/* spring */
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
			RND_DAD_END(ctx->dlg);

			/* close button */
			RND_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
		RND_DAD_END(ctx->dlg);

	RND_DAD_END(ctx->dlg);

	RND_DAD_DEFSIZE(ctx->dlg, 500, 400);
	RND_DAD_NEW("file_selection_dialog", ctx->dlg, title, ctx, rnd_true, fsd_close_cb);

	ctx->cwd = rnd_strdup(rnd_get_wd(ctx->cwd_buf));
	fsd_cd(ctx, NULL);
	fsd_shand_load(ctx);

	RND_DAD_RUN(ctx->dlg);
	RND_DAD_FREE(ctx->dlg);


	ctx->active = 0;
	res_path = ctx->res_path;
	ctx->res_path = NULL;

	return res_path;
}


const char rnd_acts_FsdTest[] = "FsdTest()";
const char rnd_acth_FsdTest[] = "Central, DAD based File Selection Dialog demo";
fgw_error_t rnd_act_FsdTest(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *fn;
	rnd_hid_fsd_filter_t *flt = NULL;
	rnd_hid_fsd_flags_t flags = RND_HID_FSD_MAY_NOT_EXIST;
	rnd_hid_dad_subdialog_t *sub = NULL;

	fn = rnd_dlg_fileselect(rnd_gui, "FsdTest", "DAD File Selection Dialog demo", "fsd.txt", ".txt", flt, "fsdtest", flags, sub);


	if (fn != NULL)
		rnd_message(RND_MSG_INFO, "FSD: fn='%s'\n", fn);
	else
		rnd_message(RND_MSG_INFO, "FSD: no file\n");

	return -1;
}

