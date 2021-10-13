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
#include <librnd/core/error.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/globalconst.h>
#include <librnd/core/compat_fs.h>
#include <librnd/core/safe_fs_dir.h>

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
	vtde_t des;
	rnd_hidlib_t *hidlib;
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
	long n;

	TODO("clear the tree");

	for(n = 0; n < ctx->des.used; n++)
		printf("list: %s dir=%d %ld %f\n", ctx->des.array[n].name, ctx->des.array[n].is_dir, ctx->des.array[n].size, ctx->des.array[n].mtime);
}

/* Change to directory, relative to ctx->cwd. If rel is NULL, just cd to
   ctx->cwd. */
static void fsd_cd(fsd_ctx_t *ctx, const char *rel)
{
	vtp0_t path = {0};
	char *s, *next, tmp[RND_PATH_MAX];
	int n, m;
	rnd_hid_attr_val_t hv;

	if (rel != NULL) {
		TODO("cd .. or to a dir");
	}

	s = tmp;
	strcpy(tmp, ctx->cwd);

/* Append root */
#ifdef __WIN32__
	if (s[1] == ':') {
		vtp0_append(&path, s);
		s[2] = '\0';
		s += 3;
	}
#else
	vtp0_append(&path, "/");
#endif

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
			next++;
			if (*next == '\0')
				next = NULL;
		}
		if (next - s > FSD_MAX_DIRNAME_LEN)
			strcpy(s + FSD_MAX_DIRNAME_LEN - 3, "...");
		vtp0_append(&path, s);
	}


	if (path.used > FSD_MAX_DIRS) {
		/* path too long - split the path in 2 parts and enable "..." in the middle */
		for(n = 0; n < FSD_MAX_DIRS/2; n++) {
			hv.str = (char *)path.array[n];
			rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wdir[n], &hv);
			rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wdir[n], 0);
		}
		m = n;
		for(n = path.used - FSD_MAX_DIRS/2; n < path.used; n++,m++) {
			hv.str = (char *)path.array[n];
			rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wdir[m], &hv);
			rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wdir[m], 0);
		}
		rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wdirlong, 0);
	}
	else {
		/* path short enough for hiding "..." and displaying all */
		for(n = 0; n < path.used; n++) {
			hv.str = (char *)path.array[n];
			rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wdir[n], &hv);
			rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wdir[n], 0);
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

static void resort_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	fsd_ctx_t *ctx = caller_data;
	fsd_sort(ctx);
	fsd_load(ctx);
}


char *rnd_dlg_fileselect(rnd_hid_t *hid, const char *title, const char *descr, const char *default_file, const char *default_ext, const rnd_hid_fsd_filter_t *flt, const char *history_tag, rnd_hid_fsd_flags_t flags, rnd_hid_dad_subdialog_t *sub)
{
	fsd_ctx_t *ctx = &fsd_ctx;
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", 1}, {"ok", 0}, {NULL, 0}};
	const char *shc_hdr[] = { "Shortcuts", NULL };
	const char *filelist_hdr[] = { "Name", "Size", "Modified", NULL };
	const char *help_sort = "Sort entries by this column";
	const char *help_rev = "Sort in reverse (descending) order";
	const char *help_dir_grp = "Group and sort directories separately from files";
	int n;

	if (ctx->active) {
		rnd_message(RND_MSG_ERROR, "Recursive call of rnd_dlg_fileselect\n");
		return NULL;
	}

	memset(ctx, 0, sizeof(fsd_ctx_t));
	ctx->active = 1;

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);

		/* manual edit */
		RND_DAD_BEGIN_HBOX(ctx->dlg);
			RND_DAD_STRING(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				ctx->wpath = RND_DAD_CURRENT(ctx->dlg);
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
				}
				RND_DAD_LABEL(ctx->dlg, "...");
					ctx->wdirlong = RND_DAD_CURRENT(ctx->dlg);
				for(n = 0; n < FSD_MAX_DIRS/2; n++) {
					RND_DAD_BUTTON(ctx->dlg, "");
						RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_TIGHT);
						ctx->wdir[n + FSD_MAX_DIRS/2] = RND_DAD_CURRENT(ctx->dlg);
				}
			RND_DAD_END(ctx->dlg);
		RND_DAD_END(ctx->dlg);

		/* lists */
		RND_DAD_BEGIN_HPANE(ctx->dlg);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
			RND_DAD_TREE(ctx->dlg, 1, 0, shc_hdr);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_FRAME);
				ctx->wshand = RND_DAD_CURRENT(ctx->dlg);

			RND_DAD_BEGIN_VBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_TREE(ctx->dlg, 3, 0, filelist_hdr);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_FRAME);
					ctx->wfilelist = RND_DAD_CURRENT(ctx->dlg);
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

	RND_DAD_DEFSIZE(ctx->dlg, 200, 300);
	RND_DAD_NEW("file_selection_dialog", ctx->dlg, title, ctx, rnd_true, fsd_close_cb);

	ctx->cwd = rnd_get_wd(ctx->cwd_buf);
	fsd_cd(ctx, NULL);

	RND_DAD_RUN(ctx->dlg);

	ctx->active = 0;
	return NULL;
}


const char rnd_acts_FsdTest[] = "FsdTest()";
const char rnd_acth_FsdTest[] = "Central, DAD based File Selection Dialog demo";
fgw_error_t rnd_act_FsdTest(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *fn;
	rnd_hid_fsd_filter_t *flt = NULL;
	rnd_hid_fsd_flags_t flags = RND_HID_FSD_MAY_NOT_EXIST;
	rnd_hid_dad_subdialog_t *sub = NULL;

	fn = rnd_dlg_fileselect(rnd_gui, "FsdTest", "DAD File Selection Dialog demo", "fsd.txt", ".txt", flt, "fsd_hist", flags, sub);


	if (fn != NULL)
		rnd_message(RND_MSG_INFO, "FSD: fn='%s'\n", fn);
	else
		rnd_message(RND_MSG_INFO, "FSD: no file\n");

	return -1;
}

