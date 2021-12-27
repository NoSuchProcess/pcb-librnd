#include "compat.h"

#define DLG_INCLUDES "librnd/plugins/lib_gtk4_common/dlg_includes.c"

#include <librnd/core/hid_dad.h>
#include "gtkc_trunc_label.h"

static inline GtkWidget *gtkc_dad_label_new(rnd_hid_attribute_t *attr)
{
	GtkWidget *widget;

	if (!(attr->rnd_hatt_flags & RND_HATF_TEXT_TRUNCATED) && !(attr->rnd_hatt_flags & RND_HATF_TEXT_VERTICAL)) {
		/* shortcut for the simple case */
		return gtk_label_new(attr->name);
	}

	widget = gtkc_trunc_label_new(attr->name);

	if (!(attr->rnd_hatt_flags & RND_HATF_TEXT_TRUNCATED))
		gtkc_trunc_set_notruncate(GTKC_TRUNC_LABEL(widget), 1);

	if (attr->rnd_hatt_flags & RND_HATF_TEXT_VERTICAL)
		gtkc_trunc_set_rotated(GTKC_TRUNC_LABEL(widget), 1);

	return widget;
}

#include <librnd/plugins/lib_gtk_common/dlg_attribute.c>
