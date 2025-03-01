/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
 *  Copyright (C) 2016..2021,2024 Tibor 'Igor2' Palinkas
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
 *
 */

#include <librnd/rnd_config.h>

#include <stdlib.h>
#include <locale.h>
#include <ctype.h>

#include <genvector/vts0.h>

#include <puplug/os_dep_fs.h>

#include <librnd/hid/hid.h>
#include <librnd/hid/hid_nogui.h>
#include <librnd/core/event.h>

/* for dlopen() and friends; will also solve all system-dependent includes
   and provides a dl-compat layer on windows. Also solves the opendir related
   includes. */
#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include <librnd/hid/hid_attrib.h>
#include <librnd/hid/hid_init.h>
#include <librnd/hid/hid_dad_unit.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/compat_fs.h>
#include <librnd/core/file_loaded.h>
#include <librnd/core/hidlib.h>
#include <librnd/core/rnd_conf.h>
#include <librnd/core/conf.h>
#include <librnd/core/paths.h>
#include <librnd/core/project.h>
#include <librnd/hid/grid.h>
#include <librnd/core/funchash.h>
#include <librnd/hid/hid_menu.h>
#include <librnd/core/safe_fs.h>
#include <librnd/hid/buildin.hidlib.h>
#include <librnd/hid/tool.h>
#include "../../../config.h"

static const char *hid_init_cookie = "hidlib";


int rnd_coord_t_size = sizeof(rnd_coord_t);

const char *rnd_ver_str = RND_VER_STR;

static const char *flt_any[] = {"*", "*.*", NULL};

const rnd_hid_fsd_filter_t rnd_hid_fsd_filter_any[] = {
	{ "all", NULL, flt_any },
	{ NULL, NULL, NULL }
};

rnd_hid_t **rnd_hid_list = 0;
int rnd_hid_num_hids = 0;

rnd_hid_t *rnd_render = NULL;
rnd_hid_t *rnd_exporter = NULL;

int rnd_pixel_slop = 1;

rnd_plugin_dir_t *rnd_plugin_dir_first = NULL, *rnd_plugin_dir_last = NULL;

void rnd_hid_init()
{
	char *tmp;

	/* Setup a "nogui" default HID */
	rnd_render = rnd_gui = rnd_hid_nogui_get_hid();

#ifdef LIBRNDLIBDIR
#ifdef RND_WANT_FLOATING_FHS
	tmp = rnd_concat(rnd_w32_root, RND_DIR_SEPARATOR_S, LIBRNDLIBDIR_RELATIVE, RND_DIR_SEPARATOR_S, "plugins", RND_DIR_SEPARATOR_S, HOST, NULL);
	rnd_plugin_add_dir(tmp);
	free(tmp);

	tmp = rnd_concat(rnd_w32_root, RND_DIR_SEPARATOR_S, LIBRNDLIBDIR_RELATIVE, RND_DIR_SEPARATOR_S, "plugins", NULL);
	rnd_plugin_add_dir(tmp);
	free(tmp);

#else
	/* librnd's own */
	tmp = LIBRNDLIBDIR RND_DIR_SEPARATOR_S "plugins" RND_DIR_SEPARATOR_S HOST;
	rnd_plugin_add_dir(tmp);

	tmp = LIBRNDLIBDIR RND_DIR_SEPARATOR_S "plugins";
	rnd_plugin_add_dir(tmp);
#endif
#endif

	/* host app's */
TODO("make this configurable - add to conf_prj_dsg_ignores avoid plugin injection")
	tmp = rnd_concat(rnd_conf.rc.path.exec_prefix, RND_DIR_SEPARATOR_S, "lib", RND_DIR_SEPARATOR_S, rnd_app.package, RND_DIR_SEPARATOR_S, "plugins", RND_DIR_SEPARATOR_S, HOST, NULL);
	rnd_plugin_add_dir(tmp);
	free(tmp);

	tmp = rnd_concat(rnd_conf.rc.path.exec_prefix, RND_DIR_SEPARATOR_S, "lib", RND_DIR_SEPARATOR_S, rnd_app.package, RND_DIR_SEPARATOR_S, "plugins", NULL);
	rnd_plugin_add_dir(tmp);
	free(tmp);

	/* hardwired libdir, just in case exec-prefix goes wrong (e.g. linstall) */
	if (rnd_app.lib_dir != NULL) {
		tmp = rnd_concat(rnd_app.lib_dir, RND_DIR_SEPARATOR_S, "plugins", RND_DIR_SEPARATOR_S, HOST, NULL);
		rnd_plugin_add_dir(tmp);
		free(tmp);
		tmp = rnd_concat(rnd_app.lib_dir, RND_DIR_SEPARATOR_S, "plugins", NULL);
		rnd_plugin_add_dir(tmp);
		free(tmp);
	}

	/* rnd_conf.rc.path.home is set by the conf_core immediately on startup */
	if ((rnd_conf.rc.path.home != NULL) && (rnd_app.dot_dir != NULL)) {
		tmp = rnd_concat(rnd_conf.rc.path.home, RND_DIR_SEPARATOR_S, rnd_app.dot_dir, RND_DIR_SEPARATOR_S, "plugins", RND_DIR_SEPARATOR_S, HOST, NULL);
		rnd_plugin_add_dir(tmp);
		free(tmp);

		tmp = rnd_concat(rnd_conf.rc.path.home, RND_DIR_SEPARATOR_S, rnd_app.dot_dir, RND_DIR_SEPARATOR_S, "plugins", NULL);
		rnd_plugin_add_dir(tmp);
		free(tmp);
	}

	tmp = rnd_concat("plugins", RND_DIR_SEPARATOR_S, HOST, NULL);
	rnd_plugin_add_dir(tmp);
	free(tmp);

	rnd_plugin_add_dir("plugins");
}

