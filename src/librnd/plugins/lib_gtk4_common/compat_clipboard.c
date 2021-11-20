int gtkc_clipboard_set_text(GtkWidget *widget, const char *text)
{
	GdkClipboard *cbrd = gtk_widget_get_clipboard(widget);
	gdk_clipboard_set_text(cbrd, text);
	return 0;
}

typedef struct {
	void **data;
	size_t *len;
	int valid;
	GMainLoop *loop;
	guint timer_id;
} ghid_clip_get_t;


static void ghid_paste_received(GObject  *source_object, GAsyncResult *result, gpointer  user_data)
{
	GdkClipboard *clipboard;
	char *text;
	GError *error = NULL;
	ghid_clip_get_t *cd = user_data;

	clipboard = GDK_CLIPBOARD(source_object);

	/* Get the resulting text of the read operation */
	text = gdk_clipboard_read_text_finish(clipboard, result, &error);
	if (text != NULL) {
		*cd->data = text;
		*cd->len = strlen(text) + 1;
		cd->valid = 1;
	}

	g_main_loop_quit(cd->loop);
}

static gboolean ghid_paste_timer(void *user_data)
{
	ghid_clip_get_t *cd = user_data;

	g_main_loop_quit(cd->loop);
	cd->timer_id = 0;

	return FALSE;  /* Turns timer off */
}



int gtkc_clipboard_get_text(GtkWidget *wdg, void **data, size_t *len)
{
	GdkClipboard *cbrd = gtk_widget_get_clipboard(wdg);
	ghid_clip_get_t cd;

	cd.data = data;
	cd.len = len;
	cd.valid = 0;

	gdk_clipboard_read_text_async(cbrd, NULL, ghid_paste_received, &cd);
	cd.timer_id = g_timeout_add(200, (GSourceFunc)ghid_paste_timer, &cd);

	cd.loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(cd.loop);

	if (cd.timer_id != 0)
		g_source_remove(cd.timer_id);
	g_main_loop_unref(cd.loop);

	return !cd.valid;
}
