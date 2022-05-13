#include <stdio.h>
#include <string.h>
#include "arg.h"
#include "log.h"
#include "dep.h"
#include "libs.h"
#include "db.h"
#include "tmpasm.h"
#include "tmpasm_scconfig.h"
#include "generator.h"
#include "util/arg_auto_set.h"
#include "Rev.h"

#define version "3.2.0-dev"

#define LIBRND_ROOT         "../src/librnd"
#define LIBRND_PLUGIN_ROOT  "../src/librnd/plugins"
#define PLUGIN_STAT_FN      "../src/librnd/plugin.state"

#include "../src_3rd/puplug/scconfig_hooks.h"
#include "../src_3rd/libfungw/scconfig_hooks.h"
#include "../src_3rd/libporty_net/hooks_net.c"

/* this turns off some code that is required for applications only */
#define LIBRNDS_SCCONFIG 1

/* we are doing /local/librnd/ */
#define LIBRND_SCCONFIG_APP_TREE "librnd"

#include "librnd/scconfig/plugin_3state.h"
#include "librnd/scconfig/hooks_common.h"
#include "librnd/scconfig/rnd_hook_detect.h"

const arg_auto_set_t disable_libs[] = { /* list of --disable-LIBs and the subtree they affect */
	{"disable-gd",        "libs/gui/gd",                  arg_lib_nodes, "$do not use gd (many exporters need it)"},
	{"disable-gd-gif",    "libs/gui/gd/gdImageGif",       arg_lib_nodes, "$no gif support in the png rnd_exporter"},
	{"disable-gd-png",    "libs/gui/gd/gdImagePng",       arg_lib_nodes, "$no png support in the png rnd_exporter"},
	{"disable-gd-jpg",    "libs/gui/gd/gdImageJpeg",      arg_lib_nodes, "$no jpeg support in the png rnd_exporter"},
	{"disable-xrender",   "libs/gui/xrender",             arg_lib_nodes, "$do not use xrender for lesstif"},
	{"disable-xinerama",  "libs/gui/xinerama",            arg_lib_nodes, "$do not use xinerama for lesstif"},

#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_) plugin3_args(name, desc)
#define plugin_header(sect)
#define plugin_dep(plg, on)
#include "plugins.h"

	{NULL, NULL, NULL, NULL}
};

static void help1(void)
{
	rnd_help1("librnd");
	printf(" --coord=32|64              set coordinate integer type's width in bits\n");
	printf(" --workaround-gtk-ctrl      enable GTK control key query workaround\n");
	printf(" --disable-so               do not compile or install dynamic libs (.so files)\n");
	printf(" --static-librnd            static link librnd (will fail with plugins!)\n");
}

int want_coord_bits;

/* Runs when a custom command line argument is found
 returns true if no further argument processing should be done */
int hook_custom_arg(const char *key, const char *value)
{
	if (strcmp(key, "coord") == 0) {
		int v;
		need_value("use --coord=32|64");
		v = atoi(value);
		if ((v != 32) && (v != 64)) {
			report("ERROR: --coord needs to be 32 or 64.\n");
			exit(1);
		}
		put("/local/pcb/coord_bits", value);
		put("/local/librnd/coord_bits", value);
		want_coord_bits = v;
		return 1;
	}
	if (strncmp(key, "workaround-", 11) == 0) {
		const char *what = key+11;
		if (strcmp(what, "gtk-ctrl") == 0) {
			append("/local/pcb/workaround_defs", "\n#define RND_WORKAROUND_GTK_CTRL 1");
			append("/local/librnd/workaround_defs", "\n#define RND_WORKAROUND_GTK_CTRL 1");
		}
		else if (strcmp(what, "gtk-shift") == 0) {
			append("/local/pcb/workaround_defs", "\n#define RND_WORKAROUND_GTK_SHIFT 1");
			append("/local/librnd/workaround_defs", "\n#define RND_WORKAROUND_GTK_SHIFT 1");
		}
		else {
			report("ERROR: unknown workaround '%s'\n", what);
			exit(1);
		}
		return 1;
	}
	if (strcmp(key, "disable-so") == 0) {
		put("/local/pcb/disable_so", strue);
		put("/local/librnd/disable_so", strue);
		put("/local/pcb/want_static_librnd", strue);
		put("/local/librnd/want_static_librnd", strue);
		pup_set_debug(strue);
		return 1;
	}
	if (strcmp(key, "static-librnd") == 0) {
		put("/local/pcb/want_static_librnd", strue);
		put("/local/librnd/want_static_librnd", strue);
		pup_set_debug(strue);
		return 1;
	}

	rnd_hook_custom_arg(key, value, disable_libs); /* call arg_auto_print_options() instead */

	if (arg_auto_set(key, value, disable_libs) == 0) {
		fprintf(stderr, "Error: unknown argument %s\n", key);
		exit(1);
	}
	return 1; /* handled by arg_auto_set() */
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
	db_mkdir("/local/librnd");

	rnd_hook_postinit();

	put("/local/pcb/coord_bits", "32");
	put("/local/librnd/coord_bits", "32");
	want_coord_bits = 32;

	put("/local/librnd/no_confdir", strue);

	return 0;
}

