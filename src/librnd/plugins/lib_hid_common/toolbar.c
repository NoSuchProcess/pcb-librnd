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

/* optional, generic toolbar docked DAD dialog that uses librnd tool infra
   (and in turn works from the menu file) */

#include <librnd/rnd_config.h>

#include <genvector/vti0.h>
#include <liblihata/tree.h>

#include <librnd/hid/hid.h>
#include <librnd/core/hid_cfg.h>
#include <librnd/hid/hid_dad.h>
#include <librnd/hid/tool.h>
#include <librnd/hid/hid_menu.h>
#include <librnd/core/rnd_conf.h>
#include <librnd/core/conf_hid.h>
#include <librnd/core/actions.h>

#include "toolbar.h"

typedef struct {
	rnd_hid_dad_subdialog_t sub;
	int sub_inited, lock;
	vti0_t tid2wid; /* tool ID to widget ID conversion - value 0 means no widget */
} toolbar_ctx_t;

static toolbar_ctx_t toolbar;

static void toolbar_design2dlg()
{
	rnd_toolid_t tid;

	if (!toolbar.sub_inited)
		return;

	toolbar.lock = 1;

	for(tid = 0; tid < toolbar.tid2wid.used; tid++) {
		int st, wid = toolbar.tid2wid.array[tid];
		if (wid == 0)
			continue;
		st = (tid == rnd_conf.editor.mode) ? 2 : 1;
		rnd_gui->attr_dlg_widget_state(toolbar.sub.dlg_hid_ctx, wid, st);
	}
	toolbar.lock = 0;
}

static void toolbar_select_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	ptrdiff_t tid;

	if (toolbar.lock)
		return;

	tid = (ptrdiff_t)attr->user_data;
	rnd_tool_select_by_id(rnd_gui->get_dad_design(hid_ctx), tid);
}

static void toolbar_create_tool(rnd_toolid_t tid, rnd_tool_t *tool, const char *menufile_help)
{
	int wid;
	const char *help = tool->help;

	if (menufile_help != NULL)
		help = menufile_help;

	if (tool->icon != NULL)
		RND_DAD_PICBUTTON(toolbar.sub.dlg, tool->icon);
	else
		RND_DAD_BUTTON(toolbar.sub.dlg, tool->name);
	RND_DAD_CHANGE_CB(toolbar.sub.dlg, toolbar_select_cb);
	RND_DAD_COMPFLAG(toolbar.sub.dlg, RND_HATF_TIGHT | RND_HATF_TOGGLE);
	if (help != NULL)
		RND_DAD_HELP(toolbar.sub.dlg, help);
	wid = RND_DAD_CURRENT(toolbar.sub.dlg);
	toolbar.sub.dlg[wid].user_data = (void *)(ptrdiff_t)tid;
	vti0_set(&toolbar.tid2wid, tid, wid);
}

static void toolbar_create_static(rnd_hid_cfg_t *cfg)
{
	const lht_node_t *t, *ts = rnd_hid_cfg_get_menu(cfg, "/toolbar_static");

	if ((ts != NULL) && (ts->type == LHT_LIST)) {
		for(t = ts->data.list.first; t != NULL; t = t->next) {
			rnd_toolid_t tid = rnd_tool_lookup(t->name);
			rnd_tool_t **tool;
			const char *mf_help;
			lht_node_t *nhelp;
			lht_err_t err;


			tool = (rnd_tool_t **)vtp0_get(&rnd_tools, tid, 0);
			if ((tid < 0) || (tool == NULL)) {
				rnd_message(RND_MSG_ERROR, "toolbar: tool '%s' not found (referenced from the menu file %s:%d)\n", t->name, t->file_name, t->line);
				continue;
			}

			nhelp = lht_tree_path_(t->doc, t, "tip", 1, 0, &err);
			if ((nhelp != NULL) && (nhelp->type == LHT_TEXT))
				mf_help = nhelp->data.text.value;
			else
				mf_help = NULL;
			toolbar_create_tool(tid, *tool, mf_help);
		}
	}
	else {
		RND_DAD_LABEL(toolbar.sub.dlg, "No toolbar found in the menu file.");
		RND_DAD_HELP(toolbar.sub.dlg, "Check your menu file. If you use a locally modified or custom\nmenu file, make sure you merge upstream changes\n(such as the new toolbar subtree)");
	}
}

static void toolbar_create_dyn_all(void)
{
	rnd_tool_t **t;
	rnd_toolid_t tid;
	for(tid = 0, t = (rnd_tool_t **)rnd_tools.array; tid < rnd_tools.used; tid++,t++) {
		int *wid = vti0_get(&toolbar.tid2wid, tid, 0);
		if (((*t)->flags & RND_TLF_AUTO_TOOLBAR) == 0)
			continue; /* static or inivisible */
		if ((wid != NULL) && (*wid != 0))
			continue; /* already has an icon */
		toolbar_create_tool(tid, *t, NULL);
	}
}

