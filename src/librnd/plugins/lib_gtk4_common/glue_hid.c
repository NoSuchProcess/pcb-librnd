#include "compat.h"

static GtkApplication *app;
#define gtkc_topwin_new() gtk_application_window_new(app)

#include <librnd/plugins/lib_gtk_common/glue_hid.c>


static void activate(GtkApplication* app, gpointer user_data)
{
	rnd_gtkg_main_export_widgets(user_data);
}

static void rnd_gtkg_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	rnd_gtk_t *gctx = hid->hid_data;

	rnd_gtkg_main_export_init(gctx);

	g_signal_connect(app, "activate", G_CALLBACK(rnd_gtk_main_activate), gctx);
	g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	rnd_gtkg_main_export_uninit(gctx, hid);
}

static void rnd_gtkg_do_exit(rnd_hid_t *hid)
{
	rnd_gtk_t *gctx = hid->hid_data;
	rnd_gtkg_do_exit_(gctx);
	gtk_main_quit();
}

int rnd_gtk_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_gtk_t *gctx = hid->hid_data;

	rnd_gtk_parse_arguments_first(gctx, hid, argc, argv);

	app = gtk_application_new("pcb-rnd", G_APPLICATION_FLAGS_NONE);

	/* this can't wait until do_export because of potential change to argc/argv */
	rnd_gtk_topwin_create(gctx, argc, argv);

	return 0;
}