void rnd_hid_uninit(void)
{
	rnd_plugin_dir_t *pd, *next;

	if (rnd_hid_num_hids > 0) {
		int i;
		for (i = rnd_hid_num_hids-1; i >= 0; i--) {
			if (rnd_hid_list[i]->uninit != NULL)
				rnd_hid_list[i]->uninit(rnd_hid_list[i]);
		}
	}

	pup_uninit(&rnd_pup);

	rnd_export_uninit();

	free(rnd_hid_list);

	for(pd = rnd_plugin_dir_first; pd != NULL; pd = next) {
		next = pd->next;
		free(pd->path);
		free(pd);
	}
	rnd_plugin_dir_first = rnd_plugin_dir_last = NULL;
}

void rnd_hid_register_hid(rnd_hid_t *hid)
{
	int i;
	int sz = (rnd_hid_num_hids + 2) * sizeof(rnd_hid_t *);

	if (hid->struct_size != sizeof(rnd_hid_t)) {
		fprintf(stderr, "Warning: hid \"%s\" has an incompatible ABI.\n", hid->name);
		return;
	}

	for (i = 0; i < rnd_hid_num_hids; i++)
		if (hid == rnd_hid_list[i])
			return;

	rnd_hid_num_hids++;
	if (rnd_hid_list)
		rnd_hid_list = (rnd_hid_t **) realloc(rnd_hid_list, sz);
	else
		rnd_hid_list = (rnd_hid_t **) malloc(sz);

	rnd_hid_list[rnd_hid_num_hids - 1] = hid;
	rnd_hid_list[rnd_hid_num_hids] = 0;
}

void rnd_hid_remove_hid(rnd_hid_t *hid)
{
	int i;

	for (i = 0; i < rnd_hid_num_hids; i++) {
		if (hid == rnd_hid_list[i]) {
			rnd_hid_list[i] = rnd_hid_list[rnd_hid_num_hids - 1];
			rnd_hid_list[rnd_hid_num_hids - 1] = 0;
			rnd_hid_num_hids--;
			return;
		}
	}
}

static void rnd_hid_load_gui_plugin(const char *plugname);

void rnd_hid_do_all_gui_plugins(void *ctx, int (*cb)(void *ctx, const char *name, const char *dir, const char *fn))
{
	char **dir;
	gds_t tmp = {0};

	if (rnd_conf.rc.verbose >= RND_MSG_INFO)
		fprintf(stderr, " no HID preference; auto-loading all HID plugins available...\n");

	for(dir = rnd_pup_paths; *dir != NULL; dir++) {
		void *d = pup_open_dir(*dir);
		const char *fn;

		if (d == NULL) continue;

		while((fn = pup_read_dir(d)) != NULL) {
			if (strncmp(fn, "hid_", 4) == 0) {
				char *s;
				tmp.used = 0;
				gds_append_str(&tmp, fn);
				if (tmp.used < 4)
					continue;

				/* accept *.pup only and truncate .pup from the end */
				s = tmp.array + tmp.used - 4;
				if (strcmp(s, ".pup") != 0)
					continue;
				*s = '\0';
				if (cb(ctx, tmp.array, *dir, fn) != 0)
					break;
			}
		}

		pup_close_dir(d);
	}
	gds_uninit(&tmp);
}

