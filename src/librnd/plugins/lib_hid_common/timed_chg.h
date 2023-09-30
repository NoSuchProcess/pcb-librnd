/*
 *                            COPYRIGHT
 *
 *  cschem - modular/flexible schematics editor - GUI
 *  Copyright (C) 2022,2023 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 PET Fund in 2022)
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

/* Manage timer for postponed change for any dialog */

/* caller needs to define:
#define RND_TIMED_CHG_TIMEOUT conf_core.editor.edit_time
*/

typedef struct rnd_timed_chg_s {
	rnd_hidval_t timer;
	void (*cb)(void *uctx);
	void *uctx;
	void *hid_ctx;
	int wtiming;
	char active;
} rnd_timed_chg_t;

/* set up a timed change with a callback */
RND_INLINE void rnd_timed_chg_init(rnd_timed_chg_t *chg, void (*cb)(void *uctx), void *uctx);

/* (re-)schedule a change; (re)start timer and call cb on timeout */
RND_INLINE void rnd_timed_chg_schedule(rnd_timed_chg_t *chg);

/* if timer is active, force-call it and stop it */
RND_INLINE void rnd_timed_chg_finalize(rnd_timed_chg_t *chg);

/* stop the timer without calling back anything */
RND_INLINE void rnd_timed_chg_cancel(rnd_timed_chg_t *chg);


/* Creates a "pending" label; when the dialog box is created, call
   rnd_timed_chg_timing_init() */
#define RND_DAD_TIMING(dlg, chg, what) \
	do { \
		RND_DAD_LABEL(dlg, "(Editing; pending " what " changes)"); \
			(chg)->wtiming = RND_DAD_CURRENT(dlg); \
	} while(0)

/* Call this after the dialog has been realized if RND_DAD_TIMING() has been
   used */
RND_INLINE void rnd_timed_chg_timing_init(rnd_timed_chg_t *chg, void *hid_ctx);


/*** Implementation ***/

static void rnd_timed_chg_cb(rnd_hidval_t user_data)
{
	rnd_timed_chg_t *chg = user_data.ptr;
	chg->active = 0;
	chg->cb(chg->uctx);

	if (chg->wtiming >= 0)
		rnd_gui->attr_dlg_widget_hide(chg->hid_ctx, chg->wtiming, 1);
}

RND_INLINE void rnd_timed_chg_finalize(rnd_timed_chg_t *chg)
{
	if (!chg->active)
		return;

	/* first stop the timer... */
	rnd_gui->stop_timer(rnd_gui, chg->timer);

	/* ... but we have a race here, the timer could be fired between if it
	   was really async; if we are still active now, the timer is stopped
	   and we wanted to run once */
	if (chg->active) {
		rnd_hidval_t user_data;
		user_data.ptr = chg;
		rnd_timed_chg_cb(user_data);
	}
}

RND_INLINE void rnd_timed_chg_cancel(rnd_timed_chg_t *chg)
{
	if (!chg->active)
		return;

	rnd_gui->stop_timer(rnd_gui, chg->timer);
}

RND_INLINE void rnd_timed_chg_schedule(rnd_timed_chg_t *chg)
{
	rnd_hidval_t user_data;

	if (chg->active)
		rnd_gui->stop_timer(rnd_gui, chg->timer);

	user_data.ptr = chg;
	chg->active = 1;
	chg->timer = rnd_gui->add_timer(rnd_gui, rnd_timed_chg_cb, RND_TIMED_CHG_TIMEOUT, user_data);

	if (chg->wtiming >= 0)
		rnd_gui->attr_dlg_widget_hide(chg->hid_ctx, chg->wtiming, 0);
}

RND_INLINE void rnd_timed_chg_init(rnd_timed_chg_t *chg, void (*cb)(void *uctx), void *uctx)
{
	chg->active = 0;
	chg->cb = cb;
	chg->uctx = uctx;
	chg->hid_ctx = NULL;
	chg->wtiming = -1;
}

RND_INLINE void rnd_timed_chg_timing_init(rnd_timed_chg_t *chg, void *hid_ctx)
{
	if (chg->wtiming >= 0) {
		chg->hid_ctx = hid_ctx;
		rnd_gui->attr_dlg_widget_hide(hid_ctx, chg->wtiming, 1);
	}
}
