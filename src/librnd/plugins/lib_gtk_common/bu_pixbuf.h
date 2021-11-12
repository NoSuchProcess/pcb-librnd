#ifndef RND_GTK_PIXBUF_H
#define RND_GTK_PIXBUF_H

#include <math.h>
#include "hid_gtk_conf.h"

RND_INLINE GdkPixbuf *rnd_gtk_xpm2pixbuf(const char **xpm, int allow_scale)
{
	GdkPixbuf *pixbuf, *p1;
	double w, h, icon_scale = rnd_gtk_conf_hid.plugins.hid_gtk.icon_scale;

	if ((icon_scale <= 0.1) || (fabs(icon_scale - 1) < 0.01))
		allow_scale = 0;

	p1 = gdk_pixbuf_new_from_xpm_data(xpm);
	if (!allow_scale)
		return p1;

	w = gdk_pixbuf_get_width(p1) * icon_scale;
	h = gdk_pixbuf_get_height(p1) * icon_scale;

	if (w < 2) w = 2;
	if (h < 2) h = 2;

	pixbuf = gdk_pixbuf_scale_simple(p1, w, h, GDK_INTERP_BILINEAR);
	g_object_unref(p1);
	return pixbuf;
}

#endif
