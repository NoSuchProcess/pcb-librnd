/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Alain Vigne
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* This code was originally written by Bill Wilson for the PCB Gtk port. */

#include "config.h"
#include "in_keyboard.h"
#include <gdk/gdkkeysyms.h>
#include <ctype.h>
#include <string.h>

#include <librnd/core/event.h>
#include <librnd/core/compat_misc.h>
#include "glue_common.h"

rnd_hid_cfg_keys_t rnd_gtk_keymap;
GdkModifierType rnd_gtk_glob_mask;

gboolean rnd_gtk_is_modifier_key_sym(gint ksym)
{
	if (ksym == RND_GTK_KEY(Shift_R) || ksym == RND_GTK_KEY(Shift_L) || ksym == RND_GTK_KEY(Control_R) || ksym == RND_GTK_KEY(Control_L))
		return TRUE;
	return FALSE;
}

ModifierKeysState rnd_gtk_modifier_keys_state(GtkWidget *drawing_area, GdkModifierType *state)
{
	GdkModifierType mask;
	ModifierKeysState mk;
	gboolean shift, control, mod1;

	if (!state)
		gdkc_window_get_pointer(drawing_area, NULL, NULL, &mask);
	else
		mask = *state;

	shift = (mask & GDK_SHIFT_MASK);
	control = (mask & GDK_CONTROL_MASK);
	mod1 = (mask & GDKC_MOD1_MASK);

	if (shift && !control && !mod1)
		mk = SHIFT_PRESSED;
	else if (!shift && control && !mod1)
		mk = CONTROL_PRESSED;
	else if (!shift && !control && mod1)
		mk = MOD1_PRESSED;
	else if (shift && control && !mod1)
		mk = SHIFT_CONTROL_PRESSED;
	else if (shift && !control && mod1)
		mk = SHIFT_MOD1_PRESSED;
	else if (!shift && control && mod1)
		mk = CONTROL_MOD1_PRESSED;
	else if (shift && control && mod1)
		mk = SHIFT_CONTROL_MOD1_PRESSED;
	else
		mk = NONE_PRESSED;

	return mk;
}

int rnd_gtk_key_translate(int in_keyval, int in_state, int in_key_raw, int *out_mods, unsigned short int *out_key_raw, unsigned short int *out_kv)
{
	int mods = 0;
	unsigned short int key_raw = in_key_raw, kv;
	int state = in_state;

	kv = in_keyval;
	rnd_gtk_glob_mask = state;

	if (state & GDK_MOD1_MASK)    mods |= RND_M_Alt;
	if (state & GDK_CONTROL_MASK) mods |= RND_M_Ctrl;
	if (state & GDK_SHIFT_MASK)   mods |= RND_M_Shift;

	if (kv == RND_GTK_KEY(ISO_Left_Tab)) kv = RND_GTK_KEY(Tab);
	if (kv == RND_GTK_KEY(KP_Add)) key_raw = kv = '+';
	if (kv == RND_GTK_KEY(KP_Subtract)) key_raw = kv = '-';
	if (kv == RND_GTK_KEY(KP_Multiply)) key_raw = kv = '*';
	if (kv == RND_GTK_KEY(KP_Divide)) key_raw = kv = '/';
	if (kv == RND_GTK_KEY(KP_Enter)) key_raw = kv = RND_GTK_KEY(Return);

	*out_mods = mods;
	*out_key_raw = key_raw;
	*out_kv = kv;

	return 0;
}

gboolean rnd_gtk_key_press_cb(GtkWidget *drawing_area, long mods, long key_raw, long kv, gpointer data)
{
	rnd_gtk_t *gctx = data;
	int slen;

	if (rnd_gtk_is_modifier_key_sym(kv))
		return FALSE;

	rnd_gtk_note_event_location(0, 0, 0);

	slen = rnd_hid_cfg_keys_input(&rnd_gtk_keymap, mods, key_raw, kv);
	if (slen > 0) {
		rnd_hid_cfg_keys_action(gctx->hidlib, &rnd_gtk_keymap);
		return TRUE;
	}

	return FALSE;
}

unsigned short int rnd_gtk_translate_key(const char *desc, int len)
{
	guint key;

	if (rnd_strcasecmp(desc, "enter") == 0)
		desc = "Return";

	key = gdk_keyval_from_name(desc);
	if (key > 0xffff) {
		rnd_message(RND_MSG_WARNING, "Ignoring invalid/exotic key sym: '%s'\n", desc);
		return 0;
	}
	return key;
}

int rnd_gtk_key_name(unsigned short int key_char, char *out, int out_len)
{
	char *name = gdk_keyval_name(key_char);
	if (name == NULL)
		return -1;
	strncpy(out, name, out_len);
	out[out_len - 1] = '\0';
	return 0;
}
