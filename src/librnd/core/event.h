/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2020 Tibor 'Igor2' Palinkas
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

#ifndef RND_EVENT_H
#define RND_EVENT_H
#include <librnd/config.h>
#include <librnd/core/unit.h>
#include <librnd/core/global_typedefs.h>
#include <librnd/core/hidlib.h>

/* Prefix:
   [d]: per-design: generated for a specific rnd_design_t
   [a]: per-app: generated once, not targeted or specific for a rnd_design_t
*/
typedef enum {
	RND_EVENT_GUI_INIT,               /* [a] finished initializing the GUI called right before the main loop of the GUI; args: (void) */
	RND_EVENT_CLI_ENTER,              /* [a] the user pressed enter on a CLI command - called before parsing the line for actions; args: (str commandline) */

	RND_EVENT_TOOL_REG,               /* [a] called after a new tool has been registered; arg is (rnd_tool_t *) of the new tool */
	RND_EVENT_TOOL_SELECT_PRE,        /* [a] called before a tool is selected; arg is (int *ok, int toolid); if ok is non-zero if the selection is accepted */
	RND_EVENT_TOOL_RELEASE,           /* [a] no arg */
	RND_EVENT_TOOL_PRESS,             /* [a] no arg */

	RND_EVENT_BUSY,                   /* [a] called before/after CPU-intensive task begins; argument is an integer: 1 is before, 0 is after */
	RND_EVENT_LOG_APPEND,             /* [a] called after a new log line is appended; arg is a pointer to the log line */
	RND_EVENT_LOG_CLEAR,              /* [a] called after a clear; args: two pointers; unsigned long "from" and "to" */

	RND_EVENT_GUI_LEAD_USER,          /* [a] GUI aid to lead the user attention to a specific location on the design in the main drawing area; args: (coord x, coord y, int enabled) */
	RND_EVENT_GUI_DRAW_OVERLAY_XOR,   /* [a] called in design draw after finished drawing the xor marks, still in xor draw mode; argument is a pointer to the GC to use for drawing */
	RND_EVENT_USER_INPUT_POST,        /* [a] generated any time any user input reaches core, after processing it */
	RND_EVENT_USER_INPUT_KEY,         /* [a] generated any time a keypress is registered by the hid_cfg_key code (may be one key stroke of a multi-stroke sequence) */

	RND_EVENT_CROSSHAIR_MOVE,         /* [a] called when the crosshair changed coord; arg is an integer which is zero if more to come */

	RND_EVENT_DAD_NEW_DIALOG,         /* [a] called by the GUI after a new DAD dialog is open; args are pointer hid_ctx,  string dialog id and a pointer to int[4] for getting back preferre {x,y,w,h} (-1 means unknown) */
	RND_EVENT_DAD_NEW_GEO,            /* [a] called by the GUI after the window geometry _may_ have changed; args are: void *hid_ctx, const char *dialog id, int x1, int y1, int width, int height */

	RND_EVENT_EXPORT_SESSION_BEGIN,   /* [a] called before an export session (e.g. CAM script execution) starts; should not be nested; there's no guarantee that options are parsed before or after this event */
	RND_EVENT_EXPORT_SESSION_END,     /* [a] called after an export session (e.g. CAM script execution) ends */

	RND_EVENT_STROKE_START,           /* [a] parameters: rnd_coord_t x, rnd_coord_t y */
	RND_EVENT_STROKE_RECORD,          /* [a] parameters: rnd_coord_t x, rnd_coord_t y */
	RND_EVENT_STROKE_FINISH,          /* [a] parameters: int *handled; if it is non-zero, stroke has handled the request and Tool() should return 1, breaking action script execution */

	RND_EVENT_DESIGN_SET_CURRENT,     /* [d] called after the current design on display got _replaced_; should be emitted when the design data got replaced in memory or GUI switched between designs loaded (in case of multiple designs); argument is rnd_design_t *now_active */
	RND_EVENT_DESIGN_SET_CURRENT_INTR,/* [d] same as RND_EVENT_DESIGN_SET_CURRENT but called first, only for librnd internal usage */
	RND_EVENT_DESIGN_META_CHANGED,    /* [d] called if the metadata of the design has changed (size, changed flag, etc; anything except the file names) */
	RND_EVENT_DESIGN_FN_CHANGED,      /* [d] called after the file name of the design has changed */

	RND_EVENT_SAVE_PRE,               /* [d] called before saving the design (required for window placement) */
	RND_EVENT_SAVE_POST,              /* [d] called after saving the design (required for window placement) */
	RND_EVENT_LOAD_PRE,               /* [d] called before loading a new design (required for window placement) */
	RND_EVENT_LOAD_POST,              /* [d] called after loading a new design, whether it was successful or not (required for window placement) */

	RND_EVENT_MENU_CHANGED,           /* [a] called after a menu merging (which means actual menu system change) */
	RND_EVENT_GUI_BATCH_TIMER,        /* [a] request timed batch GUI update, see rnd_hid_gui_batch_timer() */
	RND_EVENT_MAINLOOP_CHANGE,        /* [a] called after the mainloop variable has changed */

	RND_EVENT_DAD_NEW_PANE,           /* [a] called by the GUI after a new paned widget is created; args are pointer hid_ctx, string dialog, string paned id and a (double *) for getting back preferred ratio (-1 means unknown) */
	RND_EVENT_DAD_PANE_GEO_CHG,       /* [a] called by the GUI after the pane geometry _may_ have changed; args are: void *hid_ctx, const char *dialog id, const char *pane id, double ratio */

	RND_EVENT_CONF_FILE_SAVE_POST,    /* [a] called after a stand-alone config/project file has been saved. Args: (const char *) file name, int role */

	RND_EVENT_last                    /* not a real event */
} rnd_event_id_t;

