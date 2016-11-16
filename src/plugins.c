/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2015 Tibor 'Igor2' Palinkas
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "plugins.h"

/* for the action */
#include "config.h"
#include "genvector/gds_char.h"
#include "compat_misc.h"
#include "hid.h"

pcb_plugin_info_t *plugins = NULL;

pcb_plugin_info_t *plugin_find(const char *name)
{
	pcb_plugin_info_t *i;
	for(i = plugins; i != NULL; i = i->next)
		if (strcmp(i->name, name) == 0)
			return i;
	return NULL;
}

void pcb_plugin_register(const char *name, const char *path, void *handle, int dynamic_loaded, void (*uninit)(void))
{
	pcb_plugin_info_t *i = malloc(sizeof(pcb_plugin_info_t));

	i->name = pcb_strdup(name);
	i->path = pcb_strdup(path);
	i->handle = handle;
	i->dynamic_loaded = dynamic_loaded;
	i->uninit = uninit;

	i->next = plugins;
	plugins = i;
}

void pcb_plugins_init(void)
{
}


void pcb_plugins_uninit(void)
{
	pcb_plugin_info_t *i, *next;
	for(i = plugins; i != NULL; i = next) {
		next = i->next;
		free(i->name);
		free(i->path);
		if (i->uninit != NULL)
			i->uninit();
		free(i);
	}
	plugins = NULL;
}



/* ---------------------------------------------------------------- */
static const char manageplugins_syntax[] = "ManagePlugins()\n";

static const char manageplugins_help[] = "Manage plugins dialog.";

static int ActionManagePlugins(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_plugin_info_t *i;
	int nump = 0, numb = 0;
	gds_t str;

	gds_init(&str);

	for (i = plugins; i != NULL; i = i->next)
		if (i->dynamic_loaded)
			nump++;
		else
			numb++;

	gds_append_str(&str, "Plugins loaded:\n");
	if (nump > 0) {
		for (i = plugins; i != NULL; i = i->next) {
			if (i->dynamic_loaded) {
				gds_append(&str, ' ');
				gds_append_str(&str, i->name);
				gds_append(&str, ' ');
				gds_append_str(&str, i->path);
				gds_append(&str, '\n');
			}
		}
	}
	else
		gds_append_str(&str, " (none)\n");

	gds_append_str(&str, "\n\nBuildins:\n");
	if (numb > 0) {
		for (i = plugins; i != NULL; i = i->next) {
			if (!i->dynamic_loaded) {
				gds_append(&str, ' ');
				gds_append_str(&str, i->name);
				gds_append(&str, '\n');
			}
		}
	}
	else
		gds_append_str(&str, " (none)\n");

	gds_append_str(&str, "\n\nNOTE: this is the alpha version, can only list plugins/buildins\n");
	gui->report_dialog("Manage plugins", str.array);
	gds_uninit(&str);
	return 0;
}


pcb_hid_action_t plugins_action_list[] = {
	{"ManagePlugins", 0, ActionManagePlugins,
	 manageplugins_help, manageplugins_syntax}
};

PCB_REGISTER_ACTIONS(plugins_action_list, NULL)
