/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
 *  pcb-rnd Copyright (C) 2016, 2017, 2019 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "config.h"
#include <librnd/hid/pixmap.h>

#include "glue_common.h"

#include <librnd/core/conf.h>
#include "rnd_gtk.h"
#include <librnd/core/hidlib.h>
#include "dlg_topwin.h"
#include "hid_gtk_conf.h"
#include "in_keyboard.h"
#include "wt_preview.h"
#include <librnd/core/safe_fs.h>
#include <librnd/core/rnd_conf.h>
#include <librnd/plugins/lib_hid_common/lib_hid_common.h>

rnd_gtk_t _ghidgui, *ghidgui = &_ghidgui;

/*** win32 workarounds ***/

#ifdef __WIN32__

#include <librnd/hid/hid_init.h>
#include <librnd/core/compat_fs.h>

static void rnd_gtkg_win32_init(void)
{
	char *cache, *cmd, *s;
	/* set up gdk pixmap modules - without this XPMs won't be loaded */
	cache = rnd_concat(rnd_w32_cachedir, "\\gdk-pixmap-loaders.cache", NULL);
	rnd_setenv("GDK_PIXBUF_MODULE_FILE", cache, 1);


	for(s = cache; *s != '\0'; s++)
		if (*s == '\\')
			*s = '/';
	if (!rnd_file_readable_(cache)) {
		fprintf(stderr, "setenv: GDK_PIXBUF_MODULE_FILE: '%s'\n", cache);
		cmd = rnd_concat(rnd_w32_bindir, "\\gdk-pixbuf-query-loaders --update-cache", NULL);
		fprintf(stderr, "librnd: updating gdk loader cache: '%s'...\n", cache);
		rnd_system(NULL, cmd);
		free(cmd);
	}
	free(cache);
}
#else
static void rnd_gtkg_win32_init(void) {} /* no-op on non-win32 */
#endif


/*** config ***/

static const char *cookie_menu = "gtk hid menu";

static void rnd_gtk_confchg_fullscreen(rnd_conf_native_t *cfg, int arr_idx, void *user_data)
{
	if (ghidgui->hid_active)
		rnd_gtk_fullscreen_apply(&ghidgui->topwin);
}


void rnd_gtk_confchg_checkbox(rnd_conf_native_t *cfg, int arr_idx, void *user_data)
{
	if ((ghidgui->hid_active) && (ghidgui->hidlib != NULL))
		rnd_gtk_update_toggle_flags(ghidgui->hidlib, &ghidgui->topwin, NULL);
}

static void rnd_gtk_confchg_cli(rnd_conf_native_t *cfg, int arr_idx, void *user_data)
{
	rnd_gtk_command_update_prompt(&ghidgui->topwin.cmd);
}

static void rnd_gtk_confchg_flip(rnd_conf_native_t *cfg, int arr_idx, void *user_data)
{
	rnd_gtk_previews_flip(ghidgui);
}

static void rnd_gtk_confchg_spec_color(rnd_conf_native_t *cfg, int arr_idx, void *user_data)
{
	if (!ghidgui->hid_active)
		return;

	if (ghidgui->impl.set_special_colors != NULL)
		ghidgui->impl.set_special_colors(cfg);
}



static void init_conf_watch(rnd_conf_hid_callbacks_t *cbs, const char *path, void (*func)(rnd_conf_native_t *, int, void *))
{
	rnd_conf_native_t *n = rnd_conf_get_field(path);
	if (n != NULL) {
		memset(cbs, 0, sizeof(rnd_conf_hid_callbacks_t));
		cbs->val_change_post = func;
		rnd_conf_hid_set_cb(n, ghidgui->conf_id, cbs);
	}
}

