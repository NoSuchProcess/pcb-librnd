/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include <librnd/rnd_config.h>

#include <librnd/core/conf.h>
#include <librnd/core/error.h>
#include <librnd/core/color.h>
#include <librnd/core/hidlib.h>

#include <librnd/core/rnd_conf.h>

rnd_conf_t rnd_conf;

int rnd_hidlib_conf_init()
{
	int cnt = 0;

	/* historical default value for gtk */
	*(rnd_coord_t *)&rnd_conf.editor.min_zoom = 200;
	*(RND_CFT_BOOLEAN *)&rnd_conf.editor.unlimited_pan = 0;

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(rnd_conf, field,isarray,type_name,cpath,cname,desc,flags);
#include <librnd/core/hidlib_conf_fields.h>

	return cnt;
}

