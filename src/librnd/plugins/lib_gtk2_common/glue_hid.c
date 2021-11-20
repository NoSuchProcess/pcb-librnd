#include "compat.h"

#define gtkc_topwin_new() gtk_window_new(GTK_WINDOW_TOPLEVEL)
#define gdkc_widget_window_get_origin(wdg, x, y)   gdk_window_get_origin(gtkc_widget_get_window(wdg), x, y)

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


int rnd_gtk_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_gtk_t *gctx = hid->hid_data;

	rnd_gtk_parse_arguments_first(gctx, hid, argc, argv);

	if (!gtk_init_check(argc, argv)) {
		fprintf(stderr, "gtk_init_check() fail - maybe $DISPLAY not set or X/GUI not accessible?\n");
		return 1; /* recoverable error - try another HID */
	}

	/* this can't wait until do_export because of potential change to argc/argv */
	rnd_gtk_topwin_create(gctx, argc, argv);

	return 0;
}

static void rnd_gtkg_beep(rnd_hid_t *hid)
{
	gdk_beep();
}

static void rnd_gtkg_iterate(rnd_hid_t *hid)
{
	while(gtk_events_pending())
		gtk_main_iteration_do(0);
}

static double rnd_gtkg_benchmark(rnd_hid_t *hid)
{
	rnd_gtk_t *gctx = hid->hid_data;
	int i = 0;
	time_t start, end;
	GdkDisplay *display;
	GdkWindow *window;

	window = gtkc_widget_get_window(gctx->port.drawing_area);
	display = gtk_widget_get_display(gctx->port.drawing_area);

	gdk_display_sync(display);
	time(&start);
	do {
		rnd_gui->invalidate_all(rnd_gui);
		gdk_window_process_updates(window, FALSE);
		time(&end);
		i++;
	}
	while (end - start < 10);

	return i/10.0;
}
