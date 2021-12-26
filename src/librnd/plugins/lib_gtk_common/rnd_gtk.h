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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#ifndef RND_GTK_H
#define RND_GTK_H

#include <librnd/config.h>
#include <librnd/core/hid.h>
#include <librnd/core/conf.h>

typedef struct rnd_gtk_port_s  rnd_gtk_port_t;
typedef struct rnd_gtk_s rnd_gtk_t;
typedef struct rnd_gtk_mouse_s rnd_gtk_mouse_t;
typedef struct rnd_gtk_topwin_s rnd_gtk_topwin_t;
typedef struct rnd_gtk_impl_s rnd_gtk_impl_t;
typedef struct rnd_gtk_pixmap_s rnd_gtk_pixmap_t;


extern rnd_gtk_t _ghidgui, *ghidgui;

#include <gtk/gtk.h>

/* The HID using rnd_gtk_common needs to fill in this struct and pass it
   on to most of the calls. This is the only legal way rnd_gtk_common can
   back reference to the HID. This lets multiple HIDs use gtk_common code
   without linker errors. */
struct rnd_gtk_impl_s {
	void *gport;      /* Opaque pointer back to the HID's internal struct - used when common calls a HID function */

	/* rendering */
	void (*drawing_realize)(GtkWidget *w, void *gport);
	gboolean (*drawing_area_expose)(GtkWidget *w, rnd_gtk_expose_t *p, void *gport);
	void (*drawing_area_configure_hook)(void *);

	GtkWidget *(*new_drawing_widget)(rnd_gtk_impl_t *impl);
	void (*init_drawing_widget)(GtkWidget *widget, void *gport);
	gboolean (*preview_expose)(GtkWidget *widget, rnd_gtk_expose_t *p, rnd_hid_expose_t expcall, rnd_hid_expose_ctx_t *ctx); /* p == NULL when called from the code, not from a GUI expose event */
	void (*load_bg_image)(void);
	void (*init_renderer)(int *argc, char ***argv, void *port);
	void (*draw_grid_local)(rnd_hidlib_t *hidlib, rnd_coord_t cx, rnd_coord_t cy);

	/* screen */
	void (*screen_update)(void);
	void (*shutdown_renderer)(void *port);

	rnd_bool (*map_color)(const rnd_color_t *inclr, rnd_gtk_color_t *color);
	const gchar *(*get_color_name)(rnd_gtk_color_t *color);

	void (*set_special_colors)(rnd_conf_native_t *cfg);
	void (*draw_pixmap)(rnd_hidlib_t *hidlib, rnd_gtk_pixmap_t *gpm, rnd_coord_t ox, rnd_coord_t oy, rnd_coord_t dw, rnd_coord_t dh);
	void (*uninit_pixmap)(rnd_hidlib_t *hidlib, rnd_gtk_pixmap_t *gpm);
};

#include <librnd/plugins/lib_gtk_common/ui_zoompan.h>

typedef struct {
	rnd_gtkc_cursor_type_t shape;
	GdkCursor *X_cursor;
	GdkPixbuf *pb;
} rnd_gtk_cursor_t;

#define GVT(x) vtmc_ ## x
#define GVT_ELEM_TYPE rnd_gtk_cursor_t
#define GVT_SIZE_TYPE int
#define GVT_DOUBLING_THRS 256
#define GVT_START_SIZE 8
#define GVT_FUNC
#define GVT_SET_NEW_BYTES_TO 0

#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
#include <genvector/genvector_undef.h>

struct rnd_gtk_mouse_s {
	GdkCursor *X_cursor;          /* used X cursor */
	rnd_gtkc_cursor_type_t X_cursor_shape; /* and its shape */
	vtmc_t cursor;
	int last_cursor_idx; /* tool index of the tool last selected */
};

typedef struct gtkc_event_xyz_s { /* drawing area resize event binding; compat.h will implement this */
	gboolean (*cb)(GtkWidget *widget, long x, long y, long z, void *user_data);
	void *user_data;
} gtkc_event_xyz_t;

