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

#include "event.h"
#include "netlist2.h"

const char *dlg_netlist_cookie = "netlist dialog";

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	int wnetlist, wprev, wtermlist;
	int active; /* already open - allow only one instance */
} netlist_ctx_t;

netlist_ctx_t netlist_ctx;

static void netlist_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	netlist_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(netlist_ctx_t));
}

static void netlist_data2dlg(netlist_ctx_t *ctx)
{
	pcb_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;
	pcb_hid_row_t *r;
	char *cell[4], *cursor_path = NULL;

	attr = &ctx->dlg[ctx->wnetlist];
	tree = (pcb_hid_tree_t *)attr->enumerations;

	/* remember cursor */
	r = pcb_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = pcb_strdup(r->cell[0]);

	/* remove existing items */
	for(r = gdl_first(&tree->rows); r != NULL; r = gdl_first(&tree->rows))
		pcb_dad_tree_remove(attr, r);

#if 0
	cell[2] = NULL;
	for(i = ; i != NULL; ) {
		payload = "<unknown>";
		cell[0] = pcb_strdup(c1);
		cell[1] = pcb_strdup(c2);
		pcb_dad_tree_append(attr, NULL, cell);
	}
#endif

	/* restore cursor */
	if (cursor_path != NULL) {
		pcb_hid_attr_val_t hv;
		hv.str_value = cursor_path;
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wnetlist, &hv);
		free(cursor_path);
	}
}

