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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Preferences dialog, library tab */

#include <liblihata/tree.h>
#include "dlg_pref.h"
#include <librnd/core/conf.h>
#include "conf_core.h"
#include <librnd/core/paths.h>

static const char *SRC_BRD = "<board file>";

static void libhelp_btn(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);

static void pref_lib_update_buttons(void)
{
	rnd_hid_attribute_t *attr = &pref_ctx.dlg[pref_ctx.lib.wlist];
	pcb_hid_row_t *r = pcb_dad_tree_get_selected(attr);
	int en = (r != NULL);

	rnd_gui->attr_dlg_widget_state(pref_ctx.dlg_hid_ctx, pref_ctx.lib.wedit, en);
	rnd_gui->attr_dlg_widget_state(pref_ctx.dlg_hid_ctx, pref_ctx.lib.wremove, en);
	rnd_gui->attr_dlg_widget_state(pref_ctx.dlg_hid_ctx, pref_ctx.lib.wmoveup, en);
	rnd_gui->attr_dlg_widget_state(pref_ctx.dlg_hid_ctx, pref_ctx.lib.wmovedown, en);
}

static void pref_lib_select_cb(rnd_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	pref_lib_update_buttons();
}

static void pref_lib_row_free(rnd_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	free(row->cell[0]);
	free(row->cell[1]);
	free(row->cell[2]);
	row->cell[0] = row->cell[1] = row->cell[2] = NULL;
}

/* Current libraries from config to dialog box: remove everything from
   the widget first */
static void pref_lib_conf2dlg_pre(rnd_conf_native_t *cfg, int arr_idx)
{
	rnd_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;
	pcb_hid_row_t *r;

	if ((pref_ctx.lib.lock) || (!pref_ctx.active))
		return;

	attr = &pref_ctx.dlg[pref_ctx.lib.wlist];
	tree = attr->wdata;

	r = pcb_dad_tree_get_selected(attr);
	if (r != NULL) {
		free(pref_ctx.lib.cursor_path);
		pref_ctx.lib.cursor_path = rnd_strdup(r->cell[0]);
	}

	/* remove all existing entries */
	for(r = gdl_first(&tree->rows); r != NULL; r = gdl_first(&tree->rows)) {
		pcb_dad_tree_remove(attr, r);
	}
}

static const char *pref_node_src(lht_node_t *nd)
{
	if (nd->file_name != NULL)
		return nd->file_name;
	return rnd_conf_role_name(rnd_conf_lookup_role(nd));
}

/* Current libraries from config to dialog box: after the change, fill
   in all widget rows from the conf */
static void pref_lib_conf2dlg_post(rnd_conf_native_t *cfg, int arr_idx)
{
	rnd_conf_listitem_t *i;
	int idx;
	const char *s;
	char *cell[4];
	rnd_hid_attribute_t *attr;
	rnd_hid_attr_val_t hv;

	if ((pref_ctx.lib.lock) || (!pref_ctx.active))
		return;

	attr = &pref_ctx.dlg[pref_ctx.lib.wlist];

	/* copy everything from the config tree to the dialog */
	rnd_conf_loop_list_str(&conf_core.rc.library_search_paths, i, s, idx) {
		char *tmp;
		cell[0] = rnd_strdup(i->payload);
		pcb_path_resolve(&PCB->hidlib, cell[0], &tmp, 0, pcb_false);
		cell[1] = rnd_strdup(tmp == NULL ? "" : tmp);
		cell[2] = rnd_strdup(pref_node_src(i->prop.src));
		cell[3] = NULL;
		pcb_dad_tree_append(attr, NULL, cell);
	}

	hv.str = pref_ctx.lib.cursor_path;
	if (rnd_gui->attr_dlg_set_value(pref_ctx.dlg_hid_ctx, pref_ctx.lib.wlist, &hv) == 0) {
		free(pref_ctx.lib.cursor_path);
		pref_ctx.lib.cursor_path = NULL;
	}
	pref_lib_update_buttons();
}

TODO(": move this to liblihata")
static void lht_clean_list(lht_node_t * lst)
{
	lht_node_t *n;
	while (lst->data.list.first != NULL) {
		n = lst->data.list.first;
		if (n->doc == NULL) {
			if (lst->data.list.last == n)
				lst->data.list.last = NULL;
			lst->data.list.first = n->next;
		}
		else
			lht_tree_unlink(n);
		lht_dom_node_free(n);
	}
	lst->data.list.last = NULL;
}

