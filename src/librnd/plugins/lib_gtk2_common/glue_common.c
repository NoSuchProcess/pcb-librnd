#include "compat.h"

#include <librnd/plugins/lib_gtk_common/in_keyboard.h>

static void kbd_input_signals_connect(int idx, void *obj)
{
	ghidgui->key_press_handler[idx] = g_signal_connect(G_OBJECT(obj), "key_press_event", G_CALLBACK(rnd_gtk_key_press_cb), ghidgui);
	ghidgui->key_release_handler[idx] = g_signal_connect(G_OBJECT(obj), "key_release_event", G_CALLBACK(rnd_gtk_key_release_cb), &ghidgui->topwin);
}

static void kbd_input_signals_disconnect(int idx, void *obj)
{
	if (ghidgui->key_press_handler[idx] != 0) {
		g_signal_handler_disconnect(G_OBJECT(obj), ghidgui->key_press_handler[idx]);
		ghidgui->key_press_handler[idx] = 0;
	}
	if (ghidgui->key_release_handler[idx] != 0) {
		g_signal_handler_disconnect(G_OBJECT(obj), ghidgui->key_release_handler[idx]);
		ghidgui->key_release_handler[idx] = 0;
	}
}

static void mouse_input_singals_connect(void *obj)
{
	ghidgui->button_press_handler = g_signal_connect(G_OBJECT(obj), "button_press_event", G_CALLBACK(rnd_gtk_button_press_cb), ghidgui);
	ghidgui->button_release_handler = g_signal_connect(G_OBJECT(obj), "button_release_event", G_CALLBACK(rnd_gtk_button_release_cb), ghidgui);
}

static void mouse_input_singals_disconnect(void *obj)
{
	if (ghidgui->button_press_handler != 0)
		g_signal_handler_disconnect(G_OBJECT(ghidgui->port.drawing_area), ghidgui->button_press_handler);

	if (ghidgui->button_release_handler != 0)
		g_signal_handler_disconnect(ghidgui->port.drawing_area, ghidgui->button_release_handler);

	ghidgui->button_press_handler = ghidgui->button_release_handler = 0;
}


#include <librnd/plugins/lib_gtk_common/glue_common.c>
