/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

/* Register custom (plugin) config file names, reference counted. These files
   are loaded from system and user config paths */

#include <genht/htsi.h>

static void conf_files_init(void)
{
	htsi_init(&conf_interns, strhash, strkeyeq);
	conf_files_inited = 1;
}

void rnd_conf_files_uninit(void)
{
	htsi_uninit(&conf_interns);
	conf_files_inited = 0;
}

void rnd_conf_reg_intern(const char *intern)
{
	htsi_entry_t *e;
	if (!conf_files_inited) conf_files_init();
	e = htsi_getentry(&conf_interns, intern);
	if (e == NULL)
		htsi_set(&conf_interns, (char *)intern, 1);
	else
		e->value++;

	if (rnd_conf_in_production) {
		if (conf_load_plug_interns(RND_CFR_INTERNAL))
			rnd_conf_merge_all(NULL);
	}
}

void rnd_conf_reg_file(const char *path, const char *intern)
{
	rnd_conf_reg_intern(intern);
}


static void conf_unreg_any(htsi_t *ht, const char *key, int free_key)
{
	htsi_entry_t *e;

	e = htsi_getentry(ht, key);
	assert(e != NULL);
	if (e == NULL) return;

	e->value--;
	if (e->value == 0) {
		htsi_delentry(ht, e);
		if (free_key)
			free(e->key);
	}
}

void rnd_conf_unreg_intern(const char *intern)
{
	assert(conf_files_inited);
	if (!conf_files_inited) return;

	conf_unreg_any(&conf_interns, intern, 0);
}

void rnd_conf_unreg_file(const char *path, const char *intern)
{
	rnd_conf_unreg_intern(intern);
}

