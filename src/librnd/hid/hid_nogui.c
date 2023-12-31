/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
 *  Copyright (C) 2016..2018 Tibor 'Igor2' Palinkas
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
 *
 */

#include <librnd/rnd_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <librnd/core/actions.h>
#include <librnd/hid/hid.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/rnd_conf.h>

/* This is the "gui" that is installed at startup, and is used when
   there is no other real GUI to use.  For the most part, it just
   stops the application from (1) crashing randomly, and (2) doing
   gui-specific things when it shouldn't.  */

#define CRASH(func) fprintf(stderr, "HID error: pcb called GUI function %s without having a GUI available.\n", func); abort()

static const char rnd_acth_cli[] = "Intenal: CLI frontend action. Do not use directly.";

static rnd_hid_t nogui_hid;

typedef struct rnd_hid_gc_s {
	int nothing_interesting_here;
} rnd_hid_gc_s;

static const rnd_export_opt_t *nogui_get_export_options(rnd_hid_t *hid, int *n_ret, rnd_design_t *dsg, void *appspec)
{
	if (n_ret != NULL)
		*n_ret = 0;
	return NULL;
}

static void nogui_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	CRASH("do_export");
}

static int nogui_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	CRASH("parse_arguments");
}

static void nogui_invalidate_lr(rnd_hid_t *hid, rnd_coord_t l, rnd_coord_t r, rnd_coord_t t, rnd_coord_t b)
{
	CRASH("invalidate_lr");
}

static void nogui_invalidate_all(rnd_hid_t *hid)
{
	CRASH("invalidate_all");
}

static int nogui_set_layer_group(rnd_hid_t *hid, rnd_design_t *design, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	CRASH("set_layer_group");
	return 0;
}

static void nogui_end_layer(rnd_hid_t *hid)
{
}

static rnd_hid_gc_t nogui_make_gc(rnd_hid_t *hid)
{
	return 0;
}

static void nogui_destroy_gc(rnd_hid_gc_t gc)
{
}

static void nogui_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
	CRASH("set_drawing_mode");
}

static void nogui_render_burst(rnd_hid_t *hid, rnd_burst_op_t op, const rnd_box_t *screen)
{
	/* the HID may decide to ignore this hook */
}

static void nogui_set_color(rnd_hid_gc_t gc, const rnd_color_t *name)
{
	CRASH("set_color");
}

static void nogui_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	CRASH("set_line_cap");
}

static void nogui_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	CRASH("set_line_width");
}

static void nogui_set_draw_xor(rnd_hid_gc_t gc, int xor_)
{
	CRASH("set_draw_xor");
}

static void nogui_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	CRASH("draw_line");
}

static void nogui_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t end_angle)
{
	CRASH("draw_arc");
}

static void nogui_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	CRASH("draw_rect");
}

static void nogui_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	CRASH("fill_circle");
}

static void nogui_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t * x, rnd_coord_t * y)
{
	CRASH("fill_polygon");
}

static void nogui_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	CRASH("fill_rect");
}

static int nogui_shift_is_pressed(rnd_hid_t *hid)
{
	/* This is called from rnd_crosshair_grid_fit() when the design is loaded.  */
	return 0;
}

static int nogui_control_is_pressed(rnd_hid_t *hid)
{
	CRASH("control_is_pressed");
	return 0;
}

static int nogui_mod1_is_pressed(rnd_hid_t *hid)
{
	CRASH("mod1_is_pressed");
	return 0;
}

static int nogui_get_coords(rnd_hid_t *hid, const char *msg, rnd_coord_t *x, rnd_coord_t *y, int force)
{
	CRASH("get_coords");
}

static void nogui_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, rnd_set_crosshair_t action)
{
}

static rnd_hidval_t nogui_add_timer(rnd_hid_t *hid, void (*func)(rnd_hidval_t user_data), unsigned long milliseconds, rnd_hidval_t user_data)
{
	rnd_hidval_t rv;
	CRASH("add_timer");
	rv.lval = 0;
	return rv;
}

static void nogui_stop_timer(rnd_hid_t *hid, rnd_hidval_t timer)
{
	CRASH("stop_timer");
}

static rnd_hidval_t nogui_watch_file(rnd_hid_t *hid, int fd, unsigned int condition, rnd_bool (*func) (rnd_hidval_t watch, int fd, unsigned int condition, rnd_hidval_t user_data), rnd_hidval_t user_data)
{
	rnd_hidval_t rv;
	CRASH("watch_file");
	rv.lval = 0;
	return rv;
}

static void nogui_unwatch_file(rnd_hid_t *hid, rnd_hidval_t watch)
{
	CRASH("unwatch_file");
}

