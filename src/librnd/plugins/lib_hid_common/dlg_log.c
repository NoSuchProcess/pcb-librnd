/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include <librnd/core/actions.h>
#include <librnd/core/conf_hid.h>
#include <librnd/hid/hid_dad.h>
#include <librnd/core/event.h>
#include <librnd/core/error.h>

#include "dlg_log.h"

static const char *log_cookie = "dlg_log";

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	unsigned long last_added;
	int active;
	int wtxt, wscroll;
	int gui_inited;
	rnd_design_t *hidlib;
} log_ctx_t;

static log_ctx_t log_ctx;

static void log_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	log_ctx_t *ctx = caller_data;
	ctx->active = 0;
}

static void log_append(log_ctx_t *ctx, rnd_hid_attribute_t *atxt, rnd_logline_t *line)
{
	rnd_hid_text_t *txt = atxt->wdata;
	const char *prefix = NULL;
	int popup;

	rnd_conf_loglevel_props(line->level, &prefix, &popup);

	if (rnd_gui->supports_dad_text_markup) {
		if (prefix != NULL) {
			gds_t tmp;
			gds_init(&tmp);
			gds_enlarge(&tmp, line->len+32);
			tmp.used = 0;
			gds_append_str(&tmp, prefix);
			gds_append_len(&tmp, line->str, line->len);
			if (*prefix == '<') {
				gds_append(&tmp, '<');
				gds_append(&tmp, '/');
				gds_append_str(&tmp, prefix+1);
			}
			txt->hid_set_text(atxt, ctx->dlg_hid_ctx, RND_HID_TEXT_APPEND | RND_HID_TEXT_MARKUP, tmp.array);
			gds_uninit(&tmp);
		}
		else
			txt->hid_set_text(atxt, ctx->dlg_hid_ctx, RND_HID_TEXT_APPEND, line->str);
	}
	else {
		if ((line->prev == NULL) || (line->prev->str[line->prev->len-1] == '\n')) {
			switch(line->level) {
				case RND_MSG_DEBUG:   prefix = "D: "; break;
				case RND_MSG_INFO:    prefix = "I: "; break;
				case RND_MSG_WARNING: prefix = "W: "; break;
				case RND_MSG_ERROR:   prefix = "E: "; break;
			}
			if (prefix != NULL)
				txt->hid_set_text(atxt, ctx->dlg_hid_ctx, RND_HID_TEXT_APPEND | RND_HID_TEXT_MARKUP, prefix);
		}
		txt->hid_set_text(atxt, ctx->dlg_hid_ctx, RND_HID_TEXT_APPEND | RND_HID_TEXT_MARKUP, line->str);
	}
	if (popup && (rnd_gui->attr_dlg_raise != NULL))
		rnd_gui->attr_dlg_raise(ctx->dlg_hid_ctx);
	if (line->ID > ctx->last_added)
		ctx->last_added = line->ID;
	line->seen = 1;
}

static void log_import(log_ctx_t *ctx)
{
	rnd_logline_t *n;
	rnd_hid_attribute_t *atxt = &ctx->dlg[ctx->wtxt];

	for(n = rnd_log_find_min(ctx->last_added); n != NULL; n = n->next)
		log_append(ctx, atxt, n);
}

static void btn_clear_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	log_ctx_t *ctx = caller_data;
	rnd_actionva(ctx->hidlib, "log", "clear", NULL);
}

static void btn_export_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	log_ctx_t *ctx = caller_data;
	rnd_actionva(ctx->hidlib, "log", "export", NULL);
}

static void maybe_scroll_to_bottom()
{
	rnd_hid_attribute_t *atxt = &log_ctx.dlg[log_ctx.wtxt];
	rnd_hid_text_t *txt = atxt->wdata;

	if ((log_ctx.dlg[log_ctx.wscroll].val.lng) && (txt->hid_scroll_to_bottom != NULL))
		txt->hid_scroll_to_bottom(atxt, log_ctx.dlg_hid_ctx);
}

