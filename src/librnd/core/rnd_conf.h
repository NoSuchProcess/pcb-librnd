/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#ifndef RND_HIDLIB_CONF_H
#define RND_HIDLIB_CONF_H

#include <librnd/core/conf.h>
#include <librnd/core/color.h>

/* to @conf_gen.sh: begin librnd */

typedef struct {

	struct rnd_conf_temp_s {
		RND_CFT_BOOLEAN click_cmd_entry_active;/* true if the command line is active when the user click - this gives the command interpreter a chance to capture the click and use the coords */
	} temp;

	const struct {                       /* rc */
		RND_CFT_INTEGER verbose;
		RND_CFT_INTEGER quiet;                 /* print only errors on stderr */
		RND_CFT_BOOLEAN dup_log_to_stderr;     /* copy log messages to stderr even if there is a HID that can show them */
		RND_CFT_STRING cli_prompt;             /* plain text prompt to prefix the command entry */
		RND_CFT_STRING cli_backend;            /* command parser action */
		RND_CFT_BOOLEAN export_basename;       /* if an exported file contains the source file name, remove path from it, keeping the basename only */
		RND_CFT_STRING menu_file;              /* where to load the default menu file from. If empty/unset, fall back to 'default'. If contains slash, take it as a full path, if no slash, it is taken as SUFFIX for a normal menu search for menu-SUFFIX.lht */
		RND_CFT_LIST menu_patches;             /* file paths to extra menu patches to load */
		RND_CFT_LIST preferred_gui;            /* if set, try GUI HIDs in this order when no GUI is explicitly selected */
		RND_CFT_BOOLEAN hid_fallback;          /* if there is no explicitly specified HID (--gui) and the preferred GUI fails, automatically fall back on other HIDs, eventually running in batch mode */
		const struct {
			RND_CFT_STRING home;                 /* user's home dir, determined run-time */
			RND_CFT_STRING exec_prefix;          /* exec prefix path (extracted from argv[0]) */
		} path;
		RND_CFT_LIST anyload_persist;          /* if not empty, load all anyload directories/files from this path on startup */
	} rc;

	const struct {
		RND_CFT_REAL layer_alpha;              /* alpha value for layer drawing */
		RND_CFT_REAL drill_alpha;              /* alpha value for drill drawing */

		const struct {
			RND_CFT_STRING   debug_tag;          /* log style tag of debug messages */
			RND_CFT_BOOLEAN  debug_popup;        /* whether a debug line should pop up the log window */
			RND_CFT_STRING   info_tag;           /* log style tag of info messages */
			RND_CFT_BOOLEAN  info_popup;         /* whether an info line should pop up the log window */
			RND_CFT_STRING   warning_tag;        /* log style tag of warnings */
			RND_CFT_BOOLEAN  warning_popup;      /* whether a warning should pop up the log window */
			RND_CFT_STRING   error_tag;          /* log style tag of errors */
			RND_CFT_BOOLEAN  error_popup;        /* whether an error should pop up the log window */
		} loglevels;
		const struct {
			RND_CFT_COLOR background;            /* background and cursor color ... */
			RND_CFT_COLOR off_limit;             /* on-screen background beyond the configured drawing area */
			RND_CFT_COLOR grid;                  /* on-screen grid */
			RND_CFT_COLOR cross;                 /* on-screen crosshair color (inverted) */
		} color;
	} appearance;

	const struct {
		RND_CFT_INTEGER mode;                  /* currently active tool */
		RND_CFT_UNIT grid_unit;                /* select whether you draw in mm or mil */
		RND_CFT_COORD grid;                    /* grid spacing in librnd coord units */
		RND_CFT_LIST grids;                    /* grid in grid-string format */
		RND_CFT_INTEGER grids_idx;             /* the index of the currently active grid from grids */
		RND_CFT_BOOLEAN draw_grid;             /* draw grid points */
		RND_CFT_BOOLEAN cross_grid;            /* when drawing the grid, draw little 3x3 pixel crosses instead of single pixel dots */
		RND_CFT_BOOLEAN auto_place;            /* force placement of GUI windows (dialogs), trying to override the window manager */
		RND_CFT_BOOLEAN fullscreen;            /* hide widgets to make more room for the drawing */
		RND_CFT_INTEGER crosshair_shape_idx;   /* OBSOLETE: do not use */
		RND_CFT_BOOLEAN enable_stroke;         /* Enable libstroke gestures on middle mouse button when non-zero */
		RND_CFT_LIST translate_key;

		const struct {
			RND_CFT_BOOLEAN flip_x;              /* view: flip the design along the X (horizontal) axis */
			RND_CFT_BOOLEAN flip_y;              /* view: flip the design along the Y (vertical) axis */
		} view;

		const struct {
			RND_CFT_BOOLEAN enable;              /* enable local grid to draw grid points only in a small radius around the crosshair - speeds up software rendering on large screens */
			RND_CFT_INTEGER radius;              /* radius, in number of grid points, around the local grid */
		} local_grid;

		const struct {
			RND_CFT_INTEGER min_dist_px;         /* never try to draw a grid so dense that the distance between grid points is smaller than this */
			RND_CFT_BOOLEAN sparse;              /* enable drawing sparse grid: when zoomed out beyond min_dist_px draw every 2nd, 4th, 8th, etc. grid point; if disabled the grid is turned off when it'd get too dense */
		} global_grid;

		RND_CFT_COORD min_zoom;                /* minimum GUI zoom in coord-per-pixel - controls how deep the user can zoom in; needs to be set before startup */
		RND_CFT_BOOLEAN unlimited_pan;         /* if set, do not limit pan to around the drawing area */
		RND_CFT_BOOLEAN hide_hid_crosshair;    /* if set, the standard HID-crosshair is not drawn; does not affect application drawn crosshair and attached objects */
	} editor;
} rnd_conf_t;

/* to @conf_gen.sh: end librnd */


extern rnd_conf_t rnd_conf;

int rnd_hidlib_conf_init();


#endif
