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

#include "glue_hid.h"
#include "mbtk_common.h"
#include "attr_dlg.h"

#include <librnd/core/actions.h>
#include <librnd/core/hid_nogui.h>
#include <librnd/core/hid_menu.h>
#include <librnd/core/hid_cfg_input.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/pixmap.h>
#include <librnd/core/event.h>
#include <librnd/core/hidlib_conf.h>

#include <librnd/plugins/lib_hid_common/lib_hid_common.h>
#include <librnd/plugins/lib_hid_common/menu_helper.h>

#include "keyboard.c"
#include "topwin.c"
#include "io.c"

static int (*init_backend_cb)(rnd_mbtk_t *mctx, int *argc, char **argv[]);

static void rnd_mbtk_gui_inited(rnd_mbtk_t *mctx, int main, int resized)
{
	static int im = 0, ic = 0, first = 1;
	if (main) im = 1;
	if (resized) ic = 1;

	if (im && ic && first) {
		first = 0;
		rnd_hid_announce_gui_init(mctx->hidlib);
TODO(
		"zoom_view_win(ctx, 0, 0, mctx->hidlib->size_x, mctx->hidlib->size_y, 0);"
		"pan_view_abs(ctx, mctx->hidlib->size_x/2, mctx->hidlib->size_y/2, mctx->port.view.canvas_width/2.0, mctx->port.view.canvas_height/2.0);"
);
	}
}

#if 0
static gboolean rnd_mbtk_drawing_area_resize_cb()
{
	rnd_mbtk_gui_inited(mctx, 0, 1);

	TODO("set scaled on the scrollbars");
	return 0;
}
#endif

static void rnd_mbtk_main_export_init(rnd_mbtk_t *mctx)
{
	mctx->hid_active = 1;

	rnd_hid_cfg_keys_init(&rnd_mbtk_keymap);
	rnd_mbtk_keymap.translate_key = rnd_mbtk_translate_key;
	rnd_mbtk_keymap.key_name = rnd_mbtk_key_name;
	rnd_mbtk_keymap.auto_chr = 1;
	rnd_mbtk_keymap.auto_tr = rnd_hid_cfg_key_default_trans;
}

static void rnd_mbtk_main_export_widgets(rnd_mbtk_t *mctx)
{
	rnd_mbtk_populate_topwin(mctx);

	TODO("drawing area bindings");
#if 0
	/* Mouse and key events will need to be intercepted when PCB needs a location from the user. */
	bind_mouse_scroll(mctx->topwin->drawing_area, rnd_mbtk_window_mouse_scroll_cb, mctx);
	bind_mouse_enter(mctx->topwin->drawing_area, rnd_mbtk_window_enter_cb, mctx);
	bind_mouse_leave(mctx->topwin->drawing_area, rnd_mbtk_window_leave_cb, mctx);
	bind_mouse_motion(mctx->topwin->drawing_area, rnd_mbtk_window_motion_cb, mctx);
	bind_resize_dwg(mctx->topwin->drawing_area, rnd_mbtk_drawing_area_configure_event_cb, mctx);

	rnd_mbtk_interface_input_signals_connect();
#endif

	TODO("--listen");
#if 0
	if (rnd_mtbk_conf_hid.plugins.hid_mbtk.listen)
		rnd_mbtk_create_listener(mctx);
#endif

	mctx->gui_is_up = 1;

	rnd_mbtk_gui_inited(mctx, 1, 0);

	TODO("kbd focus");
#if 0
	/* Make sure drawing area has keyboard focus so that keys are handled
	   while the mouse cursor is over the top window or children widgets,
	   before first entering the drawing area */
	mbtk_widget_grab_focus(mctx->topwin->drawing_area);
#endif
}

static void rnd_mbtk_main_export_uninit(rnd_mbtk_t *mctx, rnd_hid_t *hid)
{
	rnd_hid_cfg_keys_uninit(&rnd_mbtk_keymap);

	mctx->hid_active = 0;
	mctx->gui_is_up = 0;
	hid->menu = NULL;
	hid->hid_data = NULL;
}

static void rnd_mbtk_do_exit_(rnd_mbtk_t *mctx)
{
#if 0
	rnd_mbtk_attr_dlg_free_all(mctx);

	/* Need to force-close the command entry first because it has its own main
	   loop that'd block the exit until the user closes the entry */
	rnd_mbtk_cmd_close(&mctx->topwin->cmd);

	rnd_mbtk_tw_dock_uninit();
#endif
}

static void rnd_mbtk_do_exit(rnd_hid_t *hid);