static int load_all_gui_plugins_cb(void *ctx, const char *name, const char *dir, const char *fn)
{
	if (rnd_conf.rc.verbose >= RND_MSG_INFO)
		fprintf(stderr, "-- all-hid-load: '%s' ('%s' from %s)\n", name, fn, dir);
	rnd_hid_load_gui_plugin(name);
	return 0;
}

/* Load all hid_* plugins from all puplug search paths */
static void rnd_hid_load_all_gui_plugins(void)
{
	rnd_hid_do_all_gui_plugins(NULL, load_all_gui_plugins_cb);
}

static int list_gui_cb(void *ctx, const char *name, const char *dir, const char *fn)
{
	gds_t *tmp = ctx;
	char *end, *line, buff[1024], *desc = "<description not available>";
	FILE *f;

	tmp->used = 0;
	gds_append_str(tmp, dir);
	gds_append(tmp, RND_DIR_SEPARATOR_C);
	gds_append_str(tmp, fn);

	f = rnd_fopen(NULL, tmp->array, "r");
	if (f != NULL) {
		while((line = fgets(buff, sizeof(buff), f)) != NULL) {
			while(isspace(*line)) line++;
			if (strncmp(line, "$short", 6) == 0) {
				desc = line + 6;
				while(isspace(*desc)) desc++;
				end = strpbrk(desc, "\r\n");
				if (end != NULL)
					*end = '\0';
				break;
			}
		}
		fclose(f);
	}

	fprintf(stderr, "\t%-16s %s\n", name, desc);
	return 0;
}

void rnd_hid_print_all_gui_plugins(void)
{
	gds_t tmp = {0};
	const pup_buildin_t *b;

	rnd_hid_do_all_gui_plugins(&tmp, list_gui_cb);

	/* print buildins */
	for(b = pup_buildins; b->name != NULL; b++)
		if (strncmp(b->name, "hid_", 4) == 0)
			fprintf(stderr, "\t%-16s\n", b->name);

	gds_uninit(&tmp);
}


/* low level (pup) load a plugin if it is not already loaded; if plugname is
   NULL, try to load all plugins */
static void rnd_hid_load_gui_plugin(const char *plugname)
{
	int st;

	if (rnd_conf.rc.verbose >= RND_MSG_INFO)
		fprintf(stderr, "GUI: want '%s'\n", plugname);
	if (plugname == NULL) {
		/* load all available GUI plugins; with a workaround: lesstif needs to be
		   loaded first because it is sensitive to link order of -lXm and -lXt;
		   if gtk or other X related GUI is loaded before, lesstif yields error */
		rnd_hid_load_gui_plugin("hid_lesstif");
		rnd_hid_load_all_gui_plugins();
	}
	else {
		char tmp[256];

		/* prefix plugin name with hid_ */
		if ((plugname[0] != 'h') || strncmp(plugname, "hid_", 4) != 0) {
			int l = strlen(plugname);
			if (l > sizeof(tmp) - 8) {
				fprintf(stderr, "rnd_hid_load_gui_plugin: plugin name too long: '%s'\n", plugname);
				return;
			}
			strcpy(tmp, "hid_");
			memcpy(tmp+4, plugname, l+1);
			plugname = tmp;
		}

		/* do not load if already loaded */
		if (pup_lookup(&rnd_pup, plugname) != NULL) {
			if (rnd_conf.rc.verbose >= RND_MSG_INFO)
				fprintf(stderr, " already loaded %s\n", plugname);
			return;
		}

		/* load the plugin */
		if (rnd_conf.rc.verbose >= RND_MSG_INFO)
			fprintf(stderr, " loading %s\n", plugname);
		if (pup_load(&rnd_pup, (const char **)rnd_pup_paths, plugname, 0, &st) == NULL) {
			if (rnd_conf.rc.verbose >= RND_MSG_INFO)
				fprintf(stderr, "  failed.\n");
		}
	}
}

