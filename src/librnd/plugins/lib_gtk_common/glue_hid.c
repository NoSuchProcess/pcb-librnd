#include <librnd/rnd_config.h>
#include "glue_hid.h"

#include <locale.h>

#include "rnd_gtk.h"
#include <librnd/core/actions.h>
#include "glue_hid.h"
#include <librnd/core/hid_nogui.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/pixmap.h>
#include "coord_conv.h"

#include "in_keyboard.h"
#include "bu_dwg_tooltip.h"
#include "ui_crosshair.h"
#include "dlg_fileselect.h"
#include "dlg_attribute.h"
#include "util_listener.h"
#include "util_timer.h"
#include "util_watch.h"
#include "hid_gtk_conf.h"
#include "lib_gtk_config.h"
#include "glue_common.h"
#include <librnd/plugins/lib_hid_common/lib_hid_common.h>
#include <librnd/plugins/lib_hid_common/menu_helper.h>

extern rnd_hid_cfg_keys_t rnd_gtk_keymap;

static gint rnd_gtkg_window_enter_cb(GtkWidget *widget, long x, long y, long z, void *ctx_)
{
	rnd_gtk_t *gctx = ctx_;
	rnd_gtk_port_t *out = &gctx->port;
	int force_update = 0;

	/* printf("enter: mode: %d detail: %d\n", ev->mode, ev->detail); */

	out->view.has_entered = TRUE;
	force_update = 1; /* force a redraw for the crosshair */
	if (!gctx->topwin.cmd.command_entry_status_line_active)
		gtk_widget_grab_focus(out->drawing_area); /* Make sure drawing area has keyboard focus when we are in it (don't steal keys from the command line, tho) */

	if (force_update || x)
		gctx->impl.screen_update();
	return FALSE;
}

static gint rnd_gtkg_window_leave_cb(GtkWidget *widget, long x, long y, long z, void *ctx_)
{
	rnd_gtk_t *gctx = ctx_;
	rnd_gtk_port_t *out = &gctx->port;

	/* printf("leave mode: %d detail: %d\n", ev->mode, ev->detail); */

	out->view.has_entered = FALSE;

	gctx->impl.screen_update();

	return FALSE;
}

static gboolean check_object_tooltips(rnd_gtk_t *gctx)
{
	rnd_gtk_port_t *out = &gctx->port;
	return rnd_gtk_dwg_tooltip_check_object(gctx->hidlib, out->drawing_area, out->view.crosshair_x, out->view.crosshair_y);
}

static gint rnd_gtkg_window_motion_cb(GtkWidget *widget, long x, long y, long z, void *ctx_)
{
	rnd_gtk_t *gctx = ctx_;
	rnd_gtk_port_t *out = &gctx->port;
	gdouble dx, dy;
	static gint x_prev = -1, y_prev = -1;

	if (out->view.panning) {
		dx = gctx->port.view.coord_per_px * (x_prev - x);
		dy = gctx->port.view.coord_per_px * (y_prev - y);
		if (x_prev > 0)
			rnd_gtk_pan_view_rel(&gctx->port.view, dx, dy);
		x_prev = x;
		y_prev = y;
		return FALSE;
	}
	x_prev = y_prev = -1;
	rnd_gtk_note_event_location(x, y, 1);

	rnd_gtk_dwg_tooltip_queue(out->drawing_area, (GSourceFunc)check_object_tooltips, gctx);

	return FALSE;
}

static void rnd_gtkg_gui_inited(rnd_gtk_t *gctx, int main, int conf)
{
	static int im = 0, ic = 0, first = 1;
	if (main) im = 1;
	if (conf) ic = 1;

	if (im && ic && first) {
		first = 0;
		rnd_hid_announce_gui_init(gctx->hidlib);
		rnd_gtk_zoom_view_win(&gctx->port.view, 0, 0, gctx->hidlib->size_x, gctx->hidlib->size_y, 0);
		rnd_gtk_pan_view_abs(&gctx->port.view, gctx->hidlib->size_x/2, gctx->hidlib->size_y/2, gctx->port.view.canvas_width/2.0, gctx->port.view.canvas_height/2.0);
	}
}

