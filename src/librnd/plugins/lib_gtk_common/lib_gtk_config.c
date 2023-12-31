/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2018 Tibor Palinkas
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "config.h"

#include <stdlib.h>
#include <assert.h>
#include "lib_gtk_config.h"
#include "hid_gtk_conf.h"
#include <librnd/core/plugins.h>
#include <librnd/plugins/lib_hid_common/place.h>

static const char *lib_gtk_config_cookie = "lib_gtk_config";

rnd_conf_hid_id_t rnd_gtk_conf_id = -1;
conf_hid_gtk_t rnd_gtk_conf_hid;

void rnd_gtk_conf_uninit(void)
{
	rnd_conf_hid_unreg(lib_gtk_config_cookie);
	rnd_conf_unreg_fields("plugins/hid_gtk/");
}

/* Config paths that got removed but can be converted. Pairs of old,new strings */
static const char *legacy_paths[] = {
	"plugins/hid_gtk/window_geometry/netlist_x", "plugins/dialogs/window_geometry/netlist/x",
	"plugins/hid_gtk/window_geometry/netlist_y", "plugins/dialogs/window_geometry/netlist/y",
	"plugins/hid_gtk/window_geometry/netlist_width", "plugins/dialogs/window_geometry/netlist/width",
	"plugins/hid_gtk/window_geometry/netlist_height", "plugins/dialogs/window_geometry/netlist/height",

	"plugins/hid_gtk/window_geometry/pinout_x", "plugins/dialogs/window_geometry/pinout/x",
	"plugins/hid_gtk/window_geometry/pinout_y", "plugins/dialogs/window_geometry/pinout/y",
	"plugins/hid_gtk/window_geometry/pinout_width", "plugins/dialogs/window_geometry/pinout/width",
	"plugins/hid_gtk/window_geometry/pinout_height", "plugins/dialogs/window_geometry/pinout/height",

	"plugins/hid_gtk/window_geometry/library_x", "plugins/dialogs/window_geometry/library/x",
	"plugins/hid_gtk/window_geometry/library_y", "plugins/dialogs/window_geometry/library/y",
	"plugins/hid_gtk/window_geometry/library_width", "plugins/dialogs/window_geometry/library/width",
	"plugins/hid_gtk/window_geometry/library_height", "plugins/dialogs/window_geometry/library/height",

	"plugins/hid_gtk/window_geometry/log_x", "plugins/dialogs/window_geometry/log/x",
	"plugins/hid_gtk/window_geometry/log_y", "plugins/dialogs/window_geometry/log/y",
	"plugins/hid_gtk/window_geometry/log_width", "plugins/dialogs/window_geometry/log/width",
	"plugins/hid_gtk/window_geometry/log_height", "plugins/dialogs/window_geometry/log/height",

	"plugins/hid_gtk/window_geometry/drc_x", "plugins/dialogs/window_geometry/drc/x",
	"plugins/hid_gtk/window_geometry/drc_y", "plugins/dialogs/window_geometry/drc/y",
	"plugins/hid_gtk/window_geometry/drc_width", "plugins/dialogs/window_geometry/drc/width",
	"plugins/hid_gtk/window_geometry/drc_height", "plugins/dialogs/window_geometry/drc/height",

	"plugins/hid_gtk/window_geometry/top_x", "plugins/dialogs/window_geometry/top/x",
	"plugins/hid_gtk/window_geometry/top_y", "plugins/dialogs/window_geometry/top/y",
	"plugins/hid_gtk/window_geometry/top_width", "plugins/dialogs/window_geometry/top/width",
	"plugins/hid_gtk/window_geometry/top_height", "plugins/dialogs/window_geometry/top/height",

	"plugins/hid_gtk/window_geometry/keyref_x", "plugins/dialogs/window_geometry/keyref/x",
	"plugins/hid_gtk/window_geometry/keyref_y", "plugins/dialogs/window_geometry/keyref/y",
	"plugins/hid_gtk/window_geometry/keyref_width", "plugins/dialogs/window_geometry/keyref/width",
	"plugins/hid_gtk/window_geometry/keyref_height", "plugins/dialogs/window_geometry/keyref/height",

	"plugins/hid_gtk/auto_save_window_geometry/to_design", "plugins/dialogs/auto_save_window_geometry/to_design",
	"plugins/hid_gtk/auto_save_window_geometry/to_project", "plugins/dialogs/auto_save_window_geometry/to_project",
	"plugins/hid_gtk/auto_save_window_geometry/to_user", "plugins/dialogs/auto_save_window_geometry/to_user",

	NULL, NULL
};

void rnd_gtk_conf_init(void)
{
	int warned = 0;
	const char **p;
	static int dummy_gtk_conf_init;
	int dirty[RND_CFR_max_real] = {0};
	rnd_conf_role_t r;

	rnd_gtk_conf_id = rnd_conf_hid_reg(lib_gtk_config_cookie, NULL);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(rnd_gtk_conf_hid, field,isarray,type_name,cpath,cname,desc,flags);
#include <librnd/plugins/lib_gtk_common/hid_gtk_conf_fields.h>

	/* check for legacy win geo settings */
	for(p = legacy_paths; *p != NULL; p+=2) {
		rnd_conf_native_t *nat;
		char *end, dirname[128];
		
		rnd_conf_update(p[0], -1);
		nat = rnd_conf_get_field(p[0]);
		if ((nat == NULL) || (nat->prop->src == NULL))
			continue;
		if (!warned) {
			rnd_message(RND_MSG_WARNING, "Some of your config sources contain old, gtk-only window placement nodes.\nThose settings got removed from pcb-rnd - your nodes just got converted\ninto the new config, but you will need to remove the\nold config nodes manually from the following places:\n");
			warned = 1;
		}
		rnd_message(RND_MSG_WARNING, "%s from %s:%d\n", nat->hash_path, nat->prop->src->file_name, nat->prop->src->line);

		strcpy(dirname, p[1]);
		end = strrchr(dirname, '/');
		assert(end != NULL);
		*end = '\0';
		if (rnd_conf_get_field(p[1]) == NULL)
			rnd_conf_reg_field_(&dummy_gtk_conf_init, 1, RND_CFN_INTEGER, p[1], "", 0);
		r = rnd_conf_lookup_role(nat->prop->src);
		rnd_conf_setf(r, p[1], -1, "%d", nat->val.integer[0]);
		dirty[r] = 1;
	}
	for(r = 0; r < RND_CFR_max_real; r++)
		if (dirty[r])
			rnd_wplc_load(r);
}
