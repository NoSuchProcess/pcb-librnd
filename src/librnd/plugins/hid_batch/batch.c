#include <librnd/rnd_config.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <librnd/hid/hid.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/plugins.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/event.h>
#include <librnd/core/rnd_conf.h>

#include <librnd/hid/hid_nogui.h>
#include <librnd/core/actions.h>
#include <librnd/hid/hid_init.h>
#include <librnd/hid/hid_attrib.h>

static const char *batch_cookie = "batch HID";

static int batch_active = 0;
static void batch_begin(void);
static void batch_end(void);

/* This is a text-line "batch" HID, which exists for scripting and
   non-GUI needs.  */

typedef struct rnd_hid_gc_s {
	rnd_core_gc_t core_gc;
} rnd_hid_gc_s;

static const rnd_export_opt_t *batch_get_export_options(rnd_hid_t *hid, int *n_ret, rnd_design_t *dsg, void *appspec)
{
	if (n_ret != NULL)
		*n_ret = 0;
	return NULL;
}

static int batch_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nbatch GUI command line arguments: none\n\n");
	fprintf(stderr, "\nInvocation: %s --gui batch [options]\n", rnd_app.package);
	return 0;
}


static char *prompt = NULL;

static void uninit_batch(void)
{
	rnd_event_unbind_allcookie(batch_cookie);
	if (prompt != NULL) {
		free(prompt);
		prompt = NULL;
	}
}

static void ev_design_changed(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if (prompt != NULL)
		free(prompt);
	if ((hidlib != NULL) && (hidlib->loadname != NULL)) {
		prompt = strrchr(hidlib->loadname, '/');
		if (prompt)
			prompt++;
		else
			prompt = hidlib->loadname;
		if (prompt != NULL)
			prompt = rnd_strdup(prompt);
	}
	else
		prompt = rnd_strdup("no-design");
}

static void log_append(rnd_logline_t *line)
{
	if ((line->level < RND_MSG_INFO) && !rnd_conf.rc.verbose)
		return;

	if ((line->prev == NULL) || (line->prev->str[line->prev->len-1] == '\n')) {
		switch(line->level) {
			case RND_MSG_DEBUG:   printf("D: "); break;
			case RND_MSG_INFO:    printf("I: "); break;
			case RND_MSG_WARNING: printf("W: "); break;
			case RND_MSG_ERROR:   printf("E: "); break;
		}
	}
	printf("%s", line->str);
	line->seen = 1;
}

static void ev_log_append(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if (!batch_active)
		return;

	log_append((rnd_logline_t *)argv[1].d.p);
}

static void log_import(void)
{
	rnd_logline_t *n;
	for(n = rnd_log_first; n != NULL; n = n->next)
		log_append(n);
}

extern int isatty();

/* TODO - this could use some enhancement to actually use the other
   args */
static char *nogui_fileselect(rnd_hid_t *hid, const char *title, const char *descr, const char *default_file, const char *default_ext, const rnd_hid_fsd_filter_t *flt, const char *history_tag, rnd_hid_fsd_flags_t flags, rnd_hid_dad_subdialog_t *sub)
{
	char *answer;

	if (rnd_conf.rc.quiet)
		return rnd_strdup("");

	if (default_file)
		printf("%s [%s] : ", title, default_file);
	else
		printf("%s : ", title);

	answer = rnd_nogui_read_stdin_line();
	if (answer == NULL)
		return (default_file != NULL) ? rnd_strdup(default_file) : NULL;
	else
		return rnd_strdup(answer);
}

static int batch_stay;
static void batch_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	int interactive;
	char line[1000];

	rnd_hid_fileselect_imp = nogui_fileselect;

	batch_begin();

	if (isatty(0))
		interactive = 1;
	else
		interactive = 0;

	log_import();

	if ((interactive) && (!rnd_conf.rc.quiet)) {
		printf("Entering %s version %s batch mode.\n", rnd_app.package, rnd_app.version);
		printf("See %s for project information\n", rnd_app.url);
	}

	batch_stay = 1;
	while (batch_stay) {
		if ((interactive) && (!rnd_conf.rc.quiet)) {
			printf("%s:%s> ", prompt, rnd_cli_prompt(NULL));
			fflush(stdout);
		}
		if (fgets(line, sizeof(line) - 1, stdin) == NULL) {
			uninit_batch();
			goto quit;
		}
		rnd_parse_command(hid->hid_data, line, rnd_false);
	}

	quit:;
	batch_end();
}

static void batch_do_exit(rnd_hid_t *hid)
{
	batch_stay = 0;
}

static int batch_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	return rnd_hid_parse_command_line(argc, argv);
}

static void batch_invalidate_lr(rnd_hid_t *hid, rnd_coord_t l, rnd_coord_t r, rnd_coord_t t, rnd_coord_t b)
{
}

static void batch_invalidate_all(rnd_hid_t *hid)
{
}

static int batch_set_layer_group(rnd_hid_t *hid, rnd_design_t *design, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	return 0;
}

static rnd_hid_gc_t batch_make_gc(rnd_hid_t *hid)
{
	static rnd_core_gc_t hc;
	return (rnd_hid_gc_t)&hc;
}

static void batch_destroy_gc(rnd_hid_gc_t gc)
{
}

static void batch_set_color(rnd_hid_gc_t gc, const rnd_color_t *name)
{
}

static void batch_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
}

static void batch_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
}

static void batch_set_draw_xor(rnd_hid_gc_t gc, int xor_set)
{
}

static void batch_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
}

static void batch_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t end_angle)
{
}

static void batch_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
}

static void batch_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
}

static void batch_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t * x, rnd_coord_t * y)
{
}

