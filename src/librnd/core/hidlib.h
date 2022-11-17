/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019,2021 Tibor 'Igor2' Palinkas
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

#ifndef RND_HIDLIB_H
#define RND_HIDLIB_H

#include <librnd/core/global_typedefs.h>

typedef struct rnd_mark_s {
	rnd_bool status;
	rnd_coord_t X, Y;
	unsigned user_placed:1;   /* if 1, the user has explicitly placed the mark - do not move it */

	/* Spare: see doc/developer/spare.txt */
	void (*spare_f1)(void), (*spare_f2)(void);
	long spare_l1, spare_l2, spare_l3, spare_l4;
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
	double spare_d1, spare_d2, spare_d3, spare_d4;
	rnd_coord_t spare_c1, spare_c2, spare_c3, spare_c4;
} rnd_mark_t;

struct rnd_hidlib_s {
	rnd_coord_t grid;                  /* grid resolution */
	rnd_coord_t grid_ox, grid_oy;      /* grid offset */
	rnd_coord_t size_x, size_y;        /* drawing area extents (or design dimensions) */
	char *name;                        /* name of the design */
	char *filename;                    /* name of the file (from load) */

	/* tool state */
	rnd_coord_t ch_x, ch_y, tool_x, tool_y; /* ch is crosshair */
	unsigned int tool_hit;                  /* optional: type of a hit object of RND_MOVE_TYPES; 0 if there was no RND_MOVE_TYPES object under the crosshair */
	unsigned int tool_click:1;              /* optional: true if clicked somewhere with the arrow tool */
	rnd_mark_t tool_grabbed;                /* point where a drag&drop operation started */
	rnd_box_t *tool_snapped_obj_bbox;

	/* internal */
	int *batch_ask_ovr;                /* if not NULL, override local ask-overwrite state - useful when manu operations that need to write files are ran in batch, e.g. in a cam job */

	/* Spare: see doc/developer/spare.txt */
	void (*spare_f1)(void), (*spare_f2)(void);
	long spare_l1, spare_l2, spare_l3, spare_l4;
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
	double spare_d1, spare_d2, spare_d3, spare_d4;
	rnd_coord_t spare_c1, spare_c2, spare_c3, spare_c4;
};

/* Global application callbacks and configuration: these do not depend on
   the designbeing edited, but on the application implementation. The
   app is required to fill in global rnd_app fields before calling
   the first librnd call. */
