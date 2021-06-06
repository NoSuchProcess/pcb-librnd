/* the caller needs to provide these: */
static void help1(void);
const arg_auto_set_t disable_libs[];
int hook_custom_arg(const char *key, const char *value);

/*** implementation ***/
int want_coord_bits;

const arg_auto_set_t rnd_disable_libs[] = { /* list of --disable-LIBs and the subtree they affect */
	{"disable-xrender",   "libs/gui/xrender",             arg_lib_nodes, "$do not use xrender for lesstif"},
	{"disable-xinerama",  "libs/gui/xinerama",            arg_lib_nodes, "$do not use xinerama for lesstif"},
/*
#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_, hidlib_) plugin3_args(name, desc)
#define plugin_header(sect)
#define plugin_dep(plg, on, hidlib)
#include "plugins.h"
*/
	{NULL, NULL, NULL, NULL}
};

static void all_plugin_select(const char *state, int force);

static void rnd_help1(const char *progname)
{
	printf("./configure: configure %s.\n", progname);
	printf("\n");
	printf("Usage: ./configure [options]\n");
	printf("\n");
	printf("options are:\n");
	printf(" --prefix=path              change installation prefix from /usr/local to path\n");
	printf(" --debug                    build full debug version (-g -O0, extra asserts)\n");
	printf(" --profile                  build profiling version if available (-pg)\n");
	printf(" --symbols                  include symbols (add -g, but no -O0 or extra asserts)\n");
	printf(" --man1dir=path             change installation path of man1 files (under prefix)\n");
	printf(" --libarchdir=relpath       relative path under prefix for arch-lib-dir (e.g. lib64)\n");
	printf(" --confdir=path             change installed conf path (normally matches sharedir)\n");
#ifdef LIBRNDS_SCCONFIG
	printf(" --coord=32|64              set coordinate integer type's width in bits\n");
	printf(" --workaround-gtk-ctrl      enable GTK control key query workaround\n");
	printf(" --disable-so               do not compile or install dynamic libs (.so files)\n");
	printf(" --static-librnd            static link librnd (will fail with plugins!)\n");
#endif
	printf(" --all=plugin               enable all working plugins for dynamic load\n");
	printf(" --all=buildin              enable all working plugins for static link\n");
	printf(" --all=disable              disable all plugins (compile core only)\n");
	printf(" --force-all=plugin         enable even broken plugins for dynamic load\n");
	printf(" --force-all=buildin        enable even broken plugins for static link\n");
}