static gboolean rnd_gtkg_drawing_area_configure_event_cb(GtkWidget *widget, long sx, long sy, long z, void *ctx_)
{
	rnd_gtk_t *gctx = ctx_;
	rnd_gtk_port_t *out = &gctx->port;

	gctx->port.view.canvas_width = sx;
	gctx->port.view.canvas_height = sy;
	gctx->impl.drawing_area_configure_hook(out);
	rnd_gtkg_gui_inited(gctx, 0, 1);

	rnd_gtk_tw_ranges_scale(gctx);
	rnd_gui->invalidate_all(rnd_gui);
	return 0;
}

static void rnd_gtkg_main_export_init(rnd_gtk_t *gctx)
{
	gctx->hid_active = 1;

	rnd_hid_cfg_keys_init(&rnd_gtk_keymap);
	rnd_gtk_keymap.translate_key = rnd_gtk_translate_key;
	rnd_gtk_keymap.key_name = rnd_gtk_key_name;
	rnd_gtk_keymap.auto_chr = 1;
	rnd_gtk_keymap.auto_tr = rnd_hid_cfg_key_default_trans;
}

static void rnd_gtkg_main_export_widgets(rnd_gtk_t *gctx)
{
	rnd_gtk_create_topwin_widgets(gctx, &gctx->topwin, gctx->port.top_window);

	gctx->port.drawing_area = gctx->topwin.drawing_area;

TODO(": move this to render init")
	/* Mouse and key events will need to be intercepted when PCB needs a location from the user. */
	gtkc_bind_mouse_scroll(gctx->port.drawing_area, rnd_gtkc_xy_ev(&gctx->topwin.dwg_rs, rnd_gtk_window_mouse_scroll_cb, gctx));
	gtkc_bind_mouse_enter(gctx->port.drawing_area, rnd_gtkc_xy_ev(&gctx->topwin.dwg_enter, rnd_gtkg_window_enter_cb, gctx));
	gtkc_bind_mouse_leave(gctx->port.drawing_area, rnd_gtkc_xy_ev(&gctx->topwin.dwg_leave, rnd_gtkg_window_leave_cb, gctx));
	gtkc_bind_mouse_motion(gctx->port.drawing_area, rnd_gtkc_xy_ev(&gctx->topwin.dwg_motion, rnd_gtkg_window_motion_cb, gctx));
	gtkc_bind_resize_dwg(gctx->port.drawing_area, rnd_gtkc_xy_ev(&gctx->topwin.dwg_sc, rnd_gtkg_drawing_area_configure_event_cb, gctx));

	rnd_gtk_interface_input_signals_connect();

	if (rnd_gtk_conf_hid.plugins.hid_gtk.listen)
		rnd_gtk_create_listener(gctx);

	gctx->gui_is_up = 1;

	rnd_gtkg_gui_inited(gctx, 1, 0);

	/* Make sure drawing area has keyboard focus so that keys are handled
	   while the mouse cursor is over the top window or children widgets,
	   before first entering the drawing area */
	gtk_widget_grab_focus(gctx->port.drawing_area);
}

static void rnd_gtkg_main_export_uninit(rnd_gtk_t *gctx, rnd_hid_t *hid)
{
	rnd_hid_cfg_keys_uninit(&rnd_gtk_keymap);

	gctx->hid_active = 0;
	gctx->gui_is_up = 0;
	hid->menu = NULL;
	hid->hid_data = NULL;
}

static void rnd_gtkg_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options);

static void rnd_gtkg_do_exit_(rnd_gtk_t *gctx)
{
	/* Need to force-close the command entry first because it has its own main
	   loop that'd block the exit until the user closes the entry */
	rnd_gtk_cmd_close(&gctx->topwin.cmd);

	rnd_gtk_tw_dock_uninit();
}

static void rnd_gtkg_do_exit(rnd_hid_t *hid);


static void rnd_gtk_topwinplace(rnd_hidlib_t *hidlib, GtkWidget *dialog, const char *id)
{
	int plc[4] = {-1, -1, -1, -1};

	rnd_event(hidlib, RND_EVENT_DAD_NEW_DIALOG, "psp", NULL, id, plc);

	if (rnd_conf.editor.auto_place) {
		if ((plc[2] > 0) && (plc[3] > 0))
			gtkc_window_resize(GTK_WINDOW(dialog), plc[2], plc[3]);
		if ((plc[0] >= 0) && (plc[1] >= 0))
			gtkc_window_move(GTK_WINDOW(dialog), plc[0], plc[1]);
	}
}