TODO("in mbtk this can be done when creating the window");
#if 0
void rnd_mbtk_topwinplace(rnd_hidlib_t *hidlib, *dialog, const char *id)
{
}
#endif

static int rnd_mbtk_topwin_create(rnd_mbtk_t *mctx, int *argc, char ***argv)
{
	mbtk_window_t *window;
	int plc[4] = {-1, -1, -1, -1};

TODO("draw");
#if 0
	mctx->view.use_max_hidlib = 1;
	mctx->view.coord_per_px = 300.0;
	rnd_pixel_slop = 300;

	mctx->impl.init_renderer(argc, argv, &mctx->port);
#endif


	if (rnd_conf.editor.auto_place)
		rnd_event(mctx->hidlib, RND_EVENT_DAD_NEW_DIALOG, "psp", NULL, "top", plc);

	rnd_mbtk_topwin_alloc(mctx);
	mctx->topwin->win = window = mbtk_window_new(&mctx->disp, MBTK_WNTY_NORMAL, "TODO: title", plc[0], plc[1], plc[2], plc[3]);

	TODO("return title so it can be set above");
#if 0
	mbtk_window_set_title(GTK_WINDOW(window), rnd_app.package);
#endif

	return 0;
}

int rnd_mbtk_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	int res;
	rnd_mbtk_t *mctx = hid->hid_data;
	rnd_conf_parse_arguments("plugins/hid_mbtk/", argc, argv);

	res = init_backend_cb(mctx, argc, argv);

	if (mbtk_be_init(&mctx->be) != 0) {
		fprintf(stderr, "Unable to initialize the %s backend\n", mctx->be.name);
		return -1;
	}

	if (mbtk_display_init(&mctx->be, &mctx->disp, argc, argv) != 0) {
		fprintf(stderr, "Unable to initialize mbtk display with the %s backend\n", mctx->be.name);
		return -1;
	}

	/* Set default style and font ID for the display */
	mbtk_f3d_style(&mctx->disp);
	mctx->disp.default_fid = mbtk_resolve_font(&mctx->disp, 1, &mctx->default_font);
	if (mctx->disp.default_fid.l == -1) {
		fprintf(stderr, "Unable to initialize mbtk display with the %s font: '%s'\n", mctx->be.name, mctx->default_font);
		return -1;
	}

	return rnd_mbtk_topwin_create(mctx, argc, argv);
}

static void rnd_mbtk_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int action)
{
	rnd_mbtk_t *mctx = hid->hid_data;
	int offset_x, offset_y;

	TODO("crosshair");
#if 0
	if ((mctx->topwin->drawing_area == NULL) || (mctx->hidlib == NULL))
		return;

	mbtk->impl.draw_grid_local(mctx->hidlib, x, y);
	mbtk_widget_window_get_origin(mctx->topwin->drawing_area, &offset_x, &offset_y);
	rnd_mbtk_crosshair_set(x, y, action, offset_x, offset_y, &mctx->port.view);
#endif
}

static int rnd_mbtk_get_coords(rnd_hid_t *hid, const char *msg, rnd_coord_t *x, rnd_coord_t *y, int force)
{
	rnd_mbtk_t *mctx = hid->hid_data;
	TODO("implemet");
}

static char *rnd_mbtk_fileselect(rnd_hid_t *hid, const char *title, const char *descr, const char *default_file, const char *default_ext, const rnd_hid_fsd_filter_t *flt, const char *history_tag, rnd_hid_fsd_flags_t flags, rnd_hid_dad_subdialog_t *sub)
{
	TODO("don't implemet: librnd4 removes this anyway");
}

static void *rnd_mbtk_attr_dlg_new_(rnd_hid_t *hid, const char *id, rnd_hid_attribute_t *attrs, int n_attrs, const char *title, void *caller_data, rnd_bool modal, void (*button_cb)(void *caller_data, rnd_hid_attr_ev_t ev), int defx, int defy, int minx, int miny)
{
	TODO("implemet");
}

static void PointCursor(rnd_hid_t *hid, rnd_bool grabbed)
{
	rnd_mbtk_t *mctx = hid->hid_data;

	if (mctx == NULL)
		return;

	TODO("implemet");
}

static int rnd_mbtk_remove_menu_node(rnd_hid_t *hid, lht_node_t *node)
{
	TODO("implemet");
}

static int rnd_mbtk_create_menu_by_node(rnd_hid_t *hid, int is_popup, const char *name, int is_main, lht_node_t *parent, lht_node_t *ins_after, lht_node_t *menu_item)
{
	TODO("implemet");
}