static void batch_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
}

static void batch_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
}

static int batch_shift_is_pressed(rnd_hid_t *hid)
{
	return 0;
}

static int batch_control_is_pressed(rnd_hid_t *hid)
{
	return 0;
}

static int batch_mod1_is_pressed(rnd_hid_t *hid)
{
	return 0;
}

static int batch_get_coords(rnd_hid_t *hid, const char *msg, rnd_coord_t *x, rnd_coord_t *y, int force)
{
	return -1;
}

static void batch_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, rnd_set_crosshair_t action)
{
}

static rnd_hidval_t batch_add_timer(rnd_hid_t *hid, void (*func)(rnd_hidval_t user_data), unsigned long milliseconds, rnd_hidval_t user_data)
{
	rnd_hidval_t rv;
	rv.lval = 0;
	return rv;
}

static void batch_stop_timer(rnd_hid_t *hid, rnd_hidval_t timer)
{
}

rnd_hidval_t batch_watch_file(rnd_hid_t *hid, int fd, unsigned int condition, rnd_bool (*func) (rnd_hidval_t watch, int fd, unsigned int condition, rnd_hidval_t user_data), rnd_hidval_t user_data)
{
	rnd_hidval_t ret;
	ret.ptr = NULL;
	return ret;
}

void batch_unwatch_file(rnd_hid_t *hid, rnd_hidval_t data)
{
}

static void batch_zoom_win(rnd_hid_t *hid, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_bool set_crosshair)
{
}

static void batch_zoom(rnd_hid_t *hid, rnd_coord_t center_x, rnd_coord_t center_y, double factor, int relative)
{
}

static void batch_pan(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int relative)
{
}

static void batch_pan_mode(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, rnd_bool mode)
{
}

static void batch_set_design(rnd_hid_t *hid, rnd_design_t *design)
{
	hid->hid_data = design;
}

static void batch_view_get(rnd_hid_t *hid, rnd_box_t *viewbox)
{
	rnd_design_t *hidlib = hid->hid_data;
	viewbox->X1 = hidlib->dwg.X1;
	viewbox->Y1 = hidlib->dwg.Y1;
	viewbox->X2 = hidlib->dwg.X2;
	viewbox->Y2 = hidlib->dwg.Y2;
}

static void batch_open_command(rnd_hid_t *hid)
{

}

static int batch_open_popup(rnd_hid_t *hid, const char *menupath)
{
	return 1;
}


static rnd_hid_t batch_hid;

int pplg_check_ver_hid_batch(int ver_needed) { return 0; }

void pplg_uninit_hid_batch(void)
{
	rnd_event_unbind_allcookie(batch_cookie);
	rnd_export_remove_opts_by_cookie(batch_cookie);
}

int pplg_init_hid_batch(void)
{
	RND_API_CHK_VER;
	memset(&batch_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&batch_hid);

	batch_hid.struct_size = sizeof(rnd_hid_t);
	batch_hid.name = "batch";
	batch_hid.description = "Batch-mode GUI for non-interactive use.";
	batch_hid.gui = 1;

	batch_hid.set_design = batch_set_design;
	batch_hid.get_export_options = batch_get_export_options;
	batch_hid.do_export = batch_do_export;
	batch_hid.do_exit = batch_do_exit;
	batch_hid.parse_arguments = batch_parse_arguments;
	batch_hid.invalidate_lr = batch_invalidate_lr;
	batch_hid.invalidate_all = batch_invalidate_all;
	batch_hid.set_layer_group = batch_set_layer_group;
	batch_hid.make_gc = batch_make_gc;
	batch_hid.destroy_gc = batch_destroy_gc;
	batch_hid.set_color = batch_set_color;
	batch_hid.set_line_cap = batch_set_line_cap;
	batch_hid.set_line_width = batch_set_line_width;
	batch_hid.set_draw_xor = batch_set_draw_xor;
	batch_hid.draw_line = batch_draw_line;
	batch_hid.draw_arc = batch_draw_arc;
	batch_hid.draw_rect = batch_draw_rect;
	batch_hid.fill_circle = batch_fill_circle;
	batch_hid.fill_polygon = batch_fill_polygon;
	batch_hid.fill_polygon_offs = batch_fill_polygon_offs;
	batch_hid.fill_rect = batch_fill_rect;
	batch_hid.shift_is_pressed = batch_shift_is_pressed;
	batch_hid.control_is_pressed = batch_control_is_pressed;
	batch_hid.mod1_is_pressed = batch_mod1_is_pressed;
	batch_hid.get_coords = batch_get_coords;
	batch_hid.set_crosshair = batch_set_crosshair;
	batch_hid.add_timer = batch_add_timer;
	batch_hid.stop_timer = batch_stop_timer;
	batch_hid.watch_file = batch_watch_file;
	batch_hid.unwatch_file = batch_unwatch_file;
	batch_hid.usage = batch_usage;

	batch_hid.zoom_win = batch_zoom_win;
	batch_hid.zoom = batch_zoom;
	batch_hid.pan = batch_pan;
	batch_hid.pan_mode = batch_pan_mode;
	batch_hid.view_get = batch_view_get;
	batch_hid.open_command = batch_open_command;
	batch_hid.open_popup = batch_open_popup;

	rnd_event_bind(RND_EVENT_DESIGN_SET_CURRENT, ev_design_changed, NULL, batch_cookie);
	rnd_event_bind(RND_EVENT_LOG_APPEND, ev_log_append, NULL, batch_cookie);

	rnd_hid_register_hid(&batch_hid);
	return 0;
}


static void batch_begin(void)
{
	batch_active = 1;
}

static void batch_end(void)
{
	batch_active = 0;
}