static void cb_netlist_flagchg(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{

}

static void cb_netlist_frchg(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{

}

static void cb_netlist_add_rats(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{

}

static void cb_netlist_ripup(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{

}


static void netlist_expose(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{

}

static pcb_bool netlist_mouse(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_false;
}

static void pcb_dlg_netlist(void)
{
	static const char *hdr[] = {"network", "FR", NULL};
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	pcb_box_t bb_prv;
	int wvpan;

	if (netlist_ctx.active)
		return; /* do not open another */

	bb_prv.X1 = 0;
	bb_prv.Y1 = 0;
	bb_prv.X2 = PCB->MaxWidth;
	bb_prv.Y2 = PCB->MaxHeight;

	PCB_DAD_BEGIN_HPANE(netlist_ctx.dlg);
		PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL);

		PCB_DAD_BEGIN_VBOX(netlist_ctx.dlg); /* left */
			PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL);
			PCB_DAD_TREE(netlist_ctx.dlg, 2, 0, hdr);
				PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
				netlist_ctx.wnetlist = PCB_DAD_CURRENT(netlist_ctx.dlg);

			PCB_DAD_BEGIN_HBOX(netlist_ctx.dlg);
				PCB_DAD_BEGIN_VBOX(netlist_ctx.dlg);
					PCB_DAD_BUTTON(netlist_ctx.dlg, "select");
						PCB_DAD_CHANGE_CB(netlist_ctx.dlg, cb_netlist_flagchg);
					PCB_DAD_BUTTON(netlist_ctx.dlg, "unselect");
						PCB_DAD_CHANGE_CB(netlist_ctx.dlg, cb_netlist_flagchg);
				PCB_DAD_END(netlist_ctx.dlg);
				PCB_DAD_BEGIN_VBOX(netlist_ctx.dlg);
					PCB_DAD_BUTTON(netlist_ctx.dlg, "find");
						PCB_DAD_CHANGE_CB(netlist_ctx.dlg, cb_netlist_flagchg);
					PCB_DAD_BUTTON(netlist_ctx.dlg, "clear");
						PCB_DAD_CHANGE_CB(netlist_ctx.dlg, cb_netlist_flagchg);
				PCB_DAD_END(netlist_ctx.dlg);
				PCB_DAD_BEGIN_VBOX(netlist_ctx.dlg);
					PCB_DAD_BUTTON(netlist_ctx.dlg, "rats frz");
						PCB_DAD_CHANGE_CB(netlist_ctx.dlg, cb_netlist_frchg);
					PCB_DAD_BUTTON(netlist_ctx.dlg, "rats unfrz");
						PCB_DAD_CHANGE_CB(netlist_ctx.dlg, cb_netlist_frchg);
				PCB_DAD_END(netlist_ctx.dlg);
				PCB_DAD_BEGIN_VBOX(netlist_ctx.dlg);
					PCB_DAD_BUTTON(netlist_ctx.dlg, "add rats");
						PCB_DAD_CHANGE_CB(netlist_ctx.dlg, cb_netlist_add_rats);
					PCB_DAD_BUTTON(netlist_ctx.dlg, "rip up");
						PCB_DAD_CHANGE_CB(netlist_ctx.dlg, cb_netlist_ripup);
				PCB_DAD_END(netlist_ctx.dlg);
			PCB_DAD_END(netlist_ctx.dlg);
		PCB_DAD_END(netlist_ctx.dlg);

		PCB_DAD_BEGIN_VBOX(netlist_ctx.dlg); /* right */
			PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL);
			PCB_DAD_BEGIN_VPANE(netlist_ctx.dlg);
				PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL);
				wvpan = PCB_DAD_CURRENT(netlist_ctx.dlg);
				PCB_DAD_BEGIN_VBOX(netlist_ctx.dlg); /* right-top */
					PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL);
					PCB_DAD_PREVIEW(netlist_ctx.dlg, netlist_expose, netlist_mouse, NULL, &bb_prv, 100, 100, &netlist_ctx);
						PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL);
						netlist_ctx.wprev = PCB_DAD_CURRENT(netlist_ctx.dlg);
				PCB_DAD_END(netlist_ctx.dlg);
				PCB_DAD_BEGIN_VBOX(netlist_ctx.dlg); /* right-bottom */
					PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL);
					PCB_DAD_TREE(netlist_ctx.dlg, 1, 0, NULL);
						PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL);
						netlist_ctx.wtermlist = PCB_DAD_CURRENT(netlist_ctx.dlg);
					PCB_DAD_BUTTON_CLOSES(netlist_ctx.dlg, clbtn);
				PCB_DAD_END(netlist_ctx.dlg);
		PCB_DAD_END(netlist_ctx.dlg);
	PCB_DAD_END(netlist_ctx.dlg);

	/* set up the context */
	netlist_ctx.active = 1;

	PCB_DAD_DEFSIZE(netlist_ctx.dlg, 300, 400);
	PCB_DAD_NEW("netlist", netlist_ctx.dlg, "pcb-rnd netlist", &netlist_ctx, pcb_false, netlist_close_cb);

	{
		pcb_hid_attr_val_t hv;
		hv.real_value = 0.1;
		pcb_gui->attr_dlg_set_value(netlist_ctx.dlg_hid_ctx, wvpan, &hv);
	}

	netlist_data2dlg(&netlist_ctx);
}

static const char pcb_acts_NetlistDialog[] = "NetlistDialog()\n";
static const char pcb_acth_NetlistDialog[] = "Open the netlist dialog.";
static fgw_error_t pcb_act_NetlistDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_dlg_netlist();
	PCB_ACT_IRES(0);
	return 0;
}

/* update the dialog after a netlist change */
static void pcb_dlg_netlist_ev(void *user_data, int argc, pcb_event_arg_t argv[])
{
	netlist_ctx_t *ctx = user_data;
	if (!ctx->active)
		return;
	netlist_data2dlg(ctx);
}
static void pcb_dlg_netlist_init(void)
{
	pcb_event_bind(PCB_EVENT_NETLIST_CHANGED, pcb_dlg_netlist_ev, &netlist_ctx, dlg_netlist_cookie);
}

static void pcb_dlg_netlist_uninit(void)
{
	pcb_event_unbind_allcookie(dlg_netlist_cookie);
}
