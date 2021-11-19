#include "compat.h"
#include <librnd/plugins/lib_gtk_common/glue_hid.c>

static void rnd_gtkg_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	rnd_gtk_t *gctx = hid->hid_data;

	rnd_gtkg_main_export_init(gctx);
	rnd_gtkg_main_export_widgets(gctx);
	gtk_main();
	rnd_gtkg_main_export_uninit(gctx, hid);
}

static void rnd_gtkg_do_exit(rnd_hid_t *hid)
{
	rnd_gtk_t *gctx = hid->hid_data;
	rnd_gtkg_do_exit_(gctx);
	gtk_main_quit();
}
