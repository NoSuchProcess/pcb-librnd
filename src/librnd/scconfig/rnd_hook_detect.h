#ifndef LIBRND_SCCONFIG_APP_TREE
/* for compatibility */
/* TODO:pcb1 - make this an #error in librnd4 */
#define APP "pcb"
#else
#define APP LIBRND_SCCONFIG_APP_TREE
#endif

static void rnd_hook_detect_cc(void)
{
	int need_inl = 0;
	const char *host_ansi, *host_ped, *target_ansi, *target_ped, *target_pg, *target_no_pie;
	const char *tmp, *fpic, *debug;

	require("cc/fpic",  0, 0);
	host_ansi = get("/host/cc/argstd/ansi");
	host_ped = get("/host/cc/argstd/pedantic");
	target_ansi = get("/target/cc/argstd/ansi");
	target_ped = get("/target/cc/argstd/pedantic");
	target_pg = get("/target/cc/argstd/pg");
	target_no_pie = get("/target/cc/argstd/no-pie");
	require("cc/pragma_message",  0, 0);
	require("fstools/ar",  0, 1);
	require("fstools/ranlib",  0, 0);

	/* need to set debug flags here to make sure libs are detected with the modified cflags; -ansi matters in what #defines we need for some #includes */
	fpic = get("/target/cc/fpic");
	if (fpic == NULL) fpic = "";
	debug = get("/arg/debug");
	if (debug == NULL) debug = "";
	tmp = str_concat(" ", fpic, debug, NULL);
	put("/local/global_cflags", tmp);

	/* for --debug mode, use -ansi -pedantic for all detection */
	put("/local/cc_flags_save", get("/target/cc/cflags"));
	if (istrue(get("/local/" APP "/debug"))) {
		if (target_ansi != NULL) {
			append("/target/cc/cflags", " ");
			append("/target/cc/cflags", target_ansi);
		}
		if (target_ped != NULL) {
			append("/target/cc/cflags", " ");
			append("/target/cc/cflags", target_ped);
		}
	}
	if (istrue(get("/local/" APP "/profile"))) {
		append("/target/cc/cflags", " ");
		append("/target/cc/cflags", target_pg);
		append("/target/cc/cflags", " ");
		append("/target/cc/cflags", target_no_pie);
	}

	/* set cflags for C89 */
	put("/local/" APP "/c89flags", "");
	if (istrue(get("/local/" APP "/debug"))) {

		if ((target_ansi != NULL) && (*target_ansi != '\0')) {
			append("/local/" APP "/c89flags", " ");
			append("/local/" APP "/c89flags", target_ansi);
			need_inl = 1;
		}
		if ((target_ped != NULL) && (*target_ped != '\0')) {
			append("/local/" APP "/c89flags", " ");
			append("/local/" APP "/c89flags", target_ped);
			need_inl = 1;
		}
	}

	if (istrue(get("/local/" APP "/profile"))) {
		append("/local/" APP "/cflags_profile", " ");
		append("/local/" APP "/cflags_profile", target_pg);
		append("/local/" APP "/cflags_profile", " ");
		append("/local/" APP "/cflags_profile", target_no_pie);
	}

	if (!istrue(get("cc/inline")))
		need_inl = 1;

	if (need_inl) {
		/* disable inline for C89 */
		append("/local/" APP "/c89flags", " ");
		append("/local/" APP "/c89flags", "-Dinline= ");
	}
}

