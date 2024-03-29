/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2022 Tibor 'Igor2' Palinkas
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
#include <librnd/core/event.h>
#include <librnd/hid/hid.h>
#include <librnd/hid/hid_dad.h>
#include <librnd/hid/hid_dad_unit.h>
#include <librnd/hid/hid_init.h>
#include "dlg_export.h"

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
	void *appspec;

	int tabs, len;

	/* per exporter data */
	rnd_hid_t **hid;
	const char **tab_name;
	int **exp_attr;           /* widget IDs of the attributes holding the actual data; outer array is indexed by exporter, inner array is by exporter option index from 0*/
	int *button;              /* widget ID of the export button for a specific exporter */
	int *numo;                /* number of exporter options */
	rnd_hid_attr_val_t **aa;  /* original export argument arrays */
} export_ctx_t;

export_ctx_t export_ctx;

static rnd_hid_attr_val_t *get_results(export_ctx_t *export_ctx, int id)
{
	rnd_hid_attr_val_t *r;
	int *exp_attr, n, numo = export_ctx->numo[id];

	r = malloc(sizeof(rnd_hid_attr_val_t) * numo);

	exp_attr = export_ctx->exp_attr[id];
	for(n = 0; n < numo; n++) {
		int src = exp_attr[n];
		memcpy(&r[n], &(export_ctx->dlg[src].val), sizeof(rnd_hid_attr_val_t));
	}
	return r;
}

static void export_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	export_ctx_t *export_ctx = caller_data;
	rnd_design_t *hl = rnd_gui->get_dad_design(hid_ctx);
	int h, wid;


	wid = attr - export_ctx->dlg;
	for(h = 0; h < export_ctx->len; h++) {
		if (export_ctx->button[h] == wid) {
			rnd_hid_t *render_save = rnd_render;
			rnd_hid_attr_val_t *results = get_results(export_ctx, h);

			rnd_render = export_ctx->hid[h];
			rnd_event(hl, RND_EVENT_EXPORT_SESSION_BEGIN, NULL);
			export_ctx->hid[h]->do_export(export_ctx->hid[h], hl, results, export_ctx->appspec);
			rnd_event(hl, RND_EVENT_EXPORT_SESSION_END, NULL);
			rnd_render = render_save;
			free(results);
			rnd_message(RND_MSG_INFO, "Export done using exporter: %s\n", export_ctx->hid[h]->name);
			goto done;
		}
	}

	rnd_message(RND_MSG_ERROR, "Internal error: can not find which exporter to call\n");
	done:;
}

/* copy back the attribute values from the DAD dialog to exporter dialog so
   that values are preserved */
static void copy_attrs_back(export_ctx_t *ctx)
{
	int n, i;

	for(n = 0; n < ctx->len; n++) {
		int *exp_attr = export_ctx.exp_attr[n];
		int numo = export_ctx.numo[n];
		rnd_hid_attr_val_t *args = export_ctx.aa[n];

		for(i = 0; i < numo; i++)
			memcpy(&args[i], &ctx->dlg[exp_attr[i]].val, sizeof(rnd_hid_attr_val_t));
	}
}

