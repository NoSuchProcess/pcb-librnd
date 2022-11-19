#include "compat.h"

#include <librnd/hid/hid_dad.h>

#define DLG_INCLUDES "librnd/plugins/lib_gtk2_common/dlg_includes.c"

static inline GtkWidget *gtkc_dad_label_new(rnd_hid_attribute_t *attr)
{
	GtkWidget *widget;
	GtkRequisition req;
	int theight;

	if (attr->rnd_hatt_flags & RND_HATF_TEXT_TRUNCATED) {
		/* workaround: need to get the real height - create a temporary, non-truncated label */
		widget = gtk_label_new(attr->name);
		gtk_widget_size_request(widget, &req);
		gtk_widget_destroy(widget);

		widget = gtkc_trunctext_new(attr->name);
	}
	else {
		widget = gtk_label_new(attr->name);
		gtk_widget_size_request(widget, &req);
	}

	theight = req.height;
	if (theight < 12)
		theight = 12;
	else if (theight > 128)
		theight = 128;

	if (attr->rnd_hatt_flags & RND_HATF_TEXT_VERTICAL)
		gtk_label_set_angle(GTK_LABEL(widget), 90);

	if (attr->rnd_hatt_flags & RND_HATF_TEXT_TRUNCATED) {
		if (attr->rnd_hatt_flags & RND_HATF_TEXT_VERTICAL) {
			gtk_misc_set_alignment(GTK_MISC(widget), 0, 1);
			gtk_widget_set_size_request(widget, theight, 1);
		}
		else {
			gtk_misc_set_alignment(GTK_MISC(widget), 1, 0);
			gtk_widget_set_size_request(widget, 1, theight);
		}
	}

	if (!(attr->rnd_hatt_flags & RND_HATF_TEXT_TRUNCATED))
		gtk_misc_set_alignment(GTK_MISC(widget), 0., 0.5);

	return widget;
}

#include <librnd/plugins/lib_gtk_common/dlg_attribute.c>