#define MAX_LINE_LENGTH 1024
static const char *CANCEL = "CANCEL";
char *rnd_nogui_read_stdin_line(void)
{
	static char buf[MAX_LINE_LENGTH];
	char *s;
	int i;

	s = fgets(buf, MAX_LINE_LENGTH, stdin);
	if (s == NULL) {
		printf("\n");
		return rnd_strdup(CANCEL);
	}

	/* Strip any trailing newline characters */
	for (i = strlen(s) - 1; i >= 0; i--)
		if (s[i] == '\r' || s[i] == '\n')
			s[i] = '\0';

	if (s[0] == '\0')
		return NULL;

	return rnd_strdup(s);
}

#undef MAX_LINE_LENGTH

static fgw_error_t rnd_act_cli_PromptFor(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *answer;
	const char *label, *default_str = "", *title = NULL;
	const char *rnd_acts_cli_PromptFor = rnd_acth_cli;

	RND_ACT_CONVARG(1, FGW_STR, cli_PromptFor, label = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, cli_PromptFor, default_str = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, cli_PromptFor, title = argv[3].val.str);

	if (!rnd_conf.rc.quiet) {
		char *tmp;
		if (title != NULL)
			printf("*** %s ***\n", title);
		if (default_str)
			printf("%s [%s] : ", label, default_str);
		else
			printf("%s : ", label);

		tmp = rnd_nogui_read_stdin_line();
		if (tmp == NULL)
			answer = rnd_strdup((default_str != NULL) ? default_str : "");
		else
			answer = tmp; /* always allocated */
	}
	else
		answer = rnd_strdup("");

	res->type = FGW_STR | FGW_DYN;
	res->val.str = answer;
	return 0;
}

static fgw_error_t rnd_act_cli_MessageBox(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *rnd_acts_cli_MessageBox = rnd_acth_cli;
	const char *icon, *title, *label, *txt, *answer;
	char *end;
	int n, ret;

	res->type = FGW_INT;
	if (rnd_conf.rc.quiet) {
		cancel:;
		res->val.nat_int = -1;
		return 0;
	}

	RND_ACT_CONVARG(1, FGW_STR, cli_MessageBox, icon = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_STR, cli_MessageBox, title = argv[2].val.str);
	RND_ACT_CONVARG(3, FGW_STR, cli_MessageBox, label = argv[3].val.str);

	printf("[%s] *** %s ***\n", icon, title);

	retry:;
	printf("%s:\n", label);
	for(n = 4; n < argc; n+=2) {
		RND_ACT_CONVARG(n+0, FGW_STR, cli_MessageBox, txt = argv[n+0].val.str);
		printf(" %d = %s\n", (n - 4)/2+1, txt);
	}
	printf("\nChose a number from above: ");
	fflush(stdout);
	answer = rnd_nogui_read_stdin_line();
	if ((answer == CANCEL) || (strcmp(answer, "cancel") == 0))
		goto cancel;
	if (answer == NULL)
		goto retry;
	ret = strtol(answer, &end, 10);
	if (((*end != '\0') && (*end != '\n') && (*end != '\r')) || (ret < 1) || (ret > (argc - 3) / 2)) {
		printf("\nERROR: please type a number between 1 and %d\n", (argc - 4)/2+1);
		goto retry;
	}
	n = (ret-1)*2+5;
	RND_ACT_CONVARG(n, FGW_INT, cli_MessageBox, res->val.nat_int = argv[n].val.nat_int);
	return 0;
}

void rnd_nogui_attr_dlg_new(rnd_hid_t *hid, const char *id, rnd_hid_attribute_t *attrs, int n_attrs, const char *title, void *caller_data, rnd_bool modal, void (*button_cb)(void *caller_data, rnd_hid_attr_ev_t ev), int defx, int defy, int minx, int miny, void **hid_ctx_out)
{
	CRASH("attr_dlg_new");
}

static int nogui_attr_dlg_run(void *hid_ctx)
{
	CRASH("attr_dlg_run");
}

static void nogui_attr_dlg_raise(void *hid_ctx)
{
	CRASH("attr_dlg_raise");
}

static void nogui_attr_dlg_close(void *hid_ctx)
{
	CRASH("attr_dlg_close");
}

static void nogui_attr_dlg_free(void *hid_ctx)
{
	CRASH("attr_dlg_free");
}

static void nogui_attr_dlg_property(void *hid_ctx, rnd_hat_property_t prop, const rnd_hid_attr_val_t *val)
{
	CRASH("attr_dlg_dlg_property");
}

