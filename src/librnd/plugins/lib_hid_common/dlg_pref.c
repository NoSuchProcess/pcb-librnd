/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2021 Tibor 'Igor2' Palinkas
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

/* The preferences dialog, invoked by the Preferences() action */

#include <librnd/rnd_config.h>

#include <genvector/gds_char.h>
#include <limits.h>
#include <ctype.h>
#include <librnd/core/actions.h>
#include <librnd/core/globalconst.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/error.h>
#include <librnd/core/event.h>
#include <librnd/core/conf.h>
#include <librnd/core/conf_hid.h>
#include <librnd/core/hidlib.h>
#include <librnd/hid/hid_dad.h>
#include <librnd/hid/hid_dad_tree.h>

#include <librnd/plugins/lib_hid_common/dlg_pref.h>

static const char *pref_tabs[RND_PREF_MAX_TAB];
static int pref_tab_cfgs[RND_PREF_MAX_TAB];

static const char *bi_pref_tabs[]   = { "Window", "Key", "Menu", "Config tree", NULL };
static const int bi_pref_tab_cfgs[] = {  1,        0,     0,          0      };


void rnd_pref_init_func_dummy(pref_ctx_t *ctx, int tab) { }

/* built-in tabs */
#include "dlg_pref_win.c"
#include "dlg_pref_key.c"
#include "dlg_pref_menu.c"
#include "dlg_pref_conf.c"

#define call_hook(pctx, dsg, HOOK) \
	do { \
		int t; \
		for(t = 0; t < (pctx)->tabs; t++) \
			if ((pctx)->tab[t].hooks->HOOK != NULL) \
				(pctx)->tab[t].hooks->HOOK(pctx, dsg); \
	} while(0)

pref_ctx_t pref_ctx;
static const char *pref_cookie = "preferences dialog";
rnd_conf_hid_id_t pref_hid;

static const char *role_names[] =  { "user",   "project",   "design",   "cli", NULL };
static const rnd_conf_role_t roles[] = { RND_CFR_USER, RND_CFR_PROJECT, RND_CFR_DESIGN, RND_CFR_CLI, 0 };

pref_ctx_t *rnd_pref_get_ctx(rnd_design_t *hidlib)
{
	return &pref_ctx;
}

lht_node_t *rnd_pref_dlg2conf_pre(rnd_design_t *hidlib, pref_ctx_t *ctx)
{
	return rnd_pref_ensure_conf_root(hidlib, ctx->role);
}


void rnd_pref_dlg2conf_post(rnd_design_t *hidlib, pref_ctx_t *ctx)
{
	if ((ctx->role == RND_CFR_USER) || (ctx->role == RND_CFR_PROJECT))
		rnd_conf_save_file(hidlib, NULL, (hidlib == NULL ? NULL : hidlib->loadname), ctx->role, NULL);
	else if (ctx->role == RND_CFR_DESIGN)
		rnd_event(hidlib, RND_EVENT_DESIGN_META_CHANGED, NULL);
}

void rnd_pref_conf2dlg_item(rnd_conf_native_t *cn, pref_confitem_t *item)
{
	switch(cn->type) {
		case RND_CFN_COORD:
			RND_DAD_SET_VALUE(pref_ctx.dlg_hid_ctx, item->wid, crd, cn->val.coord[0]);
			break;
		case RND_CFN_BOOLEAN:
		case RND_CFN_INTEGER:
			RND_DAD_SET_VALUE(pref_ctx.dlg_hid_ctx, item->wid, lng, cn->val.integer[0]);
			break;
		case RND_CFN_REAL:
			RND_DAD_SET_VALUE(pref_ctx.dlg_hid_ctx, item->wid, dbl, cn->val.real[0]);
			break;
		case RND_CFN_STRING:
			RND_DAD_SET_VALUE(pref_ctx.dlg_hid_ctx, item->wid, str, cn->val.string[0]);
			break;
		default: rnd_message(RND_MSG_ERROR, "rnd_pref_conf2dlg_item(): widget type not handled\n");
	}
}

