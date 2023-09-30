/*
 *                            COPYRIGHT
 *
 *  librnd, modular 2D CAD framework
 *  (this file is based on pcb-rnd)
 *  Copyright (C) 2023 Tibor 'Igor2' Palinkas
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

#include <librnd/rnd_config.h>
#include <librnd/hid/hid.h>
#include <librnd/hid/hid_init.h>
#include <librnd/core/actions.h>
#include <librnd/core/event.h>

#include "hid_export.h"

const char rnd_acts_Export[] = "Export(exporter, [exporter-args])";
const char rnd_acth_Export[] = "Export the current layout, e.g. Export(png, --dpi, 600)";
fgw_error_t rnd_act_Export(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_design_t *dsg = RND_ACT_DESIGN;
	char *args[128];
	char **a;
	int n;

	if (argc < 1) {
		rnd_message(RND_MSG_ERROR, "Export() needs at least one argument, the name of the export plugin\n");
		return 1;
	}

	if (argc > sizeof(args)/sizeof(args[0])) {
		rnd_message(RND_MSG_ERROR, "Export(): too many arguments\n");
		return 1;
	}

	args[0] = NULL;
	for(n = 1; n < argc; n++)
		RND_ACT_CONVARG(n, FGW_STR, Export, args[n-1] = argv[n].val.str);

	rnd_exporter = rnd_hid_find_exporter(args[0]);
	if (rnd_exporter == NULL) {
		rnd_message(RND_MSG_ERROR, "Export plugin %s not found. Was it enabled in ./configure?\n", args[0]);
		return 1;
	}

	/* remove the name of the exporter */
	argc-=2;

	/* call the exporter */
	rnd_event(RND_ACT_DESIGN, RND_EVENT_EXPORT_SESSION_BEGIN, NULL);
	a = args;
	a++;
	rnd_exporter->parse_arguments(rnd_exporter, &argc, &a);
	rnd_exporter->do_export(rnd_exporter, dsg, NULL, NULL);
	rnd_event(RND_ACT_DESIGN, RND_EVENT_EXPORT_SESSION_END, NULL);

	rnd_exporter = NULL;
	RND_ACT_IRES(0);
	return 0;
}
