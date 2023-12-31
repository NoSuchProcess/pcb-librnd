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

#include <stdarg.h>
#include <ctype.h>
#include <librnd/core/rnd_printf.h>
#define proto_vsnprintf rnd_vsnprintf
#include "proto_lowcommon.h"
#include "proto_lowsend.h"
#include "proto_lowparse.h"

static const int proto_ver = 1;
static proto_ctx_t pctx;
static const char *cfmt = "%.08mm";

static proto_node_t *proto_error;
static proto_node_t *remote_proto_parse(const char *wait_for, int cache_msgs);

void remote_proto_send_ver()
{
	send_begin(&pctx, "ver");
	send_open(&pctx, 0, 1);
	sendf(&pctx, "%ld", proto_ver);
	send_close(&pctx);
	send_end(&pctx);
}

void remote_proto_send_unit()
{
	send_begin(&pctx, "unit");
	send_open(&pctx, 0, 1);
	sendf(&pctx, "mm");
	send_close(&pctx);
	send_end(&pctx);
}

void remote_proto_send_brddim(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	send_begin(&pctx, "brddim");
	send_open(&pctx, 0, 1);
	sendf(&pctx, cfmt, x1);
	sendf(&pctx, cfmt, y1);
	sendf(&pctx, cfmt, x2);
	sendf(&pctx, cfmt, y2);
	send_close(&pctx);
	send_end(&pctx);
}

int remote_proto_send_ready()
{
	proto_node_t *targ;
	send_begin(&pctx, "ready");
	send_open(&pctx, 0, 1);
	send_close(&pctx);
	send_end(&pctx);
	targ = remote_proto_parse("Ready", 0);
	if (targ == proto_error)
		return -1;
	proto_node_free(targ);
	return 0;
}

void proto_send_invalidate(int l, int r, int t, int b)
{
	send_begin(&pctx, "inval");
	send_open(&pctx, 0, 1);
	sendf(&pctx, "%d", l);
	sendf(&pctx, "%d", r);
	sendf(&pctx, "%d", t);
	sendf(&pctx, "%d", b);
	send_close(&pctx);
	send_end(&pctx);
}

void proto_send_invalidate_all(void)
{
	send_begin(&pctx, "inval");
	send_open(&pctx, 0, 1);
	send_close(&pctx);
	send_end(&pctx);
}

int proto_send_set_layer_group(rnd_layergrp_id_t group, rnd_design_t *design, const char *purpose, int is_empty, rnd_xform_t **xform)
{
	send_begin(&pctx, "setlg");
	send_open(&pctx, 0, 1);
	sendf(&pctx, "%ld", group);
	sendf(&pctx, "%d", is_empty);
	sendf(&pctx, "%s", purpose);
	send_close(&pctx);
	send_end(&pctx);
	return 0;
}

int rnd_remote_new_layer(const char *name, rnd_layer_id_t lid, unsigned int gid)
{
	send_begin(&pctx, "newly");
	send_open(&pctx, str_is_bin(name), 1);
	sends(&pctx, name);
	sendf(&pctx, "%ld", lid);
	sendf(&pctx, "%ld", gid);
	send_close(&pctx);
	send_end(&pctx);
	return 0;
}

int proto_send_make_gc(void)
{
	proto_node_t *targ;
	int gci;

	send_begin(&pctx, "makeGC");
	send_open(&pctx, 0, 1);
	send_close(&pctx);
	send_end(&pctx);
	targ = remote_proto_parse("MakeGC", 1);
	if (targ == proto_error)
		return -1;
	gci = aint(child1(targ), -1);
	if (gci < 0)
		return -1;
/*	printf("New GC: %s\n", gcs);*/
	proto_node_free(targ);
	return gci;
}

int proto_send_del_gc(int gc)
{
	send_begin(&pctx, "delGC");
	send_open(&pctx, 0, 1);
	sendf(&pctx, "%d", gc);
	send_close(&pctx);
	send_end(&pctx);
	return 0;
}

void proto_send_set_color(int gc, const char *name)
{
	send_begin(&pctx, "clr");
	send_open(&pctx, str_is_bin(name), 1);
	sendf(&pctx, "%d", gc);
	sends(&pctx, name);
	send_close(&pctx);
	send_end(&pctx);
}

void proto_send_set_line_cap(int gc, char style)
{
	send_begin(&pctx, "cap");
	send_open(&pctx, 0, 1);
	sendf(&pctx, "%d", gc);
	sendf(&pctx, "%c", style);
	send_close(&pctx);
	send_end(&pctx);
}