/* Runs after all arguments are read and parsed */
int hook_postarg()
{
	return rnd_hook_postarg(NULL, "librnd");
}



/* Runs when things should be detected for the host system */
int hook_detect_host()
{
	return rnd_hook_detect_host();
}

static void rnd_hook_detect_hid()
{
	int want_gtk2, has_gtk2 = 0, expl_gtk2, want_gtk4, has_gtk4 = 0, expl_gtk4;
	int need_gtklibs = 0, want_gtk, want_gl, want_glib = 0, expl_gtk2_bi, expl_gtk4_bi;

	want_gtk2   = plug_is_enabled("hid_gtk2_gdk") || plug_is_enabled("hid_gtk2_gl");
	want_gtk4   = plug_is_enabled("hid_gtk4_gl");
	want_gtk    = want_gtk2 | want_gtk4;
	expl_gtk2   = plug_is_explicit("hid_gtk2_gdk") || plug_is_explicit("hid_gtk2_gl");
	expl_gtk4   = plug_is_explicit("hid_gtk4_gl");
	expl_gtk2_bi = plug_is_explicit_buildin("hid_gtk2_gdk") || plug_is_explicit_buildin("hid_gtk2_gl");
	expl_gtk4_bi = plug_is_explicit_buildin("hid_gtk4_gl");

	if (want_gtk2) {
		require("libs/gui/gtk2/presents", 0, 0);
		if (istrue(get("libs/gui/gtk2/presents"))) {
			require("libs/gui/gtk2/key_prefix", 0, 1);
			require("libs/gui/gtk2gl/presents", 0, 0);
			if (!istrue(get("libs/gui/gtk2gl/presents"))) {
				report_repeat("WARNING: Since there's no gl support for gtk2 found, disabling the hid_gtk2_gl rendering... (you may need to install gtkglext1)\n");
				hook_custom_arg("disable-hid_gtk2_gl", NULL);
			}
			need_gtklibs = 1;
			has_gtk2 = 1;
		}
		else {
			report_repeat("WARNING: Since there's no libgtk2 found, disabling hid_gtk2*...\n");
			hook_custom_arg("disable-hid_gtk2_gdk", NULL);
			hook_custom_arg("disable-hid_gtk2_gl", NULL);
			hook_custom_arg("disable-lib_gtk2_common", NULL);
		}
	}

	if (want_gtk4) {
		require("libs/gui/gtk4/presents", 0, 0);
		require("libs/gui/epoxy/presents", 0, 0);
		if (istrue(get("libs/gui/gtk4/presents")) && istrue(get("libs/gui/epoxy/presents"))) {
			need_gtklibs = 1;
			has_gtk4 = 1;
		}
		else {
			report_repeat("WARNING: Since there's no libgtk4 and libepoxy found, disabling hid_gtk4_gl...\n");
			hook_custom_arg("disable-hid_gtk4_gl", NULL);
			hook_custom_arg("disable-lib_gtk4_common", NULL);
		}
	}

	want_gl = plug_is_enabled("hid_gtk2_gl") || plug_is_enabled("hid_gtk4_gl");
	if (want_gl) {
		require("libs/gui/glu/presents", 0, 0);
		if (!istrue(get("libs/gui/glu/presents"))) {
			report_repeat("WARNING: Since there's no GLU found, disabling the hid_gtk*_gl plugin...\n");
			goto disable_gl;
		}
		else {
			put("/local/pcb/has_glu", strue);
			put("/local/librnd/has_glu", strue);
		}
		require("libs/gui/gl/vao/presents", 0, 0);
		require("libs/gui/gl/fb_attachment/presents", 0, 0);
	}
	else {
		disable_gl:;
		hook_custom_arg("disable-lib_hid_gl", NULL);
		hook_custom_arg("disable-hid_gtk2_gl", NULL);
		hook_custom_arg("disable-hid_gtk4_gl", NULL);
	}

	if (has_gtk2 && has_gtk4 && (plug_is_buildin("lib_gtk2_common") || plug_is_buildin("lib_gtk4_common"))) {
		if (expl_gtk2 && expl_gtk4) {
			report_repeat("WARNING: you explicitly requested both gtk2 and gtk4, at least one of them as builtin, which is impossible. Converting them both to plugin.\n");

			if (plug_is_enabled("hid_gtk2_gdk"))
				hook_custom_arg("plugin-hid_gtk2_gdk", NULL);

			if (plug_is_enabled("hid_gtk2_gl"))
				hook_custom_arg("plugin-hid_gtk2_gl", NULL);

			if (plug_is_enabled("hid_gtk4_gl"))
				hook_custom_arg("plugin-hid_gtk4_gl", NULL);

			hook_custom_arg("plugin-lib_gtk2_common", NULL);
			hook_custom_arg("plugin-lib_gtk4_common", NULL);
		}
		else if (!expl_gtk2 && expl_gtk4) {
			if (plug_is_buildin("lib_gtk4_common"))
				report_repeat("Note: you explicitly requested gtk4 as builtin; gtk2 would be available, but if gtk4 is builtin, I have to disable gtk2. If you need both, request both to be plugins.\n");
			else
				report_repeat("Note: you explicitly requested gtk4 as plugin; gtk2 would be available, but can not be a builtin (default preference), so I have to disable gtk2. If you need both, request both to be plugins.\n");
			goto choose_gtk4;
		}
		else if (expl_gtk2 && !expl_gtk4) {
			if (plug_is_buildin("lib_gtk2_common"))
				report_repeat("Note: you explicitly requested gtk2 as builtin; gtk4 would be available, but if gtk2 is builtin, I have to disable gtk4. If you need both, request both gtk4 and gtk2 to be plugins.\n");
			else
				report_repeat("Note: you explicitly requested gtk2 as plugin; gtk4 would be available, but can not be a builtin, so I have to disable gtk4. If you need both, request both to be plugins.\n");
			goto choose_gtk2;
		}
		else if (plug_is_buildin("lib_gtk2_common")) { /* implicit, prefer gtk2 */
			report_repeat("WARNING: you have both gtk2 and gtk4 installed; you can't have any gtk built-in and use the other as well. Since gtk2 is builtin, disabling gtk4.\n");
			choose_gtk2:;
			hook_custom_arg("disable-hid_gtk4_gl", NULL);
			hook_custom_arg("disable-lib_gtk4_common", NULL);
		}
		else if (plug_is_buildin("lib_gtk4_common")) { /* implicit, prefer gtk4 */
			report_repeat("WARNING: you have both gtk2 and gtk4 installed; you can't have any gtk built-in and use the other as well. Since gtk4 is builtin, disabling gtk2.\n");
			choose_gtk4:;
			hook_custom_arg("disable-hid_gtk2_gl", NULL);
			hook_custom_arg("disable-lib_gtk2_common", NULL);
		}
	}

	if (!need_gtklibs) {
		report("No gtk support available, disabling lib_gtk*_common...\n");
		hook_custom_arg("disable-lib_gtk2_common", NULL);
		hook_custom_arg("disable-lib_gtk4_common", NULL);
	}

	if (plug_is_enabled("hid_lesstif")) {
		require("libs/gui/lesstif2/presents", 0, 0);
		require("libs/gui/lesstif2/exthi/presents", 0, 0);
		if (istrue(get("libs/gui/lesstif2/exthi/presents"))) {
			require("libs/gui/xinerama/presents", 0, 0);
			require("libs/gui/xrender/presents", 0, 0);
		}
		else {
			if (plug_is_explicit("hid_lesstif"))
				report_repeat("WARNING: Since there's no usable lesstif2 found, disabling the lesstif HID and xinerama and xrender...\n");
			hook_custom_arg("disable-xinerama", NULL);
			hook_custom_arg("disable-xrender", NULL);
			hook_custom_arg("disable-hid_lesstif", NULL);
		}
	}
	else {
		hook_custom_arg("disable-xinerama", NULL);
		hook_custom_arg("disable-xrender", NULL);
	}

	if (want_gtk)
		want_glib = 1;

	rnd_hook_detect_glib(want_glib);

	if (!istrue(get("libs/sul/glib/presents"))) {
		if (want_gtk) {
			report_repeat("WARNING: Since GLIB is not found, disabling the GTK HID...\n");
			hook_custom_arg("disable-hid_gtk2_gdk", NULL);
			hook_custom_arg("disable-hid_gtk2_gl", NULL);
			hook_custom_arg("disable-hid_gtk4_gl", NULL);
			hook_custom_arg("disable-lib_gtk2_common", NULL);
			hook_custom_arg("disable-lib_gtk4_common", NULL);
		}
	}
}

	/* figure coordinate bits */