static void rnd_gtk_topwin_create(rnd_gtk_t *gctx, int *argc, char ***argv)
{
	GtkWidget *window;

	gctx->port.view.use_max_hidlib = 1;
	gctx->port.view.coord_per_px = 300.0;
	rnd_pixel_slop = 300;

	gctx->impl.init_renderer(argc, argv, &gctx->port);
	gctx->wtop_window = window = gctx->port.top_window = gtkc_topwin_new();

	rnd_gtk_topwinplace(gctx->hidlib, window, "top");
	gtk_window_set_title(GTK_WINDOW(window), rnd_app.package);

	gtkc_widget_show_all(gctx->port.top_window);
}

static void rnd_gtk_parse_arguments_first(rnd_gtk_t *gctx, rnd_hid_t *hid, int *argc, char ***argv)
{
#ifdef WIN32
	/* make sure gtk won't ruin the locale */
	gtk_disable_setlocale();
#endif

	rnd_conf_parse_arguments("plugins/hid_gtk/", argc, argv);

}

/* Create top level window for routines that will need top_window before rnd_gtk_create_topwin_widgets() is called. */
int rnd_gtk_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv);

static void rnd_gtkg_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int action)
{
	rnd_gtk_t *gctx = hid->hid_data;
	int offset_x, offset_y;

	if ((gctx->port.drawing_area == NULL) || (gctx->hidlib == NULL))
		return;

	gctx->impl.draw_grid_local(gctx->hidlib, x, y);
	gdkc_widget_window_get_origin(gctx->port.drawing_area, &offset_x, &offset_y);
	rnd_gtk_crosshair_set(x, y, action, offset_x, offset_y, &gctx->port.view);
}

static int rnd_gtkg_get_coords(rnd_hid_t *hid, const char *msg, rnd_coord_t *x, rnd_coord_t *y, int force)
{
	rnd_gtk_t *gctx = hid->hid_data;
	return rnd_gtk_get_coords(gctx, &gctx->port.view, msg, x, y, force);
}

rnd_hidval_t rnd_gtkg_add_timer(rnd_hid_t *hid, void (*func)(rnd_hidval_t user_data), unsigned long milliseconds, rnd_hidval_t user_data)
{
	return rnd_gtk_add_timer((rnd_gtk_t *)hid->hid_data, func, milliseconds, user_data);
}

static rnd_hidval_t rnd_gtkg_watch_file(rnd_hid_t *hid, int fd, unsigned int condition,
	rnd_bool (*func)(rnd_hidval_t, int, unsigned int, rnd_hidval_t), rnd_hidval_t user_data)
{
	return rnd_gtk_watch_file((rnd_gtk_t *)hid->hid_data, fd, condition, func, user_data);
}

static char *rnd_gtkg_fileselect(rnd_hid_t *hid, const char *title, const char *descr, const char *default_file, const char *default_ext, const rnd_hid_fsd_filter_t *flt, const char *history_tag, rnd_hid_fsd_flags_t flags, rnd_hid_dad_subdialog_t *sub)
{
	return rnd_gtk_fileselect(hid, (rnd_gtk_t *)hid->hid_data, title, descr, default_file, default_ext, flt, history_tag, flags, sub);
}

static void *rnd_gtk_attr_dlg_new_(rnd_hid_t *hid, const char *id, rnd_hid_attribute_t *attrs, int n_attrs, const char *title, void *caller_data, rnd_bool modal, void (*button_cb)(void *caller_data, rnd_hid_attr_ev_t ev), int defx, int defy, int minx, int miny)
{
	return rnd_gtk_attr_dlg_new((rnd_gtk_t *)hid->hid_data, id, attrs, n_attrs, title, caller_data, modal, button_cb, defx, defy, minx, miny);
}

static void rnd_gtkg_beep(rnd_hid_t *hid);

static void PointCursor(rnd_hid_t *hid, rnd_bool grabbed)
{
	rnd_gtk_t *gctx = hid->hid_data;

	if (gctx == NULL)
		return;

	rnd_gtk_point_cursor(gctx, grabbed);
}