static int rnd_hook_detect_sys(void)
{
	if (istrue(get("/local/" APP "/disable_so")))
		put("/local/pup/disable_dynlib", strue);

	pup_hook_detect_target();

	require("signal/names/*",  0, 0);
	require("libs/env/setenv/*",  0, 0);
	if (!istrue(get("libs/env/setenv/presents")))
		require("libs/env/putenv/*",  0, 0);
	require("libs/fs/mkdtemp/*",  0, 0);
	require("libs/fs/realpath/*",  0, 0);
	require("libs/fs/readdir/*",  0, 1);
	require("libs/io/fileno/*",  0, 1);
	require("libs/io/popen/*",  0, 1);
	require("libs/math/rint/*",  0, 0);
	require("libs/math/round/*",  0, 0);
	require("libs/userpass/getpwuid/*",  0, 0);
	require("libs/script/fungw/*",  0, 0);

	if (istrue(get("libs/script/fungw/presents"))) {
		require("libs/script/fungw/user_call_ctx/*",  0, 0);
		require("libs/script/fungw/cfg_pupdir/*",  0, 0);
		if (!istrue(get("libs/script/fungw/user_call_ctx/presents")) || !istrue(get("libs/script/fungw/cfg_pupdir/presents"))) {
			put("libs/script/fungw/presents", sfalse);
			report_repeat("\nWARNING: system installed fungw is too old, can not use it, please install a newer version (falling back to minimal fungw shipped with pcb-rnd).\n\n");
		}
	}

	if (!istrue(get("libs/script/fungw/presents")))
		fungw_hook_detect_target();

	{
		int miss_select = require("libs/socket/select/*",  0, 0);
		if (require("libs/time/usleep/*",  0, 0) && require("libs/time/Sleep/*",  0, 0) && miss_select) {
			report_repeat("\nERROR: can not find usleep() or Sleep() or select() - no idea how to sleep ms.\n\n");
			return 1;
		}
	}

	require("libs/time/gettimeofday/*",  0, 1);

	if (istrue(get("/local/" APP "/disable_so"))) {
		if (require("libs/ldl",  0, 0) != 0) {
			if (require("libs/LoadLibrary",  0, 0) != 0)
				report_repeat("\nWARNING: no dynamic linking found on your system. Dynamic plugin loading is disabled.\n\n");
		}
	}

	if (require("libs/proc/wait",  0, 0) != 0) {
		if (require("libs/proc/_spawnvp",  0, 0) != 0) {
			report_repeat("\nERROR: no fork or _spawnvp. Can not compile pcb-rnd.\n\n");
			return 1;
		}
	}

	if (require("libs/fs/_mkdir",  0, 0) != 0) {
		if (require("libs/fs/mkdir",  0, 0) != 0) {
			report_repeat("\nERROR: no mkdir() or _mkdir(). Can not compile pcb-rnd.\n\n");
			return 1;
		}
	}

	if (require("libs/fs/getcwd/*",  0, 0) != 0) {
		if (require("libs/fs/_getcwd/*",  0, 0) != 0) {
			if (require("libs/fs/getwd/*",  0, 0) != 0) {
				report_repeat("\nERROR: Can not find any getcwd() variant.\n\n");
				return 1;
			}
		}
	}

	if (!istrue(get("libs/script/fungw/presents"))) {
		if (plug_is_enabled("script"))
			report_repeat("WARNING: Since there's no suitable system-installed fungw, only limited scripting is available using libfawk - if you need more scripting languages, install fungw and reconfigure librnd.\n");
		put("/local/librnd/fungw_system", sfalse); /* don't use APP here, only librnd should detect fungw */
		put("/local/pcb/fungw_system", sfalse); /* compatibility */
	}
	else {
		put("/local/librnd/fungw_system", strue); /* don't use APP here, only librnd should detect fungw */
		put("/local/pcb/fungw_system", strue); /* compatibility */
	}

	/* generic utils for Makefiles */
	require("sys/ext_exe", 0, 1);
	require("sys/sysid", 0, 1);

	/* options for config.h */
	require("sys/path_sep", 0, 1);
	require("sys/types/size/*", 0, 1);
	require("cc/rdynamic", 0, 0);
	require("cc/soname", 0, 0);
	require("cc/so_undefined", 0, 0);
	require("libs/snprintf", 0, 0);
	require("libs/vsnprintf", 0, 0);
	require("libs/fs/getcwd", 0, 0);
	require("libs/fs/stat/macros/*", 0, 0);

	/* libporty net detections */
		require("sys/ptrwidth", 1, 1);
		require("sys/types/size_t/broken", 1, 0);
		require("sys/types/off_t/broken", 1, 0);
		require("sys/types/ptrdiff_t/broken", 1, 0);
		require("libs/socket/ioctl/presents", 1, 0);
		require("libs/socket/ioctl/fionbio/presents", 1, 1); /* fatal because there is no alternative at the moment */


	if (get("cc/rdynamic") == NULL)
		put("cc/rdynamic", "");

	return 0;
}

static void rnd_hook_detect_glib(int want_glib)
{
	if (want_glib) {
		require("libs/sul/glib", 0, 0);
	}
	else {
		report("No need for glib, skipping GLIB detection\n");
		put("libs/sul/glib/presents", "false");
		put("libs/sul/glib/cflags", "");
		put("libs/sul/glib/ldflags", "");
	}

	if (!istrue(get("libs/sul/glib/presents"))) {
		/* Makefile templates will still reference these variables, they should be empty */
		put("libs/sul/glib/cflags", "");
		put("libs/sul/glib/ldflags", "");
	}
}

static int rnd_hook_detect_host(void)
{
	pup_hook_detect_host();
	fungw_hook_detect_host();

	require("fstools/ar",  0, 1);
	require("fstools/mkdir", 0, 1);
	require("fstools/rm",  0, 1);
	require("fstools/cp",  0, 1);
	require("fstools/ln",  0, 1);

/* until we rewrite the generators in C */
	require("fstools/awk",  0, 1);

	require("cc/argstd/*", 0, 0);

	require("cc/func_attr/unused/*", 0, 0);
	require("cc/inline", 0, 0);

	return 0;
}


#undef APP
