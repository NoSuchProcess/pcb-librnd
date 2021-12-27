static void tree_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, rnd_hid_attribute_t *attr, int call_changed);

/* Return the x coord of the first non-arrow pixel or -1 if there's no arrow */
static int rnd_gtk_tree_table_arrow_width(GtkTreeView *tv, GtkTreePath *tp)
{
	GdkRectangle rect;
	GtkTreeViewColumn *col = gtk_tree_view_get_expander_column(tv);

	if (col == NULL)
		return -1;
	gtk_tree_view_get_cell_area(tv, tp, col, &rect);

	return rect.x;
}

/* Handle the double-click to be equivalent to "row-activated" signal */
static gboolean rnd_gtk_tree_table_button_press_cb(GtkWidget *widget, GdkEvent *ev, rnd_hid_attribute_t *attr)
{
	GtkTreeView *tv = GTK_TREE_VIEW(widget);
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;

	/*if (ev->button.type == GDK_2BUTTON_PRESS)*/ {
		model = gtk_tree_view_get_model(tv);
		gtk_tree_view_get_path_at_pos(tv, ev->button.x, ev->button.y, &path, NULL, NULL, NULL);
		if (path != NULL) {
			int minx = rnd_gtk_tree_table_arrow_width(tv, path);
			if (ev->button.x < minx)
				return FALSE; /* do not activate whenclicked on the arrow (interferes with gtk auto expand) */

			gtk_tree_model_get_iter(model, &iter, path);
			tree_row_activated(tv, path, NULL, attr, 0);
		}
	}

	return FALSE;
}

static gboolean rnd_gtk_tree_table_button_release_cb(GtkWidget *widget, GdkEvent *ev, rnd_hid_attribute_t *attr)
{
	GtkTreeView *tv = GTK_TREE_VIEW(widget);
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;

	model = gtk_tree_view_get_model(tv);
	gtk_tree_view_get_path_at_pos(tv, ev->button.x, ev->button.y, &path, NULL, NULL, NULL);
	if (path != NULL) {
		int minx = rnd_gtk_tree_table_arrow_width(tv, path);
		if (ev->button.x < minx)
			return FALSE; /* do not activate whenclicked on the arrow (interferes with gtk auto expand) */

		gtk_tree_model_get_iter(model, &iter, path);
		/* Do not activate the row if LEFT-click on a "parent category" row. */
		if (ev->button.button != 1 || !gtk_tree_model_iter_has_child(model, &iter))
			tree_row_activated(tv, path, NULL, attr, 0);
	}

	return FALSE;
}

static void gtkc_tree_workarounds(GtkWidget *view, rnd_hid_attribute_t *attr, rnd_hid_tree_t *tree)
{
	g_object_set(view, "rules-hint", TRUE, "headers-visible", (tree->hdr != NULL), NULL);
	g_signal_connect(G_OBJECT(view), "button-press-event", G_CALLBACK(rnd_gtk_tree_table_button_press_cb), attr);
	g_signal_connect(G_OBJECT(view), "button-release-event", G_CALLBACK(rnd_gtk_tree_table_button_release_cb), attr);
}


#include <librnd/plugins/lib_gtk_common/dlg_attr_tree.c>