/* Dialog box to current libraries in config */
static void pref_lib_dlg2conf(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	pcb_hid_tree_t *tree = attr->wdata;
	lht_node_t *m, *lst, *nd;
	pcb_hid_row_t *r;

	ctx->lib.lock++;

	/* get the list and clean it */
	m = rnd_conf_lht_get_first(ctx->role, 0);
	lst = lht_tree_path_(m->doc, m, "rc/library_search_paths", 1, 0, NULL);
	if (lst == NULL)
		rnd_conf_set(ctx->role, "rc/library_search_paths", 0, "", RND_POL_OVERWRITE);
	lst = lht_tree_path_(m->doc, m, "rc/library_search_paths", 1, 0, NULL);
	assert(lst != NULL);
	lht_clean_list(lst);

	/* append items from the widget */
	for(r = gdl_first(&tree->rows); r != NULL; r = gdl_next(&tree->rows, r)) {
		nd = lht_dom_node_alloc(LHT_TEXT, "");
		nd->data.text.value = rnd_strdup(r->cell[0]);
		nd->doc = m->doc;
		lht_dom_list_append(lst, nd);
		pcb_dad_tree_modify_cell(attr, r, 2, rnd_strdup(pref_node_src(nd)));
	}

	rnd_conf_update("rc/library_search_paths", -1);
	rnd_conf_makedirty(ctx->role); /* low level lht_dom_node_alloc() wouldn't make user config to be saved! */
	if (ctx->role == RND_CFR_DESIGN)
		pcb_board_set_changed_flag(1);

	ctx->lib.lock--;
}

static void lib_btn_remove(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *btn_attr)
{
	rnd_hid_attribute_t *attr = &pref_ctx.dlg[pref_ctx.lib.wlist];
	pcb_hid_row_t *r = pcb_dad_tree_get_selected(attr);

	if (r == NULL)
		return;

	if (pcb_dad_tree_remove(attr, r) == 0) {
		pref_lib_dlg2conf(hid_ctx, caller_data, attr);
		pref_lib_update_buttons();
	}
}

static void lib_btn_up(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *btn_attr)
{
	rnd_hid_attribute_t *attr = &pref_ctx.dlg[pref_ctx.lib.wlist];
	pcb_hid_row_t *prev, *r = pcb_dad_tree_get_selected(attr);
	pcb_hid_tree_t *tree = attr->wdata;
	char *cell[4];

	if (r == NULL)
		return;

	prev = gdl_prev(&tree->rows, r);
	if (prev == NULL)
		return;

	cell[0] = rnd_strdup(r->cell[0]); /* have to copy because this is also the primary key (path for the hash) */
	cell[1] = r->cell[1]; r->cell[1] = NULL;
	cell[2] = r->cell[2]; r->cell[2] = NULL;
	cell[3] = NULL;
	if (pcb_dad_tree_remove(attr, r) == 0) {
		rnd_hid_attr_val_t hv;
		pcb_dad_tree_insert(attr, prev, cell);
		pref_lib_dlg2conf(hid_ctx, caller_data, attr);
		hv.str = cell[0];
		rnd_gui->attr_dlg_set_value(pref_ctx.dlg_hid_ctx, pref_ctx.lib.wlist, &hv);
	}
}

static void lib_btn_down(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *btn_attr)
{
	rnd_hid_attribute_t *attr = &pref_ctx.dlg[pref_ctx.lib.wlist];
	pcb_hid_row_t *next, *r = pcb_dad_tree_get_selected(attr);
	pcb_hid_tree_t *tree = attr->wdata;
	char *cell[4];

	if (r == NULL)
		return;

	next = gdl_next(&tree->rows, r);
	if (next == NULL)
		return;

	cell[0] = rnd_strdup(r->cell[0]); /* have to copy because this is also the primary key (path for the hash) */
	cell[1] = r->cell[1]; r->cell[1] = NULL;
	cell[2] = r->cell[2]; r->cell[2] = NULL;
	cell[3] = NULL;
	if (pcb_dad_tree_remove(attr, r) == 0) {
		rnd_hid_attr_val_t hv;
		pcb_dad_tree_append(attr, next, cell);
		pref_lib_dlg2conf(hid_ctx, caller_data, attr);
		hv.str = cell[0];
		rnd_gui->attr_dlg_set_value(pref_ctx.dlg_hid_ctx, pref_ctx.lib.wlist, &hv);
	}
}

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	int wpath, wexp;
	pref_ctx_t *pctx;
} cell_edit_ctx_t;

static void lib_cell_edit_update(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *btn_attr)
{
	cell_edit_ctx_t *ctx = caller_data;
	char *tmp;

	pcb_path_resolve(&PCB->hidlib, ctx->dlg[ctx->wpath].val.str, &tmp, 0, pcb_true);
	if (tmp != NULL)
		PCB_DAD_SET_VALUE(hid_ctx, ctx->wexp, str, tmp);
}

