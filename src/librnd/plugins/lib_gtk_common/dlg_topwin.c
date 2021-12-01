/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* This file was originally written by Bill Wilson for the PCB Gtk
   port.  It was later heavily modified by Dan McMahill to provide
   user customized menus then by the pcb-rnd development team to
   modularize gtk support.

   This handles creation of the top level window and all its widgets.

   Hotkeys and menu keys are handled manually because gtk menu system had
   problem with multi-stroke keys.
*/

#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif

#include <librnd/rnd_config.h>
#include "dlg_topwin.h"
#include <genht/htsp.h>
#include <genht/hash.h>
#include <librnd/core/hidlib.h>
#include <librnd/core/hidlib_conf.h>

#include <librnd/core/rnd_printf.h>
#include <librnd/core/actions.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/tool.h>

#include "bu_pixbuf.h"
#include "dlg_attribute.h"
#include "util_listener.h"
#include "in_mouse.h"
#include "in_keyboard.h"
#include "lib_gtk_config.h"
#include "hid_gtk_conf.h"
#include "glue_common.h"

/*** docking code (dynamic parts) ***/
static int rnd_gtk_dock_poke(rnd_hid_dad_subdialog_t *sub, const char *cmd, rnd_event_arg_t *res, int argc, rnd_event_arg_t *argv)
{
	return -1;
}

typedef struct {
	void *hid_ctx;
	GtkWidget *hvbox;
	rnd_gtk_topwin_t *tw;
	rnd_hid_dock_t where;
} docked_t;

rnd_gtk_color_t clr_orange = GTKC_LITERAL_COLOR(0,  0xffff, 0xaaaa, 0x3333);

static rnd_gtk_color_t *rnd_dock_color[RND_HID_DOCK_max] = {NULL, NULL, &clr_orange, NULL, NULL, NULL}; /* force change color when docked */
static htsp_t pck_dock_pos[RND_HID_DOCK_max];

static void rnd_gtk_tw_dock_init(void)
{
	int n;
	for(n = 0; n < RND_HID_DOCK_max; n++)
		htsp_init(&pck_dock_pos[n], strhash, strkeyeq);
}

void rnd_gtk_tw_dock_uninit(void)
{
	int n;
	for(n = 0; n < RND_HID_DOCK_max; n++) {
		htsp_entry_t *e;
		for(e = htsp_first(&pck_dock_pos[n]); e != NULL; e = htsp_next(&pck_dock_pos[n], e))
			free(e->key);
		htsp_uninit(&pck_dock_pos[n]);
	}
}


int rnd_gtk_tw_dock_enter(rnd_gtk_topwin_t *tw, rnd_hid_dad_subdialog_t *sub, rnd_hid_dock_t where, const char *id)
{
	docked_t *docked;
	GtkWidget *frame;
	int expfill = 0;


	docked = calloc(sizeof(docked_t), 1);
	docked->where = where;

	/* named "frames" are packed in the docking site; when a frame has
	   content, it's docked->hvbox; on leave, the hvbox is removed but
	   the frame is kept so a re-enter will happen into the same frame */

	if (rnd_dock_is_vert[where])
		docked->hvbox = gtkc_vbox_new(FALSE, 0);
	else
		docked->hvbox = gtkc_hbox_new(TRUE, 0);

	frame = htsp_get(&pck_dock_pos[where], id);
	if (frame == NULL) {
		if (rnd_dock_has_frame[where])
			frame = gtk_frame_new(id);
		else
			frame = gtkc_vbox_new(FALSE, 0);

		if (RND_HATT_IS_COMPOSITE(sub->dlg[0].type))
			expfill = (sub->dlg[0].rnd_hatt_flags & RND_HATF_EXPFILL);

		gtkc_box_pack_append(tw->dockbox[where], frame, expfill, 0);
		htsp_set(&pck_dock_pos[where], rnd_strdup(id), frame);
	}

	if (rnd_dock_has_frame[where])
		gtkc_frame_set_child(frame, docked->hvbox);
	else
		gtkc_box_pack_append(frame, docked->hvbox, 0, 0);

	if ((sub->dlg_minx > 0) && (sub->dlg_miny > 0))
		gtk_widget_set_size_request(frame, sub->dlg_minx, sub->dlg_miny);

	gtkc_widget_show_all(frame); /* can not show after creating the sub: some widgets may start out as hidden! */

	sub->parent_poke = rnd_gtk_dock_poke;
	sub->dlg_hid_ctx = docked->hid_ctx = rnd_gtk_attr_sub_new(ghidgui, docked->hvbox, sub->dlg, sub->dlg_len, sub);
	docked->tw = tw;
	sub->parent_ctx = docked;

	gdl_append(&tw->dock[where], sub, link);

	if (rnd_dock_color[where] != NULL)
		rnd_gtk_dad_fixcolor(sub->dlg_hid_ctx, rnd_dock_color[where]);

	/* workaround: the left dock is in a pane that should be adjusted to the default x size of newcomers if narrower */
	if ((where == RND_HID_DOCK_LEFT) && (sub->dlg_defx > 0)) {
		int curr = gtk_paned_get_position(GTK_PANED(tw->hpaned_middle));
		if (curr < sub->dlg_defx)
			gtk_paned_set_position(GTK_PANED(tw->hpaned_middle), sub->dlg_defx);
	}

	return 0;
}