static int rnd_gtk_remove_menu_node(rnd_hid_t *hid, lht_node_t *node)
{
	rnd_gtk_t *gctx = hid->hid_data;
	return rnd_hid_cfg_remove_menu_node(hid->menu, node, rnd_gtk_remove_menu_widget, gctx->topwin.menu.menu_bar);
}

static int rnd_gtk_create_menu_by_node(rnd_hid_t *hid, int is_popup, const char *name, int is_main, lht_node_t *parent, lht_node_t *ins_after, lht_node_t *menu_item)
{
	rnd_gtk_t *gctx = hid->hid_data;
	return rnd_gtk_create_menu_widget(&gctx->topwin.menu, is_popup, name, is_main, parent, ins_after, menu_item);
}


static void rnd_gtk_update_menu_checkbox(rnd_hid_t *hid, const char *cookie)
{
	rnd_gtk_t *gctx = hid->hid_data;
	if ((gctx->hid_active) && (gctx->hidlib != NULL))
		rnd_gtk_update_toggle_flags(gctx->hidlib, &gctx->topwin, cookie);
}

rnd_hid_cfg_t *rnd_gtkg_get_menu_cfg(rnd_hid_t *hid)
{
	rnd_gtk_t *gctx = hid->hid_data;
	if (!gctx->hid_active)
		return NULL;
	return hid->menu;
}

static int rnd_gtkg_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nGTK GUI command line arguments:\n\n");
	rnd_conf_usage("plugins/hid_gtk", rnd_hid_usage_option);
	fprintf(stderr, "\nInvocation:\n");
	fprintf(stderr, "  %s --gui gtk2_gdk [options]\n", rnd_app.package);
	fprintf(stderr, "  %s --gui gtk2_gl [options]\n", rnd_app.package);
	fprintf(stderr, "  (depending on which gtk plugin(s) are compiled and installed)\n");
	return 0;
}

static const char *rnd_gtk_command_entry(rnd_hid_t *hid, const char *ovr, int *cursor)
{
	rnd_gtk_t *gctx = hid->hid_data;
	return rnd_gtk_cmd_command_entry(&gctx->topwin.cmd, ovr, cursor);
}

static int rnd_gtkg_clip_set(rnd_hid_t *hid, rnd_hid_clipfmt_t format, const void *data, size_t len)
{
	rnd_gtk_t *gctx = hid->hid_data;

	switch(format) {
		case RND_HID_CLIPFMT_TEXT:
			gtkc_clipboard_set_text(gctx->topwin.drawing_area, data);
			break;
	}
	return 0;
}

int rnd_gtkg_clip_get(rnd_hid_t *hid, rnd_hid_clipfmt_t *format, void **data, size_t *len)
{
	rnd_gtk_t *gctx = hid->hid_data;
	int res = gtkc_clipboard_get_text(gctx->topwin.drawing_area, data, len);

	if (res == 0)
		*format = RND_HID_CLIPFMT_TEXT;

	return res;
}

void rnd_gtkg_clip_free(rnd_hid_t *hid, rnd_hid_clipfmt_t format, void *data, size_t len)
{
	switch(format) {
		case RND_HID_CLIPFMT_TEXT:
			g_free(data);
			break;
	}
}

static void rnd_gtkg_iterate(rnd_hid_t *hid);
static double rnd_gtkg_benchmark(rnd_hid_t *hid);

static int rnd_gtkg_dock_enter(rnd_hid_t *hid, rnd_hid_dad_subdialog_t *sub, rnd_hid_dock_t where, const char *id)
{
	rnd_gtk_t *gctx = hid->hid_data;
	return rnd_gtk_tw_dock_enter(&gctx->topwin, sub, where, id);
}

static void rnd_gtkg_dock_leave(rnd_hid_t *hid, rnd_hid_dad_subdialog_t *sub)
{
	rnd_gtk_t *gctx = hid->hid_data;
	rnd_gtk_tw_dock_leave(&gctx->topwin, sub);
}

static void rnd_gtkg_zoom_win(rnd_hid_t *hid, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_bool set_crosshair)
{
	rnd_gtk_t *gctx = hid->hid_data;
	rnd_gtk_zoom_view_win(&gctx->port.view, x1, y1, x2, y2, set_crosshair);
}