typedef struct gtkc_event_xyz_fwd_s { /* same as gtkc_event_xyz_t but capable fo forwarding */
	gboolean (*cb)(GtkWidget *widget, long x, long y, long z, void *fwd, void *user_data);
	void *user_data;
} gtkc_event_xyz_fwd_t;

RND_INLINE gtkc_event_xyz_t *rnd_gtkc_xy_ev(gtkc_event_xyz_t *xyev, gboolean (*cb)(GtkWidget *widget, long x, long y, long z, void *user_data), void *user_data)
{
	xyev->cb = cb;
	xyev->user_data = user_data;
	return xyev;
}

RND_INLINE gtkc_event_xyz_fwd_t *rnd_gtkc_xyz_fwd_ev(gtkc_event_xyz_fwd_t *xyev, gboolean (*cb)(GtkWidget *widget, long x, long y, long z, void *fwd, void *user_data), void *user_data)
{
	xyev->cb = cb;
	xyev->user_data = user_data;
	return xyev;
}

#include RND_GTK_BU_MENU_H_FN
#include "bu_command.h"

struct rnd_gtk_topwin_s {
	/* util/builder states */
	rnd_gtk_menu_ctx_t menu;
	rnd_gtk_command_t cmd;

	/* own widgets */
	GtkWidget *drawing_area;
	GtkWidget *bottom_hbox;

	GtkWidget *top_hbox, *top_bar_background, *menu_hbox, *position_hbox, *menubar_toolbar_vbox;
	GtkWidget *left_toolbar;
	GtkWidget *layer_selector;
	GtkWidget *vbox_middle, *hpaned_middle;

	GtkWidget *h_range, *v_range;

	/* own internal states */
	gboolean adjustment_changed_holdoff;
	gboolean small_label_markup;
	int active; /* 0 before init finishes */
	int placed;
	gtkc_event_xyz_t dwg_rs, dwg_sc, dwg_enter, dwg_leave, dwg_motion;

	/* docking */
	GtkWidget *dockbox[RND_HID_DOCK_max];
	gdl_list_t dock[RND_HID_DOCK_max];
};

/* needed for a type in rnd_gtk_t - DO NOT ADD .h files that are not required for the structs! */
#include <librnd/plugins/lib_gtk_common/dlg_topwin.h>

#include <librnd/core/event.h>
#include <librnd/core/conf_hid.h>
#include <librnd/core/rnd_bool.h>

struct rnd_gtk_pixmap_s {
	rnd_pixmap_t *pxm;        /* core-side pixmap (raw input image) */
	GdkPixbuf *image;         /* input image converted to gdk */

	/* backend/renderer cache */
	int h_scaled, w_scaled;  /* current scale of the cached image (for gdk) */
	union {
		GdkPixbuf *pb;         /* for gdk */
		unsigned long int lng; /* for opengl */
	} cache;
};

/* The output viewport */
struct rnd_gtk_port_s {
	GtkWidget *top_window,        /* toplevel widget */
	          *drawing_area;      /* and its drawing area */

	rnd_bool drawing_allowed;     /* track if a drawing area is available for rendering */

	struct render_priv_s *render_priv;

	rnd_gtk_mouse_t *mouse;

	rnd_gtk_view_t view;
};

struct rnd_gtk_s {
	rnd_gtk_impl_t impl;
	rnd_gtk_port_t port;

	rnd_hidlib_t *hidlib;

	GtkWidget *wtop_window;

	rnd_gtk_topwin_t topwin;
	rnd_conf_hid_id_t conf_id;

	rnd_gtk_pixmap_t bg_pixmap;

	int hid_active; /* 1 if the currently running hid (rnd_gui) is up */
	int gui_is_up; /*1 if all parts of the gui is up and running */

	gulong button_press_handler, button_release_handler, key_press_handler[5], key_release_handler[5];
	gtkc_event_xyz_t mpress_rs, mrelease_rs, kpress_rs, krelease_rs, wtop_rs, wtop_del, wtop_destr, wtop_enter;

	rnd_gtk_mouse_t mouse;

	gdl_list_t previews; /* all widget lists */
};

#endif /* RND_GTK_H */
