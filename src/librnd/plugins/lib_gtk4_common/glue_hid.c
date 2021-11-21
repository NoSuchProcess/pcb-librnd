#include "compat.h"

#define gtkc_topwin_new() gtk_window_new()
#define gdkc_widget_window_get_origin(wdg, x, y)  gtkc_widget_window_origin(wdg, x, y)

#define gtkc_mod1_in_mask(mask) ((mask & GDK_ALT_MASK) ? TRUE : FALSE)

#include <librnd/plugins/lib_gtk_common/glue_hid.c>

static int rnd_gtkg_gtk4_stay = 1;

static void rnd_gtkg_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	rnd_gtk_t *gctx = hid->hid_data;

	rnd_gtkg_main_export_init(gctx);
	rnd_gtkg_main_export_widgets(gctx);

	while(rnd_gtkg_gtk4_stay)
		g_main_context_iteration(NULL, TRUE);

	rnd_gtkg_main_export_uninit(gctx, hid);
}

static void rnd_gtkg_do_exit(rnd_hid_t *hid)
{
	rnd_gtk_t *gctx = hid->hid_data;
	rnd_gtkg_do_exit_(gctx);
	rnd_gtkg_gtk4_stay = 0;
	g_main_context_wakeup(NULL);
}

int rnd_gtk_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_gtk_t *gctx = hid->hid_data;

	rnd_gtk_parse_arguments_first(gctx, hid, argc, argv);

	if (!gtk_init_check()) {
		fprintf(stderr, "gtk_init_check() fail - maybe $DISPLAY not set or X/GUI not accessible?\n");
		return 1; /* recoverable error - try another HID */
	}

	/* this can't wait until do_export because of potential change to argc/argv */
	rnd_gtk_topwin_create(gctx, argc, argv);

	return 0;
}

static void rnd_gtkg_beep(rnd_hid_t *hid)
{
	rnd_gtk_t *gctx = hid->hid_data;
	GdkSurface *surf;

	if (gctx->port.drawing_area == NULL)
		return;

	surf = gtkc_win_surface(gctx->port.drawing_area);
	gdk_surface_beep(surf);
}

static void rnd_gtkg_iterate(rnd_hid_t *hid)
{
	gtkc_wait_pending_events();
}

static double rnd_gtkg_benchmark(rnd_hid_t *hid)
{
	rnd_gtk_t *gctx = hid->hid_data;
	int i = 0;
	time_t start, end;
	GdkDisplay *display = gtk_widget_get_display(gctx->port.drawing_area);

	gdk_display_sync(display);
	time(&start);
	do {
		rnd_gui->invalidate_all(rnd_gui);
		gtkc_wait_pending_events();
		time(&end);
		i++;
	}
	while (end - start < 10);

	return i/10.0;
}
