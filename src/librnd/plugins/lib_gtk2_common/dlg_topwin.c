#include <gtk/gtk.h>
#include <librnd/plugins/lib_gtk_common/bu_pixbuf.h>
#include "compat.h"
#include <librnd/plugins/lib_gtk_common/dlg_topwin.h>

/* obsolete resize grip */
#include "dlg_topwin_grip.c"

static void get_widget_styles(rnd_gtk_topwin_t *tw, GtkStyle **menu_bar_style)
{
	/* Grab the various styles we need */
	gtk_widget_ensure_style(tw->menu.menu_bar);
	*menu_bar_style = gtk_widget_get_style(tw->menu.menu_bar);
}

static void do_fix_topbar_theming(rnd_gtk_topwin_t *tw)
{
	GtkStyle *menu_bar_style;

	get_widget_styles(tw, &menu_bar_style);

	/* Style the top bar background as if it were all a menu bar */
	gtk_widget_set_style(tw->top_bar_background, menu_bar_style);
}

/* Attempt to produce a conststent style for our extra menu-bar items by
   copying aspects from the menu bar style set by the user's GTK theme.
   Setup signal handlers to update our efforts if the user changes their
   theme whilst we are running. */
static void fix_topbar_theming(rnd_gtk_topwin_t *tw)
{
	do_fix_topbar_theming(tw);

	/* disabled for now: it crashes on some user but there is no easy way
	   to reproduce it; risking a crash is not worth the feature of auto-updating
	   the toolbar without restart.
	   WARNING: this also depends on top_bar_background being an event_box
	   which has changed for th gtk4 port - it's a plain old box now.
	{
		GtkSettings *settings;
		settings = gtk_widget_get_settings(tw->top_bar_background);
		g_signal_connect(settings, "notify::gtk-theme-name", G_CALLBACK(do_fix_topbar_theming), NULL);
		g_signal_connect(settings, "notify::gtk-font-name", G_CALLBACK(do_fix_topbar_theming), NULL);
	}
	*/
}

static inline GtkWidget *gtkc_vscrollbar_new(GCallback vchg, void *cbdata)
{
	GtkWidget *scb;
	GObject *adj = G_OBJECT(gtk_adjustment_new(0.0, 0.0, 100.0, 10.0, 10.0, 10.0));
	scb = gtk_vscrollbar_new(GTK_ADJUSTMENT(adj));
	g_signal_connect(adj, "value_changed", vchg, cbdata);
	return scb;
}

static inline GtkWidget *gtkc_hscrollbar_new(GCallback vchg, void *cbdata)
{
	GtkWidget *scb;
	GObject *adj = G_OBJECT(gtk_adjustment_new(0.0, 0.0, 100.0, 10.0, 10.0, 10.0));
	scb = gtk_hscrollbar_new(GTK_ADJUSTMENT(adj));
	g_signal_connect(adj, "value_changed", vchg, cbdata);
	return scb;
}

#define gtkc_unify_hvscroll(hscb, vscb)

#include <librnd/plugins/lib_gtk_common/dlg_topwin.c>
