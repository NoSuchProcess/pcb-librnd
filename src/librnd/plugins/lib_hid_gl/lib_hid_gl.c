/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2021 Tibor 'Igor2' Palinkas
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
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <librnd/core/plugins.h>

#include <librnd/plugins/lib_hid_gl/conf_internal.c>
#include "lib_hid_gl_conf.h"

conf_lib_hid_gl_t conf_lib_hid_gl;

int pplg_check_ver_lib_hid_gl(int ver_needed) { return 0; }

void pplg_uninit_lib_hid_gl(void)
{
	rnd_conf_unreg_intern(lib_hid_gl_conf_internal);
	rnd_conf_unreg_fields("plugins/lib_hid_gl/");
}

int pplg_init_lib_hid_gl(void)
{
	RND_API_CHK_VER;
	rnd_conf_reg_intern(lib_hid_gl_conf_internal);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_lib_hid_gl, field,isarray,type_name,cpath,cname,desc,flags);
#include "lib_hid_gl_conf_fields.h"
	return 0;
}