static void rnd_mbtk_update_menu_checkbox(rnd_hid_t *hid, const char *cookie)
{
	TODO("implemet");
}

rnd_hid_cfg_t *rnd_mbtk_get_menu_cfg(rnd_hid_t *hid)
{
	rnd_mbtk_t *mctx = hid->hid_data;
	if (!mctx->hid_active)
		return NULL;
	return hid->menu;
}

static int rnd_mbtk_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nMiniboxtk GUI command line arguments:\n\n");
	rnd_conf_usage("plugins/hid_mbtk", rnd_hid_usage_option);
	fprintf(stderr, "\nInvocation:\n");
	fprintf(stderr, "  %s --gui mbtk_xlib [options]\n", rnd_app.package);
	fprintf(stderr, "  (depending on which mbtk plugin(s) are compiled and installed)\n");
	return 0;
}

static const char *rnd_mbtk_command_entry(rnd_hid_t *hid, const char *ovr, int *cursor)
{
	rnd_mbtk_t *mctx = hid->hid_data;
	TODO("implemet");
}

static int rnd_mbtk_clip_set(rnd_hid_t *hid, rnd_hid_clipfmt_t format, const void *data, size_t len)
{
	rnd_mbtk_t *mctx = hid->hid_data;

	switch(format) {
		case RND_HID_CLIPFMT_TEXT:
			TODO("implemet");
			break;
	}
	return 0;
}

int rnd_mbtk_clip_get(rnd_hid_t *hid, rnd_hid_clipfmt_t *format, void **data, size_t *len)
{
	rnd_mbtk_t *mctx = hid->hid_data;
	int res = -1;
	TODO("implemet");

	if (res == 0)
		*format = RND_HID_CLIPFMT_TEXT;

	return res;
}

void rnd_mbtk_clip_free(rnd_hid_t *hid, rnd_hid_clipfmt_t format, void *data, size_t len)
{
	TODO("implemet");
	switch(format) {
		case RND_HID_CLIPFMT_TEXT:
			free(data);
			break;
	}
}

static int rnd_mbtk_dock_enter(rnd_hid_t *hid, rnd_hid_dad_subdialog_t *sub, rnd_hid_dock_t where, const char *id)
{
	rnd_mbtk_t *mctx = hid->hid_data;
	TODO("implemet");
}

static void rnd_mbtk_dock_leave(rnd_hid_t *hid, rnd_hid_dad_subdialog_t *sub)
{
	rnd_mbtk_t *mbctx = hid->hid_data;
	TODO("implemet");
}

static void rnd_mbtk_zoom_win(rnd_hid_t *hid, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_bool set_crosshair)
{
	rnd_mbtk_t *mctx = hid->hid_data;
	TODO("implemet");
}

static void rnd_mbtk_zoom(rnd_hid_t *hid, rnd_coord_t center_x, rnd_coord_t center_y, double factor, int relative)
{
	rnd_mbtk_t *mctx = hid->hid_data;

TODO("implemet");
#if 0
	if (relative)
		rnd_mbtk_zoom_view_rel(&mctx->view, center_x, center_y, factor);
	else
		rnd_mbtk_zoom_view_abs(&mctx->view, center_x, center_y, factor);
#endif
}

static void rnd_mbtk_pan(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int relative)
{
	rnd_mbtk_t *mctx = hid->hid_data;
TODO("implemet");
#if 0
	if (relative)
		rnd_mbtk_pan_view_rel(&mctx->view, x, y);
	else
		rnd_mbtk_pan_view_abs(&mctx->view, x, y, mctx->view.canvas_width/2.0, mctx->view.canvas_height/2.0);
#endif
}

static void rnd_mbtk_pan_mode(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, rnd_bool mode)
{
	rnd_mbtk_t *mctx = hid->hid_data;
	mctx->view.panning = mode;
}

static void rnd_mbtk_view_get(rnd_hid_t *hid, rnd_box_t *viewbox)
{
	rnd_mbtk_t *mctx = hid->hid_data;
	viewbox->X1 = mctx->view.x0;
	viewbox->Y1 = mctx->view.y0;
	viewbox->X2 = rnd_round((double)mctx->view.x0 + (double)mctx->view.canvas_width * mctx->view.coord_per_px);
	viewbox->Y2 = rnd_round((double)mctx->view.y0 + (double)mctx->view.canvas_height * mctx->view.coord_per_px);
}

static void rnd_mbtk_open_command(rnd_hid_t *hid)
{
	rnd_mbtk_t *mctx = hid->hid_data;
TODO("implemet");
}