static int lib_cell_edit(pref_ctx_t *pctx, char **cell)
{
	cell_edit_ctx_t ctx;
	pcb_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {"ok", 0}, {NULL, 0}};

	memset(&ctx, 0, sizeof(ctx));
	ctx.pctx = pctx;

	PCB_DAD_BEGIN_VBOX(ctx.dlg);
		PCB_DAD_BEGIN_TABLE(ctx.dlg, 2);
			PCB_DAD_LABEL(ctx.dlg, "Path:");
			PCB_DAD_STRING(ctx.dlg);
				ctx.wpath = PCB_DAD_CURRENT(ctx.dlg);
				ctx.dlg[ctx.wpath].val.str = rnd_strdup(cell[0]);
				PCB_DAD_CHANGE_CB(ctx.dlg, lib_cell_edit_update);

			PCB_DAD_LABEL(ctx.dlg, "Expanded\nversion:");
			PCB_DAD_LABEL(ctx.dlg, rnd_strdup(cell[1]));
				ctx.wexp = PCB_DAD_CURRENT(ctx.dlg);
				ctx.dlg[ctx.wexp].val.str = rnd_strdup(cell[1]);

			PCB_DAD_LABEL(ctx.dlg, "");
			PCB_DAD_BUTTON(ctx.dlg, "Help...");
				PCB_DAD_CHANGE_CB(ctx.dlg, libhelp_btn);
		PCB_DAD_END(ctx.dlg);
		PCB_DAD_BUTTON_CLOSES(ctx.dlg, clbtn);
	PCB_DAD_END(ctx.dlg);

	PCB_DAD_NEW("pref_lib_path", ctx.dlg, "Edit library path", &ctx, pcb_true, NULL);
	if (PCB_DAD_RUN(ctx.dlg) != 0) {
		PCB_DAD_FREE(ctx.dlg);
		return -1;
	}

	free(cell[0]);
	cell[0] = rnd_strdup(PCB_EMPTY(ctx.dlg[ctx.wpath].val.str));
	free(cell[1]);
	cell[1] = rnd_strdup(PCB_EMPTY(ctx.dlg[ctx.wexp].val.str));

	PCB_DAD_FREE(ctx.dlg);
	return 0;
}

static void lib_btn_insert(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *btn_attr, int pos)
{
	pref_ctx_t *ctx = caller_data;
	rnd_hid_attribute_t *attr = &pref_ctx.dlg[pref_ctx.lib.wlist];
	pcb_hid_row_t *r = pcb_dad_tree_get_selected(attr);
	char *cell[4];

	if (r == NULL) {
		pcb_hid_tree_t *tree = attr->wdata;

		switch(pos) {
			case 0: /* replace */
				rnd_message(RND_MSG_ERROR, "need to select a library path row first\n");
				return;

			case -1: /* before */
				r = gdl_first(&tree->rows);
				break;
			case +1: /* after */
				r = gdl_last(&tree->rows);
				break;
		}
	}

	if (pos != 0) {
		cell[0] = rnd_strdup("");
		cell[1] = rnd_strdup("");
		cell[2] = rnd_strdup(SRC_BRD);
	}
	else {
		cell[0] = rnd_strdup(r->cell[0]);
		cell[1] = rnd_strdup(r->cell[1]);
		cell[2] = rnd_strdup(r->cell[2]);
	}
	cell[3] = NULL;

	if (lib_cell_edit(ctx, cell) != 0) {
		free(cell[0]);
		free(cell[1]);
		free(cell[2]);
		return;
	}

	switch(pos) {
		case -1: /* before */
			pcb_dad_tree_insert(attr, r, cell);
			break;
		case +1: /* after */
			pcb_dad_tree_append(attr, r, cell);
			break;
		case 0: /* replace */
			pcb_dad_tree_modify_cell(attr, r, 0, cell[0]);
			pcb_dad_tree_modify_cell(attr, r, 1, cell[1]);
			pcb_dad_tree_modify_cell(attr, r, 2, cell[2]);
			break;
	}
	pref_lib_dlg2conf(hid_ctx, caller_data, attr);
}

static void lib_btn_insert_before(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *btn_attr)
{
	lib_btn_insert(hid_ctx, caller_data, btn_attr, -1);
}

static void lib_btn_insert_after(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *btn_attr)
{
	lib_btn_insert(hid_ctx, caller_data, btn_attr, +1);
}

static void lib_btn_edit(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *btn_attr)
{
	lib_btn_insert(hid_ctx, caller_data, btn_attr, 0);
}

void pcb_dlg_pref_lib_close(pref_ctx_t *ctx)
{
	if (ctx->lib.help.active)
		PCB_DAD_FREE(ctx->lib.help.dlg);
}