int rnd_nogui_progress(long so_far, long total, const char *message)
{
	static int on = 0;
	static double nextt;
	double now;

	if (rnd_conf.rc.quiet)
		return 0;
	if (message == NULL) {
		if ((on) && (rnd_conf.rc.verbose >= RND_MSG_INFO))
			fprintf(stderr, "progress: finished\n");
		on = 0;
	}
	else {
		if ((rnd_conf.rc.verbose >= RND_MSG_INFO) || (rnd_gui != &nogui_hid)) {
			now = rnd_dtime();
			if (now >= nextt) {
				fprintf(stderr, "progress: %ld/%ld %s\n", so_far, total, message);
				nextt = now + 0.2;
			}
		}
		on = 1;
	}
	return 0;
}

static int clip_warn(void)
{
	static int warned = 0;
	if (!warned) {
		rnd_message(RND_MSG_ERROR, "The current GUI HID does not support clipboard.\nClipboard is emulated, not shared withother programs\n");
		warned = 1;
	}
	return 0;
}

static char *clip_data = NULL;

static int nogui_clip_set(rnd_hid_t *hid, const char *str)
{
	free(clip_data);
	if (str != NULL)
		clip_data = rnd_strdup(str);
	else
		clip_data = NULL;
	return clip_warn();
}

static char *nogui_clip_get(rnd_hid_t *hid)
{
	if (clip_data == NULL)
		return NULL;
	clip_warn();
	return rnd_strdup(clip_data);
}

static void nogui_reg_mouse_cursor(rnd_hid_t *hid, int idx, const char *name, const unsigned char *pixel, const unsigned char *mask)
{
}

static void nogui_set_mouse_cursor(rnd_hid_t *hid, int idx)
{
}

static void nogui_set_top_title(rnd_hid_t *hid, const char *title)
{
}

void rnd_hid_nogui_init(rnd_hid_t * hid)
{
	hid->get_export_options = nogui_get_export_options;
	hid->do_export = nogui_do_export;
	hid->parse_arguments = nogui_parse_arguments;
	hid->invalidate_lr = nogui_invalidate_lr;
	hid->invalidate_all = nogui_invalidate_all;
	hid->set_layer_group = nogui_set_layer_group;
	hid->end_layer = nogui_end_layer;
	hid->make_gc = nogui_make_gc;
	hid->destroy_gc = nogui_destroy_gc;
	hid->set_drawing_mode = nogui_set_drawing_mode;
	hid->render_burst = nogui_render_burst;
	hid->set_color = nogui_set_color;
	hid->set_line_cap = nogui_set_line_cap;
	hid->set_line_width = nogui_set_line_width;
	hid->set_draw_xor = nogui_set_draw_xor;
	hid->draw_line = nogui_draw_line;
	hid->draw_arc = nogui_draw_arc;
	hid->draw_rect = nogui_draw_rect;
	hid->fill_circle = nogui_fill_circle;
	hid->fill_polygon = nogui_fill_polygon;
	hid->fill_rect = nogui_fill_rect;
	hid->shift_is_pressed = nogui_shift_is_pressed;
	hid->control_is_pressed = nogui_control_is_pressed;
	hid->mod1_is_pressed = nogui_mod1_is_pressed;
	hid->get_coords = nogui_get_coords;
	hid->set_crosshair = nogui_set_crosshair;
	hid->add_timer = nogui_add_timer;
	hid->stop_timer = nogui_stop_timer;
	hid->watch_file = nogui_watch_file;
	hid->unwatch_file = nogui_unwatch_file;
	hid->attr_dlg_new = rnd_nogui_attr_dlg_new;
	hid->attr_dlg_run = nogui_attr_dlg_run;
	hid->attr_dlg_raise = nogui_attr_dlg_raise;
	hid->attr_dlg_close = nogui_attr_dlg_close;
	hid->attr_dlg_free = nogui_attr_dlg_free;
	hid->attr_dlg_property = nogui_attr_dlg_property;
	hid->clip_set = nogui_clip_set;
	hid->clip_get = nogui_clip_get;
	hid->set_mouse_cursor = nogui_set_mouse_cursor;
	hid->reg_mouse_cursor = nogui_reg_mouse_cursor;
	hid->set_top_title = nogui_set_top_title;
}


rnd_hid_t *rnd_hid_nogui_get_hid(void)
{
	memset(&nogui_hid, 0, sizeof(rnd_hid_t));

	nogui_hid.struct_size = sizeof(rnd_hid_t);
	nogui_hid.name = "nogui";
	nogui_hid.description = "Default GUI when no other GUI is present.  " "Does nothing.";

	rnd_hid_nogui_init(&nogui_hid);

	return &nogui_hid;
}


static rnd_action_t cli_dlg_action_list[] = {
	{"cli_PromptFor", rnd_act_cli_PromptFor, rnd_acth_cli, NULL},
	{"cli_MessageBox", rnd_act_cli_MessageBox, rnd_acth_cli, NULL}
};


void rnd_hid_nogui_init2(void)
{
	RND_REGISTER_ACTIONS(cli_dlg_action_list, NULL);
}