static int rnd_mbtk_open_popup(rnd_hid_t *hid, const char *menupath)
{
	rnd_mbtk_t *mctx = hid->hid_data;
	void *menu = NULL;
	lht_node_t *menu_node = rnd_hid_cfg_get_menu(hid->menu, menupath);

	if (menu_node == NULL)
		return 1;

#if 0
	menu = rnd_mbtk_menu_popup_pre(menu_node);
	if (menu == NULL) {
		rnd_message(RND_MSG_ERROR, "The specified popup menu \"%s\" has not been defined.\n", menupath);
		return 1;
	}
#endif

	mctx->view.panning = 0; /* corner case: on a popup mbtk may not deliver button releases because the popup takes focus; it's safer to turn off panning */

#if 0
	mbtk_widget_grab_focus(mctx->topwin->drawing_area);
	rnd_mbtk_menu_popup(mctx, menu);
#endif

	return 0;
}

static void rnd_mbtk_set_hidlib(rnd_hid_t *hid, rnd_hidlib_t *hidlib)
{
	rnd_mbtk_t *mctx = hid->hid_data;

	if (mctx == NULL)
		return;

	mctx->hidlib = hidlib;

	if(!mctx->hid_active)
		return;

	if (hidlib == NULL)
		return;

	if (!mctx->drawing_allowed)
		return;

TODO("implemet");
#if 0
	rnd_mbtk_tw_ranges_scale(mctx);
	rnd_mbtk_zoom_view_win(&mctx->view, 0, 0, hidlib->size_x, hidlib->size_y, 0);
#endif
}

static rnd_hidlib_t *rnd_mbtk_get_hidlib(rnd_hid_t *hid)
{
	rnd_mbtk_t *mctx = hid->hid_data;

	if (mctx == NULL)
		return NULL;

	return mctx->hidlib;
}

static void rnd_mbtk_reg_mouse_cursor(rnd_hid_t *hid, int idx, const char *name, const unsigned char *pixel, const unsigned char *mask)
{
	TODO("implemet");
}

static void rnd_mbtk_set_mouse_cursor(rnd_hid_t *hid, int idx)
{
	TODO("implemet");
}

static void rnd_mbtk_set_top_title(rnd_hid_t *hid, const char *title)
{
	rnd_mbtk_t *mctx = hid->hid_data;
	TODO("implemet");
}

static void rnd_mbtk_busy(rnd_hid_t *hid, rnd_bool busy)
{
	TODO("Del: will be removed in librnd4");
}

static int rnd_mbtk_shift_is_pressed(rnd_hid_t *hid)
{
	rnd_mbtk_t *mctx = hid->hid_data;

	if (!mctx->gui_is_up)
		return 0;

	TODO("implemet");
}

static int rnd_mbtk_control_is_pressed(rnd_hid_t *hid)
{
	rnd_mbtk_t *mctx = hid->hid_data;

	if (!mctx->gui_is_up)
		return 0;

	TODO("implemet");
}

static int rnd_mbtk_mod1_is_pressed(rnd_hid_t *hid)
{
	rnd_mbtk_t *mctx = hid->hid_data;

	if (!mctx->gui_is_up)
		return 0;

	TODO("implemet");
}

static void rnd_mbtk_init_pixmap(rnd_hid_t *hid, rnd_pixmap_t *pxm)
{
	TODO("implemet");
}

static void rnd_mbtk_draw_pixmap(rnd_hid_t *hid, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t sx, rnd_coord_t sy, rnd_pixmap_t *pixmap)
{
	TODO("implemet");
}

static void rnd_mbtk_uninit_pixmap(rnd_hid_t *hid, rnd_pixmap_t *pixmap)
{
	TODO("implemet");
}

void rnd_mbtk_iterate(rnd_hid_t *hid)
{
	rnd_mbtk_t *mctx = hid->hid_data;
	mbtk_event_t tmp;

	mbtk_iterate(&mctx->disp, &tmp, 0);
}

static double rnd_mbtk_benchmark(rnd_hid_t *hid)
{
	TODO("implemet");
	return 0;
}

static void rnd_mbtk_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	rnd_mbtk_t *mctx = hid->hid_data;

	rnd_mbtk_main_export_widgets(mctx);
	mbtk_run_window(mctx->topwin->win);
	rnd_mbtk_topwin_free(mctx);
}

static void rnd_mbtk_do_exit(rnd_hid_t *hid)
{
}

static rnd_mbtk_t mbtk_global;

