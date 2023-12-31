/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include <librnd/core/conf.h>
#include <librnd/core/error.h>
#include <librnd/core/plugins.h>
#include "hid-logger.h"
#include <librnd/hid/hid_init.h>
#include <librnd/hid/hid_attrib.h>
#include <librnd/hid/hid_nogui.h>

static const char *loghid_cookie = "loghid plugin";

rnd_hid_t loghid_gui;
rnd_hid_t loghid_exp;

static const rnd_export_opt_t loghid_attribute_list[] = {
	{"target-hid", "the real GUI or export HID to relay calls to",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0}
#define HA_target_hid 0
};
#define NUM_OPTIONS sizeof(loghid_attribute_list) / sizeof(loghid_attribute_list[0])

static rnd_hid_attr_val_t loghid_values[NUM_OPTIONS];

static int loghid_parse_arguments_real(rnd_hid_t *hid, int *argc, char ***argv, int is_gui)
{
	rnd_hid_t *target, *me;
	const char *target_name;

	rnd_export_register_opts2(&loghid_exp, loghid_attribute_list, NUM_OPTIONS, loghid_cookie, 0);
	rnd_hid_parse_command_line(argc, argv);

	target_name = loghid_values[HA_target_hid].str;

	if (is_gui) {
		target = rnd_hid_find_gui(NULL, target_name);
		me = &loghid_gui;
	}
	else {
		target = rnd_hid_find_exporter(target_name);
		me = &loghid_exp;
	}

	create_log_hid(stdout, me, target);
	return target->parse_arguments(target, argc, argv);
}

static int loghid_parse_arguments_gui(rnd_hid_t *hid, int *argc, char ***argv)
{
	return loghid_parse_arguments_real(hid, argc, argv, 1);
}

static int loghid_parse_arguments_exp(rnd_hid_t *hid, int *argc, char ***argv)
{
	return loghid_parse_arguments_real(hid, argc, argv, 0);
}


static int loghid_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nloghid command line arguments:\n\n");
	rnd_hid_usage(loghid_attribute_list, NUM_OPTIONS);
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: pcb-rnd [generic_options] --gui loghid-gui --target-hid gtk2_gdk foo.pcb\n");
	fprintf(stderr, "Usage: pcb-rnd [generic_options] --x loghid-exp --target-hid png foo.pcb\n");
	fprintf(stderr, "\n");
	return 0;
}

int pplg_check_ver_loghid(int ver_needed) { return 0; }

void pplg_uninit_loghid(void)
{
	rnd_export_remove_opts_by_cookie(loghid_cookie);
}

int pplg_init_loghid(void)
{
	RND_API_CHK_VER;

	rnd_hid_nogui_init(&loghid_gui);
	rnd_hid_nogui_init(&loghid_exp);

	/* gui version */
	loghid_gui.struct_size = sizeof(rnd_hid_t);
	loghid_gui.name = "loghid-gui";
	loghid_gui.description = "log GUI HID calls";
	loghid_gui.gui = 1;
	loghid_gui.hide_from_gui = 1;
	loghid_gui.override_render = 1;

	loghid_gui.usage = loghid_usage;
	loghid_gui.parse_arguments = loghid_parse_arguments_gui;
	loghid_gui.argument_array = loghid_values;

	rnd_hid_register_hid(&loghid_gui);

	/* export version */
	loghid_exp.struct_size = sizeof(rnd_hid_t);
	loghid_exp.name = "loghid-exp";
	loghid_exp.description = "log export HID calls";
	loghid_exp.exporter = 1;
	loghid_exp.hide_from_gui = 1;
	loghid_exp.override_render = 1;

	loghid_exp.usage = loghid_usage;
	loghid_exp.parse_arguments = loghid_parse_arguments_exp;
	loghid_exp.argument_array = loghid_values;

	rnd_hid_register_hid(&loghid_exp);

	return 0;
}
