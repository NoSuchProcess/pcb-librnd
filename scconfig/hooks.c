#include <stdio.h>
#include <string.h>
#include "arg.h"
#include "log.h"
#include "dep.h"
#include "libs.h"
#include "db.h"
#include "tmpasm.h"
#include "tmpasm_scconfig.h"
#include "util/arg_auto_set.h"

#define version "1.0.8"

const char *gui_list[] = {
	"GTK",     "libs/gui/gtk2/presents",
	"lesstif", "libs/gui/lesstif2/presents",
	NULL, NULL
};

#include "plugin_3state.h"

const arg_auto_set_t disable_libs[] = { /* list of --disable-LIBs and the subtree they affect */
	{"disable-gtk",       "libs/gui/gtk2",                arg_lib_nodes, "$do not compile the gtk HID"},
	{"disable-lesstif",   "libs/gui/lesstif2",            arg_lib_nodes, "$do not compile the lesstif HID"},
	{"disable-xrender",   "libs/gui/xrender",             arg_lib_nodes, "$do not use xrender for lesstif"},
	{"disable-xinerama",  "libs/gui/xinerama",            arg_lib_nodes, "$do not use xinerama for lesstif"},
	{"disable-gd",        "libs/gui/gd",                  arg_lib_nodes, "$do not use gd (many exporters need it)"},
	{"disable-gd-gif",    "libs/gui/gd/gdImageGif",       arg_lib_nodes, "$no gif support in the png exporter"},
	{"disable-gd-png",    "libs/gui/gd/gdImagePng",       arg_lib_nodes, "$no png support in the png exporter"},
	{"disable-gd-jpg",    "libs/gui/gd/gdImageJpeg",      arg_lib_nodes, "$no jpeg support in the png exporter"},
	{"disable-gpmi",      "libs/script/gpmi",             arg_lib_nodes, "$do not compile the gpmi (scripting) plugin"},

	{"buildin-gpmi",      "/local/pcb/gpmi/buildin",      arg_true,      "$static link the gpmi plugin into the executable"},
	{"plugin-gpmi",       "/local/pcb/gpmi/buildin",      arg_false,     "$the gpmi plugin is a dynamic loadable"},

#undef plugin_def
#undef plugin_sep
#define plugin_def(name, desc, default_) plugin3_args(name, desc)
#define plugin_sep()
#include "plugins.h"

	{NULL, NULL, NULL, NULL}
};


static void help1(void)
{
	printf("./configure: configure pcn-rnd.\n");
	printf("\n");
	printf("Usage: ./configure [options]\n");
	printf("\n");
	printf("options are:\n");
	printf(" --prefix=path              change installation prefix from /usr to path\n");
}

static void help2(void)
{
	printf("\n");
	printf("Some of the --disable options will make ./configure to skip detection of the given feature and mark them \"not found\".");
	printf("\n");
}

/* Runs when a custom command line argument is found
 returns true if no furhter argument processing should be done */
int hook_custom_arg(const char *key, const char *value)
{
	if (strcmp(key, "prefix") == 0) {
		report("Setting prefix to '%s'\n", value);
		put("/local/prefix", strclone(value));
		return 1;
	}
	else if (strcmp(key, "help") == 0) {
		help1();
		arg_auto_print_options(stdout, " ", "                         ", disable_libs);
		help2();
		exit(0);
	}

	return arg_auto_set(key, value, disable_libs);
}


/* Runs before anything else */
int hook_preinit()
{
	return 0;
}

/* Runs after initialization */
int hook_postinit()
{
	db_mkdir("/local");
	db_mkdir("/local/pcb");

	/* DEFAULTS */
	db_mkdir("/local/pcb/gpmi");
	put("/local/pcb/gpmi/buildin", strue);
	put("/local/prefix", "/usr/local");

#undef plugin_def
#undef plugin_sep
#define plugin_def(name, desc, default_) plugin3_default(name, default_)
#define plugin_sep()
#include "plugins.h"

	return 0;
}

/* Runs after all arguments are read and parsed */
int hook_postarg()
{
	return 0;
}

/* Runs when things should be detected for the host system */
int hook_detect_host()
{
	return 0;
}

char *repeat = NULL;
#define report_repeat(msg) \
do { \
	report(msg); \
	if (repeat != NULL) { \
		char *old = repeat; \
		repeat = str_concat("", old, msg, NULL); \
		free(old); \
	} \
	else \
		repeat = strclone(msg); \
} while(0)

