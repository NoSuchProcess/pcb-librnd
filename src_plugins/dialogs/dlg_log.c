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
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include "actions.h"
#include "hid_dad.h"
#include "event.h"
#include "error.h"
#include "dlg_log.h"

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	unsigned long last_added;
	int active;
	int wtxt;
} log_ctx_t;

static log_ctx_t log_ctx;

static void log_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	log_ctx_t *ctx = caller_data;
	ctx->active = 0;
}

static void log_import(log_ctx_t *ctx)
{
	pcb_logline_t *n;
	pcb_hid_attribute_t *atxt = &ctx->dlg[ctx->wtxt];
	pcb_hid_text_t *txt = (pcb_hid_text_t *)atxt->enumerations;

	for(n = pcb_log_find_min(ctx->last_added); n != NULL; n = n->next) {
		txt->hid_set_text(atxt, ctx->dlg_hid_ctx, PCB_HID_TEXT_APPEND | PCB_HID_TEXT_MARKUP, n->str);
		ctx->last_added = n->ID;
	}
}

static void log_window_create(void)
{
	log_ctx_t *ctx = &log_ctx;

	if (ctx->active)
		return;

	memset(ctx, 0, sizeof(log_ctx_t));

	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_TEXT(ctx->dlg, NULL);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_SCROLL | PCB_HATF_EXPFILL);
			ctx->wtxt = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_BUTTON(ctx->dlg, "clear");
			PCB_DAD_BUTTON(ctx->dlg, "save");
			PCB_DAD_BEGIN_VBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BUTTON_CLOSE(ctx->dlg, "close", 0);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);

	ctx->active = 1;
	ctx->last_added = -1;
	PCB_DAD_NEW("log", ctx->dlg, "pcb-rnd message log", ctx, pcb_false, log_close_cb);

	{
		pcb_hid_attribute_t *atxt = &ctx->dlg[ctx->wtxt];
		pcb_hid_text_t *txt = (pcb_hid_text_t *)atxt->enumerations;
		txt->hid_set_readonly(atxt, ctx->dlg_hid_ctx, 1);
	}
	log_import(ctx);
}


const char pcb_acts_LogDialog[] = "LogDialog()\n";
const char pcb_acth_LogDialog[] = "Open the log dialog.";
fgw_error_t pcb_act_LogDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	log_window_create();
	PCB_ACT_IRES(0);
	return 0;
}