rnd_hid_t *rnd_hid_find_gui(rnd_design_t *hidlib, const char *preference)
{
	rnd_hid_t *gui;
	int i;

	if ((preference != NULL) && strncmp(preference, "hid_", 4) == 0)
		preference += 4;

	/* ugly hack for historical reasons: some old configs and veteran users are used to the --gui gtk option */
	if ((preference != NULL) && (strcmp(preference, "gtk") == 0)) {
		gui = rnd_hid_find_gui(hidlib, "gtk2_gl");
		if (gui != NULL)
			return gui;

		gui = rnd_hid_find_gui(hidlib, "gtk2_gdk");
		if (gui != NULL)
			return gui;

		gui = rnd_hid_find_gui(hidlib, "gtk4_gl");
		if (gui != NULL)
			return gui;

		return NULL;
	}

	rnd_hid_load_gui_plugin(preference);

	/* normal search */
	if (preference != NULL) {
		for (i = 0; i < rnd_hid_num_hids; i++) {
			if (!rnd_hid_list[i]->printer && !rnd_hid_list[i]->exporter && !strcmp(rnd_hid_list[i]->name, preference)) {
				gui = rnd_hid_list[i];
				goto found;
			}
		}
		return NULL;
	}

	for (i = 0; i < rnd_hid_num_hids; i++) {
		if (!rnd_hid_list[i]->printer && !rnd_hid_list[i]->exporter && !rnd_hid_list[i]->hide_from_gui) {
			gui = rnd_hid_list[i];
			goto found;
		}
	}

	fprintf(stderr, "Error: No GUI available.\n");
	exit(1);

	found:;
	if (gui->gui) {
		char *fn = NULL;
		int exact_fn = 0;
		rnd_menu_patch_t *mp;

		if ((rnd_conf.rc.menu_file != NULL) && (*rnd_conf.rc.menu_file != '\0')) {
			fn = rnd_strdup_printf(rnd_app.menu_name_fmt, rnd_conf.rc.menu_file);
			exact_fn = (strchr(rnd_conf.rc.menu_file, '/') != NULL);
		}

		mp = rnd_hid_menu_load(gui, hidlib, "librnd", 0, fn, exact_fn, rnd_app.default_embedded_menu, "base menu file");
		free(fn);
		if (mp == NULL) {
			fprintf(stderr, "Failed to load the menu file - can not start a GUI HID.\n");
			exit(1);
		}
	}
	return gui;
}

rnd_hid_t *rnd_hid_find_printer()
{
	int i;

	for (i = 0; i < rnd_hid_num_hids; i++)
		if (rnd_hid_list[i]->printer)
			return rnd_hid_list[i];

	return 0;
}

void rnd_hid_print_exporter_list(FILE *f, const char *prefix, const char *suffix)
{
	int i;
	for (i = 0; i < rnd_hid_num_hids; i++)
		if (rnd_hid_list[i]->exporter)
			fprintf(f, "%s%s%s", prefix, rnd_hid_list[i]->name, suffix);
}

rnd_hid_t *rnd_hid_find_exporter(const char *which)
{
	int i;

	if (which == NULL) {
		fprintf(stderr, "Invalid exporter: need an exporter name, one of:");
		goto list;
	}

	if (strcmp(which, "-list-") == 0) {
		rnd_hid_print_exporter_list(stdout, "", "\n");
		return 0;
	}

	for (i = 0; i < rnd_hid_num_hids; i++)
		if (rnd_hid_list[i]->exporter && strcmp(which, rnd_hid_list[i]->name) == 0)
			return rnd_hid_list[i];

	fprintf(stderr, "Invalid exporter %s, available ones:", which);

	list:;
	rnd_hid_print_exporter_list(stderr, " ", "");
	fprintf(stderr, "\n");

	return 0;
}

rnd_hid_t **rnd_hid_enumerate()
{
	return rnd_hid_list;
}

const char *rnd_hid_export_fn(const char *filename)
{
	if (rnd_conf.rc.export_basename) {
		const char *outfn = strrchr(filename, RND_DIR_SEPARATOR_C);
		if (outfn == NULL)
			return filename;
		return outfn + 1;
	}
	else
		return filename;
}

extern void rnd_hid_dlg_uninit(void);
extern void rnd_hid_dlg_init(void);

static void hidlib_gui_init_ev(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	rnd_tool_gui_init();
	rnd_gui->set_mouse_cursor(rnd_gui, rnd_conf.editor.mode); /* make sure the mouse cursor is set up now that it is registered */
}

static void hidlib_design_set_current_ev(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	rnd_design_t *dsg = argv[1].d.p;

	if ((rnd_gui != NULL) && (rnd_gui->set_design != NULL))
		rnd_gui->set_design(rnd_gui, dsg);
	if ((rnd_render != NULL) && (rnd_render->set_design != NULL) && (rnd_render != rnd_gui))
		rnd_render->set_design(rnd_render, dsg);
}

