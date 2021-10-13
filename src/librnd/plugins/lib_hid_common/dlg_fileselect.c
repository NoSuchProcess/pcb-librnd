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

#include <genht/hash.h>
#include <librnd/core/error.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/globalconst.h>
#include <librnd/core/compat_fs.h>

#include "dlg_fileselect.h"

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	int active;
	int wpath, wdir[FSD_MAX_DIRS], wdirlong, wshand, wfilelist;
	char cwd_buf[RND_PATH_MAX];
	char *cwd;
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

}

char *rnd_dlg_fileselect(rnd_hid_t *hid, const char *title, const char *descr, const char *default_file, const char *default_ext, const rnd_hid_fsd_filter_t *flt, const char *history_tag, rnd_hid_fsd_flags_t flags, rnd_hid_dad_subdialog_t *sub)
{
	fsd_ctx_t *ctx = &fsd_ctx;
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", 1}, {"ok", 0}, {NULL, 0}};
	const char *shc_hdr[] = { "Shortcuts", NULL };
	const char *filelist_hdr[] = { "Name", "Size", "Modified", NULL };
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
			RND_DAD_TREE(ctx->dlg, 3, 0, filelist_hdr);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_FRAME);
				ctx->wfilelist = RND_DAD_CURRENT(ctx->dlg);
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