/* Runs when things should be detected for the target system */
int hook_detect_target()
{
	int want_gtk, want_glib = 0;

	want_gtk = (get("libs/gui/gtk2/presents") == NULL) || (istrue(get("libs/gui/gtk2/presents")));

	require("cc/fpic",  0, 1);
	require("fstools/mkdir", 0, 1);
	require("libs/gui/gtk2/presents", 0, 0);
	require("libs/gui/lesstif2/presents", 0, 0);

	if (istrue(get("libs/gui/lesstif2/presents"))) {
		require("libs/gui/xinerama/presents", 0, 0);
		require("libs/gui/xrender/presents", 0, 0);
	}
	else {
		report_repeat("WARNING: Since there's no lesstif2, disabling xinerama and xrender...\n");	
		hook_custom_arg("disable-xinerama", NULL);
		hook_custom_arg("disable-xrender", NULL);
	}

	/* for the exporters */
	require("libs/gui/gd/presents", 0, 0);

	if (!istrue(get("libs/gui/gd/presents"))) {
		report_repeat("WARNING: Since there's no libgd, disabling raster output formats (gif, png, jpg)...\n");
		hook_custom_arg("disable-gd-gif", NULL);
		hook_custom_arg("disable-gd-png", NULL);
		hook_custom_arg("disable-gd-jpg", NULL);
	}

	if (want_gtk)
		want_glib = 1;

	if (plug_is_enabled("toporouter"))
		want_glib = 1;

	if (plug_is_enabled("puller"))
		want_glib = 1;

	if (want_glib) {
		require("libs/sul/glib", 0, 0);
		if (!istrue(get("libs/sul/glib/presents"))) {
			if (want_gtk) {
				report_repeat("WARNING: Since GLIB is not found, disabling the GTK HID...\n");
				hook_custom_arg("disable-gtk", NULL);
			}
			if (plug_is_enabled("toporouter")) {
				report_repeat("WARNING: Since GLIB is not found, disabling the toporouter...\n");
				hook_custom_arg("disable-toporouter", NULL);
			}
			if (plug_is_enabled("puller")) {
				report_repeat("WARNING: Since GLIB is not found, disabling the puller...\n");
				hook_custom_arg("disable-puller", NULL);
			}
		}
	}
	else {
		report("No need for glib, skipping GLIB detection\n");
		put("libs/sul/glib/presents", "false");
		put("libs/sul/glib/cflags", "");
		put("libs/sul/glib/ldflags", "");
	}

	/* generic utils for Makefiles */
	require("fstools/rm",  0, 1);
	require("fstools/ar",  0, 1);
	require("fstools/cp",  0, 1);
	require("fstools/ln",  0, 1);
	require("fstools/mkdir",  0, 1);
	require("sys/ext_exe", 0, 1);
	require("sys/sysid", 0, 1);

	/* options for config.h */
	require("sys/path_sep", 0, 1);
	require("cc/alloca/presents", 0, 0);
	require("cc/rdynamic", 0, 0);
	require("libs/env/putenv/presents", 0, 0);
	require("libs/env/setenv/presents", 0, 0);
	require("libs/snprintf", 0, 0);
	require("libs/vsnprintf", 0, 0);
	require("libs/fs/getcwd", 0, 0);
	require("libs/math/expf", 0, 0);
	require("libs/math/logf", 0, 0);
	require("libs/script/m4/bin/*", 0, 0);
	require("libs/gui/gd/gdImagePng/presents", 0, 0);
	require("libs/gui/gd/gdImageGif/presents", 0, 0);
	require("libs/gui/gd/gdImageJpeg/presents", 0, 0);
	require("libs/fs/stat/macros/*", 0, 0);

	require("libs/script/gpmi/presents", 0, 0);


	if (get("cc/rdynamic") == NULL)
		put("cc/rdynamic", "");


	{
		const char *tmp, *fpic, *debug;
		fpic = get("/target/cc/fpic");
		if (fpic == NULL) fpic = "";
		debug = get("/arg/debug");
		if (debug == NULL) debug = "";
		tmp = str_concat(" ", fpic, debug, NULL);
		put("/local/global_cflags", tmp);
	}

	/* some internal dependencies */
	if (!plug_is_buildin("export_ps")) {
		if (plug_is_enabled("export_lpr")) {
			report_repeat("WARNING: disabling the lpr exporter because the ps exporter is not enabled as a buildin...\n");
			hook_custom_arg("disable-export_lpr", NULL);
		}
	}

	if (!istrue(get("libs/gui/gd/presents"))) {
		if (plug_is_enabled("export_nelma")) {
			report_repeat("WARNING: disabling the nelma exporter because libgd is not found or not configured...\n");
			hook_custom_arg("disable-export_nelma", NULL);
		}
		if (plug_is_enabled("export_png")) {
			report_repeat("WARNING: disabling the png exporter because libgd is not found or not configured...\n");
			hook_custom_arg("disable-export_png", NULL);
		}
	}

	return 0;
}

#ifdef GENCALL
/* If enabled, generator implements ###call *### and generator_callback is
   the callback function that will be executed upon ###call### */