static void rnd_hid_init_init(void)
{
	rnd_event_bind(RND_EVENT_GUI_INIT, hidlib_gui_init_ev, NULL, hid_init_cookie);
	rnd_event_bind(RND_EVENT_DESIGN_SET_CURRENT_INTR, hidlib_design_set_current_ev, NULL, hid_init_cookie);
}

static void rnd_hid_init_uninit(void)
{
	rnd_event_unbind_allcookie(hid_init_cookie);
}


extern void rnd_menu_init1(void);

static char *rnd_hidlib_init_exec_prefix;
void rnd_hidlib_init1(void (*conf_core_init)(void), const char *exec_prefix)
{
	char **s;

	/* can't conf-set yet, roots are not initialized before conf_load_all() */
	rnd_hidlib_init_exec_prefix = rnd_strdup(exec_prefix);

	s = (char **)&rnd_conf.rc.path.exec_prefix;
	*s = rnd_hidlib_init_exec_prefix;

	rnd_projects_init();
	rnd_events_init();
	rnd_file_loaded_init();
	rnd_conf_init();
	conf_core_init();
	rnd_conf_postproc();
	rnd_hidlib_conf_init();
	rnd_hid_init_init();
	rnd_hid_dlg_init();
	rnd_hid_init();
	rnd_grid_init();
	rnd_color_init();
	rnd_menu_init1();
}

static vts0_t hidlib_conffile;

extern void rnd_hidlib_error_init2(void);
extern void rnd_hid_dlg_init2(void);
extern void rnd_hid_nogui_init2(void);
extern void rnd_conf_act_init2(void);
extern void rnd_safe_fs_act_init2(void);
extern void rnd_hid_act_init2(void);
extern void rnd_tool_act_init2(void);
extern void rnd_main_act_init2(void);
extern void rnd_menu_act_init2(void);
extern void rnd_anyload_init2(void);
extern void rnd_anyload_act_init2(void);
extern void rnd_conf_init2(void);

void rnd_hidlib_init2(const pup_buildin_t *buildins, const pup_buildin_t *local_buildins)
{
	rnd_units_init();
	rnd_actions_init();

	/* load custom config files in the order they were specified */
	if (hidlib_conffile.used > 0) {
		int n;
		for(n = 0; n < hidlib_conffile.used; n++) {
			rnd_conf_role_t role = RND_CFR_CLI;
			char *srole, *sep, *fn = hidlib_conffile.array[n];
			sep = strchr(fn, ';');
			if (sep != NULL) {
				srole = fn;
				*sep = '\0';
				fn = sep+1;
				role = rnd_conf_role_parse(srole);
				if (role == RND_CFR_invalid) {
					fprintf(stderr, "Can't load -C config file '%s': invalid role '%s'\n", fn, srole);
					free(hidlib_conffile.array[n]);
					continue;
				}
			}
			rnd_conf_load_as(role, fn, 0);
			free(hidlib_conffile.array[n]);
		}
		vts0_uninit(&hidlib_conffile);
	}

	rnd_conf_load_all(NULL, NULL);

	rnd_conf_set(RND_CFR_INTERNAL, "rc/path/exec_prefix", -1, rnd_hidlib_init_exec_prefix, RND_POL_OVERWRITE);
	free(rnd_hidlib_init_exec_prefix);

	pup_init(&rnd_pup);
	rnd_pup.error_stack_enable = 1;

	/* core actions */
	rnd_hidlib_error_init2();
	rnd_hid_dlg_init2();
	rnd_hid_nogui_init2();
	rnd_conf_act_init2();
	rnd_safe_fs_act_init2();
	rnd_hid_act_init2();
	rnd_tool_act_init2();
	rnd_main_act_init2();
	rnd_menu_act_init2();
	rnd_conf_init2();

	/* plugins: buildins */
	pup_buildin_load(&rnd_pup, buildins);
	if (local_buildins != NULL)
		pup_buildin_load(&rnd_pup, local_buildins);
	pup_autoload_dir(&rnd_pup, NULL, NULL);

	rnd_conf_load_extra(NULL, NULL);
	rnd_funchash_init();
	rnd_anyload_act_init2();

	rnd_anyload_init2(); /* must be at the end because any init2() may register an anyload */
}

static void print_pup_err(pup_err_stack_t *entry, char *string)
{
	rnd_message(RND_MSG_ERROR, "puplug: %s\n", string);
}