void rnd_pref_dlg2conf_item(pref_ctx_t *ctx, pref_confitem_t *item, rnd_hid_attribute_t *attr)
{
	pref_confitem_t *old = ctx->rnd_conf_lock;
	rnd_conf_native_t *cn = rnd_conf_get_field(item->confpath);

	if (cn == NULL)
		return;

	ctx->rnd_conf_lock = item;
	switch(cn->type) {
		case RND_CFN_COORD:
			if (cn->val.coord[0] != attr->val.crd)
				rnd_conf_setf(ctx->role, item->confpath, -1, "%.8$mm", attr->val.crd);
			break;
		case RND_CFN_BOOLEAN:
		case RND_CFN_INTEGER:
			if (cn->val.integer[0] != attr->val.lng)
				rnd_conf_setf(ctx->role, item->confpath, -1, "%d", attr->val.lng);
			break;
		case RND_CFN_REAL:
			if (cn->val.real[0] != attr->val.dbl)
				rnd_conf_setf(ctx->role, item->confpath, -1, "%f", attr->val.dbl);
			break;
		case RND_CFN_STRING:
			if (strcmp(cn->val.string[0], attr->val.str) != 0)
				rnd_conf_set(ctx->role, item->confpath, -1, attr->val.str, RND_POL_OVERWRITE);
			break;
		default: rnd_message(RND_MSG_ERROR, "rnd_pref_dlg2conf_item(): widget type not handled\n");
	}
	ctx->rnd_conf_lock = old;
}

rnd_bool rnd_pref_dlg2conf_table(pref_ctx_t *ctx, pref_confitem_t *list, rnd_hid_attribute_t *attr)
{
	pref_confitem_t *c;
	int wid = attr - ctx->dlg;

	for(c = list; c->confpath != NULL; c++) {
		if (c->wid == wid) {
			rnd_pref_dlg2conf_item(ctx, c, attr);
			return 1;
		}
	}
	return 0;
}


void rnd_pref_create_conf_item(pref_ctx_t *ctx, pref_confitem_t *item, void (*change_cb)(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr))
{
	rnd_conf_native_t *cn = rnd_conf_get_field(item->confpath);

	if (cn == NULL) {
		rnd_message(RND_MSG_ERROR, "Internal error: rnd_pref_create_conf_item(): invalid conf node %s\n", item->confpath);
		item->wid = -1;
		return;
	}

	RND_DAD_LABEL(ctx->dlg, item->label);
		RND_DAD_HELP(ctx->dlg, cn->description);

	switch(cn->type) {
		case RND_CFN_COORD:
			RND_DAD_COORD(ctx->dlg);
				item->wid = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_MINMAX(ctx->dlg, 0, RND_MAX_COORD);
				RND_DAD_DEFAULT_NUM(ctx->dlg, cn->val.coord[0]);
				RND_DAD_HELP(ctx->dlg, cn->description);
				RND_DAD_CHANGE_CB(ctx->dlg, change_cb);
			break;
		case RND_CFN_BOOLEAN:
			RND_DAD_BOOL(ctx->dlg);
				item->wid = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_DEFAULT_NUM(ctx->dlg, cn->val.integer[0]);
				RND_DAD_HELP(ctx->dlg, cn->description);
				RND_DAD_CHANGE_CB(ctx->dlg, change_cb);
			break;
		case RND_CFN_INTEGER:
			RND_DAD_INTEGER(ctx->dlg);
				item->wid = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_MINMAX(ctx->dlg, 0, INT_MAX);
				RND_DAD_DEFAULT_NUM(ctx->dlg, cn->val.integer[0]);
				RND_DAD_HELP(ctx->dlg, cn->description);
				RND_DAD_CHANGE_CB(ctx->dlg, change_cb);
			break;
		case RND_CFN_REAL:
			RND_DAD_REAL(ctx->dlg);
				item->wid = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_MINMAX(ctx->dlg, 0, INT_MAX);
				ctx->dlg[item->wid].val.dbl = cn->val.real[0];
				RND_DAD_HELP(ctx->dlg, cn->description);
				RND_DAD_CHANGE_CB(ctx->dlg, change_cb);
			break;
		case RND_CFN_STRING:
			RND_DAD_STRING(ctx->dlg);
				item->wid = RND_DAD_CURRENT(ctx->dlg);
				ctx->dlg[item->wid].val.str = rnd_strdup(cn->val.string[0]);
				RND_DAD_HELP(ctx->dlg, cn->description);
				RND_DAD_CHANGE_CB(ctx->dlg, change_cb);
			break;
		default:
			RND_DAD_LABEL(ctx->dlg, "Internal error: rnd_pref_create_conf_item(): unhandled type");
			item->wid = -1;
			return;
	}

	item->cnext = rnd_conf_hid_get_data(cn, pref_hid);
	rnd_conf_hid_set_data(cn, pref_hid, item);
}

