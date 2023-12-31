/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2015 Tibor 'Igor2' Palinkas
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

#include <stdlib.h>
#include <string.h>
#include <librnd/core/plugins.h>

/* for the action */
#include <librnd/rnd_config.h>
#include <genvector/gds_char.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/actions.h>

unsigned long rnd_api_ver = RND_API_VER;

pup_context_t rnd_pup;
char **rnd_pup_paths = NULL;
static int paths_used = 0, paths_alloced = 0;

void rnd_plugin_add_dir(const char *dir)
{
	if ((dir == NULL) || (*dir == '\0'))
		return;
	if (paths_used+1 >= paths_alloced) {
		paths_alloced += 16;
		rnd_pup_paths = realloc(rnd_pup_paths, sizeof(char *) * paths_alloced);
	}
	rnd_pup_paths[paths_used] = rnd_strdup(dir);
	paths_used++;
	rnd_pup_paths[paths_used] = NULL;
}

void rnd_plugin_uninit(void)
{
	int n;
	for(n = 0; n < paths_used; n++)
		free(rnd_pup_paths[n]);
	free(rnd_pup_paths);
	rnd_pup_paths = NULL;
}