void rnd_hidlib_init3_auto(void)
{
	char **sp;

	if (rnd_pup.err_stack != NULL) {
		rnd_message(RND_MSG_ERROR, "Some of the static linked buildins could not be loaded:\n");
		pup_err_stack_process_str(&rnd_pup, print_pup_err);
	}

	for(sp = rnd_pup_paths; *sp != NULL; sp++) {
		rnd_message(RND_MSG_DEBUG, "Loading plugins from '%s'\n", *sp);
		pup_autoload_dir(&rnd_pup, *sp, (const char **)rnd_pup_paths);
	}

	if (rnd_pup.err_stack != NULL) {
		rnd_message(RND_MSG_ERROR, "Some of the dynamic linked plugins could not be loaded:\n");
		pup_err_stack_process_str(&rnd_pup, print_pup_err);
	}
}


extern void rnd_menu_uninit(void);
extern void rnd_anyload_uninit(void);
extern void rnd_hid_cfg_keys_uninit_module(void);
extern void rnd_tool_act_uninit(void);
extern void rnd_hid_cursor_uninit(void);


void rnd_hidlib_uninit(void)
{
	rnd_hid_cursor_uninit();
	rnd_hid_menu_merge_inhibit_inc(); /* make sure no menu merging happens during the uninitialization */
	rnd_grid_uninit();
	rnd_menu_uninit();
	rnd_hid_init_uninit();
	rnd_hid_dlg_uninit();
	rnd_tool_act_uninit();

	if (rnd_conf_isdirty(RND_CFR_USER))
		rnd_conf_save_file(NULL, NULL, NULL, RND_CFR_USER, NULL);

	rnd_hid_uninit();
	rnd_conf_uninit();
	rnd_plugin_uninit();
	rnd_actions_uninit();
	rnd_dad_unit_uninit();
	rnd_hid_cfg_keys_uninit_module();
	rnd_anyload_uninit();
	rnd_events_uninit();
	rnd_units_uninit();
	rnd_projects_uninit();
}

/* parse arguments using the gui; if fails and fallback is enabled, try the next gui */
int rnd_gui_parse_arguments(int autopick_gui, int *hid_argc, char **hid_argv[])
{
	rnd_conf_listitem_t *apg = NULL;

	if ((autopick_gui >= 0) && (rnd_conf.rc.hid_fallback)) { /* start from the GUI we are initializing first */
		int n;
		const char *g;

		rnd_conf_loop_list_str(&rnd_conf.rc.preferred_gui, apg, g, n) {
			if (n == autopick_gui)
				break;
		}
	}

	for(;;) {
		int res;
		if (rnd_gui->get_export_options != NULL)
			rnd_gui->get_export_options(rnd_gui, NULL, NULL, NULL);
		res = rnd_gui->parse_arguments(rnd_gui, hid_argc, hid_argv);
		if (res == 0)
			break; /* HID accepted, don't try anything else */
		if (res < 0) {
			rnd_message(RND_MSG_ERROR, "Failed to initialize HID %s (unrecoverable, have to give up)\n", rnd_gui->name);
			return -1;
		}
		fprintf(stderr, "Failed to initialize HID %s (recoverable)\n", rnd_gui->name);
		if (apg == NULL) {
			if (rnd_conf.rc.hid_fallback) {
				ran_out_of_hids:;
				rnd_message(RND_MSG_ERROR, "Tried all available HIDs, all failed, giving up.\n");
			}
			else
				rnd_message(RND_MSG_ERROR, "Not trying any other hid as fallback because rc/hid_fallback is disabled.\n");
			return -1;
		}

		/* falling back to the next HID */
		do {
			int n;
			const char *g;

			apg = rnd_conf_list_next_str(apg, &g, &n);
			if (apg == NULL)
				goto ran_out_of_hids;
			rnd_render = rnd_gui = rnd_hid_find_gui(NULL, g);
		} while(rnd_gui == NULL);
	}
	return 0;
}

void rnd_fix_locale_and_env_()
{
	static const char *lcs[] = { "LANG", "LC_NUMERIC", "LC_ALL", NULL };
	const char **lc;

	/* some Xlib calls tend ot hardwire setenv() to "" or NULL so a simple
	   setlocale() won't do the trick on GUI. Also set all related env var
	   to "C" so a setlocale(LC_ALL, "") will also do the right thing. */
	for(lc = lcs; *lc != NULL; lc++)
		rnd_setenv(*lc, "C", 1);

	setlocale(LC_ALL, "C");
}