void rnd_pref_create_conftable(pref_ctx_t *ctx, pref_confitem_t *list, void (*change_cb)(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr))
{
	pref_confitem_t *c;
	for(c = list; c->confpath != NULL; c++)
		rnd_pref_create_conf_item(ctx, c, change_cb);
}

void rnd_pref_conflist_remove(pref_ctx_t *ctx, pref_confitem_t *list)
{
	pref_confitem_t *c;
	for(c = list; c->confpath != NULL; c++) {
		rnd_conf_native_t *cn = rnd_conf_get_field(c->confpath);
		c->cnext = NULL;
		if (cn != NULL)
			rnd_conf_hid_set_data(cn, pref_hid, NULL);
	}
}

static void pref_role_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	ctx->role = roles[attr->val.lng];
}

static void pref_tab_chosen(pref_ctx_t *ctx, int tab)
{
	rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wrolebox, !pref_tab_cfgs[tab]);
}

static void pref_tab_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pref_tab_chosen(caller_data, attr->val.lng);
}

static void pref_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	long n;
	pref_ctx_t *ctx = caller_data;

	call_hook(ctx, rnd_multi_get_current(), close_cb);

	rnd_dlg_pref_win_close(ctx);
	rnd_dlg_pref_menu_close(ctx);

	for(n = 0; n < ctx->auto_free.used; n++)
		free(ctx->auto_free.array[n]);
	vtp0_uninit(&ctx->auto_free);

	RND_DAD_FREE(ctx->dlg);

	ctx->active = 0;
	ctx->rnd_conf_lock = NULL;
}

/* Compare inp to fixed in case insensitive manner; accept full length match
   or if inp contains the first word of fixed. Returns 0 on accept. */
static int pref_strcmp(const char *fixed, const char *inp)
{
	for(;;) {
		if ((*inp == '\0') && ((*fixed == '\0') || (*fixed == ' ')))
			return 0;
		if (tolower(*fixed) != tolower(*inp))
			return -1;
		fixed++;
		inp++;
	}
}

