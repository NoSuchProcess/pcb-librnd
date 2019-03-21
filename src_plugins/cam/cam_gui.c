/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  cam export jobs plugin: GUI dialog
 *  pcb-rnd Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include <genht/hash.h>
#include "hid_dad.h"
#include "hid_dad_tree.h"

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	int wjobs;
} cam_dlg_t;

static void cam_gui_jobs2dlg(cam_dlg_t *ctx)
{
	pcb_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;
	pcb_hid_row_t *r;
	char *cell[2], *cursor_path = NULL;
	conf_native_t *cn;

	attr = &ctx->dlg[ctx->wjobs];
	tree = (pcb_hid_tree_t *)attr->enumerations;

	/* remember cursor */
	r = pcb_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = pcb_strdup(r->cell[0]);

	/* remove existing items */
	pcb_dad_tree_clear(tree);

	/* add all new items */
	cn = conf_get_field("plugins/cam/jobs");
	if (cn != NULL) {
		conf_listitem_t *item;
		int idx;

		cell[1] = NULL;
		conf_loop_list(cn->val.list, item, idx) {
			cell[0] = pcb_strdup(item->name);
			pcb_dad_tree_append(attr, NULL, cell);
		}
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		pcb_hid_attr_val_t hv;
		hv.str_value = cursor_path;
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wjobs, &hv);
	}
}

static void cam_gui_filter_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr_inp)
{
	cam_dlg_t *ctx = caller_data;
	pcb_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;
	const char *text;

	attr = &ctx->dlg[ctx->wjobs];
	tree = (pcb_hid_tree_t *)attr->enumerations;
	text = attr_inp->default_val.str_value;

	pcb_dad_tree_hide_all(tree, &tree->rows, 1);
	pcb_dad_tree_unhide_filter(tree, &tree->rows, 0, text);
	pcb_dad_tree_update_hide(attr);
}


static void cam_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	cam_dlg_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	free(ctx);
}

static int cam_gui(const char *arg)
{
	cam_dlg_t *ctx = calloc(sizeof(cam_dlg_t), 1);
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_HPANE(ctx->dlg);

			PCB_DAD_BEGIN_VBOX(ctx->dlg); /* left */
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_TREE(ctx->dlg, 1, 0, NULL);
					PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
					ctx->wjobs = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_BEGIN_HBOX(ctx->dlg); /* command section */
					PCB_DAD_STRING(ctx->dlg);
						PCB_DAD_HELP(ctx->dlg, "Filter text:\nlist jobs with matching name only");
						PCB_DAD_CHANGE_CB(ctx->dlg, cam_gui_filter_cb);
					PCB_DAD_BUTTON(ctx->dlg, "export!");
				PCB_DAD_END(ctx->dlg);
			PCB_DAD_END(ctx->dlg);

			PCB_DAD_BEGIN_VBOX(ctx->dlg); /* right */
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_BEGIN_VPANE(ctx->dlg);
					PCB_DAD_BEGIN_VBOX(ctx->dlg); /* top */
						PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
						PCB_DAD_TEXT(ctx->dlg, ctx);
						PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
					PCB_DAD_END(ctx->dlg);
					PCB_DAD_BEGIN_VBOX(ctx->dlg); /* bottom */
						PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
						PCB_DAD_LABEL(ctx->dlg, "TODO");
					PCB_DAD_END(ctx->dlg);
				PCB_DAD_END(ctx->dlg);
				PCB_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
			PCB_DAD_END(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);

	PCB_DAD_NEW("cam", ctx->dlg, "CAM export", ctx, pcb_false, cam_close_cb);

	cam_gui_jobs2dlg(ctx);

	return 0;
}
