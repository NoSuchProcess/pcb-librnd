/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017, 2018, 2021 Tibor 'Igor2' Palinkas
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

#include <stdlib.h>
#include <librnd/core/plugins.h>
#include <librnd/core/conf_hid.h>
#include <librnd/core/event.h>
#include <librnd/core/hid_menu.h>
#include <librnd/core/actions.h>
#include "grid_menu.h"
#include "cli_history.h"
#include "lead_user.h"
#include "place.h"

#include "lib_hid_common.h"
#include "dialogs_conf.h"
#include "dlg_comm_m.h"
#include "dlg_log.h"
#include "dlg_fileselect.h"
#include "dlg_plugins.h"
#include "dlg_pref.h"
#include "act_dad.h"
#include "toolbar.h"
#include "zoompan.h"
#include "xpm.h"
#include "conf_internal.c"

conf_dialogs_t dialogs_conf;

void rnd_hid_announce_gui_init(rnd_hidlib_t *hidlib)
{
	rnd_hid_menu_merge_inhibit_inc();
	rnd_event(hidlib, RND_EVENT_GUI_INIT, NULL);
	rnd_hid_menu_merge_inhibit_dec();
}

static const char *grid_cookie = "lib_hid_common/grid";
static const char *lead_cookie = "lib_hid_common/user_lead";
static const char *wplc_cookie = "lib_hid_common/window_placement";

extern void rnd_dad_spin_update_global_coords(void);
static void grid_unit_chg_ev(rnd_conf_native_t *cfg, int arr_idx)
{
	rnd_dad_spin_update_global_coords();
}

const char rnd_acts_Command[] = "Command()";
const char rnd_acth_Command[] = "Displays the command line input in the status area.";
/* DOC: command */
fgw_error_t rnd_act_Command(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	RND_GUI_NOGUI();
	rnd_gui->open_command(rnd_gui);
	RND_ACT_IRES(0);
	return 0;
}

extern fgw_error_t rnd_act_dlg_xpm_by_name(fgw_arg_t *res, int argc, fgw_arg_t *argv);

static const char rnd_acth_gui[] = "Intenal: GUI frontend action. Do not use directly.";

rnd_action_t hid_common_action_list[] = {
	{"dad", rnd_act_dad, rnd_acth_dad, rnd_acts_dad},
	{"Pan", rnd_act_Pan, rnd_acth_Pan, rnd_acts_Pan},
	{"Center", rnd_act_Center, rnd_acth_Center, rnd_acts_Center},
	{"Scroll", rnd_act_Scroll, rnd_acth_Scroll, rnd_acts_Scroll},
	{"LogDialog", rnd_act_LogDialog, rnd_acth_LogDialog, rnd_acts_LogDialog},
	{"FsdTest", rnd_act_FsdTest, rnd_acth_FsdTest, rnd_acts_FsdTest},
	{"Command", rnd_act_Command, rnd_acth_Command, rnd_acts_Command},
	{"ManagePlugins", pcb_act_ManagePlugins, pcb_acth_ManagePlugins, pcb_acts_ManagePlugins},
	{"gui_PromptFor", rnd_act_gui_PromptFor, rnd_acth_gui, NULL},
	{"gui_MessageBox", rnd_act_gui_MessageBox, rnd_acth_gui, NULL},
	{"gui_FallbackColorPick", rnd_act_gui_FallbackColorPick, rnd_acth_gui, NULL},
	{"gui_MayOverwriteFile", rnd_act_gui_MayOverwriteFile, rnd_acth_gui, NULL},
	{"rnd_toolbar_init", rnd_act_rnd_toolbar_init, rnd_acth_rnd_toolbar_init, NULL},
	{"rnd_toolbar_uninit", rnd_act_rnd_toolbar_uninit, rnd_acth_rnd_toolbar_uninit, NULL},
	{"rnd_zoom", rnd_gui_act_zoom, rnd_acth_Zoom_default, rnd_acts_Zoom_default},
	{"rnd_dlg_xpm_by_name", rnd_act_dlg_xpm_by_name, rnd_acth_gui, NULL},
	{"Preferences", rnd_act_Preferences, rnd_acth_Preferences, rnd_acts_Preferences},
	{"dlg_confval_edit", rnd_act_dlg_confval_edit, rnd_acth_dlg_confval_edit, rnd_acts_dlg_confval_edit}
};

extern const char *rnd_acts_Zoom;
extern const char rnd_acts_Zoom_default[];

static const char *hid_common_cookie = "lib_hid_common plugin";

int pplg_check_ver_lib_hid_common(int ver_needed) { return 0; }

static rnd_conf_hid_id_t conf_id;

void pplg_uninit_lib_hid_common(void)
{
	rnd_conf_unreg_intern(dialogs_conf_internal);
	rnd_clihist_save();
	rnd_clihist_uninit();
	rnd_event_unbind_allcookie(grid_cookie);
	rnd_event_unbind_allcookie(lead_cookie);
	rnd_event_unbind_allcookie(wplc_cookie);
	rnd_conf_hid_unreg(grid_cookie);
	rnd_dialog_place_uninit();
	rnd_remove_actions_by_cookie(hid_common_cookie);
	rnd_act_dad_uninit();
	rnd_conf_unreg_fields("plugins/lib_hid_common/");
	rnd_conf_unreg_fields("plugins/dialogs/");
	rnd_dlg_log_uninit();
}

int pplg_init_lib_hid_common(void)
{
	static rnd_conf_hid_callbacks_t ccb, ccbu;
	rnd_conf_native_t *nat;

	RND_API_CHK_VER;
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(dialogs_conf, field,isarray,type_name,cpath,cname,desc,flags);
/*#include "lib_hid_common_conf_fields.h"*/
#include "dialogs_conf_fields.h"

	rnd_dlg_log_init();
	RND_REGISTER_ACTIONS(hid_common_action_list, hid_common_cookie)
	rnd_act_dad_init();

	rnd_conf_reg_intern(dialogs_conf_internal);

	rnd_dialog_place_init();

	rnd_event_bind(RND_EVENT_GUI_INIT, rnd_grid_update_ev, NULL, grid_cookie);
	rnd_event_bind(RND_EVENT_GUI_LEAD_USER, rnd_lead_user_ev, NULL, lead_cookie);
	rnd_event_bind(RND_EVENT_GUI_DRAW_OVERLAY_XOR, rnd_lead_user_draw_ev, NULL, lead_cookie);
	rnd_event_bind(RND_EVENT_DAD_NEW_DIALOG, rnd_dialog_place, NULL, wplc_cookie);
	rnd_event_bind(RND_EVENT_DAD_NEW_GEO, rnd_dialog_resize, NULL, wplc_cookie);

	conf_id = rnd_conf_hid_reg(grid_cookie, NULL);

	memset(&ccb, 0, sizeof(ccb));
	ccb.val_change_post = rnd_grid_update_conf;
	nat = rnd_conf_get_field("editor/grids");
	if (nat != NULL)
		rnd_conf_hid_set_cb(nat, conf_id, &ccb);

	memset(&ccbu, 0, sizeof(ccbu));
	ccbu.val_change_post = grid_unit_chg_ev;
	nat = rnd_conf_get_field("editor/grid_unit");
	if (nat != NULL)
		rnd_conf_hid_set_cb(nat, conf_id, &ccbu);

	return 0;
}
