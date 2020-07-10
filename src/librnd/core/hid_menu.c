/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2020 Tibor 'Igor2' Palinkas
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <liblihata/lihata.h>
#include <liblihata/tree.h>

#include "hidlib.h"
#include "file_loaded.h"
#include "hidlib_conf.h"
#include "paths.h"

#include "hid_menu.h"

rnd_hid_cfg_t *rnd_hid_cfg_load(rnd_hidlib_t *hidlib, const char *fn, int exact_fn, const char *embedded_fallback)
{
	lht_doc_t *doc;
	rnd_hid_cfg_t *hr;

	if (embedded_fallback == NULL)
		embedded_fallback = rnd_hidlib_default_embedded_menu;

	/* override HID defaults with the configured path */
	if ((rnd_conf.rc.menu_file != NULL) && (*rnd_conf.rc.menu_file != '\0')) {
		fn = rnd_conf.rc.menu_file;
		exact_fn = (strchr(rnd_conf.rc.menu_file, '/') != NULL);
	}

	if (!exact_fn) {
		/* try different paths to find the menu file inventing its exact name */
		char **paths = NULL, **p;
		int fn_len = strlen(fn);

		doc = NULL;
		rnd_paths_resolve_all(hidlib, rnd_menu_file_paths, paths, fn_len+32, rnd_false);
		for(p = paths; *p != NULL; p++) {
			if (doc == NULL) {
				char *end = *p + strlen(*p);
				sprintf(end, rnd_menu_name_fmt, fn);
				doc = rnd_hid_cfg_load_lht(hidlib, *p);
				if (doc != NULL)
					rnd_file_loaded_set_at("menu", "HID main", *p, "main menu system");
			}
			free(*p);
		}
		free(paths);
	}
	else
		doc = rnd_hid_cfg_load_lht(hidlib, fn);

	if (doc == NULL) {
		doc = rnd_hid_cfg_load_str(embedded_fallback);
		rnd_file_loaded_set_at("menu", "HID main", "<internal>", "main menu system");
	}
	if (doc == NULL)
		return NULL;

	hr = calloc(sizeof(rnd_hid_cfg_t), 1); /* make sure the cache is cleared */
	hr->doc = doc;

	return hr;
}
