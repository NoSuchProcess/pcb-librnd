/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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
#include <libmbtk/libmbtk.h>
#include <libmbtk/widget.h>

#include "mbtk_common.h"

#include "attr_dlg.h"

void *rnd_mbtk_attr_dlg_new(rnd_hid_t *hid, rnd_mbtk_t *gctx, const char *id, rnd_hid_attribute_t *attrs, int n_attrs, const char *title, void *caller_data, rnd_bool modal, void (*button_cb)(void *caller_data, rnd_hid_attr_ev_t ev), int defx, int defy, int minx, int miny)
{

}

int rnd_mbtk_attr_dlg_run(void *hid_ctx)
{

}

void rnd_mbtk_attr_dlg_raise(void *hid_ctx)
{

}

void rnd_mbtk_attr_dlg_close(void *hid_ctx)
{

}

void rnd_mbtk_attr_dlg_free(void *hid_ctx)
{

}

void rnd_mbtk_attr_dlg_property(void *hid_ctx, rnd_hat_property_t prop, const rnd_hid_attr_val_t *val)
{

}

int rnd_mbtk_attr_dlg_widget_state(void *hid_ctx, int idx, int enabled)
{

}

int rnd_mbtk_attr_dlg_widget_hide(void *hid_ctx, int idx, rnd_bool hide)
{

}

int rnd_mbtk_attr_dlg_widget_poke(void *hid_ctx, int idx, int argc, fgw_arg_t argv[])
{

}

int rnd_mbtk_attr_dlg_set_value(void *hid_ctx, int idx, const rnd_hid_attr_val_t *val)
{

}

void rnd_mbtk_attr_dlg_set_help(void *hid_ctx, int idx, const char *val)
{

}


void *rnd_mbtk_attr_sub_new(rnd_mbtk_t *gctx, mbtk_widget_t *parent_box, rnd_hid_attribute_t *attrs, int n_attrs, void *caller_data)
{

}

int rnd_mbtk_winplace_cfg(rnd_design_t *hidlib, mbtk_widget_t *widget, void *ctx, const char *id)
{

}

rnd_design_t *rnd_mbtk_attr_get_dad_hidlib(void *hid_ctx)
{

}

void rnd_mbtk_attr_dlg_free_all(rnd_mbtk_t *gctx)
{

}


