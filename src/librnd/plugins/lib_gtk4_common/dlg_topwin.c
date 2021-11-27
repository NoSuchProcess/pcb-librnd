#include "compat.h"

void gtkc_create_resize_grip(GtkWidget *parent) {}

static void fix_topbar_theming(void *tw) {}

#if GTK4_BUG_ON_SCROLLBAR_FIXED
static inline GtkWidget *gtkc_vscrollbar_new(GCallback vchg, void *cbdata)
{
	GtkWidget *scb;
	GObject *adj = G_OBJECT(gtk_adjustment_new(0.0, 0.0, 100.0, 10.0, 10.0, 10.0));
	scb = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT(adj));
	g_signal_connect(adj, "value_changed", vchg, cbdata);
	return scb;
}

static inline GtkWidget *gtkc_hscrollbar_new(GCallback vchg, void *cbdata)
{
	GtkWidget *scb;
	GObject *adj = G_OBJECT(gtk_adjustment_new(0.0, 0.0, 100.0, 10.0, 10.0, 10.0));
	scb = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_ADJUSTMENT(adj));
	g_signal_connect(adj, "value_changed", vchg, cbdata);
	return scb;
}

#else

#include "gtkc_scrollbar.h"

static inline GtkWidget *gtkc_vscrollbar_new(GCallback vchg, void *cbdata)
{
	GtkWidget *scb = gtkc_scrollbar_new(GTK_ORIENTATION_VERTICAL);
	g_signal_connect(scb, "value-changed", vchg, cbdata);
	return scb;
}

static inline GtkWidget *gtkc_hscrollbar_new(GCallback vchg, void *cbdata)
{
	GtkWidget *scb = gtkc_scrollbar_new(GTK_ORIENTATION_HORIZONTAL);
	g_signal_connect(scb, "value-changed", vchg, cbdata);
	return scb;
}

#endif

#include <librnd/plugins/lib_gtk_common/dlg_topwin.c>