void rnd_mbtk_glue_hid_init(rnd_hid_t *dst, int (*init_backend)(rnd_mbtk_t *mctx, int *argc, char **argv[]))
{
	memset(dst, 0, sizeof(rnd_hid_t));

	init_backend_cb = init_backend;

	rnd_hid_nogui_init(dst);

	dst->struct_size = sizeof(rnd_hid_t);
	dst->gui = 1;
	dst->heavy_term_layer_ind = 1;
	dst->allow_dad_before_init = 1;

	dst->do_export = rnd_mbtk_do_export;
	dst->do_exit = rnd_mbtk_do_exit;
	dst->iterate = rnd_mbtk_iterate;
	dst->parse_arguments = rnd_mbtk_parse_arguments;

	dst->shift_is_pressed = rnd_mbtk_shift_is_pressed;
	dst->control_is_pressed = rnd_mbtk_control_is_pressed;
	dst->mod1_is_pressed = rnd_mbtk_mod1_is_pressed;
	dst->get_coords = rnd_mbtk_get_coords;
	dst->set_crosshair = rnd_mbtk_set_crosshair;
	dst->add_timer = rnd_mbtk_add_timer;
	dst->stop_timer = rnd_mbtk_stop_timer;
	dst->watch_file = rnd_mbtk_watch_file;
	dst->unwatch_file = rnd_mbtk_unwatch_file;

	dst->fileselect = rnd_mbtk_fileselect;
	dst->attr_dlg_new = rnd_mbtk_attr_dlg_new_;
	dst->attr_dlg_run = rnd_mbtk_attr_dlg_run;
	dst->attr_dlg_raise = rnd_mbtk_attr_dlg_raise;
	dst->attr_dlg_close = rnd_mbtk_attr_dlg_close;
	dst->attr_dlg_free = rnd_mbtk_attr_dlg_free;
	dst->attr_dlg_property = rnd_mbtk_attr_dlg_property;
	dst->attr_dlg_widget_state = rnd_mbtk_attr_dlg_widget_state;
	dst->attr_dlg_widget_hide = rnd_mbtk_attr_dlg_widget_hide;
	dst->attr_dlg_widget_poke = rnd_mbtk_attr_dlg_widget_poke;
	dst->attr_dlg_set_value = rnd_mbtk_attr_dlg_set_value;
	dst->attr_dlg_set_help = rnd_mbtk_attr_dlg_set_help;

	dst->supports_dad_text_markup = 1;

	dst->dock_enter = rnd_mbtk_dock_enter;
	dst->dock_leave = rnd_mbtk_dock_leave;

	dst->beep = rnd_mbtk_beep;
	dst->point_cursor = PointCursor;
	dst->benchmark = rnd_mbtk_benchmark;

	dst->command_entry = rnd_mbtk_command_entry;

	dst->create_menu_by_node = rnd_mbtk_create_menu_by_node;
	dst->remove_menu_node = rnd_mbtk_remove_menu_node;
	dst->update_menu_checkbox = rnd_mbtk_update_menu_checkbox;
	dst->get_menu_cfg = rnd_mbtk_get_menu_cfg;

	dst->clip_set  = rnd_mbtk_clip_set;
	dst->clip_get  = rnd_mbtk_clip_get;
	dst->clip_free = rnd_mbtk_clip_free;

	dst->zoom_win = rnd_mbtk_zoom_win;
	dst->zoom = rnd_mbtk_zoom;
	dst->pan = rnd_mbtk_pan;
	dst->pan_mode = rnd_mbtk_pan_mode;
	dst->view_get = rnd_mbtk_view_get;
	dst->open_command = rnd_mbtk_open_command;
	dst->open_popup = rnd_mbtk_open_popup;
	dst->reg_mouse_cursor = rnd_mbtk_reg_mouse_cursor;
	dst->set_mouse_cursor = rnd_mbtk_set_mouse_cursor;
	dst->set_top_title = rnd_mbtk_set_top_title;
	dst->busy = rnd_mbtk_busy;

	dst->set_hidlib = rnd_mbtk_set_hidlib;
	dst->get_hidlib = rnd_mbtk_get_hidlib;
	dst->get_dad_hidlib = rnd_mbtk_attr_get_dad_hidlib;

	dst->key_state = &rnd_mbtk_keymap;

	dst->usage = rnd_mbtk_usage;

	dst->draw_pixmap = rnd_mbtk_draw_pixmap;
	dst->uninit_pixmap = rnd_mbtk_uninit_pixmap;

	dst->hid_data = &mbtk_global;
}