void generator_callback(char *cmd, char *args)
{
	printf("* generator_callback: '%s' '%s'\n", cmd, args);
}
#endif

static void list_presents(const char *prefix, const char *nodes[])
{
	const char **s;
	printf("%s", prefix);
	for(s = nodes; s[0] != NULL; s+=2)
		if (node_istrue(s[1]))
			printf(" %s", s[0]);
	printf("\n");
}

static int gpmi_config(void)
{
	char *tmp;
	const char *gcfg = get("libs/script/gpmi/gpmi-config");
	int generr = 0;

	printf("Generating pcb-gpmi/Makefile.conf (%d)\n", generr |= tmpasm("../src_plugins/gpmi/pcb-gpmi", "Makefile.config.in", "Makefile.config"));


	printf("Configuring gpmi packages...\n");
	tmp = str_concat("", "cd ../src_plugins/gpmi/pcb-gpmi/gpmi_plugin/gpmi_pkg && ", gcfg, " --pkggrp && ./configure", NULL);
	generr |= system(tmp);
	free(tmp);

	printf("Configuring gpmi plugin \"app\"\n");
	tmp = str_concat("", "cd ../src_plugins/gpmi//pcb-gpmi/gpmi_plugin && ", gcfg, " --app", NULL);
	generr |= system(tmp);
	free(tmp);

	printf("Finished configuring gpmi packages\n");

	return generr;
}
static void plugin_stat(const char *header, const char *path)
{
	const char *val = get(path);

	printf("%-32s", header);

	if (val == NULL)
		printf("??? (NULL)\n");
	else if (strcmp(val, sbuildin) == 0)
		printf("yes (buildin)\n");
	else if (strcmp(val, splugin) == 0)
		printf("yes (plugin)\n");
	else
		printf("no\n");
}

/* Runs after detection hooks, should generate the output (Makefiles, etc.) */
int hook_generate()
{
	char *rev = "non-svn", *tmp;
	int manual_config = 0, generr = 0;

	tmp = svn_info(0, "../src", "Revision:");
	if (tmp != NULL) {
		rev = str_concat("", "svn r", tmp, NULL);
		free(tmp);
	}
	put("/local/revision", rev);
	put("/local/version",  version);

	printf("Generating Makefile.conf (%d)\n", generr |= tmpasm("..", "Makefile.conf.in", "Makefile.conf"));

	printf("Generating gts/Makefile (%d)\n", generr |= tmpasm("../src_3rd/gts", "Makefile.in", "Makefile"));
	printf("Generating pcb/Makefile (%d)\n", generr |= tmpasm("../src", "Makefile.in", "Makefile"));

	/* Has to be after pcb/Makefile so that all the modules are loaded. */
	printf("Generating pcb/hidlist  (%d)\n", generr |= tmpasm("../src/hid/common", "hidlist.h.in", "hidlist.h"));
	printf("Generating pcb/buildin  (%d)\n", generr |= tmpasm("../src", "buildin.c.in", "buildin.c"));

	printf("Generating util/Makefile (%d)\n", generr |= tmpasm("../util", "Makefile.in", "Makefile"));

	printf("Generating config.auto.h (%d)\n", generr |= tmpasm("..", "config.auto.h.in", "config.auto.h"));

	if (node_istrue("libs/script/gpmi/presents"))
		gpmi_config();

	if (!exists("../config.manual.h")) {
		printf("Generating config.manual.h (%d)\n", generr |= tmpasm("..", "config.manual.h.in", "config.manual.h"));
		manual_config = 1;
	}

	if (!generr) {
	printf("\n\n");
	printf("=====================\n");
	printf("Configuration summary\n");
	printf("=====================\n\n");
	list_presents("GUI hids: batch", gui_list);

/* special case because the "presents" node */
	printf("%-32s", "Scripting via GPMI: ");
	if (node_istrue("libs/script/gpmi/presents")) {
		printf("yes ");
		if (node_istrue("/local/pcb/gpmi/buildin"))
			printf("(buildin)\n");
		else
			printf("(plugin)\n");
	}
	else
		printf("no\n");

#undef plugin_def
#undef plugin_sep
#define plugin_def(name, desc, default_) plugin3_stat(name, desc)
#define plugin_sep() printf("\n");
#include "plugins.h"

	if (repeat != NULL)
		printf("\n%s\n", repeat);

	if (manual_config)
		printf("\n\n * NOTE: you may want to edit config.manual.h (user preferences) *\n");
	}
	else
		fprintf(stderr, "Error generating some of the files\n");


	return 0;
}

/* Runs before everything is uninitialized */
void hook_preuninit()
{
}

/* Runs at the very end, when everything is already uninitialized */
void hook_postuninit()
{
}

