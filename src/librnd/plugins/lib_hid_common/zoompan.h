/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <librnd/hid/hid.h>

extern int rnd_hid_dlg_gui_inited;
/* Return error from an action if there's no GUI, or set result to 0
   if there's GUI */
#define RND_GUI_NOGUI() \
do { \
	if ((rnd_gui == NULL) || (!rnd_gui->gui) || !rnd_hid_dlg_gui_inited) { \
		RND_ACT_IRES(1); \
		return 0; \
	} \
	RND_ACT_IRES(0); \
} while(0)


/* Core of the generic Zoom() action; the actual action needs to be
   implemented by the application so it can extend it with things like
   Zoom(Selected). */

#define rnd_gui_acts_zoom \
	"Zoom()\n" \
	"Zoom([+|-|=]factor)\n" \
	"Zoom(x1, y1, x2, y2)\n" \
	"Zoom(?)\n" \
	"Zoom(get)\n" \

extern const char *rnd_acts_Zoom;
extern const char rnd_acth_Zoom_default[];
extern const char rnd_acts_Zoom_default[];
fgw_error_t rnd_gui_act_zoom(fgw_arg_t *res, int argc, fgw_arg_t *argv);


/*** actions ***/
extern const char rnd_acts_Pan[];
extern const char rnd_acth_Pan[];
fgw_error_t rnd_act_Pan(fgw_arg_t *res, int argc, fgw_arg_t *argv);

extern const char rnd_acts_Center[];
extern const char rnd_acth_Center[];
fgw_error_t rnd_act_Center(fgw_arg_t *res, int argc, fgw_arg_t *argv);

extern const char rnd_acts_Scroll[];
extern const char rnd_acth_Scroll[];
fgw_error_t rnd_act_Scroll(fgw_arg_t *res, int argc, fgw_arg_t *argv);