/* Maximum number of arguments for an event handler, auto-set argv[0] included */
#define RND_EVENT_MAX_ARG 16

/* Argument types in event's argv[] */
typedef enum {
	RND_EVARG_INT,     /* format char: i */
	RND_EVARG_DOUBLE,  /* format char: d */
	RND_EVARG_STR,     /* format char: s */
	RND_EVARG_PTR,     /* format char: p */
	RND_EVARG_COORD,   /* format char: c */
	RND_EVARG_ANGLE    /* format char: a */
} rnd_event_argtype_t;

/* An argument is its type and value */
struct rnd_event_arg_s {
	rnd_event_argtype_t type;
	union {
		int i;
		double d;
		const char *s;
		void *p;
		rnd_coord_t c;
		rnd_angle_t a;
	} d;
};

/* Initialize the event system */
void rnd_events_init(void);

/* Uninitialize the event system and remove all events */
void rnd_events_uninit(void);


/* Event callback prototype; user_data is the same as in rnd_event_bind().
   argv[0] is always an RND_EVARG_INT with the event id that triggered the event. */
typedef void (rnd_event_handler_t)(rnd_design_t *design, void *user_data, int argc, rnd_event_arg_t argv[]);

/* Bind: add a handler to the call-list of an event; the cookie is also remembered
   so that mass-unbind is easier later. user_data is passed to the handler. */
void rnd_event_bind(rnd_event_id_t ev, rnd_event_handler_t *handler, void *user_data, const char *cookie);

/* Unbind: remove a handler from an event */
void rnd_event_unbind(rnd_event_id_t ev, rnd_event_handler_t *handler);

/* Unbind by cookie: remove all handlers from an event matching the cookie */
void rnd_event_unbind_cookie(rnd_event_id_t ev, const char *cookie);

/* Unbind all by cookie: remove all handlers from all events matching the cookie */
void rnd_event_unbind_allcookie(const char *cookie);

/* Event trigger: call all handlers for an event. Fmt is a list of
   format characters (e.g. i for RND_EVARG_INT). */
void rnd_event(rnd_design_t *design, rnd_event_id_t ev, const char *fmt, ...);

/* Return the name of an event as seen on regisrtation */
const char *rnd_event_name(rnd_event_id_t ev);

/* The application may register its events by defining an enum
   starting from RND_EVENT_app and calling this function after hidlib_init1() */
void rnd_event_app_reg(long last_event_id, const char **event_names, long sizeof_event_names);
#define RND_EVENT_app 100

#endif
