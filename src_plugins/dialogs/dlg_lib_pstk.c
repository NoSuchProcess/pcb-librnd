/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Padstack library dialog */

#include "config.h"
#include "obj_subc.h"

htip_t pstk_libs; /* id -> pstk_lib_ctx_t */

typedef struct pstk_lib_ctx_s {
	PCB_DAD_DECL_NOINIT(dlg)
	long id;
} pstk_lib_ctx_t;

static pcb_data_t *get_data(long id, pcb_subc_t **sc_out)
{
	int type;
	void *r1, *r2, *r3;
	pcb_subc_t *sc;

	if (id < 0)
		return PCB->Data;

	type = pcb_search_obj_by_id_(PCB->Data, &r1, &r2, &r3, id, PCB_OBJ_SUBC);
	if (type != PCB_OBJ_SUBC)
		return NULL;

	sc = r2;

	if (sc_out != NULL)
		*sc_out = sc;

	return sc->data;
}

static void pstklib_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	pstk_lib_ctx_t *ctx = caller_data;
	htip_pop(&pstk_libs, ctx->id);
	PCB_DAD_FREE(ctx->dlg);
	free(ctx);
}

static void pstklib_expose(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{

}


static int pcb_dlg_pstklib(long id)
{
	pcb_subc_t *sc;
	pcb_data_t *data;
	pstk_lib_ctx_t *ctx;
	int n;
	char *name;

	if (id <= 0)
		id = -1;

	data = get_data(id, &sc);
	if (data == NULL)
		return -1;

	if (htip_get(&pstk_libs, id) != NULL)
		return 0; /* already open - have only one per id */

	ctx = calloc(sizeof(pstk_lib_ctx_t), 1);
	ctx->id = id;
	htip_set(&pstk_libs, id, ctx);

	/* create the dialog box */
	PCB_DAD_BEGIN_HPANE(ctx->dlg);
		/* left */
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_TREE(ctx->dlg, 1, 0, NULL);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_STRING(ctx->dlg);
				PCB_DAD_HELP(ctx->dlg, "Filter text:\nlist padstacks with matching name only");
		PCB_DAD_END(ctx->dlg);

		/* right */
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_PREVIEW(ctx->dlg, pstklib_expose, NULL, NULL, NULL, ctx);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_BEGIN_TABLE(ctx->dlg, 2);
				for(n = 0; n < sizeof(pse_layer) / sizeof(pse_layer[0]); n++) {
					PCB_DAD_LABEL(ctx->dlg, pse_layer[n].name);
					PCB_DAD_BOOL(ctx->dlg, "");
				}
			PCB_DAD_END(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);

	if (id > 0) {
		if (sc->refdes != NULL)
			name = pcb_strdup_printf("pcb-rnd padstacks - subcircuit #%ld (%s)", id, sc->refdes);
		else
			name = pcb_strdup_printf("pcb-rnd padstacks - subcircuit #%ld", id);
	}
	else
		name = pcb_strdup("pcb-rnd padstacks - board");

	PCB_DAD_NEW(ctx->dlg, name, "", ctx, pcb_false, pstklib_close_cb);

	free(name);
	return 0;
}

static const char pcb_acts_pstklib[] = "pstklib([board|subcid])\n";
static const char pcb_acth_pstklib[] = "Present the padstack library dialog on board padstacks or the padstacks of a subcircuit";
static fgw_error_t pcb_act_pstklib(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	long id = -1;
	PCB_ACT_MAY_CONVARG(1, FGW_LONG, pstklib, id = argv[1].val.nat_long);
	PCB_ACT_IRES(pcb_dlg_pstklib(id));
	return 0;
}

static void dlg_pstklib_init(void)
{
	htip_init(&pstk_libs, longhash, longkeyeq);
}

static void dlg_pstklib_uninit(void)
{
	htip_entry_t *e;
	for(e = htip_first(&pstk_libs); e != NULL; e = htip_next(&pstk_libs, e))
		pstklib_close_cb(e->value, 0);
	htip_uninit(&pstk_libs);
}
