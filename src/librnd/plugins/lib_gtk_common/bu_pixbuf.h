RND_INLINE GdkPixbuf *rnd_hid_gtk_xpm2pixbuf(const char **xpm, int allow_scale)
{
	GdkPixbuf *pixbuf, *p1;

	allow_scale = 0; /* temporary, until we have a conf node */

	p1 = gdk_pixbuf_new_from_xpm_data(xpm);
	if (!allow_scale)
		return p1;

	pixbuf = gdk_pixbuf_scale_simple(p1, gdk_pixbuf_get_width(p1)*2, gdk_pixbuf_get_height(p1)*2, GDK_INTERP_BILINEAR);
	g_object_unref(p1);
	return pixbuf;
}