static void rnd_gtk_conf_regs(const char *cookie)
{
	static rnd_conf_hid_callbacks_t cbs_fullscreen, cbs_cli[2], cbs_color[3], cbs_flip[2];

	ghidgui->conf_id = rnd_conf_hid_reg(cookie, NULL);

	init_conf_watch(&cbs_fullscreen, "editor/fullscreen", rnd_gtk_confchg_fullscreen);

	init_conf_watch(&cbs_cli[0], "rc/cli_prompt", rnd_gtk_confchg_cli);
	init_conf_watch(&cbs_cli[1], "rc/cli_backend", rnd_gtk_confchg_cli);

	init_conf_watch(&cbs_color[0], "appearance/color/background", rnd_gtk_confchg_spec_color);
	init_conf_watch(&cbs_color[1], "appearance/color/off_limit", rnd_gtk_confchg_spec_color);
	init_conf_watch(&cbs_color[2], "appearance/color/grid", rnd_gtk_confchg_spec_color);

	init_conf_watch(&cbs_flip[0], "editor/view/flip_x", rnd_gtk_confchg_flip);
	init_conf_watch(&cbs_flip[1], "editor/view/flip_y", rnd_gtk_confchg_flip);

	ghidgui->topwin.menu.rnd_gtk_menuconf_id = rnd_conf_hid_reg(cookie_menu, NULL);
	ghidgui->topwin.menu.confchg_checkbox = rnd_gtk_confchg_checkbox;
}


/*** drawing widget - output ***/

/* Do scrollbar scaling based on current port drawing area size and
   overall design size. */
void rnd_gtk_tw_ranges_scale(rnd_gtk_t *gctx)
{
	rnd_gtk_topwin_t *tw = &gctx->topwin;
	rnd_gtk_view_t *view = &gctx->port.view;

	/* Update the scrollbars with PCB units. So Scale the current drawing area
	   size in pixels to PCB units and that will be the page size for the Gtk adjustment. */
	rnd_gtk_zoom_post(view);

	if (!rnd_conf.editor.unlimited_pan) {
		if (rnd_conf.editor.view.flip_x)
			gtkc_scb_zoom_adjustment(tw->h_range, view->width, 0, gctx->hidlib->dwg.X2-gctx->hidlib->dwg.X1);
		else
			gtkc_scb_zoom_adjustment(tw->h_range, view->width, gctx->hidlib->dwg.X1, gctx->hidlib->dwg.X2+gctx->hidlib->dwg.X1);

		if (rnd_conf.editor.view.flip_y)
			gtkc_scb_zoom_adjustment(tw->v_range, view->height, 0, gctx->hidlib->dwg.Y2-gctx->hidlib->dwg.Y1);
		else
			gtkc_scb_zoom_adjustment(tw->v_range, view->height, gctx->hidlib->dwg.Y1, gctx->hidlib->dwg.Y2+gctx->hidlib->dwg.Y1);
	}
}

void rnd_gtk_port_ranges_changed(void)
{
	if (!rnd_conf.editor.unlimited_pan) {
		ghidgui->port.view.x0 = gtkc_scb_getval(ghidgui->topwin.h_range);
		ghidgui->port.view.y0 = gtkc_scb_getval(ghidgui->topwin.v_range);
	}

	rnd_gui->invalidate_all(rnd_gui);
}

void rnd_gtk_pan_common(void)
{
	if (!rnd_conf.editor.unlimited_pan) {
		ghidgui->topwin.adjustment_changed_holdoff = TRUE;
		gtkc_scb_setval(ghidgui->topwin.h_range, ghidgui->port.view.x0);
		gtkc_scb_setval(ghidgui->topwin.v_range, ghidgui->port.view.y0);
		ghidgui->topwin.adjustment_changed_holdoff = FALSE;
	}

	rnd_gtk_port_ranges_changed();
}

static void command_post_entry(void)
{
#if RND_GTK_DISABLE_MOUSE_DURING_CMD_ENTRY
	rnd_gtk_interface_input_signals_connect();
#endif
	rnd_gtk_interface_set_sensitive(TRUE);
	gtk_widget_grab_focus(ghidgui->port.drawing_area);
}

