/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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

#include <genvector/vti0.h>

#include "hid.h"
#include "hid_cfg.h"
#include "hid_dad.h"
#include "tool.h"
#include "conf_core.h"

#include "toolbar.h"

typedef struct {
	pcb_hid_dad_subdialog_t sub;
	int sub_inited;
	vti0_t tid2wid; /* tool ID to widget ID conversion - value 0 means no widget */
} toolbar_ctx_t;

static toolbar_ctx_t toolbar;

static void toolbar_create_static(pcb_hid_cfg_t *cfg)
{
	const lht_node_t *t, *ts = pcb_hid_cfg_get_menu(cfg, "/toolbar_static");

	if ((ts != NULL) || (ts->type != LHT_LIST)) {
		for(t = ts->data.list.first; t != NULL; t = t->next) {
			int wid;
			pcb_toolid_t tid = pcb_tool_lookup(t->name);
			pcb_tool_t **tool;

			tool = vtp0_get(&pcb_tools, tid, 0);
			if ((tid < 0) || (tool == NULL)) {
				pcb_message(PCB_MSG_ERROR, "toolbar: tool '%s' not found (referenced from the menu file %s:%d)\n", t->name, t->file_name, t->line);
				continue;
			}
			if ((*tool)->icon != NULL)
				PCB_DAD_PICBUTTON(toolbar.sub.dlg, (*tool)->icon);
			else
				PCB_DAD_BUTTON(toolbar.sub.dlg, t->name);
			PCB_DAD_COMPFLAG(toolbar.sub.dlg, PCB_HATF_TIGHT | PCB_HATF_TOGGLE);
			PCB_DAD_HELP(toolbar.sub.dlg, "TODO: tooltip");
			wid = PCB_DAD_CURRENT(toolbar.sub.dlg);
			vti0_set(&toolbar.tid2wid, tid, wid);
		}
	}
	else {
		PCB_DAD_LABEL(toolbar.sub.dlg, "No toolbar found in the menu file.");
		PCB_DAD_HELP(toolbar.sub.dlg, "Check your menu file. If you use a locally modified or custom\nmenu file, make sure you merge upstream changes\n(such as the new toolbar subtree)");
	}
}

static void toolbar_pcb2dlg()
{
	pcb_toolid_t tid;

	if (!toolbar.sub_inited)
		return;

	for(tid = 0; tid < toolbar.tid2wid.used; tid++) {
		int st, wid = toolbar.tid2wid.array[tid];
		if (wid == 0)
			continue;
		st = (tid == conf_core.editor.mode) ? 2 : 1;
		pcb_gui->attr_dlg_widget_state(toolbar.sub.dlg_hid_ctx, wid, st);
	}
}

static void toolbar_docked_create(pcb_hid_cfg_t *cfg)
{
	toolbar.tid2wid.used = 0;

	PCB_DAD_BEGIN_HBOX(toolbar.sub.dlg);
		PCB_DAD_COMPFLAG(toolbar.sub.dlg, PCB_HATF_EXPFILL | PCB_HATF_TIGHT);

		toolbar_create_static(cfg);

	/* eat up remaining space in the middle before displaying the dynamic tools */
		PCB_DAD_BEGIN_HBOX(toolbar.sub.dlg);
			PCB_DAD_COMPFLAG(toolbar.sub.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_END(toolbar.sub.dlg);

	/* later on dynamic tools would be displayed here */

	PCB_DAD_END(toolbar.sub.dlg);
}


void pcb_toolbar_gui_init_ev(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((PCB_HAVE_GUI_ATTR_DLG) && (pcb_gui->get_menu_cfg != NULL)) {
		pcb_hid_cfg_t *cfg = pcb_gui->get_menu_cfg();
		if (cfg == NULL)
			return;
		toolbar_docked_create(cfg);
		if (pcb_hid_dock_enter(&toolbar.sub, PCB_HID_DOCK_TOP_LEFT, "Toolbar") == 0) {
			toolbar.sub_inited = 1;
			toolbar_pcb2dlg();
		}
	}
}

void pcb_toolbar_update_conf(conf_native_t *cfg, int arr_idx)
{
	toolbar_pcb2dlg();
}