static void rnd_hook_detect_coord_bits(void)
{
	int int_bits       = safe_atoi(get("sys/types/size/signed_int")) * 8;
	int long_bits      = safe_atoi(get("sys/types/size/signed_long_int")) * 8;
	int long_long_bits = safe_atoi(get("sys/types/size/signed_long_long_int")) * 8;
	int int64_bits     = safe_atoi(get("sys/types/size/uint64_t")) * 8;
	const char *chosen, *postfix;
	char tmp[64];
	int need_stdint = 0;

	if (want_coord_bits == int_bits)             { postfix="U";   chosen = "int";           }
	else if (want_coord_bits == long_bits)       { postfix="UL";  chosen = "long int";      }
	else if (want_coord_bits == int64_bits)      { postfix="ULL"; chosen = "int64_t";       need_stdint = 1; }
	else if (want_coord_bits == long_long_bits)  { postfix="ULL"; chosen = "long long int"; }
	else {
		report("ERROR: can't find a suitable integer type for coord to be %d bits wide\n", want_coord_bits);
		exit(1);
	}

	sprintf(tmp, "((1%s<<%d)-1)", postfix, want_coord_bits - 1);
	put("/local/pcb/coord_type", chosen);
	put("/local/librnd/coord_type", chosen);
	put("/local/pcb/coord_max", tmp);
	put("/local/librnd/coord_max", tmp);

	chosen = NULL;
	if (istrue(get("/local/librnd/debug"))) { /* debug: c89 */
		if (int64_bits >= 64) {
			/* to suppress warnings on systems that support c99 but are forced to compile in c89 mode */
			chosen = "int64_t";
			need_stdint = 1;
		}
	}

	if (chosen == NULL) { /* non-debug, thus non-c89 */
		if (long_long_bits >= 64) chosen = "long long int";
		else if (long_bits >= 64) chosen = "long int";
		else chosen = "double";
	}
	put("/local/pcb/long64", chosen);
	put("/local/librnd/long64", chosen);
	if (need_stdint) {
		put("/local/pcb/include_stdint", "#include <stdint.h>");
		put("/local/librnd/include_stdint", "#include <stdint.h>");
	}
}


