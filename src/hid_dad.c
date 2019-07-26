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

/* widget-type-independent DAD functions */

#include "config.h"
#include "hid_dad.h"

int pcb_dock_is_vert[PCB_HID_DOCK_max]   = {0, 0, 0, 1, 0, 1}; /* Update this if pcb_hid_dock_t changes */
int pcb_dock_has_frame[PCB_HID_DOCK_max] = {0, 0, 0, 1, 0, 0}; /* Update this if pcb_hid_dock_t changes */

pcb_hid_attr_node_t *hid_attr_nodes = 0;

typedef struct {
	pcb_hatt_compflags_t flag;
	const char *name;
} comflag_name_t;

static comflag_name_t compflag_names[] = {
	{PCB_HATF_FRAME,         "frame"},
	{PCB_HATF_SCROLL,        "scroll"},
	{PCB_HATF_HIDE_TABLAB,   "hide_tablab"},
	{PCB_HATF_LEFT_TAB,      "left_tab"},
	{PCB_HATF_TREE_COL,      "tree_col"},
	{PCB_HATF_EXPFILL,       "expfill"},
	{0, NULL}
};

const char *pcb_hid_compflag_bit2name(pcb_hatt_compflags_t bit)
{
	comflag_name_t *n;
	for(n = compflag_names; n->flag != 0; n++)
		if (n->flag == bit)
			return n->name;
	return NULL;
}

pcb_hatt_compflags_t pcb_hid_compflag_name2bit(const char *name)
{
	comflag_name_t *n;
	for(n = compflag_names; n->flag != 0; n++)
		if (strcmp(n->name, name) == 0)
			return n->flag;
	return 0;
}

void pcb_hid_dad_close(void *hid_ctx, pcb_dad_retovr_t *retovr, int retval)
{
	retovr->valid = 1;
	retovr->value = retval;
	pcb_gui->attr_dlg_free(hid_ctx);
}

void pcb_hid_dad_close_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_dad_retovr_t **retovr = attr->wdata;
	pcb_hid_dad_close(hid_ctx, *retovr, attr->val.lng);
}

int pcb_hid_dad_run(void *hid_ctx, pcb_dad_retovr_t *retovr)
{
	int ret;

	retovr->valid = 0;
	retovr->dont_free++;
	ret = pcb_gui->attr_dlg_run(hid_ctx);
	if (retovr->valid)
		ret = retovr->value;
	retovr->dont_free--;
	return ret;
}

