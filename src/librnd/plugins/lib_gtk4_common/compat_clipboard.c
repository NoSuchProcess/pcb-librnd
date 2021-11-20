int gtkc_clipboard_set_text(GtkWidget *widget, const char *text)
{
	GdkClipboard *cbrd = gtk_widget_get_clipboard(widget);
	gdk_clipboard_set_text(cbrd, text);
	return 0;
}

int gtkc_clipboard_get_text(GtkWidget *wdg, void **data, size_t *len)
{
	
}