static void rnd_gtkg_zoom(rnd_hid_t *hid, rnd_coord_t center_x, rnd_coord_t center_y, double factor, int relative)
{
	rnd_gtk_t *gctx = hid->hid_data;
	if (relative)
		rnd_gtk_zoom_view_rel(&gctx->port.view, center_x, center_y, factor);
	else
		rnd_gtk_zoom_view_abs(&gctx->port.view, center_x, center_y, factor);
}

static void rnd_gtkg_pan(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int relative)
{
	rnd_gtk_t *gctx = hid->hid_data;
	if (relative)
		rnd_gtk_pan_view_rel(&gctx->port.view, x, y);
	else
		rnd_gtk_pan_view_abs(&gctx->port.view, x, y, gctx->port.view.canvas_width/2.0, gctx->port.view.canvas_height/2.0);
}

static void rnd_gtkg_pan_mode(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, rnd_bool mode)
{
	rnd_gtk_t *gctx = hid->hid_data;
	gctx->port.view.panning = mode;
}

static void rnd_gtkg_view_get(rnd_hid_t *hid, rnd_box_t *viewbox)
{
	rnd_gtk_t *gctx = hid->hid_data;
	viewbox->X1 = gctx->port.view.x0;
	viewbox->Y1 = gctx->port.view.y0;
	viewbox->X2 = rnd_round((double)gctx->port.view.x0 + (double)gctx->port.view.canvas_width * gctx->port.view.coord_per_px);
	viewbox->Y2 = rnd_round((double)gctx->port.view.y0 + (double)gctx->port.view.canvas_height * gctx->port.view.coord_per_px);
}

static void rnd_gtkg_open_command(rnd_hid_t *hid)
{
	rnd_gtk_t *gctx = hid->hid_data;
	rnd_gtk_handle_user_command(gctx->hidlib, &gctx->topwin.cmd, TRUE);
}

static int rnd_gtkg_open_popup(rnd_hid_t *hid, const char *menupath)
{
	rnd_gtk_t *gctx = hid->hid_data;
	void *menu = NULL;
	lht_node_t *menu_node = rnd_hid_cfg_get_menu(hid->menu, menupath);

	if (menu_node == NULL)
		return 1;

	menu = rnd_gtk_menu_popup_pre(menu_node);
	if (menu == NULL) {
		rnd_message(RND_MSG_ERROR, "The specified popup menu \"%s\" has not been defined.\n", menupath);
		return 1;
	}

	gctx->port.view.panning = 0; /* corner case: on a popup gtk won't deliver button releases because the popup takes focus; it's safer to turn off panning */

	gtk_widget_grab_focus(gctx->port.drawing_area);
	gtkc_menu_popup(gctx, menu);

	return 0;
}

static void rnd_gtkg_set_hidlib(rnd_hid_t *hid, rnd_hidlib_t *hidlib)
{
	rnd_gtk_t *gctx = hid->hid_data;

	if (gctx == NULL)
		return;

	gctx->hidlib = hidlib;

	if(!gctx->hid_active)
		return;

	if (hidlib == NULL)
		return;

	if (!gctx->port.drawing_allowed)
		return;

	rnd_gtk_tw_ranges_scale(gctx);
	rnd_gtk_zoom_view_win(&gctx->port.view, 0, 0, hidlib->size_x, hidlib->size_y, 0);
}

static void rnd_gtkg_reg_mouse_cursor(rnd_hid_t *hid, int idx, const char *name, const unsigned char *pixel, const unsigned char *mask)
{
	rnd_gtk_reg_mouse_cursor((rnd_gtk_t *)hid->hid_data, idx, name, pixel, mask);
}

static void rnd_gtkg_set_mouse_cursor(rnd_hid_t *hid, int idx)
{
	rnd_gtk_set_mouse_cursor((rnd_gtk_t *)hid->hid_data, idx);
}

static void rnd_gtkg_set_top_title(rnd_hid_t *hid, const char *title)
{
	rnd_gtk_t *gctx = hid->hid_data;
	rnd_gtk_tw_set_title(&gctx->topwin, title);
}