static void command_pre_entry(void)
{
#if RND_GTK_DISABLE_MOUSE_DURING_CMD_ENTRY
	rnd_gtk_interface_input_signals_disconnect();
#endif
	rnd_gtk_interface_set_sensitive(FALSE);
}

/*** input ***/

void rnd_gtk_interface_set_sensitive(gboolean sensitive)
{
	rnd_gtk_tw_interface_set_sensitive(&ghidgui->topwin, sensitive);
}

void rnd_gtk_mode_cursor_main(void)
{
	rnd_gtk_mode_cursor(ghidgui);
}

/* low level keyboard and mouse signal connect/disconnect from top window drawing area */
static void kbd_input_signals_connect(int idx, void *obj)
{
	ghidgui->key_press_handler[idx] = gtkc_bind_key_press(obj, rnd_gtkc_xy_ev(&ghidgui->kpress_rs, rnd_gtk_key_press_cb, ghidgui));
	ghidgui->key_release_handler[idx] = gtkc_bind_key_release(obj, rnd_gtkc_xy_ev(&ghidgui->krelease_rs, rnd_gtk_key_release_cb, &ghidgui->topwin));
}

static void kbd_input_signals_disconnect(int idx, void *obj)
{
	if (ghidgui->key_press_handler[idx] != 0) {
		gtkc_unbind_key(G_OBJECT(obj), ghidgui->key_press_handler[idx]);
		ghidgui->key_press_handler[idx] = 0;
	}
	if (ghidgui->key_release_handler[idx] != 0) {
		gtkc_unbind_key(G_OBJECT(obj), ghidgui->key_release_handler[idx]);
		ghidgui->key_release_handler[idx] = 0;
	}
}

static void mouse_input_singals_connect(void *obj)
{
	ghidgui->button_press_handler = gtkc_bind_mouse_press(obj, rnd_gtkc_xy_ev(&ghidgui->mpress_rs, rnd_gtk_button_press_cb, ghidgui));
	ghidgui->button_release_handler = gtkc_bind_mouse_release(obj, rnd_gtkc_xy_ev(&ghidgui->mrelease_rs, rnd_gtk_button_release_cb, ghidgui));
}

static void mouse_input_singals_disconnect(void *obj)
{
	if (ghidgui->button_press_handler != 0)
		gtkc_unbind_mouse_btn(ghidgui->port.drawing_area, ghidgui->button_press_handler);

	if (ghidgui->button_release_handler != 0)
		gtkc_unbind_mouse_btn(ghidgui->port.drawing_area, ghidgui->button_release_handler);

	ghidgui->button_press_handler = ghidgui->button_release_handler = 0;
}

/* Connect and disconnect just the signals a g_main_loop() will need.
   Cursor and motion events still need to be handled by the top level
   loop, so don't connect/reconnect these.
   A g_main_loop will be running when pcb-rnd wants the user to select a
   location or if command entry is needed in the status line.
   During these times normal button/key presses are intercepted, either
   by new signal handlers or the command_combo_box entry. */
void rnd_gtk_interface_input_signals_connect(void)
{
	mouse_input_singals_connect(ghidgui->port.drawing_area);
	kbd_input_signals_connect(0, ghidgui->port.drawing_area);
	kbd_input_signals_connect(3, ghidgui->topwin.left_toolbar);
}

void rnd_gtk_interface_input_signals_disconnect(void)
{
	kbd_input_signals_disconnect(0, ghidgui->port.drawing_area);
	kbd_input_signals_disconnect(3, ghidgui->topwin.left_toolbar);
	mouse_input_singals_disconnect(ghidgui->port.drawing_area);
}

/*** misc ***/