/* if any of these are enabled, we need the "dialog plugin"; "dialog" can not be
   a pup dep because it is not in librnd but in the host app and can have a
   different na,e */
static const char *dialog_deps[] = {
	"/local/pcb/dialogs/controls",         /* so we don't relax user's explicit request */
	"/local/pcb/hid_remote/controls",
	"/local/pcb/lib_gtk2_common/controls",
	"/local/pcb/lib_gtk4_common/controls",
	"/local/pcb/hid_lesstif/controls",
	NULL
};


void rnd_calc_dialog_deps(void)
{
	const char **p;
	int buildin = 0, plugin = 0;

	for(p = dialog_deps; *p != NULL; p++) {
		const char *st = get(*p);
		if (st == NULL) continue;
		if (strcmp(st, "buildin") == 0) {
			buildin = 1;
			break;
		}
		if (strcmp(st, "plugin") == 0)
			plugin = 1;
	}

	put("/target/librnd/dialogs/buildin", buildin ? strue : sfalse);
	put("/target/librnd/dialogs/plugin",  plugin  ? strue : sfalse);
}

/* Runs when things should be detected for the target system */
int hook_detect_target()
{
	int want_gd, want_stroke;

	want_gd     = plug_is_enabled("lib_exp_pixmap") || plug_is_enabled("import_pixmap_gd");
	want_stroke = plug_is_enabled("stroke");

	rnd_hook_detect_cc();
	if (rnd_hook_detect_sys() != 0)
		return 1;

	if (want_stroke) {
		require("libs/gui/libstroke/presents", 0, 0);
		if (!istrue(get("libs/gui/libstroke/presents"))) {
			if (plug_is_explicit("stroke"))
				report_repeat("WARNING: Since there's no libstroke found, disabling the stroke plugin...\n");
			hook_custom_arg("disable-stroke", NULL);
		}
	}

	rnd_hook_detect_hid();
	rnd_calc_dialog_deps();

	if (want_gd) {
		require("libs/gui/gd/presents", 0, 0);
		if (!istrue(get("libs/gui/gd/presents"))) {
			report_repeat("WARNING: Since there's no libgd, disabling gd based exports and pixmap imports (png, gif, jpeg)...\n");
			hook_custom_arg("disable-gd-gif", NULL);
			hook_custom_arg("disable-gd-png", NULL);
			hook_custom_arg("disable-gd-jpg", NULL);
			hook_custom_arg("disable-lib_exp_pixmap", NULL);
			hook_custom_arg("disable-import_pixmap_gd", NULL);
			want_gd = 0;
			goto disable_gd_formats;
		}
		else {
			require("libs/gui/gd/gdImagePng/presents", 0, 0);
			require("libs/gui/gd/gdImageGif/presents", 0, 0);
			require("libs/gui/gd/gdImageJpeg/presents", 0, 0);
			require("libs/gui/gd/gdImageSetResolution/presents", 0, 0);
			if (!istrue(get("libs/gui/gd/gdImagePng/presents"))) {
				report_repeat("WARNING: libgd is installed, but its png code fails, some exporters will be compiled with reduced functionality\n");
			}
		}
	}
	else {
		put("libs/gui/gd/presents", sfalse);
		disable_gd_formats:;
		put("libs/gui/gd/gdImagePng/presents", sfalse);
		put("libs/gui/gd/gdImageGif/presents", sfalse);
		put("libs/gui/gd/gdImageJpeg/presents", sfalse);
	}

	if (libporty_net_detect_target() != 0) {
		hook_custom_arg("disable-lib_portynet", NULL);
		report_repeat("WARNING: porty-net did not configure, some network-related features are disabled\n");
	}

	/* plugin dependencies */
	plugin_deps(1);

	rnd_hook_detect_coord_bits(); /* remove this after the librnd split */

	/* restore the original CFLAGS, without the effects of --debug, so Makefiles can decide when to use what cflag (c99 needs different ones) */
	/* this should be removed after the librnd split */
	put("/target/cc/cflags", get("/local/cc_flags_save"));

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

static int rnd_hook_generate()
{
	int generr = 0;

	printf("Generating librnd.mak (%d)\n", generr |= tmpasm(LIBRND_ROOT "/core", "librnd.mak.in", "librnd.mak"));
	printf("Generating compat_inc.h (%d)\n", generr |= tmpasm(LIBRND_ROOT "/core", "compat_inc.h.in", "compat_inc.h"));
	printf("Generating librnd config.h (%d)\n", generr |= tmpasm(LIBRND_ROOT, "config.h.in", "config.h"));
	printf("Generating opengl.h (%d)\n", generr |= tmpasm(LIBRND_PLUGIN_ROOT "/lib_hid_gl", "opengl.h.in", "opengl.h"));
	printf("Generating draw_INIT.h (%d)\n", generr |= tmpasm(LIBRND_PLUGIN_ROOT "/lib_hid_gl", "draw_INIT.h.in", "draw_INIT.h"));

	return generr;
}

/* Runs after detection hooks, should generate the output (Makefiles, etc.) */
int hook_generate()
{
	char *next, *rev = "non-svn", *curr, *tmp, *sep;
	int generr = 0;
	int res = 0;
	char apiver[32], version_major[32];
	int v1, v2, v3;

	tmp = svn_info(0, "../src", "Revision:");
	if (tmp != NULL) {
		rev = str_concat("", "svn r", tmp, NULL);
		free(tmp);
	}
	strcpy(apiver, version);
	curr = apiver; next = strchr(curr, '.'); *next = '\n';
	v1 = atoi(curr);
	curr = next+1; next = strchr(curr, '.'); *next = '\n';
	v2 = atoi(curr);
	v3 = atoi(next+1);
	sprintf(apiver, "0x%02d%02d%02d", v1, v2, v3);

	strcpy(version_major, version);
	sep = strchr(version_major, '.');
	if (sep != NULL)
		*sep = '\0';

	logprintf(0, "scconfig generate version info: version='%s' rev='%s'\n", version, rev);
	put("/local/revision", rev);
	put("/local/version",  version);
	put("/local/version_major",  version_major);
	put("/local/apiver", apiver);
	put("/local/pup/sccbox", "../../scconfig/sccbox");

	printf("\n");

	printf("Generating Makefile.conf (%d)\n", generr |= tmpasm("..", "Makefile.conf.in", "Makefile.conf"));

	printf("Generating src/Makefile (%d)\n", generr |= tmpasm("../src", "Makefile.in", "Makefile"));

	generr |= rnd_hook_generate();

	printf("Generating src_3rd/libporty_net/os_includes.h (%d)\n", generr |= generate("../src_3rd/libporty_net/os_includes.h.in", "../src_3rd/libporty_net/os_includes.h"));
	printf("Generating src_3rd/libporty_net/pnet_config.h (%d)\n", generr |= generate("../src_3rd/libporty_net/pnet_config.h.in", "../src_3rd/libporty_net/pnet_config.h"));
	printf("Generating src_3rd/libporty_net/phost_types.h (%d)\n", generr |= generate("../src_3rd/libporty_net/phost_types.h.in", "../src_3rd/libporty_net/phost_types.h"));

	printf("Generating pcb-rnd config.h (%d)\n", generr |= tmpasm("..", "config.h.in", "config.h"));

	printf("Generating opengl.h (%d)\n", generr |= tmpasm("../src/librnd/plugins/lib_hid_gl", "opengl.h.in", "opengl.h"));

	printf("Generating tests/librnd/inc_all.h (%d)\n", generr |= tmpasm("../tests/librnd", "inc_all.h.in", "inc_all.h"));

	generr |= pup_hook_generate("../src_3rd/puplug");

	if (!istrue(get("libs/script/fungw/presents")))
		generr |= fungw_hook_generate("../src_3rd/libfungw");


	if (!generr) {
	printf("\n\n");
	printf("=====================\n");
	printf("Configuration summary\n");
	printf("=====================\n");

	print_sum_setting("/local/librnd/debug",          "Compilation for debugging");
	print_sum_setting_or("/local/librnd/symbols",     "Include debug symbols", istrue(get("/local/pcb/debug")));
	print_sum_cfg_val("/local/librnd/coord_bits",     "Coordinate type bits");

#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_) plugin3_stat(name, desc)
#define plugin_header(sect) printf(sect);
#define plugin_dep(plg, on)
#include "plugins.h"

	{
		const char *tmp;
		FILE *f = fopen(PLUGIN_STAT_FN, "w");
		if (f != NULL) {
#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_) fprintf(f, "#state %s %s\n", name, get("/local/pcb/" name "/controls"));
#define plugin_header(sect)
#define plugin_dep(plg, on)
#include "plugins.h"

#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_) fprintf(f, "/local/pcb/%s/controls=%s\n/local/pcb/%s/external=true\n", name, get("/local/pcb/" name "/controls"), name);
#define plugin_header(sect)
#define plugin_dep(plg, on)
#include "plugins.h"

			fprintf(f, "\n# misc settings\n");
/*			fprintf(f, "/local/librnd/fungw/cflags=%s", get("libs/script/fungw/cflags"));*/
			tmp = get("libs/script/fungw/ldflags");
			fprintf(f, "/local/librnd/fungw/ldflags=%s\n", (tmp == NULL ? "" : tmp));
			fclose(f);
		}
		else {
			fprintf(stderr, "failed to save plugin states in %s\n", PLUGIN_STAT_FN);
			exit(1);
		}
	}

	if (repeat != NULL) {
		printf("\n%s\n", repeat);
	}

	if (all_plugin_check_explicit()) {
		printf("\nNevertheless the configuration is complete, if you accept these differences\nyou can go on compiling.\n\n");
		res = 1;
	}
	else
		printf("\nConfiguration complete, ready to compile.\n\n");

	{
		FILE *f;
		f = fopen("Rev.stamp", "w");
		fprintf(f, "%d", myrev);
		fclose(f);
	}

	}
	else {
		fprintf(stderr, "Error generating some of the files\n");
		res = 1;
	}

	return res;
}

/* Runs before everything is uninitialized */
void hook_preuninit()
{
}

/* Runs at the very end, when everything is already uninitialized */
void hook_postuninit()
{
}