void proto_send_set_line_width(int gc, rnd_coord_t width)
{
	send_begin(&pctx, "linwid");
	send_open(&pctx, 0, 1);
	sendf(&pctx, "%d", gc);
	sendf(&pctx, cfmt, width);
	send_close(&pctx);
	send_end(&pctx);
}

void proto_send_set_draw_xor(int gc, int xor_set)
{
	send_begin(&pctx, "setxor");
	send_open(&pctx, 0, 1);
	sendf(&pctx, "%d", gc);
	sendf(&pctx, "%d", xor_set);
	send_close(&pctx);
	send_end(&pctx);
}

void proto_send_draw_line(int gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	send_begin(&pctx, "line");
	send_open(&pctx, 0, 1);
	sendf(&pctx, "%d", gc);
	sendf(&pctx, cfmt, x1);
	sendf(&pctx, cfmt, y1);
	sendf(&pctx, cfmt, x2);
	sendf(&pctx, cfmt, y2);
	send_close(&pctx);
	send_end(&pctx);
}

void proto_send_draw_arc(int gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
	send_begin(&pctx, "arc");
	send_open(&pctx, 0, 1);
	sendf(&pctx, "%d", gc);
	sendf(&pctx, cfmt, cx);
	sendf(&pctx, cfmt, cy);
	sendf(&pctx, cfmt, width);
	sendf(&pctx, cfmt, height);
	sendf(&pctx, "%f", start_angle);
	sendf(&pctx, "%f", delta_angle);
	send_close(&pctx);
	send_end(&pctx);
}



void proto_send_draw_rect(int gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, int is_filled)
{
	send_begin(&pctx, "rect");
	send_open(&pctx, 0, 1);
	sendf(&pctx, "%d", gc);
	sendf(&pctx, cfmt, x1);
	sendf(&pctx, cfmt, y1);
	sendf(&pctx, cfmt, x2);
	sendf(&pctx, cfmt, y2);
	sendf(&pctx, "%d", is_filled);
	send_close(&pctx);
	send_end(&pctx);
}

void proto_send_fill_circle(int gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	send_begin(&pctx, "fcirc");
	send_open(&pctx, 0, 1);
	sendf(&pctx, "%d", gc);
	sendf(&pctx, cfmt, cx);
	sendf(&pctx, cfmt, cy);
	sendf(&pctx, cfmt, radius);
	send_close(&pctx);
	send_end(&pctx);
}

void proto_send_draw_poly(int gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	int n;
	send_begin(&pctx, "poly");
	send_open(&pctx, 0, 1);
	sendf(&pctx, "%d", gc);
	sendf(&pctx, "%d", n_coords);
	send_open(&pctx, 0, 1);
	for(n = 0; n < n_coords; n++) {
		send_open(&pctx, 0, 1);
		sendf(&pctx, cfmt, x[n] + dx);
		sendf(&pctx, cfmt, y[n] + dy);
		send_close(&pctx);
	}
	send_close(&pctx);
	send_close(&pctx);
	send_end(&pctx);
}

int proto_send_set_drawing_mode(const char *name, int direct)
{
	send_begin(&pctx, "drwmod");
	send_open(&pctx, str_is_bin(name), 1);
	sends(&pctx, name);
	sendf(&pctx, "%d", direct);
	send_close(&pctx);
	send_end(&pctx);
	return 0;
}

void proto_exec_cmd(const char *cmd, proto_node_t *targ)
{

}

static proto_node_t *remote_proto_parse(const char *wait_for, int cache_msgs)
{
	for(;;) {
		int c = getchar();
		if (c == EOF)
			return proto_error;
		switch(parse_char(&pctx, c)) {
			case PRES_ERR:
				fprintf(stderr, "Protocol error.\n");
				return proto_error;
			case PRES_PROCEED:
				break;
			case PRES_GOT_MSG:
				if ((wait_for != NULL) && (strcmp(wait_for, pctx.pcmd) == 0))
					return pctx.targ;
				if (cache_msgs) {
TODO("TODO")
				}
				else
					proto_exec_cmd(pctx.pcmd, pctx.targ);
				proto_node_free(pctx.targ);
				break;
		}
	}
}

int remote_proto_parse_all()
{
	if (remote_proto_parse(NULL, 0) == proto_error)
		return -1;
	return 0;
}
