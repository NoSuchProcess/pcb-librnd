/* Very linked to the widget ! */
#include "wt_route_style.h"

struct pcb_gtk_dlg_route_style_s {
	pcb_gtk_route_style_t *rss;
	GtkWidget *name_entry;
	GtkWidget *line_entry;
	GtkWidget *via_hole_entry;
	GtkWidget *via_size_entry;
	GtkWidget *clearance_entry;

	GtkWidget *select_box;

	GtkWidget *attr_table;				/* Attributes gtk_tree_view                     */
	GtkListStore *attr_model;			/* Attributes gtk_tree_model                    */

	int inhibit_style_change;			/* when 1, do not do anything when style changes                  */
	int attr_editing;							/* set to 1 when an attribute key or value text is being edited   */
};

/** Builds and runs the "edit route style" dialog.
    \param  rss the route style selector widget linked to this dialog.
 */
void pcb_gtk_route_style_edit_dialog(pcb_gtk_route_style_t * rss);

/* Temporary: hid_gtk call back */
extern void ghid_window_set_name_label(gchar * name);