static void toolbar_docked_create(rnd_hid_cfg_t *cfg)
{
	toolbar.tid2wid.used = 0;

	RND_DAD_BEGIN_HBOX(toolbar.sub.dlg);
		RND_DAD_COMPFLAG(toolbar.sub.dlg, RND_HATF_EXPFILL | RND_HATF_TIGHT);

		toolbar_create_static(cfg);
		toolbar_create_dyn_all();

	/* eat up remaining space in the middle before displaying the dynamic tools */
		RND_DAD_BEGIN_HBOX(toolbar.sub.dlg);
			RND_DAD_COMPFLAG(toolbar.sub.dlg, RND_HATF_EXPFILL);
		RND_DAD_END(toolbar.sub.dlg);

	/* later on dynamic tools would be displayed here */

	RND_DAD_END(toolbar.sub.dlg);
}

static void toolbar_create(void)
{
	rnd_hid_cfg_t *cfg = rnd_gui->get_menu_cfg(rnd_gui);
	if (cfg == NULL)
		return;
	toolbar_docked_create(cfg);
	if (rnd_hid_dock_enter(&toolbar.sub, RND_HID_DOCK_TOP_LEFT, "Toolbar") == 0) {
		toolbar.sub_inited = 1;
		toolbar_design2dlg();
	}
}

void rnd_toolbar_gui_init_ev(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if ((RND_HAVE_GUI_ATTR_DLG) && (rnd_gui->get_menu_cfg != NULL))
		toolbar_create();
}

void rnd_toolbar_reg_ev(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if ((toolbar.sub_inited) && (argv[1].type == RND_EVARG_PTR)) {
		rnd_tool_t *tool = argv[1].d.p;
		rnd_toolid_t tid = rnd_tool_lookup(tool->name);
		if ((tool->flags & RND_TLF_AUTO_TOOLBAR) != 0) {
			int *wid = vti0_get(&toolbar.tid2wid, tid, 0);
			if ((wid != NULL) && (*wid != 0))
				return;
			rnd_hid_dock_leave(&toolbar.sub);
			toolbar.sub_inited = 0;
			toolbar_create();
		}
	}
}

void rnd_toolbar_update_conf(rnd_conf_native_t *cfg, int arr_idx, void *user_data)
{
	toolbar_design2dlg();
}



static const char *toolbar_cookie = "lib_hid_common/toolbar";

static rnd_conf_hid_id_t install_events(const char *cookie, const char *paths[], rnd_conf_hid_callbacks_t cb[], void (*update_cb)(rnd_conf_native_t*,int,void *))
{
	const char **rp;
	rnd_conf_native_t *nat;
	int n;
	rnd_conf_hid_id_t conf_id;

	conf_id = rnd_conf_hid_reg(cookie, NULL);
	for(rp = paths, n = 0; *rp != NULL; rp++, n++) {
		memset(&cb[n], 0, sizeof(cb[0]));
		cb[n].val_change_post = update_cb;
		nat = rnd_conf_get_field(*rp);
		if (nat != NULL)
			rnd_conf_hid_set_cb(nat, conf_id, &cb[n]);
	}

	return conf_id;
}

static int rnd_toolbar_inited = 0;

void rnd_toolbar_uninit(void)
{
	if (!rnd_toolbar_inited)
		return;

	rnd_event_unbind_allcookie(toolbar_cookie);
	rnd_conf_hid_unreg(toolbar_cookie);
}

void rnd_toolbar_init(void)
{
	const char *tpaths[] = {"editor/mode",  NULL};
	static rnd_conf_hid_callbacks_t tcb[sizeof(tpaths)/sizeof(tpaths[0])];

	if (rnd_toolbar_inited)
		return;

	rnd_event_bind(RND_EVENT_GUI_INIT, rnd_toolbar_gui_init_ev, NULL, toolbar_cookie);
	rnd_event_bind(RND_EVENT_TOOL_REG, rnd_toolbar_reg_ev, NULL, toolbar_cookie);
	install_events(toolbar_cookie, tpaths, tcb, rnd_toolbar_update_conf);
	rnd_toolbar_inited = 1;
}

const char rnd_acts_rnd_toolbar_init[] = "rnd_toolbar_init()\n";
const char rnd_acth_rnd_toolbar_init[] = "For ringdove apps: initialize the toolbar.";
fgw_error_t rnd_act_rnd_toolbar_init(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_toolbar_init();
	RND_ACT_IRES(0);
	return 0;
}

const char rnd_acts_rnd_toolbar_uninit[] = "rnd_toolbar_uninit()\n";
const char rnd_acth_rnd_toolbar_uninit[] = "For ringdove apps: uninitialize the toolbar.";
fgw_error_t rnd_act_rnd_toolbar_uninit(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_toolbar_uninit();
	RND_ACT_IRES(0);
	return 0;
}