static void help2(void)
{
	printf("\n");
	printf("Some of the --disable options will make ./configure to skip detection of the given feature and mark them \"not found\".");
	printf("\n");
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

#define need_value(msg) \
	if (value == NULL) { \
		fprintf(stderr, "syntax error; %s\n", msg); \
		exit(1); \
	}

static int rnd_hook_custom_arg_(const char *key, const char *value, const arg_auto_set_t *disable_libs)
{
	if (strcmp(key, "prefix") == 0) {
		need_value("use --prefix=dir");
		report("Setting prefix to '%s'\n", value);
		put("/local/prefix", strclone(value));
		return 1;
	}
	if (strcmp(key, "debug") == 0) {
		put("/local/pcb/debug", strue);
		pup_set_debug(strue);
		return 1;
	}
	if (strcmp(key, "profile") == 0) {
		put("/local/pcb/profile", strue);
		return 1;
	}
	if (strcmp(key, "symbols") == 0) {
		put("/local/pcb/symbols", strue);
		return 1;
	}
	if ((strcmp(key, "all") == 0) || (strcmp(key, "force-all") == 0)) {
		need_value("use --all=buildin|plugin|disable\n");
		if ((strcmp(value, sbuildin) == 0) || (strcmp(value, splugin) == 0) || (strcmp(value, sdisable) == 0)) {
			all_plugin_select(value, (key[0] == 'f'));
			return 1;
		}
		report("Error: unknown --all argument: %s\n", value);
		exit(1);
	}
	if (strcmp(key, "man1dir") == 0) {
		need_value("use --man1dir=dir");
		put("/local/man1dir", value);
		return 1;
	}
	if (strcmp(key, "libarchdir") == 0) {
		need_value("use --libarchdir=dir");
		put("/local/libarchdir", value);
		return 1;
	}
	if (strcmp(key, "confdir") == 0) {
		need_value("use --confdir=dir");
		put("/local/confdir", value);
		return 1;
	}
	if (strcmp(key, "help") == 0) {
		help1();
		printf("\nplugin control:\n");
		arg_auto_print_options(stdout, " ", "                         ", rnd_disable_libs);
		if (disable_libs != NULL)
			arg_auto_print_options(stdout, " ", "                         ", disable_libs);
		help2();
		printf("\n");
		help_default_args(stdout, "");
		exit(0);
	}
	if ((strcmp(key, "with-intl") == 0) || (strcmp(key, "enable-intl") == 0)) {
		report("ERROR: --with-intl is no longer supported, please do not use it\n");
		return 1;
	}
#ifdef LIBRNDS_SCCONFIG
	if (strncmp(key, "workaround-", 11) == 0) {
		const char *what = key+11;
		if (strcmp(what, "gtk-ctrl") == 0) append("/local/pcb/workaround_defs", "\n#define RND_WORKAROUND_GTK_CTRL 1");
		else if (strcmp(what, "gtk-shift") == 0) append("/local/pcb/workaround_defs", "\n#define RND_WORKAROUND_GTK_SHIFT 1");
		else {
			report("ERROR: unknown workaround '%s'\n", what);
			exit(1);
		}
		return 1;
	}
	if (strcmp(key, "coord") == 0) {
		int v;
		need_value("use --coord=32|64");
		v = atoi(value);
		if ((v != 32) && (v != 64)) {
			report("ERROR: --coord needs to be 32 or 64.\n");
			exit(1);
		}
		put("/local/pcb/coord_bits", value);
		want_coord_bits = v;
		return 1;
	}
	if (strcmp(key, "disable-so") == 0) {
		put("/local/pcb/disable_so", strue);
		put("/local/pcb/want_static_librnd", strue);
		pup_set_debug(strue);
		return 1;
	}
	if (strcmp(key, "static-librnd") == 0) {
		put("/local/pcb/want_static_librnd", strue);
		pup_set_debug(strue);
		return 1;
	}
#endif
	if (arg_auto_set(key, value, rnd_disable_libs) > 0)
		return 1;

	return 0;
}

#define rnd_hook_custom_arg(key, value, disable_libs) \
do { \
	if (rnd_hook_custom_arg_(key, value, disable_libs)) \
		return 1; \
} while(0)

/* execute plugin dependency statements, depending on "require":
	require = 0 - attempt to mark any dep as buildin
	require = 1 - check if dependencies are met, disable plugins that have
	              unmet deps
*/
int plugin_dep1(int require, const char *plugin, const char *deps_on, int hidlib)
{
	char buff[1024];
	const char *st_plugin, *st_deps_on, *ext_deps_on;
	int dep_chg = 0;

	sprintf(buff, "/local/pcb/%s/hidlib", plugin);
	if (!hidlib) { /* may be inherited */
		hidlib = get(buff) != NULL;
	}

	if (hidlib) {
		put(buff, strue);
		sprintf(buff, "/local/pcb/%s/hidlib", deps_on);
		put(buff, strue);
	}

	sprintf(buff, "/local/pcb/%s/controls", plugin);
	st_plugin = get(buff);
	sprintf(buff, "/local/pcb/%s/external", deps_on);
	ext_deps_on = get(buff);
	sprintf(buff, "/local/pcb/%s/controls", deps_on);
	st_deps_on = get(buff);

	/* do not change buff here, code below depends on it being set on deps_on/controls */

	if (require) {
		if ((strcmp(st_plugin, sbuildin) == 0)) {
			if (strcmp(st_deps_on, sbuildin) != 0) {
				sprintf(buff, "WARNING: disabling (ex-buildin) %s because the %s is not enabled as a buildin...\n", plugin, deps_on);
				report_repeat(buff);
				sprintf(buff, "disable-%s", plugin);
				hook_custom_arg(buff, NULL);
				dep_chg++;
			}
		}
		else if ((strcmp(st_plugin, splugin) == 0)) {
			if ((strcmp(st_deps_on, sbuildin) != 0) && (strcmp(st_deps_on, splugin) != 0)) {
				sprintf(buff, "WARNING: disabling (ex-plugin) %s because the %s is not enabled as a buildin or plugin...\n", plugin, deps_on);
				report_repeat(buff);
				sprintf(buff, "disable-%s", plugin);
				hook_custom_arg(buff, NULL);
				dep_chg++;
			}
		}
	}
	else {
		if (!(istrue(ext_deps_on))) { /* only local plugin states can change */
			if (strcmp(st_plugin, sbuildin) == 0)
				put(buff, sbuildin);
			else if (strcmp(st_plugin, splugin) == 0) {
				if ((st_deps_on == NULL) || (strcmp(st_deps_on, "disable") == 0))
					put(buff, splugin);
			}
			dep_chg++;
		}
	}
	return dep_chg;
}

static void all_plugin_select(const char *state, int force)
{
	char buff[1024];

#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_, hidlib_) \
	if ((all_) || force) { \
		sprintf(buff, "/local/pcb/%s/controls", name); \
		put(buff, state); \
	}
#define plugin_header(sect)
#define plugin_dep(plg, on, hidlib)
#include "plugins.h"
}

/* set up /hidlib nodes in the db to indicate which plugins are in the hidlib */
static void plugin_db_hidlib(void)
{
	char buff[1024];

#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_, hidlib_) \
	if (hidlib_) { \
		sprintf(buff, "/local/pcb/%s/hidlib", name); \
		put(buff, strue); \
	}
#define plugin_header(sect)
#define plugin_dep(plg, on, hidlib)
#include "plugins.h"
}

#ifndef LIBRNDS_SCCONFIG
/* external plugins should force local plugins that depend on them; e.g.
   in pcb-rnd fp_wget plugin depends on librnd's lib_wget, so if lib_wget
   is a plugin, fp_wget can not be a builtin */
int plugin_dep_ext(int require, const char *plugin, const char *deps_on, int hidlib)
{
	char buff[1024];
	const char *st_plugin, *st_deps_on;
	int dep_chg = 0, is_external, is_ext_forced, is_explicit;

	if (require != 0)
		return;

	sprintf(buff, "/local/pcb/%s/external", deps_on);
	is_external = istrue(get(buff));

	sprintf(buff, "/local/pcb/%s/externally_forced", deps_on);
	is_ext_forced = istrue(get(buff));
	if (!is_external && !is_ext_forced)
		return;

	sprintf(buff, "/local/pcb/%s/controls", plugin);
	st_plugin = get(buff);
	sprintf(buff, "/local/pcb/%s/controls", deps_on);
	st_deps_on = get(buff);

	if ((strcmp(st_deps_on, "disable") == 0) && (strcmp(st_plugin, "disable") != 0)) {
		sprintf(buff, "/local/pcb/%s/explicit", plugin);
		is_explicit = get(buff);

		if (is_explicit) {
			if (is_external)
				sprintf(buff, "WARNING: disabling %s because external %s is disabled (check your librnd configuration)...\n", plugin, deps_on);
			else
				sprintf(buff, "WARNING: disabling %s because %s is disabled by external plugin constraints...\n", plugin, deps_on);
			report_repeat(buff);
		}
		sprintf(buff, "disable-%s", plugin);
		hook_custom_arg(buff, NULL);
		dep_chg++;
	}

	if ((strcmp(st_deps_on, "plugin") == 0) && (strcmp(st_plugin, "buildin") == 0)) {
		sprintf(buff, "/local/pcb/%s/explicit", plugin);
		is_explicit = get(buff);

		if (is_explicit) {
			if (is_external)
				sprintf(buff, "WARNING: %s is made plugin (from buildin) because external %s is a plugin (check your librnd configuration)...\n", plugin, deps_on);
			else
				sprintf(buff, "WARNING: %s is made plugin (from buildin) because %s is a plugin (because of external plugin constraints)...\n", plugin, deps_on);
			report_repeat(buff);
		}
		sprintf(buff, "plugin-%s", plugin);
		hook_custom_arg(buff, NULL);
		dep_chg++;
	}

	/* if we had to change a plugin because of an external, mark it, so that
	   this choice won't be overridden */
	if (dep_chg) {
		sprintf(buff, "/local/pcb/%s/externally_forced", plugin);
		put(buff, strue);
	}

	return dep_chg;
}
#endif

int plugin_deps(int require)
{
	int dep_chg = 0;

#ifndef LIBRNDS_SCCONFIG
#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_, hidlib_)
#define plugin_header(sect)
#define plugin_dep(plg, on, hidlib) dep_chg += plugin_dep_ext(require, plg, on, hidlib);
#include "plugins.h"
#endif

#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_, hidlib_)
#define plugin_header(sect)
#define plugin_dep(plg, on, hidlib) dep_chg += plugin_dep1(require, plg, on, hidlib);
#include "plugins.h"
	return dep_chg;
}


void rnd_hook_postinit()
{
	pup_hook_postinit();
	fungw_hook_postinit();

	/* DEFAULTS */
	put("/local/prefix", "/usr/local");
	put("/local/man1dir", "/share/man/man1");
	put("/local/libarchdir", "lib");
	put("/local/confdir", "");
	put("/local/pcb/debug", sfalse);
	put("/local/pcb/profile", sfalse);
	put("/local/pcb/symbols", sfalse);
	put("/local/pcb/disable_so", sfalse);
	put("/local/pcb/want_static_librnd", sfalse);

#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_, hidlib_) plugin3_default(name, default_)
#define plugin_header(sect)
#define plugin_dep(plg, on, hidlib)
#include "plugins.h"

	put("/local/pcb/coord_bits", "32");
	want_coord_bits = 32;
}


static int all_plugin_check_explicit(void)
{
	char pwanted[1024], pgot[1024];
	const char *wanted, *got;
	int tainted = 0;

#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_, hidlib_) \
	sprintf(pwanted, "/local/pcb/%s/explicit", name); \
	wanted = get(pwanted); \
	if (wanted != NULL) { \
		sprintf(pgot, "/local/pcb/%s/controls", name); \
		got = get(pgot); \
		if (strcmp(got, wanted) != 0) {\
			report("ERROR: %s was requested to be %s but I had to %s it\n", name, wanted, got); \
			tainted = 1; \
		} \
	}
#define plugin_header(sect)
#define plugin_dep(plg, on, hidlib)
#include "plugins.h"
	return tainted;
}


/* Runs after all arguments are read and parsed */
int rnd_hook_postarg()
{
	int limit = 128;

	/* repeat as long as there are changes - this makes it "recursive" on
	   resolving deps */
	while(plugin_deps(0) && (limit > 0)) limit--;

	return 0;
}

int safe_atoi(const char *s)
{
	if (s == NULL)
		return 0;
	return atoi(s);
}

static void plugin_stat(const char *header, const char *path, const char *name)
{
	const char *val = get(path);

	if (*header == '#') /* don't print hidden plugins */
		return;

	printf(" %-32s", header);

	if (val == NULL)
		printf("??? (NULL)   ");
	else if (strcmp(val, sbuildin) == 0)
		printf("yes, buildin ");
	else if (strcmp(val, splugin) == 0)
		printf("yes, PLUGIN  ");
	else
		printf("no           ");

	printf("   [%s]\n", name);
}

static void print_sum_setting_or(const char *node, const char *desc, int or)
{
	const char *res, *state;
	state = get(node);
	if (or)
		res = "enabled (implicit)";
	else if (istrue(state))
		res = "enabled";
	else if (isfalse(state))
		res = "disabled";
	else
		res = "UNKNOWN - disabled?";
	printf("%-55s %s\n", desc, res);
}

static void print_sum_setting(const char *node, const char *desc)
{
	print_sum_setting_or(node, desc, 0);
}

static void print_sum_cfg_val(const char *node, const char *desc)
{
	const char *state = get(node);
	printf("%-55s %s\n", desc, state);
}