static void log_window_create(rnd_design_t *hidlib)
{
	log_ctx_t *ctx = &log_ctx;
	rnd_hid_attr_val_t hv;
	char *title;

	if (ctx->active)
		return;

	memset(ctx, 0, sizeof(log_ctx_t));
	ctx->gui_inited = 1;
	ctx->hidlib = hidlib;

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
		RND_DAD_TEXT(ctx->dlg, NULL);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_SCROLL | RND_HATF_EXPFILL);
			ctx->wtxt = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_BEGIN_HBOX(ctx->dlg);
			RND_DAD_BUTTON(ctx->dlg, "clear");
				RND_DAD_CHANGE_CB(ctx->dlg, btn_clear_cb);
			RND_DAD_BUTTON(ctx->dlg, "export");
				RND_DAD_CHANGE_CB(ctx->dlg, btn_export_cb);
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_FRAME);
				RND_DAD_BOOL(ctx->dlg);
					ctx->wscroll = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_LABEL(ctx->dlg, "scroll");
			RND_DAD_END(ctx->dlg);
			RND_DAD_BEGIN_VBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
			RND_DAD_END(ctx->dlg);
			RND_DAD_BUTTON_CLOSE(ctx->dlg, "close", 0);
		RND_DAD_END(ctx->dlg);
	RND_DAD_END(ctx->dlg);

	ctx->active = 1;
	ctx->last_added = -1;
	RND_DAD_DEFSIZE(ctx->dlg, 200, 300);
	title = rnd_concat(rnd_app.package, " message log", NULL);
	RND_DAD_NEW("log", ctx->dlg, title, ctx, rnd_false, log_close_cb);
	free(title);

	{
		rnd_hid_attribute_t *atxt = &ctx->dlg[ctx->wtxt];
		rnd_hid_text_t *txt = atxt->wdata;
		txt->hid_set_readonly(atxt, ctx->dlg_hid_ctx, 1);
	}
	hv.lng = 1;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wscroll, &hv);
	log_import(ctx);
	maybe_scroll_to_bottom();
}


const char rnd_acts_LogDialog[] = "LogDialog()\n";
const char rnd_acth_LogDialog[] = "Open the log dialog.";
fgw_error_t rnd_act_LogDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	log_window_create(RND_ACT_DESIGN);
	RND_ACT_IRES(0);
	return 0;
}

static void log_append_ev(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	rnd_logline_t *line = argv[1].d.p;

	if (log_ctx.active) {
		rnd_hid_attribute_t *atxt = &log_ctx.dlg[log_ctx.wtxt];

		log_append(&log_ctx, atxt, line);
		maybe_scroll_to_bottom();
	}
	else if ((RND_HAVE_GUI_ATTR_DLG) && (log_ctx.gui_inited)) {
		const char *prefix;
		int popup;

		rnd_conf_loglevel_props(line->level, &prefix, &popup);
		if (popup)
			log_window_create(hidlib);
	}
}

static void log_clear_ev(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if (log_ctx.active) {
		rnd_hid_attribute_t *atxt = &log_ctx.dlg[log_ctx.wtxt];
		rnd_hid_text_t *txt = atxt->wdata;

		txt->hid_set_text(atxt, log_ctx.dlg_hid_ctx, RND_HID_TEXT_REPLACE, "");
		log_import(&log_ctx);
	}
}

static void log_gui_init_ev(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	rnd_logline_t *n;

	log_ctx.gui_inited = 1;

	/* if there's pending popup-message in the queue, pop up the dialog */
	for(n = rnd_log_first; n != NULL; n = n->next) {
		const char *prefix;
		int popup;

		rnd_conf_loglevel_props(n->level, &prefix, &popup);
		if (popup) {
			log_window_create(hidlib);
			return;
		}
	}
}

static void log_main_loop_ev(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	int in_ml = argv[1].d.i;

	/* disable the gui if we have left the main loop */
	if (!in_ml)
		log_ctx.gui_inited = 0;
}


void rnd_dlg_log_uninit(void)
{
	rnd_event_unbind_allcookie(log_cookie);
}

void rnd_dlg_log_init(void)
{
	rnd_event_bind(RND_EVENT_LOG_APPEND, log_append_ev, NULL, log_cookie);
	rnd_event_bind(RND_EVENT_LOG_CLEAR, log_clear_ev, NULL, log_cookie);
	rnd_event_bind(RND_EVENT_GUI_INIT, log_gui_init_ev, NULL, log_cookie);
	rnd_event_bind(RND_EVENT_MAINLOOP_CHANGE, log_main_loop_ev, NULL, log_cookie);
}