static void export_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	export_ctx_t *ctx = caller_data;
	int n;

	copy_attrs_back(ctx);

	RND_DAD_FREE(ctx->dlg);
	free(ctx->hid);
	free(ctx->tab_name);
	for(n = 0; n < export_ctx.len; n++)
		free(ctx->exp_attr[n]);
	free(ctx->exp_attr);
	free(ctx->button);
	free(ctx->numo);
	free(ctx->aa);
	memset(ctx, 0, sizeof(export_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

void rnd_dlg_export(const char *title, int exporters, int printers, rnd_design_t *dsg, void *appspec)
{
	rnd_hid_t **hids;
	int n, i, *exp_attr;
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	if (export_ctx.active)
		return; /* do not open another */

	hids = rnd_hid_enumerate();
	for(n = 0, export_ctx.len = 0; hids[n] != NULL; n++) {
		if (((exporters && hids[n]->exporter) || (printers && hids[n]->printer)) && (!hids[n]->hide_from_gui) && (hids[n]->argument_array != NULL))
			export_ctx.len++;
	}

	if (export_ctx.len == 0) {
		rnd_message(RND_MSG_ERROR, "Can not export: there are no export plugins available for that task\n");
		return;
	}

	export_ctx.tab_name = malloc(sizeof(char *) * (export_ctx.len+1));
	export_ctx.hid = malloc(sizeof(rnd_hid_t *) * (export_ctx.len));
	export_ctx.exp_attr = malloc(sizeof(int *) * (export_ctx.len));
	export_ctx.button = malloc(sizeof(int) * (export_ctx.len));
	export_ctx.numo = malloc(sizeof(int) * (export_ctx.len));
	export_ctx.aa = malloc(sizeof(rnd_hid_attr_val_t *) * (export_ctx.len));
	export_ctx.appspec = appspec;

	for(i = n = 0; hids[n] != NULL; n++) {
		if (((exporters && hids[n]->exporter) || (printers && hids[n]->printer)) && (!hids[n]->hide_from_gui)) {
			if (hids[n]->argument_array == NULL) {
				rnd_message(RND_MSG_ERROR, "%s can't export from GUI because of empty argument_array\n(please report this bug!)\n", hids[n]->name);
				continue;
			}

			/* skip exporters that can not export the current design with appspec */
			if ((hids[n]->can_export != NULL) && !hids[n]->can_export(hids[n], dsg, appspec))
				continue;

			export_ctx.tab_name[i] = hids[n]->name;
			export_ctx.hid[i] = hids[n];
			i++;
		}
	}

	export_ctx.len = i;
	export_ctx.tab_name[i] = NULL;

	RND_DAD_BEGIN_VBOX(export_ctx.dlg);
	RND_DAD_COMPFLAG(export_ctx.dlg, RND_HATF_EXPFILL);
		RND_DAD_BEGIN_TABBED(export_ctx.dlg, export_ctx.tab_name);
			RND_DAD_COMPFLAG(export_ctx.dlg, RND_HATF_LEFT_TAB|RND_HATF_EXPFILL);
			export_ctx.tabs = RND_DAD_CURRENT(export_ctx.dlg);
			for(n = 0; n < export_ctx.len; n++) {
				int numo;
				const rnd_export_opt_t *opts = export_ctx.hid[n]->get_export_options(export_ctx.hid[n], &numo, dsg, appspec);
				rnd_hid_attr_val_t *args = export_ctx.hid[n]->argument_array;
				export_ctx.numo[n] = numo;
				export_ctx.aa[n] = args;

				if (numo < 1) {
					RND_DAD_LABEL(export_ctx.dlg, "Exporter unavailable for direct export");
					continue;
				}
				RND_DAD_BEGIN_VBOX(export_ctx.dlg);
					if (numo > 12)
						RND_DAD_COMPFLAG(export_ctx.dlg, RND_HATF_SCROLL);
					export_ctx.exp_attr[n] = exp_attr = malloc(sizeof(int) * numo);
					for(i = 0; i < numo; i++) {
						RND_DAD_BEGIN_HBOX(export_ctx.dlg)
							
							switch(opts[i].type) {
								case RND_HATT_COORD:
									RND_DAD_COORD(export_ctx.dlg);
									RND_DAD_MINMAX(export_ctx.dlg, opts[i].min_val, opts[i].max_val);
									RND_DAD_DEFAULT_NUM(export_ctx.dlg, args[i].crd);
									break;
								case RND_HATT_INTEGER:
									RND_DAD_INTEGER(export_ctx.dlg);
									RND_DAD_MINMAX(export_ctx.dlg, opts[i].min_val, opts[i].max_val);
									RND_DAD_DEFAULT_NUM(export_ctx.dlg, args[i].lng);
									break;
								case RND_HATT_REAL:
									RND_DAD_REAL(export_ctx.dlg);
									RND_DAD_MINMAX(export_ctx.dlg, opts[i].min_val, opts[i].max_val);
									RND_DAD_DEFAULT_NUM(export_ctx.dlg, args[i].dbl);
									break;
								case RND_HATT_UNIT:
									RND_DAD_UNIT(export_ctx.dlg, RND_UNIT_METRIC | RND_UNIT_IMPERIAL);
									RND_DAD_DEFAULT_NUM(export_ctx.dlg, args[i].lng);
									break;
								default:
									if (RND_HATT_IS_COMPOSITE(opts[i].type)) {
										rnd_hid_export_opt_func_t fnc = opts[i].default_val.func;
										if (fnc != NULL)
											fnc(RND_HIDEOF_DAD, &export_ctx, &opts[i], &args[i]);
									}
									else
										RND_DAD_DUP_EXPOPT_VAL(export_ctx.dlg, &(opts[i]), args[i]);
							}
							exp_attr[i] = RND_DAD_CURRENT(export_ctx.dlg);
							if (opts[i].name != NULL)
								RND_DAD_LABEL(export_ctx.dlg, opts[i].name);
						RND_DAD_END(export_ctx.dlg);
					}
					RND_DAD_LABEL(export_ctx.dlg, " "); /* ugly way of inserting some vertical spacing */
					RND_DAD_BEGIN_HBOX(export_ctx.dlg)
						RND_DAD_LABEL(export_ctx.dlg, "Apply attributes and export: ");
						RND_DAD_BUTTON(export_ctx.dlg, "Export!");
							export_ctx.button[n] = RND_DAD_CURRENT(export_ctx.dlg);
							RND_DAD_CHANGE_CB(export_ctx.dlg, export_cb);
					RND_DAD_END(export_ctx.dlg);
				RND_DAD_END(export_ctx.dlg);
			}
		RND_DAD_END(export_ctx.dlg);
		RND_DAD_BUTTON_CLOSES(export_ctx.dlg, clbtn);
	RND_DAD_END(export_ctx.dlg);

	/* set up the context */
	export_ctx.active = 1;

	RND_DAD_DEFSIZE(export_ctx.dlg, 400, 400);

	RND_DAD_NEW("export", export_ctx.dlg, title, &export_ctx, rnd_false, export_close_cb);
}

const char rnd_acts_ExportDialog[] = "ExportDialog()\n";
const char rnd_acth_ExportDialog[] = "Open the export dialog.";
fgw_error_t rnd_act_ExportDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_dlg_export("Export to file", 1, 0, RND_ACT_DESIGN, NULL);
	return 0;
}

const char rnd_acts_PrintDialog[] = "PrintDialog()\n";
const char rnd_acth_PrintDialog[] = "Open the print dialog.";
fgw_error_t rnd_act_PrintDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_dlg_export("Print", 0, 1, RND_ACT_DESIGN, NULL);
	return 0;
}

void rnd_dialog_export_close(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if (export_ctx.active) {
		rnd_dad_retovr_t retovr;
		rnd_hid_dad_close(export_ctx.dlg_hid_ctx, &retovr, 3);
	}
}