static int rnd_pcbhl_main_arg_match(const char *in, const char *shrt, const char *lng)
{
	return ((shrt != NULL) && (strcmp(in, shrt) == 0)) || ((lng != NULL) && (strcmp(in, lng) == 0));
}

void rnd_main_args_init(rnd_main_args_t *ga, int argc, const char **action_args)
{
	memset(ga, 0, sizeof(rnd_main_args_t));
	ga->hid_argv_orig = ga->hid_argv = calloc(sizeof(char *), argc*4);
	vtp0_init(&ga->plugin_cli_conf);
	ga->action_args = action_args;
	ga->autopick_gui = -1;
}

void rnd_main_args_uninit(rnd_main_args_t *ga)
{
	vtp0_uninit(&ga->plugin_cli_conf);
	free(ga->hid_argv_orig);
}

int rnd_main_args_add(rnd_main_args_t *ga, char *cmd, char *arg)
{
	const char **cs;
	char *orig_cmd = cmd;

	if (*cmd == '-') {
		cmd++;
		if ((strcmp(cmd, "?") == 0) || (strcmp(cmd, "h") == 0) || (strcmp(cmd, "-help") == 0)) {
			if (arg != NULL) {
			/* memory leak, but who cares for --help? */
				ga->main_action = rnd_strdup_printf("PrintUsage(%s)", arg);
				return 1;
			}
			ga->main_action = "PrintUsage()";
			return 0;
		}

		if ((strcmp(cmd, "g") == 0) || (strcmp(cmd, "-gui") == 0) || (strcmp(cmd, "-hid") == 0)) {
			ga->do_what = RND_DO_GUI;
			ga->hid_name = arg;
			return 1;
		}
		if ((strcmp(cmd, "x") == 0) || (strcmp(cmd, "-export") == 0)) {
			ga->do_what = RND_DO_EXPORT;
			ga->hid_name = arg;
			return 1;
		}
		if ((strcmp(cmd, "p") == 0) || (strcmp(cmd, "-print") == 0)) {
			ga->do_what = RND_DO_PRINT;
			return 0;
		}

		for(cs = ga->action_args; cs[2] != NULL; cs += RND_ACTION_ARGS_WIDTH) {
			if (rnd_pcbhl_main_arg_match(cmd, cs[0], cs[1])) {
				if (ga->main_action == NULL) {
					ga->main_action = cs[2];
					ga->main_action_hint = cs[4];
				}
				else
					fprintf(stderr, "Warning: can't execute multiple command line actions, ignoring %s\n", orig_cmd);
				return 0;
			}
		}
		if (rnd_pcbhl_main_arg_match(cmd, "c", "-conf")) {
			const char *why;
			if (strncmp(arg, "plugins/", 8)  == 0) {
				/* plugins are not yet loaded or initialized so their configs are
				   unavailable. Store these settings until plugins are up. This
				   should not happen to non-plugin config items as those might
				   affect how plugins are searched/loaded. */
				const void **a = (const void **)vtp0_alloc_append(&ga->plugin_cli_conf, 1);
				*a = arg;
			}
			else {
				if (rnd_conf_set_from_cli(NULL, arg, NULL, &why) != 0) {
					fprintf(stderr, "Error: failed to set config %s: %s\n", arg, why);
					exit(1);
				}
			}
			return 1;
		}
		if (rnd_pcbhl_main_arg_match(cmd, "C", "-conffile")) {
			vts0_append(&hidlib_conffile, rnd_strdup(arg));
			return 1;
		}
	}
	/* didn't handle argument, save it for the HID */
	ga->hid_argv[ga->hid_argc++] = orig_cmd;
	return 0;
}

static int apply_plugin_cli_conf(rnd_main_args_t *ga, int relax)
{
	int n;
	for(n = 0; n < vtp0_len(&ga->plugin_cli_conf); n++) {
		const char *why, *arg = ga->plugin_cli_conf.array[n];

		if (ga->plugin_cli_conf.array[n] == NULL) continue;

		if (rnd_conf_set_from_cli(NULL, arg, NULL, &why) != 0) {
			if (!relax) {
				fprintf(stderr, "Error: failed to set config %s: %s\n", arg, why);
				return 1;
			}
		}
		else
			ga->plugin_cli_conf.array[n] = NULL; /* remove if succesfully set so it won't be set again on multiple calls at different stages */
	}
	return 0;
}