static void rnd_gtkg_busy(rnd_hid_t *hid, rnd_bool busy)
{
	rnd_gtk_t *gctx = hid->hid_data;
	if ((gctx == NULL) || (!gctx->hid_active))
		return;
	if (busy)
		rnd_gtk_watch_cursor(gctx);
	else
		rnd_gtk_restore_cursor(gctx);
}

static int rnd_gtkg_shift_is_pressed(rnd_hid_t *hid)
{
	rnd_gtk_t *gctx = hid->hid_data;
	GdkModifierType mask;
	rnd_gtk_port_t *out = &gctx->port;

	if (!gctx->gui_is_up)
		return 0;

	gdkc_window_get_pointer(out->drawing_area, NULL, NULL, &mask);

#ifdef RND_WORKAROUND_GTK_SHIFT
	/* On some systems the above query fails and we need to return the last known state instead */
	return rnd_gtk_glob_mask & GDK_SHIFT_MASK;
#else
	return (mask & GDK_SHIFT_MASK) ? TRUE : FALSE;
#endif
}

static int rnd_gtkg_control_is_pressed(rnd_hid_t *hid)
{
	rnd_gtk_t *gctx = hid->hid_data;
	GdkModifierType mask;
	rnd_gtk_port_t *out = &gctx->port;

	if (!gctx->gui_is_up)
		return 0;

	gdkc_window_get_pointer(out->drawing_area, NULL, NULL, &mask);

#ifdef RND_WORKAROUND_GTK_CTRL
	/* On some systems the above query fails and we need to return the last known state instead */
	return rnd_gtk_glob_mask & GDK_CONTROL_MASK;
#else
	return (mask & GDK_CONTROL_MASK) ? TRUE : FALSE;
#endif
}

static int rnd_gtkg_mod1_is_pressed(rnd_hid_t *hid)
{
	rnd_gtk_t *gctx = hid->hid_data;
	GdkModifierType mask;
	rnd_gtk_port_t *out = &gctx->port;

	if (!gctx->gui_is_up)
		return 0;

	gdkc_window_get_pointer(out->drawing_area, NULL, NULL, &mask);

	return gtkc_mod1_in_mask(mask);
}

static void rnd_gtkg_init_pixmap(rnd_hid_t *hid, rnd_pixmap_t *pxm)
{
	rnd_gtk_pixmap_t *gtk_px = calloc(sizeof(rnd_gtk_pixmap_t), 1);

	gtk_px->pxm = pxm;
	pxm->hid_data = gtk_px;
	rnd_gtkg_init_pixmap_low(gtk_px);
}

static void rnd_gtkg_uninit_pixmap_(rnd_hid_t *hid, rnd_pixmap_t *pxm)
{
	rnd_gtkg_uninit_pixmap_low((rnd_gtk_pixmap_t *)(pxm->hid_data));
	free(pxm->hid_data);
}

static void rnd_gtkg_draw_pixmap(rnd_hid_t *hid, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t sx, rnd_coord_t sy, rnd_pixmap_t *pixmap)
{
	rnd_gtk_t *gctx = hid->hid_data;

	if (pixmap->hid_data == NULL)
		rnd_gtkg_init_pixmap(hid, pixmap);
	if (pixmap->hid_data != NULL) {
		double rsx, rsy, ca = cos(pixmap->tr_rot / RND_RAD_TO_DEG), sa = sin(pixmap->tr_rot / RND_RAD_TO_DEG);
		rsx = (double)sx * ca + (double)sy * sa;
		rsy = (double)sy * ca + (double)sx * sa;
/*rnd_trace("GUI scale: %mm %mm -> %mm %mm\n", sx, sy, (rnd_coord_t)rsx, (rnd_coord_t)rsy);*/
		gctx->impl.draw_pixmap(gctx->hidlib, pixmap->hid_data, cx - rsx/2.0, cy - rsy/2.0, rsx, rsy);
	}
}

static void rnd_gtkg_uninit_pixmap(rnd_hid_t *hid, rnd_pixmap_t *pixmap)
{
	if (pixmap->hid_data != NULL) {
		rnd_gtkg_uninit_pixmap_(hid, pixmap);
		pixmap->hid_data = NULL;
	}
}