void rnd_gtk_tw_dock_leave(rnd_gtk_topwin_t *tw, rnd_hid_dad_subdialog_t *sub)
{
	docked_t *docked = sub->parent_ctx;
	GtkWidget *frame = gtk_widget_get_parent(docked->hvbox);
	gtkc_widget_destroy(docked->hvbox);
	gdl_remove(&tw->dock[docked->where], sub, link);
	free(docked);
	RND_DAD_FREE(sub->dlg);
	gtk_widget_hide(frame);
}

/*** static top window code ***/
/* sync the menu checkboxes with actual pcb state */
void rnd_gtk_update_toggle_flags(rnd_hidlib_t *hidlib, rnd_gtk_topwin_t *tw, const char *cookie)
{
	if (rnd_menu_sys.inhibit)
		return;

	rnd_gtk_main_menu_update_toggle_state(hidlib, tw->menu.menu_bar);
}

static void h_adjustment_changed_cb(GtkAdjustment *adj, rnd_gtk_topwin_t *tw)
{
	if (tw->adjustment_changed_holdoff)
		return;

	rnd_gtk_port_ranges_changed();
}

static void v_adjustment_changed_cb(GtkAdjustment *adj, rnd_gtk_topwin_t *tw)
{
	if (tw->adjustment_changed_holdoff)
		return;

	rnd_gtk_port_ranges_changed();
}

/* Save size of top window changes so PCB can restart at its size at exit. */
static gint top_window_configure_event_cb(GtkWidget *widget, long x, long y, long z, void *tw_)
{
	return rnd_gtk_winplace_cfg(ghidgui->hidlib, widget, NULL, "top");
}

gboolean rnd_gtk_idle_cb(void *topwin)
{
/*	rnd_gtk_topwin_t *tw = topwin; - just in case anything needs to be done from idle */
	return FALSE;
}

gboolean rnd_gtk_key_release_cb(GtkWidget *drawing_area, long mods, long key_raw, long kv, void *udata)
{
	rnd_gtk_topwin_t *tw = udata;
	gint ksym = kv;

	if (rnd_gtk_is_modifier_key_sym(ksym))
		rnd_gtk_note_event_location(0, 0, 0);

	if (rnd_app.adjust_attached_objects != NULL)
		rnd_app.adjust_attached_objects(ghidgui->hidlib);
	else
		rnd_tool_adjust_attached(ghidgui->hidlib);

	rnd_gui->invalidate_all(rnd_gui);
	g_idle_add(rnd_gtk_idle_cb, tw);
	return FALSE;
}

/*** Top window ***/
static gint delete_chart_cb(GtkWidget *widget, long x, long y, long z, void *data)
{
	rnd_gtk_t *gctx = data;

	rnd_action(gctx->hidlib, "Quit");

	/* Return TRUE to keep our app running.  A FALSE here would let the
	   delete signal continue on and kill our program. */
	return TRUE;
}

static gint destroy_chart_cb(GtkWidget *widget, long x, long y, long z, void *data)
{
	rnd_gtk_t *ctx = data;
	ctx->impl.shutdown_renderer(ctx->impl.gport);
	gtkc_main_quit();
	return FALSE;
}

