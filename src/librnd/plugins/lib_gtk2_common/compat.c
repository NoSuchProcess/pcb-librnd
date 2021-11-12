#include "compat.h"
#include <librnd/plugins/lib_gtk_common/rnd_gtk.h>

gboolean gtkc_resize_dwg_cb(GtkWidget *widget, GdkEventConfigure *ev, void *rs_)
{
	gtkc_resize_dwg_t *rs = rs_;
	return rs->cb(widget, ev->width, ev->height, rs->user_data);
}
