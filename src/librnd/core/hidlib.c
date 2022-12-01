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
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <librnd/rnd_config.h>

#include <librnd/core/rnd_conf.h>
#include <librnd/core/event.h>
#include <librnd/core/error.h>
#include <librnd/core/conf_multi_temp.h>

rnd_app_t rnd_app;

int rnd_hid_in_main_loop = 0;

void rnd_log_print_uninit_errs(const char *title)
{
	rnd_logline_t *n, *from = rnd_log_find_first_unseen();
	int printed = 0;

	for(n = from; n != NULL; n = n->next) {
		if ((n->level >= RND_MSG_INFO) || rnd_conf.rc.verbose) {
			if (!printed)
				fprintf(stderr, "*** %s:\n", title);
			fprintf(stderr, "%s", n->str);
			printed = 1;
		}
	}
	if (printed)
		fprintf(stderr, "\n\n");
}


/*** multi ***/

rnd_design_t *rnd_curr_dsg;

static void rnd_multi_switched_to_notify(rnd_design_t *dsg)
{
	rnd_event(dsg, RND_EVENT_DESIGN_SET_CURRENT, "p", dsg);
	rnd_event(dsg, RND_EVENT_DESIGN_REPLACED, "i", 0);
	rnd_event(dsg, RND_EVENT_DESIGN_FN_CHANGED, NULL);
}

void rnd_multi_switch_to_(rnd_design_t *dsg)
{
	rnd_curr_dsg = dsg;
	assert(dsg->saved_rnd_conf != NULL);
	rnd_conf_state_load(dsg->saved_rnd_conf);
	rnd_multi_switched_to_notify(dsg);
}

rnd_design_t *rnd_multi_switch_to(rnd_design_t *dsg)
{
	rnd_design_t *curr = rnd_curr_dsg;

	/* switch to nothing (useful when loading a new sheet) */
	if (dsg == NULL) {
		rnd_conf_state_save(curr->saved_rnd_conf);
		rnd_curr_dsg = NULL;
		return curr;
	}

	/* first switch from nothing to s */
	if (curr == NULL) {
		rnd_curr_dsg = dsg;
		rnd_multi_switched_to_notify(dsg);
		return curr;
	}

	/* switching to the current is no-op */
	if (dsg == curr)
		return curr;

	rnd_conf_state_save(curr->saved_rnd_conf);
	rnd_multi_switch_to_(dsg);

	return curr;
}

void rnd_multi_switch_to_delta(rnd_design_t *curr, int step)
{
	if (curr == NULL) {
		if (step == 0)
			return; /* would switch to the hidlib that's already active in the GUI */
		curr = rnd_curr_dsg;
	}
	else if (step == 0) {
		rnd_multi_switch_to(curr);
		return;
	}

	if (curr == NULL)
		return; /* no known starting point */

	if (step > 0) {
		for(;step > 0; step--) {
			curr = curr->link.next;
			if (curr == NULL)
				curr = gdl_first(&rnd_designs);
		}
	}
	else if (step < 0) {
		for(;step < 0; step++) {
			curr = curr->link.prev;
			if (curr == NULL)
				curr = gdl_last(&rnd_designs);
		}
	}

	if (curr != NULL)
		rnd_multi_switch_to(curr);
}

rnd_design_t *rnd_multi_neighbour_sheet(rnd_design_t *dsg)
{
	if (dsg == NULL)
		dsg = rnd_curr_dsg;

	if (dsg == NULL)
		return NULL;

	if (dsg->link.next != NULL)
		return dsg->link.next;
	return dsg->link.prev;
}

rnd_design_t *rnd_multi_get_current(void)
{
	return rnd_curr_dsg;
}