static void fullscreen_cb(GtkButton *btn, void *data)
{
	rnd_conf_setf(RND_CFR_DESIGN, "editor/fullscreen", -1, "%d", !rnd_conf.editor.fullscreen, RND_POL_OVERWRITE);
}

/* XPM */
static const char * FullScreen_xpm[] = {
"8 8 2 1",
" 	c None",
".	c #729FCF",
"        ",
" ...... ",
" .      ",
" . ...  ",
" . .    ",
" . .    ",
" .      ",
"        "};

/* Embed an XPM image in a button, and make it display as small as possible 
 *   Returns: a new image button. When freeing the object, the image needs to be freed 
 *            as well, using :
 *            g_object_unref(gtk_button_get_image(GTK_BUTTON(button)); g_object_unref(button);
 */
static GtkWidget *create_image_button_from_xpm_data(const char **xpm_data)
{
	GtkWidget *button;
	GdkPixbuf *pixbuf;
	GtkWidget *image;

	button = gtk_button_new();
	pixbuf = rnd_gtk_xpm2pixbuf(xpm_data, 1);
	image = gtk_image_new_from_pixbuf(pixbuf);
	gtkc_workaround_image_scale_bug(image, pixbuf);
	g_object_unref(pixbuf);

	gtkc_button_set_image(GTK_BUTTON(button), image);

	gtkc_workaround_image_button_border_bug(button, pixbuf);

	return button;
}

static gboolean drawing_area_enter_cb(GtkWidget *w, long x, long y, long z, void *user_data)
{
	rnd_gui->invalidate_all(rnd_gui);
	return FALSE;
}

void rnd_gtk_topwin_hide_status(void *ctx, int show)
{
	rnd_gtk_topwin_t *tw = ctx;

	if (show)
		gtk_widget_show(tw->dockbox[RND_HID_DOCK_BOTTOM]);
	else
		gtk_widget_hide(tw->dockbox[RND_HID_DOCK_BOTTOM]);
}

/* Create the top_window contents.  The config settings should be loaded
   before this is called. */
