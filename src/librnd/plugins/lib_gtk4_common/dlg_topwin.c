#include "compat.h"

void gtkc_create_resize_grip(GtkWidget *parent) {}

static void fix_topbar_theming(void *tw) {}

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

#include <librnd/plugins/lib_gtk_common/dlg_topwin.c>
