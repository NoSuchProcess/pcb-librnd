/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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
 *
 */

#include <librnd/rnd_config.h>
#include <librnd/hid/hid.h>
#include <librnd/hid/tool.h>
#include <librnd/hid/hid_menu.h>
#include <librnd/core/event.h>

void rnd_hid_notify_crosshair_change(rnd_design_t *hl, rnd_bool changes_complete)
{
	if (rnd_render->notify_crosshair_change)
		rnd_render->notify_crosshair_change(rnd_gui, changes_complete);
	rnd_event(hl, RND_EVENT_CROSSHAIR_MOVE, "i", (int)changes_complete, NULL);
}

char *(*rnd_hid_fileselect_imp)(rnd_hid_t *hid, const char *title, const char *descr, const char *default_file, const char *default_ext, const rnd_hid_fsd_filter_t *flt, const char *history_tag, rnd_hid_fsd_flags_t flags, rnd_hid_dad_subdialog_t *sub) = NULL;

char *rnd_hid_fileselect(rnd_hid_t *hid, const char *title, const char *descr, const char *default_file, const char *default_ext, const rnd_hid_fsd_filter_t *flt, const char *history_tag, rnd_hid_fsd_flags_t flags, rnd_hid_dad_subdialog_t *sub)
{
	if ((hid == NULL) || (rnd_hid_fileselect_imp == NULL))
		return NULL;

	return rnd_hid_fileselect_imp(hid, title, descr, default_file, default_ext, flt, history_tag, flags, sub);
}


/*** GUI batch timer ***/

#define BTMR_REFRESH_TIME_MS 300

static int btmr_timer_active = 0;
static rnd_hidval_t btmr_timer;
static rnd_design_t *btmr_hidlib;

static void btmr_cb(rnd_hidval_t user_data)
{
	btmr_timer_active = 0;
	rnd_hid_menu_merge_inhibit_inc();
	rnd_event(btmr_hidlib, RND_EVENT_GUI_BATCH_TIMER, NULL);
	rnd_hid_menu_merge_inhibit_dec();
}

void rnd_hid_gui_batch_timer(rnd_design_t *hidlib)
{
	rnd_hidval_t timerdata;

	if ((rnd_gui == NULL) || (!rnd_gui->gui))
		return;

	if (btmr_timer_active) {
		rnd_gui->stop_timer(rnd_gui, btmr_timer);
		btmr_timer_active = 0;
	}

	timerdata.ptr = NULL;
	btmr_timer = rnd_gui->add_timer(rnd_gui, btmr_cb, BTMR_REFRESH_TIME_MS, timerdata);
	btmr_timer_active = 1;
	btmr_hidlib = hidlib;
}

int rnd_hid_get_coords(const char *msg, rnd_coord_t *x, rnd_coord_t *y, int force)
{
	if (rnd_gui == NULL) {
		fprintf(stderr, "rnd_hid_get_coords: can not get coordinates (no gui) for '%s'\n", msg);
		*x = 0;
		*y = 0;
		return -1;
	}
	else
		return rnd_gui->get_coords(rnd_gui, msg, x, y, force);
}

/*** mouse cursor management ***/

/* need to register a dummy tool for the normal-cursor because mouse cursors
   are managed by the tool code */

static rnd_tool_t pcb_tool_normal = {
	"normal-cursor", NULL, NULL, 0, NULL, RND_TOOL_CURSOR_NAMED("left_ptr"), 0,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0 };

static int last_normal_cursor = -1, cursor_override = -1;
static int normal_id = -1;
void rnd_hid_set_mouse_cursor(int id)
{
	last_normal_cursor = id;
	if ((rnd_gui != NULL) && (cursor_override < 0))
		rnd_gui->set_mouse_cursor(rnd_gui, id);
}

void rnd_hid_override_mouse_cursor(int id)
{
	if (id < 0) {
		if (rnd_gui != NULL) {
			if (last_normal_cursor < 0) {
				if (normal_id < 0)
					normal_id = rnd_tool_reg(&pcb_tool_normal, "hid.c");
				last_normal_cursor = normal_id;
			}
			rnd_gui->set_mouse_cursor(rnd_gui, last_normal_cursor);
		}
		cursor_override = -1;
	}
	else {
		if (rnd_gui != NULL)
			rnd_gui->set_mouse_cursor(rnd_gui, id);
		cursor_override = id;
	}
}


/* need to register a dummy tool for the busy-cursor because mouse cursors
   are managed by the tool code */
static rnd_tool_t pcb_tool_point = {
	"busy-cursor", NULL, NULL, 0, NULL, RND_TOOL_CURSOR_NAMED("busy"), 0,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0 };


/* save cursor_override in this */
static int last_busy_override = -1;
static int busy_state = 0, busy_id = -1;
void rnd_hid_busy(rnd_design_t *design, rnd_bool is_busy)
{
	/* no state change */
	if (!!is_busy == busy_state)
		return;

	rnd_event(design, RND_EVENT_BUSY, "i", is_busy, NULL);

	if (is_busy) {
		last_busy_override = cursor_override; /* remember what override we override */
		if (busy_id < 0)
			busy_id = rnd_tool_reg(&pcb_tool_point, "hid.c");
		rnd_hid_override_mouse_cursor(busy_id);
	}
	else
		rnd_hid_override_mouse_cursor(last_busy_override);
	busy_state = !!is_busy;
}