static void rnd_dlg_pref(rnd_design_t *dsg, const char *target_tab_str, const char *tabarg)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	int target_tab = -1, t;

	if ((target_tab_str != NULL) && (*target_tab_str != '\0')) {
		const char **t;
		int tt;

		for(tt = 0, t = pref_tabs; *t != NULL; t++,tt++) {
			if (pref_strcmp(*t, target_tab_str) == 0) {
				target_tab = tt;
				break;
			}
		}
	}

	if (pref_ctx.active) {
		if (target_tab >= 0)
			RND_DAD_SET_VALUE(pref_ctx.dlg_hid_ctx, pref_ctx.wtab, lng, target_tab);
		return;
	}

	RND_DAD_BEGIN_VBOX(pref_ctx.dlg);
		RND_DAD_COMPFLAG(pref_ctx.dlg, RND_HATF_EXPFILL);
		RND_DAD_BEGIN_TABBED(pref_ctx.dlg, pref_tabs);
			RND_DAD_COMPFLAG(pref_ctx.dlg, RND_HATF_EXPFILL);
			pref_ctx.wtab = RND_DAD_CURRENT(pref_ctx.dlg);
			RND_DAD_CHANGE_CB(pref_ctx.dlg, pref_tab_cb);

			/* create application tabs */
			for(t = 0; t < pref_ctx.tabs; t++) {
				if (pref_ctx.tab[t].hooks->create_cb != NULL) {
					RND_DAD_BEGIN_VBOX(pref_ctx.dlg);
						pref_ctx.tab[t].hooks->create_cb(&pref_ctx, dsg);
					RND_DAD_END(pref_ctx.dlg);
				}
			}

			/* create rnd built-in tabs */
			RND_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Window */
				rnd_dlg_pref_win_create(&pref_ctx);
			RND_DAD_END(pref_ctx.dlg);

			RND_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Key */
				rnd_dlg_pref_key_create(&pref_ctx);
			RND_DAD_END(pref_ctx.dlg);

			RND_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Menu */
				rnd_dlg_pref_menu_create(&pref_ctx);
			RND_DAD_END(pref_ctx.dlg);

			RND_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Config tree */
				rnd_dlg_pref_conf_create(&pref_ctx);
			RND_DAD_END(pref_ctx.dlg);

		RND_DAD_END(pref_ctx.dlg);
		RND_DAD_BEGIN_VBOX(pref_ctx.dlg);
			RND_DAD_BEGIN_HBOX(pref_ctx.dlg);
				RND_DAD_COMPFLAG(pref_ctx.dlg, RND_HATF_EXPFILL);
				RND_DAD_BEGIN_HBOX(pref_ctx.dlg);
					pref_ctx.wrolebox = RND_DAD_CURRENT(pref_ctx.dlg);
					RND_DAD_LABEL(pref_ctx.dlg, "All changes are made to role:");
					RND_DAD_ENUM(pref_ctx.dlg, role_names);
						pref_ctx.wrole = RND_DAD_CURRENT(pref_ctx.dlg);
						RND_DAD_CHANGE_CB(pref_ctx.dlg, pref_role_cb);
				RND_DAD_END(pref_ctx.dlg);
				RND_DAD_BUTTON_CLOSES_NAKED(pref_ctx.dlg, clbtn);
			RND_DAD_END(pref_ctx.dlg);
		RND_DAD_END(pref_ctx.dlg);
	RND_DAD_END(pref_ctx.dlg);

	/* set up the context */
	pref_ctx.active = 1;

	RND_DAD_NEW("preferences", pref_ctx.dlg, "preferences", &pref_ctx, rnd_false, pref_close_cb);

	RND_DAD_SET_VALUE(pref_ctx.dlg_hid_ctx, pref_ctx.wrole, lng, 2);
	pref_ctx.role = RND_CFR_DESIGN;

	{ /* call the open hook; special case because it may need tabarg passed */
		int t;
		for(t = 0; t < pref_ctx.tabs; t++)
			if (pref_ctx.tab[t].hooks->open_cb != NULL)
				pref_ctx.tab[t].hooks->open_cb(&pref_ctx, dsg, ((target_tab == t) ? tabarg : NULL));
	}

	rnd_dlg_pref_win_open(&pref_ctx);
	rnd_dlg_pref_key_open(&pref_ctx);
	rnd_dlg_pref_menu_open(&pref_ctx);
	rnd_dlg_pref_conf_open(&pref_ctx, (target_tab == pref_ctx.tabs_total - 1) ? tabarg : NULL);
	if ((target_tab >= 0) && (target_tab < pref_ctx.tabs_total)) {
		RND_DAD_SET_VALUE(pref_ctx.dlg_hid_ctx, pref_ctx.wtab, lng, target_tab);
		pref_tab_chosen(&pref_ctx, target_tab);
	}
	else
		pref_tab_chosen(&pref_ctx, 0);
}