int rnd_main_args_setup1(rnd_main_args_t *ga)
{
	/* Now that plugins are already initialized, apply plugin config items;
	   non-existing ones are not fatal as the GUI plugin is not yet loaded */
	apply_plugin_cli_conf(ga, 1);

	/* Export pcb from command line if requested.  */
	switch(ga->do_what) {
		case RND_DO_PRINT:   rnd_render = rnd_exporter = rnd_gui = rnd_hid_find_printer(); break;
		case RND_DO_EXPORT:  rnd_render = rnd_exporter = rnd_gui = rnd_hid_find_exporter(ga->hid_name); break;
		case RND_DO_GUI:
			rnd_render = rnd_gui = rnd_hid_find_gui(NULL, ga->hid_name);
			if (rnd_gui == NULL) {
				rnd_message(RND_MSG_ERROR, "Can't find the gui (%s) requested.\n", ga->hid_name);
				return 1;
			}
			break;
		default: {
			const char *g;
			rnd_conf_listitem_t *i;

			rnd_render = rnd_gui = NULL;
			rnd_conf_loop_list_str(&rnd_conf.rc.preferred_gui, i, g, ga->autopick_gui) {
				rnd_render = rnd_gui = rnd_hid_find_gui(NULL, g);
				if (rnd_gui != NULL)
					break;
			}

			/* try anything */
			if (rnd_gui == NULL) {
				rnd_message(RND_MSG_WARNING, "Warning: can't find any of the preferred GUIs, falling back to anything available...\nYou may want to check if the plugin is loaded, try --dump-plugins and --dump-plugindirs\n");
				rnd_render = rnd_gui = rnd_hid_find_gui(NULL, NULL);
			}
		}
	}

	/* Exit with error if GUI failed to start. */
	if (!rnd_gui)
		return 1;

	/* Now that even the GUI plugin is initialized, apply remaining plugin config
	   items; non-existing ones are fatal */
	if (apply_plugin_cli_conf(ga, 0) != 0)
		return 1;

	vtp0_uninit(&ga->plugin_cli_conf);

	return 0;
}

int rnd_main_args_setup2(rnd_main_args_t *ga, int *exitval)
{
	*exitval = 0;

	/* plugins may have installed their new fields, reinterpret the config
	   (memory lht -> memory bin) to get the new fields */
	rnd_conf_update(NULL, -1);

	if (ga->main_action != NULL) {
		int res = rnd_parse_command(NULL, ga->main_action, rnd_true); /* hidlib is NULL because there is no context yet */
		if ((res != 0) && (ga->main_action_hint != NULL))
			rnd_message(RND_MSG_ERROR, "\nHint: %s\n", ga->main_action_hint);
		rnd_log_print_uninit_errs("main_action parse error");
		*exitval = res;
		return 1;
	}


	if (rnd_gui_parse_arguments(ga->autopick_gui, &ga->hid_argc, &ga->hid_argv) != 0) {
		rnd_log_print_uninit_errs("Export plugin argument parse error");
		return 1;
	}

	return 0;
}

int rnd_main_exported(rnd_main_args_t *ga, rnd_design_t *design, rnd_bool is_empty, void *appspec)
{
	if (!rnd_main_exporting)
		return 0;

	if (is_empty)
		rnd_message(RND_MSG_WARNING, "Exporting empty design (nothing loaded or drawn).\n");
	if (rnd_app.multi_design)
		rnd_multi_switch_to(design);
	else
		rnd_single_switch_to(design);
	rnd_event(design, RND_EVENT_EXPORT_SESSION_BEGIN, NULL);
	rnd_gui->do_export(rnd_gui, design, 0, appspec);
	rnd_event(design, RND_EVENT_EXPORT_SESSION_END, NULL);
	rnd_log_print_uninit_errs("Exporting");
	return 1;
}

void rnd_mainloop_interactive(rnd_main_args_t *ga, rnd_design_t *design)
{
	rnd_hid_in_main_loop = 1;
	rnd_event(design, RND_EVENT_MAINLOOP_CHANGE, "i", rnd_hid_in_main_loop);
	if (rnd_app.multi_design)
		rnd_multi_switch_to(design);
	else
		rnd_single_switch_to(design);
	rnd_gui->do_export(rnd_gui, design, 0, NULL);
	rnd_hid_in_main_loop = 0;
	rnd_event(design, RND_EVENT_MAINLOOP_CHANGE, "i", rnd_hid_in_main_loop);
}