static void rnd_gtk_build_top_window(rnd_gtk_t *ctx, rnd_gtk_topwin_t *tw)
{
	GtkWidget *vbox_main, *hbox, *hboxi, *evb, *spring;
	GtkWidget *hbox_scroll, *fullscreen_btn;
	rnd_gtk_tw_dock_init();

	vbox_main = gtkc_vbox_new(FALSE, 0);
	gtkc_window_set_child(ghidgui->wtop_window, vbox_main);

	/* -- Top control bar */
	tw->top_bar_background = gtkc_hbox_new(TRUE, 0);
	gtkc_box_pack_append(vbox_main, tw->top_bar_background, FALSE, 0);

	tw->top_hbox = gtkc_hbox_new(FALSE, 0);
	gtkc_box_pack_append(tw->top_bar_background, tw->top_hbox, TRUE, 0);

	/* menu_hbox will be made insensitive when the gui needs
	   a modal button GetLocation button press. */
	tw->menu_hbox = gtkc_hbox_new(FALSE, 0);
	gtkc_box_pack_append(tw->top_hbox, tw->menu_hbox, FALSE, 0);

	tw->menubar_toolbar_vbox = gtkc_vbox_new(FALSE, 0);
	gtkc_box_pack_append(tw->menu_hbox, tw->menubar_toolbar_vbox, FALSE, 0);

	/* Build main menu */
	tw->menu.menu_bar = rnd_gtk_load_menus(&tw->menu, ghidgui->hidlib);
	gtkc_box_pack_append(tw->menubar_toolbar_vbox, tw->menu.menu_bar, FALSE, 0);

	tw->dockbox[RND_HID_DOCK_TOP_LEFT] = gtkc_hbox_new(TRUE, 2);
	gtkc_box_pack_append(tw->menubar_toolbar_vbox, tw->dockbox[RND_HID_DOCK_TOP_LEFT], FALSE, 0);

	/* pushes the top right position box to the right */
	spring = gtkc_hbox_new(TRUE, 0);
	gtkc_box_pack_append(tw->top_hbox, spring, TRUE, 0);

	tw->position_hbox = gtkc_hbox_new(FALSE, 0);
	gtkc_box_pack_append(tw->top_hbox, tw->position_hbox, FALSE, 0);

	tw->dockbox[RND_HID_DOCK_TOP_RIGHT] = gtkc_vbox_new(FALSE, 8);
	gtkc_box_pack_append(tw->position_hbox, tw->dockbox[RND_HID_DOCK_TOP_RIGHT], FALSE, 0);

	tw->hpaned_middle = gtkc_hpaned_new();
	gtkc_box_pack_append(vbox_main, tw->hpaned_middle, TRUE, 0);

	fix_topbar_theming(tw); /* Must be called after toolbar is created */

	/* -- Left control bar */
	/* This box will be made insensitive when the gui needs
	   a modal button GetLocation button press. */
	tw->left_toolbar = gtkc_vbox_new(FALSE, 0);
	gtkc_paned_pack1(GTK_PANED(tw->hpaned_middle), tw->left_toolbar, FALSE);

	tw->dockbox[RND_HID_DOCK_LEFT] = gtkc_vbox_new(FALSE, 8);
	gtkc_box_pack_append(tw->left_toolbar, tw->dockbox[RND_HID_DOCK_LEFT], TRUE, 0);

	/* -- main content */
	tw->vbox_middle = gtkc_vbox_new(FALSE, 0);
	gtkc_paned_pack2(GTK_PANED(tw->hpaned_middle), tw->vbox_middle, TRUE);


	/* -- The PCB layout output drawing area */

	/* info bar: hboxi->bgcolor_box->hbox2:
	   hboxi is for the layout (horizontal fill)
	   the bgcolor_box is needed only for background color
	   vbox is tw->dockbox[RND_HID_DOCK_TOP_INFOBAR] where DAD widgets are packed */
	hboxi = gtkc_hbox_new(TRUE, 0);
	tw->dockbox[RND_HID_DOCK_TOP_INFOBAR] = gtkc_vbox_new(TRUE, 0);
	evb = gtkc_bgcolor_box_new();
	gtkc_bgcolor_box_set_child(evb, tw->dockbox[RND_HID_DOCK_TOP_INFOBAR]);
	gtkc_box_pack_append(hboxi, evb, TRUE, 0);
	gtkc_box_pack_append(tw->vbox_middle, hboxi, FALSE, 0);

	if (rnd_dock_color[RND_HID_DOCK_TOP_INFOBAR] != NULL)
		gtkc_widget_modify_bg(evb, GTK_STATE_NORMAL, rnd_dock_color[RND_HID_DOCK_TOP_INFOBAR]);

	hbox = gtkc_hbox_new(FALSE, 0);
	gtkc_box_pack_append(tw->vbox_middle, hbox, TRUE, 0);

	/* drawing area */
	tw->drawing_area = ghidgui->impl.new_drawing_widget(&ghidgui->impl);
	g_signal_connect(G_OBJECT(tw->drawing_area), "realize", G_CALLBACK(ghidgui->impl.drawing_realize), ghidgui->impl.gport); /* gtk2-gtk4 compatible */
	ghidgui->impl.init_drawing_widget(tw->drawing_area, ghidgui->impl.gport);

	gtkc_setup_events(tw->drawing_area, 1, 1, 1, 1, 1, 1);

	/* This is required to get the drawing_area key-press-event.  Also the
	 * enter and button press callbacks grab focus to be sure we have it
	 * when in the drawing_area. */
	gtkc_widget_set_focusable(tw->drawing_area);

	gtkc_box_pack_append(hbox, tw->drawing_area, TRUE, 0);


	tw->v_range = gtkc_vscrollbar_new(G_CALLBACK(v_adjustment_changed_cb), tw);
	gtkc_box_pack_append(hbox, tw->v_range, FALSE, 0);

	hbox_scroll = gtkc_hbox_new(FALSE, 0);
	tw->h_range = gtkc_hscrollbar_new(G_CALLBACK(h_adjustment_changed_cb), tw);
	fullscreen_btn = create_image_button_from_xpm_data(FullScreen_xpm);
	g_signal_connect(G_OBJECT(fullscreen_btn), "clicked", G_CALLBACK(fullscreen_cb), NULL); /* gtk2-gtk4 compatible */
	gtkc_box_pack_append(hbox_scroll, tw->h_range, TRUE, 0);
	gtkc_box_pack_append(hbox_scroll, fullscreen_btn, FALSE, 0);
	gtkc_box_pack_append(tw->vbox_middle, hbox_scroll, FALSE, 0);

	gtkc_unify_hvscroll(tw->h_range, tw->v_range);

	/* -- The bottom status line label */
	tw->bottom_hbox = gtkc_hbox_new(FALSE, 0);
	gtkc_box_pack_append(tw->vbox_middle, tw->bottom_hbox, FALSE, 0);

	tw->dockbox[RND_HID_DOCK_BOTTOM] = gtkc_hbox_new(TRUE, 2);
	gtkc_box_pack_append(tw->bottom_hbox, tw->dockbox[RND_HID_DOCK_BOTTOM], FALSE, 0);

	tw->cmd.prompt_label = gtk_label_new("action:");
	gtkc_box_pack_append(tw->bottom_hbox, tw->cmd.prompt_label, FALSE, 0);
	rnd_gtk_command_combo_box_entry_create(&tw->cmd, rnd_gtk_topwin_hide_status, tw);
	gtkc_box_pack_append(tw->bottom_hbox, tw->cmd.command_combo_box, FALSE, 0);


	/* optional resize grip: rightmost widget in the status line hbox */
	gtkc_create_resize_grip(tw->bottom_hbox);

	gtkc_bind_mouse_enter(tw->drawing_area, rnd_gtkc_xy_ev(&ghidgui->wtop_enter, drawing_area_enter_cb, tw));
	gtk2c_bind_win_resize(ghidgui->wtop_window, rnd_gtkc_xy_ev(&ghidgui->wtop_rs, top_window_configure_event_cb, tw));
	gtkc_bind_win_delete(ghidgui->wtop_window, rnd_gtkc_xy_ev(&ghidgui->wtop_del, delete_chart_cb, ctx));
	gtkc_bind_win_destroy(ghidgui->wtop_window, rnd_gtkc_xy_ev(&ghidgui->wtop_destr, destroy_chart_cb, ctx));

	gtkc_widget_show_all(ghidgui->wtop_window);
	gtk4c_bind_win_resize(ghidgui->wtop_window, rnd_gtkc_xy_ev(&ghidgui->wtop_rs, top_window_configure_event_cb, tw));

	rnd_gtk_fullscreen_apply(tw);
	tw->active = 1;

	gtk_widget_hide(tw->cmd.command_combo_box);
	gtk_widget_hide(tw->cmd.prompt_label);

}