typedef struct rnd_app_s {

	/* application information (to be displayed on the UI) */
	const char *package;
	const char *version;
	const char *revision; /* VCS revision info, e.g. svn rev number */
	const char *url;

	/* when non-zero, allow multiple design files to be open at once */
	unsigned multi_design:1;

	/* menu file */
	const char **menu_file_paths;      /* optional: NULL terminated list of search paths for the menu file */
	const char *menu_name_fmt;         /* optional: printf format string for the menu file name; may contain one %s that will be substituted with rc.menu_file. */
	const char *default_embedded_menu; /* optional: the whole default menu file embedded in the executable; NULL if not present */

	/* app specific installation paths */
	const char *lib_dir; /* plugins are searched in lib_dir/plugins under here; should be e.g. /usr/lib/pcb-rnd */
	const char *dot_dir; /* plugins are searched in ~/dot_dir/plugins under here; should be e.g. .pcb-rnd; script persistency also uses this */

	/* path to the user's config directory and main config file (RND_CFR_USER) */
	const char *conf_userdir_path;
	const char *conf_user_path;

	/* path to the system (installed) config directory and main config file (RND_CFR_SYSTEM) */
	const char *conf_sysdir_path;
	const char *conf_sys_path;

	/* Recommended: embedded/internal config data (lihata document) */
	const char *conf_internal;

	/* Optional: if not NULL, called after conf updates to give conf_core a chance to update dynamic entries */
	void (*conf_core_postproc)(void);

	/* Optional: if not NULL, and returns non-zero, the given
	   conf value from the given source is ignored (not merged). Role
	   is really rnd_conf_role_t and policy is rnd_conf_policy_t, lhtn is
	   (lht_node_t *). */
	int (*conf_dont_merge_node)(const char *path, void *lhtn, int role, int default_prio, int default_policy, rnd_conf_native_t *target);

	/* Optional: a NULL terminated array of config paths that shall not be
	   loaded from desing or project files. This is for security reasons:
	   any node that may execute code or scripts should be listed here */
	rnd_conf_ignore_t *conf_prj_dsg_ignores;

	/* Optional: a NULL terminated array of config paths that should not
	   generate an error message when found in a file; should list old nodes
	   that got removed from the conf system but old files could still have them */
	rnd_conf_ignore_t *conf_ignores;

	/*** callbacks ***/
	/* Optional: called to update crosshair-attached object because crosshair coords likely changed; if NULL, rnd_tool_adjust_attached() is called instead (most apps want that) */
	void (*adjust_attached_objects)(rnd_hidlib_t *hl);

/* Optional: Suspend the crosshair: save all crosshair states in a newly
   allocated and returned temp buffer, then reset the crosshair to initial
   state; the returned buffer is used to restore the crosshair states later
   on. Used in the get location loop. */
	void *(*crosshair_suspend)(rnd_hidlib_t *hl);
	void (*crosshair_restore)(rnd_hidlib_t *hl, void *susp_data);

/* Optional: move the crosshair to an absolute x;y coord on the design and
   update the GUI; if mouse_mot is non-zero, the request is a direct result
   of a mouse motion event */
	void (*crosshair_move_to)(rnd_hidlib_t *hl, rnd_coord_t abs_x, rnd_coord_t abs_y, int mouse_mot);

/* Optional: draw any fixed mark on XOR overlay; if inhibit_drawing_mode is
   true, do not call ->set_drawing_mode */
	void (*draw_marks)(rnd_hidlib_t *hidlib, rnd_bool inhibit_drawing_mode);

	/* Draw any mark following the crosshair on XOR overlay; if inhibit_drawing_mode is true, do not call ->set_drawing_mode */
	void (*draw_attached)(rnd_hidlib_t *hidlib, rnd_bool inhibit_drawing_mode);

	/*** One of these two functions will be called whenever (parts of) the screen
     needs redrawing (on screen, print or export, design or preview). The expose
     function does the following:
      - allocate any GCs needed
      - set drawing mode
      - cycle through the layers, calling set_layer for each layer to be
        drawn, and only drawing objects (all or specified) of desired
        layers. ***/

	/* Mandatory: main expose: draw the design in the top window
	   (in pcb-rnd: all layers with all flags (no .content is used) */
	void (*expose_main)(rnd_hid_t *hid, const rnd_hid_expose_ctx_t *region, rnd_xform_t *xform_caller);

	/* Mandatory: preview expose: generic, dialog based, used in preview
	   widgets */
	void (*expose_preview)(rnd_hid_t *hid, const rnd_hid_expose_ctx_t *e);

	/* Spare: see doc/developer/spare.txt */
	void (*spare_f1)(void), (*spare_f2)(void), (*spare_f3)(void), (*spare_f4)(void), (*spare_f5)(void), (*spare_f6)(void);
	long spare_l1, spare_l2, spare_l3, spare_l4;
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
	double spare_d1, spare_d2, spare_d3, spare_d4;
} rnd_app_t;

extern rnd_app_t rnd_app;

void rnd_hidlib_event_uninit(void);
void rnd_hidlib_event_init(void);

/* print pending log messages to stderr after gui uninit */
void rnd_log_print_uninit_errs(const char *title);

/*** API wrappers provided for plugins (these are translated into calls to the
     app within the hidlib and are typically called from within librnd) ***/
void rnd_hidcore_crosshair_move_to(rnd_hidlib_t *hidlib, rnd_coord_t abs_x, rnd_coord_t abs_y, int mouse_mot);

#endif