static void pref_libhelp_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
/*	pref_libhelp_ctx_t *ctx = caller_data;*/
}

static void pref_libhelp_open(pref_libhelp_ctx_t *ctx)
{
	htsp_entry_t *e;
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	if (ctx->active)
		return;

	PCB_DAD_LABEL(ctx->dlg, "The following $(variables) can be used in the path:");
		PCB_DAD_BEGIN_TABLE(ctx->dlg, 2);
			rnd_conf_fields_foreach(e) {
				rnd_conf_native_t *nat = e->value;
				char tmp[256];

				if (strncmp(e->key, "rc/path/", 8) != 0)
					continue;

				pcb_snprintf(tmp, sizeof(tmp), "$(rc.path.%s)", e->key + 8);
				PCB_DAD_LABEL(ctx->dlg, tmp);
				PCB_DAD_LABEL(ctx->dlg, nat->val.string[0] == NULL ? "" : nat->val.string[0]);
			}
		PCB_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
	PCB_DAD_END(ctx->dlg);

	ctx->active = 1;
	PCB_DAD_NEW("pref_lib_path_help", ctx->dlg, "pcb-rnd preferences: library help", ctx, pcb_true, pref_libhelp_close_cb);

	PCB_DAD_RUN(ctx->dlg);
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(pref_libhelp_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void libhelp_btn(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pref_libhelp_open(&pref_ctx.lib.help);
}

void pcb_dlg_pref_lib_create(pref_ctx_t *ctx)
{
	static const char *hdr[] = {"configured path", "actual path on the filesystem", "config source", NULL};
	pcb_hid_tree_t *tree;

	PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL); /* get the parent vbox, which is the tab's page vbox, to expand and fill */

	PCB_DAD_LABEL(ctx->dlg, "Ordered list of footprint library search directories.");

	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_FRAME | PCB_HATF_SCROLL | PCB_HATF_EXPFILL);
		PCB_DAD_TREE(ctx->dlg, 3, 0, hdr);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			ctx->lib.wlist = PCB_DAD_CURRENT(ctx->dlg);
			tree = ctx->dlg[ctx->lib.wlist].wdata;
			tree->user_free_cb = pref_lib_row_free;
			PCB_DAD_TREE_SET_CB(ctx->dlg, selected_cb, pref_lib_select_cb);
	PCB_DAD_END(ctx->dlg);

	PCB_DAD_BEGIN_HBOX(ctx->dlg);
		PCB_DAD_BUTTON(ctx->dlg, "Move up");
			PCB_DAD_CHANGE_CB(ctx->dlg, lib_btn_up);
			ctx->lib.wmoveup = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_BUTTON(ctx->dlg, "Move down");
			PCB_DAD_CHANGE_CB(ctx->dlg, lib_btn_down);
			ctx->lib.wmovedown = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_BUTTON(ctx->dlg, "Insert before");
			PCB_DAD_CHANGE_CB(ctx->dlg, lib_btn_insert_before);
		PCB_DAD_BUTTON(ctx->dlg, "Insert after");
			PCB_DAD_CHANGE_CB(ctx->dlg, lib_btn_insert_after);
		PCB_DAD_BUTTON(ctx->dlg, "Remove");
			PCB_DAD_CHANGE_CB(ctx->dlg, lib_btn_remove);
			ctx->lib.wremove = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_BUTTON(ctx->dlg, "Edit...");
			PCB_DAD_CHANGE_CB(ctx->dlg, lib_btn_edit);
			ctx->lib.wedit = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_BUTTON(ctx->dlg, "Help...");
			ctx->lib.whsbutton = PCB_DAD_CURRENT(ctx->dlg);
			PCB_DAD_CHANGE_CB(ctx->dlg, libhelp_btn);
	PCB_DAD_END(ctx->dlg);

}

void pcb_dlg_pref_lib_open(pref_ctx_t *ctx)
{
	rnd_conf_native_t *cn = rnd_conf_get_field("rc/library_search_paths");
	pref_lib_conf2dlg_post(cn, -1);
}

void pcb_dlg_pref_lib_init(pref_ctx_t *ctx)
{
	static rnd_conf_hid_callbacks_t cbs_spth;
	rnd_conf_native_t *cn = rnd_conf_get_field("rc/library_search_paths");

	if (cn != NULL) {
		memset(&cbs_spth, 0, sizeof(rnd_conf_hid_callbacks_t));
		cbs_spth.val_change_pre = pref_lib_conf2dlg_pre;
		cbs_spth.val_change_post = pref_lib_conf2dlg_post;
		rnd_conf_hid_set_cb(cn, pref_hid, &cbs_spth);
	}
}
