/*
 *                            COPYRIGHT
 *
 *  librnd, modular 2D CAD framework
 *  Copyright (C) 2022 Tibor 'Igor2' Palinkas
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

#include <librnd/rnd_config.h>

#include "compat_lrealpath.h"
#include "compat_misc.h"
#include "compat_fs.h"
#include "hidlib.h"

#include "project.h"


void rnd_project_uninit(rnd_project_t *prj)
{
	long n;

	for(n = 0; n < prj->designs.used; n++) {
		rnd_design_t *dsg = prj->designs.array[n];
		dsg->project = NULL;
	}

	vtp0_uninit(&prj->designs);
	free(prj->prjdir);
	free(prj->filename);
	free(prj->loadname);
}

int rnd_project_append_design(rnd_project_t *prj, rnd_design_t *dsg)
{
	if (dsg->project != NULL)
		return -1;
	dsg->project = prj;
	vtp0_append(&prj->designs, dsg);
	return 0;
}

int rnd_project_remove_design(rnd_project_t *prj, rnd_design_t *dsg)
{
	long n, r = 0;
	for(n = 0; n < prj->designs.used; n++) {
		if (prj->designs.array[n] == dsg) {
			dsg->project = NULL;
			vtp0_remove(&prj->designs, n, 1);
			n--;
			r++;
		}
	}
	return r;
}

int rnd_project_update_filename(rnd_project_t *prj)
{
	char *end, *real_fn = rnd_lrealpath(prj->loadname);
	if (real_fn == NULL)
		return -1;
	free(prj->filename);
	prj->filename = real_fn;

	free(prj->prjdir);
	prj->prjdir = rnd_strdup(real_fn);
	end = strrchr(prj->prjdir, '/');
	if (end == NULL) {
		free(prj->prjdir);
		prj->prjdir = rnd_strdup(rnd_get_wd(NULL));
	}
	else
		*end = '\0';

	return 0;
}