/* import a core pixmap into a gdk pixmap */
void rnd_gtkg_init_pixmap_low(rnd_gtk_pixmap_t *gpm)
{
	int rowstd, nch, x, y;
	unsigned char *dst, *dst_row, *src = gpm->pxm->p;

	gpm->image = gdk_pixbuf_new(GDK_COLORSPACE_RGB, gpm->pxm->has_transp, 8, gpm->pxm->sx, gpm->pxm->sy);
	dst_row = gdk_pixbuf_get_pixels(gpm->image);
	rowstd = gdk_pixbuf_get_rowstride(gpm->image);
	nch = gdk_pixbuf_get_n_channels(gpm->image);
	assert((nch == 3) || (nch == 4));
	for(y = 0; y < gpm->pxm->sy; y++,dst_row += rowstd) {
		dst = dst_row;
		for(x = 0; x < gpm->pxm->sx; x++) {
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			if (gpm->pxm->has_transp) {
				if ((src[0] == gpm->pxm->tr) && (src[1] == gpm->pxm->tg) && (src[2] == gpm->pxm->tb))
					dst[3] = 0;
				else
					dst[3] = 255;
			}
			dst += nch;
			src += 3;
		}
	}
}

void rnd_gtkg_uninit_pixmap_low(rnd_gtk_pixmap_t *gpm)
{
	g_object_unref(gpm->image);
}

static void rnd_gtkg_load_bg_image(void)
{
	static rnd_pixmap_t pxm;

	ghidgui->bg_pixmap.pxm = NULL;
	ghidgui->bg_pixmap.image = NULL;
	if (rnd_gtk_conf_hid.plugins.hid_gtk.bg_image != NULL) {
		if (rnd_old_pixmap_load(ghidgui->hidlib, &pxm, rnd_gtk_conf_hid.plugins.hid_gtk.bg_image) != 0) {
			rnd_message(RND_MSG_ERROR, "Failed to load pixmap %s for background image\n", rnd_gtk_conf_hid.plugins.hid_gtk.bg_image);
			return;
		}
		ghidgui->bg_pixmap.pxm = &pxm;
		rnd_gtkg_init_pixmap_low(&ghidgui->bg_pixmap);
	}
}

void rnd_gtk_previews_invalidate_lr(rnd_coord_t left, rnd_coord_t right, rnd_coord_t top, rnd_coord_t bottom)
{
	rnd_box_t screen;
	screen.X1 = left; screen.X2 = right;
	screen.Y1 = top; screen.Y2 = bottom;
	rnd_gtk_preview_invalidate(ghidgui, &screen);
}

void rnd_gtk_previews_invalidate_all(void)
{
	rnd_gtk_preview_invalidate(ghidgui, NULL);
}


void rnd_gtk_note_event_location(gint event_x, gint event_y, int valid)
{
	if (!valid)
		gdkc_window_get_pointer(ghidgui->port.drawing_area, &event_x, &event_y, NULL);

	rnd_gtk_coords_event2design(&ghidgui->port.view, event_x, event_y, &ghidgui->port.view.design_x, &ghidgui->port.view.design_y);

	rnd_hidcore_crosshair_move_to(ghidgui->port.view.ctx->hidlib, ghidgui->port.view.design_x, ghidgui->port.view.design_y, 1);
}

/*** init ***/
void rnd_gtkg_glue_common_uninit(const char *cookie)
{
	rnd_event_unbind_allcookie(cookie);
	rnd_conf_hid_unreg(cookie);
	rnd_conf_hid_unreg(cookie_menu);
}

void rnd_gtkg_glue_common_init(const char *cookie)
{
	rnd_gtkg_win32_init();

	/* Set up the glue struct to lib_gtk_common */
	ghidgui->impl.gport = &ghidgui->port;

	ghidgui->impl.load_bg_image = rnd_gtkg_load_bg_image;

	ghidgui->topwin.cmd.post_entry = command_post_entry;
	ghidgui->topwin.cmd.pre_entry = command_pre_entry;

	ghidgui->port.view.ctx = ghidgui;
	ghidgui->port.mouse = &ghidgui->mouse;

	rnd_gtk_conf_regs(cookie);
}