void rnd_gtk_glue_hid_init(rnd_hid_t *dst)
{
	memset(dst, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(dst);

	dst->struct_size = sizeof(rnd_hid_t);
	dst->gui = 1;
	dst->heavy_term_layer_ind = 1;
	dst->allow_dad_before_init = 1;

	dst->do_export = rnd_gtkg_do_export;
	dst->do_exit = rnd_gtkg_do_exit;
	dst->iterate = rnd_gtkg_iterate;
	dst->parse_arguments = rnd_gtk_parse_arguments;

	dst->shift_is_pressed = rnd_gtkg_shift_is_pressed;
	dst->control_is_pressed = rnd_gtkg_control_is_pressed;
	dst->mod1_is_pressed = rnd_gtkg_mod1_is_pressed;
	dst->get_coords = rnd_gtkg_get_coords;
	dst->set_crosshair = rnd_gtkg_set_crosshair;
	dst->add_timer = rnd_gtkg_add_timer;
	dst->stop_timer = rnd_gtk_stop_timer;
	dst->watch_file = rnd_gtkg_watch_file;
	dst->unwatch_file = rnd_gtk_unwatch_file;

	dst->fileselect = rnd_gtkg_fileselect;
	dst->attr_dlg_new = rnd_gtk_attr_dlg_new_;
	dst->attr_dlg_run = rnd_gtk_attr_dlg_run;
	dst->attr_dlg_raise = rnd_gtk_attr_dlg_raise;
	dst->attr_dlg_close = rnd_gtk_attr_dlg_close;
	dst->attr_dlg_free = rnd_gtk_attr_dlg_free;
	dst->attr_dlg_property = rnd_gtk_attr_dlg_property;
	dst->attr_dlg_widget_state = rnd_gtk_attr_dlg_widget_state;
	dst->attr_dlg_widget_hide = rnd_gtk_attr_dlg_widget_hide;
	dst->attr_dlg_widget_poke = rnd_gtk_attr_dlg_widget_poke;
	dst->attr_dlg_set_value = rnd_gtk_attr_dlg_set_value;
	dst->attr_dlg_set_help = rnd_gtk_attr_dlg_set_help;

	dst->supports_dad_text_markup = 1;

	dst->dock_enter = rnd_gtkg_dock_enter;
	dst->dock_leave = rnd_gtkg_dock_leave;

	dst->beep = rnd_gtkg_beep;
	dst->point_cursor = PointCursor;
	dst->benchmark = rnd_gtkg_benchmark;

	dst->command_entry = rnd_gtk_command_entry;

	dst->create_menu_by_node = rnd_gtk_create_menu_by_node;
	dst->remove_menu_node = rnd_gtk_remove_menu_node;
	dst->update_menu_checkbox = rnd_gtk_update_menu_checkbox;
	dst->get_menu_cfg = rnd_gtkg_get_menu_cfg;

	dst->clip_set  = rnd_gtkg_clip_set;
	dst->clip_get  = rnd_gtkg_clip_get;
	dst->clip_free = rnd_gtkg_clip_free;

	dst->zoom_win = rnd_gtkg_zoom_win;
	dst->zoom = rnd_gtkg_zoom;
	dst->pan = rnd_gtkg_pan;
	dst->pan_mode = rnd_gtkg_pan_mode;
	dst->view_get = rnd_gtkg_view_get;
	dst->open_command = rnd_gtkg_open_command;
	dst->open_popup = rnd_gtkg_open_popup;
	dst->reg_mouse_cursor = rnd_gtkg_reg_mouse_cursor;
	dst->set_mouse_cursor = rnd_gtkg_set_mouse_cursor;
	dst->set_top_title = rnd_gtkg_set_top_title;
	dst->busy = rnd_gtkg_busy;

	dst->set_hidlib = rnd_gtkg_set_hidlib;
	dst->get_dad_hidlib = rnd_gtk_attr_get_dad_hidlib;

	dst->key_state = &rnd_gtk_keymap;

	dst->usage = rnd_gtkg_usage;

	dst->draw_pixmap = rnd_gtkg_draw_pixmap;
	dst->uninit_pixmap = rnd_gtkg_uninit_pixmap;

	dst->hid_data = ghidgui;
}