/* We'll set the interface insensitive when a g_main_loop is running so the
   Gtk menus and buttons don't respond and interfere with the special entry
   the user needs to be doing. */
void rnd_gtk_tw_interface_set_sensitive(rnd_gtk_topwin_t *tw, gboolean sensitive)
{
	gtk_widget_set_sensitive(tw->left_toolbar, sensitive);
	gtk_widget_set_sensitive(tw->menu_hbox, sensitive);
}

void rnd_gtk_create_topwin_widgets(rnd_gtk_t *ctx, rnd_gtk_topwin_t *tw, GtkWidget *in_top_window)
{
	ghidgui->impl.load_bg_image();

	rnd_gtk_build_top_window(ctx, tw);
	rnd_gtk_update_toggle_flags(ghidgui->hidlib, tw, NULL);
}

void rnd_gtk_fullscreen_apply(rnd_gtk_topwin_t *tw)
{
	if (rnd_conf.editor.fullscreen) {
		gtk_widget_hide(tw->left_toolbar);
		gtk_widget_hide(tw->top_hbox);
		if (!tw->cmd.command_entry_status_line_active)
			gtk_widget_hide(tw->bottom_hbox);
	}
	else {
		gtk_widget_show(tw->left_toolbar);
		gtk_widget_show(tw->top_hbox);
		gtk_widget_show(tw->bottom_hbox);
	}
}

void rnd_gtk_tw_set_title(rnd_gtk_topwin_t *tw, const char *title)
{
	gtk_window_set_title(GTK_WINDOW(ghidgui->wtop_window), title);
}