static void pref_ev_design_replaced(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pref_ctx_t *ctx = user_data;
	if (!pref_ctx.active)
		return;

	call_hook(ctx, hidlib, design_replaced_cb);
	pref_win_brd2dlg(ctx);

	/* builtin tabs */
	rnd_dlg_pref_conf_design_replaced(ctx);
	rnd_dlg_pref_win_design_replaced(ctx);
}

static void pref_ev_board_meta_changed(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pref_ctx_t *ctx = user_data;
	if (!pref_ctx.active)
		return;

	call_hook(ctx, hidlib, meta_changed_cb);
	pref_win_brd2dlg(ctx);
}

static void pref_ev_menu_changed(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pref_ctx_t *ctx = user_data;
	if (!pref_ctx.active)
		return;
	pref_menu_brd2dlg(ctx);
}

void pref_conf_changed(rnd_conf_native_t *cfg, int arr_idx, void *user_data)
{
	pref_confitem_t *i;

	if (!pref_ctx.active)
		return;

	for(i = rnd_conf_hid_get_data(cfg, pref_hid); i != NULL; i = i->cnext)
		if (i != pref_ctx.rnd_conf_lock)
			rnd_pref_conf2dlg_item(cfg, i);

	rnd_pref_dlg_conf_changed_cb(&pref_ctx, cfg, arr_idx);
}

static rnd_conf_hid_callbacks_t pref_conf_cb;
void rnd_dlg_pref_init(int pref_tab, void (*first_init)(pref_ctx_t *ctx, int tab))
{
	int i, t;

	pref_conf_cb.val_change_post = pref_conf_changed;
	rnd_event_bind(RND_EVENT_DESIGN_SET_CURRENT, pref_ev_design_replaced, &pref_ctx, pref_cookie);
	rnd_event_bind(RND_EVENT_DESIGN_META_CHANGED, pref_ev_board_meta_changed, &pref_ctx, pref_cookie);
	rnd_event_bind(RND_EVENT_MENU_CHANGED, pref_ev_menu_changed, &pref_ctx, pref_cookie);
	pref_hid = rnd_conf_hid_reg(pref_cookie, &pref_conf_cb);
	if (first_init != NULL) {
		first_init(&pref_ctx, pref_tab);
		pref_ctx.tabs = pref_tab+1;
	}

	for(t = 0; t < pref_ctx.tabs; t++) {
		pref_tabs[t] = pref_ctx.tab[t].hooks->tab_label;
		pref_tab_cfgs[t] = !!(pref_ctx.tab[t].hooks->flags & RND_PREFTAB_NEEDS_ROLE);
	}
	for(i = 0; bi_pref_tabs[i] != NULL; i++,t++) {
		pref_tabs[t] = bi_pref_tabs[i];
		pref_tab_cfgs[t] = bi_pref_tab_cfgs[i];
	}
	pref_ctx.tabs_total = t;
	pref_ctx.tabs_inited = 1;
}

void rnd_dlg_pref_uninit(void)
{
	int t;

	rnd_event_unbind_allcookie(pref_cookie);
	rnd_conf_hid_unreg(pref_cookie);

	for(t = 0; t < pref_ctx.tabs; t++) {
		if (pref_ctx.tab[t].hooks->flags & RND_PREFTAB_AUTO_FREE_DATA) {
			free(pref_ctx.tab[t].tabdata);
			pref_ctx.tab[t].tabdata = NULL;
		}
	}
}

const char rnd_acts_Preferences[] = "Preferences([tabname])\n";
const char rnd_acth_Preferences[] = "Present the preferences dialog, optionally opening the tab requested.";
fgw_error_t rnd_act_Preferences(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *tab = NULL, *tabarg = NULL;
	RND_ACT_MAY_CONVARG(1, FGW_STR, Preferences, tab = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, Preferences, tabarg = argv[2].val.str);

	if (!pref_ctx.tabs_inited)
		rnd_dlg_pref_init(0, NULL);

	/* The preferences dialog is always following the currently active design;
	   open for that, even if action design differs */
	rnd_dlg_pref(rnd_multi_get_current(), tab, tabarg);

	RND_ACT_IRES(0);
	return 0;
}
