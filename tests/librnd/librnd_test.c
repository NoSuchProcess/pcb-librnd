#define RND_APP_PREFIX(x)  rndtest_ ## x

#include <librnd/core/unit.h>
#include <librnd/hid/hid_init.h>
#include <librnd/hid/hid.h>
#include <librnd/hid/buildin.hidlib.h>
#include <librnd/core/conf.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/plugins.h>
#include <librnd/core/hidlib.h>
#include <librnd/core/hidlib_conf.h>
#include <librnd/poly/polyarea.h>

#include "glue.c"

/*** init test ***/

static void poly_test()
{
	rnd_polyarea_t pa;
	rnd_polyarea_init(&pa);
	rnd_poly_valid(&pa);
}

int main(int argc, char *argv[])
{
	int n;
	rnd_main_args_t ga;

	rnd_app.default_embedded_menu = "";
	rnd_app.package = "rnd-test";

	rnd_fix_locale_and_env();

	rnd_plugin_add_dir("include/usr/lib/librnd4/plugins");
	rnd_conf_force_set_bool(rnd_conf.rc.dup_log_to_stderr, 1);

	rnd_main_args_init(&ga, argc, action_args);


	rnd_hidlib_init1(conf_core_init);
	for(n = 1; n < argc; n++)
		n += rnd_main_args_add(&ga, argv[n], argv[n+1]);

	rnd_hidlib_init2(pup_buildins, local_buildins);
	rnd_hidlib_init3_auto();

	rnd_conf_set(RND_CFR_CLI, "editor/view/flip_y", 0, "1", RND_POL_OVERWRITE);

	if ((ga.do_what == RND_DO_SOMETHING) && (ga.hid_name == NULL)) {
		ga.do_what = RND_DO_GUI;
		ga.hid_name = "batch";
	}

	if (rnd_main_args_setup1(&ga) != 0) {
		fprintf(stderr, "setup1 fail\n");
		rnd_main_args_uninit(&ga);
		exit(1);
	}

	if (rnd_main_args_setup2(&ga, &n) != 0) {
		fprintf(stderr, "setup2 fail\n");
		rnd_main_args_uninit(&ga);
		exit(n);
	}

	if (rnd_main_exported(&ga, &CTX.hidlib, 0)) {
		fprintf(stderr, "main_exported fail\n");
		rnd_main_args_uninit(&ga);
		exit(1);
	}

	poly_test();

	rnd_main_args_uninit(&ga);
	exit(0);
}
