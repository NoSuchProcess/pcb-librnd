#include <ctype.h>
#include <librnd/core/compat_misc.h>
#include <librnd/hid/tool.h>

static rnd_hid_cfg_keys_t rnd_mbtk_keymap;

static unsigned short int rnd_mbtk_translate_key(const char *desc, int len)
{
	unsigned int key = 0;

	if ((desc[0] > 32) && (desc[0] < 127) && (desc[1] == '\0'))
		return tolower(desc[0]);

	if (rnd_strcasecmp(desc, "Return") == 0)
		desc = "Enter";

	key = mbtk_name2keysym(desc);
	if (key == MBTK_KS_INVALID) {
		rnd_message(RND_MSG_WARNING, "Ignoring invalid/exotic key sym: '%s'\n", desc);
		return 0;
	}

	return key;
}

static int rnd_mbtk_key_name(unsigned short int key_char, char *out, int out_len)
{
	char tmp[16];
	const char *name;

	if ((key_char > 32) && (key_char < 127)) {
		tmp[0] = tolower(key_char);
		tmp[1] = '\0';
		name = tmp;
	}
	else {
		name = mbtk_keysym2name(key_char);
		if (name == NULL)
			return -1;
	}
	strncpy(out, name, out_len);
	out[out_len - 1] = '\0';
	return 0;
}

static int xlate_mods(mbtk_keymod_t km)
{
	int res = 0;

	if (km & MBTK_KM_SHIFT) res |= RND_M_Shift;
	if (km & MBTK_KM_ALT)   res |= RND_M_Alt;
	if (km & MBTK_KM_CTRL)  res |= RND_M_Ctrl;

	return res;
}

static mbtk_event_handled_t rnd_mbtk_key_press_cb(rnd_mbtk_t *mctx, mbtk_event_t *ev)
{
	return MBTK_EVENT_NOT_HANDLED;
}


static mbtk_event_handled_t rnd_mbtk_key_release_cb(rnd_mbtk_t *mctx, mbtk_event_t *ev)
{
	int slen, mods;

	if (mbtk_ks_is_modifier(ev->data.key.sym))
		rnd_mbtk_note_event_location(0, 0, 0);

	mods = xlate_mods(ev->data.key.mods);
	slen = rnd_hid_cfg_keys_input(mctx->hidlib, &rnd_mbtk_keymap, mods, ev->data.key.sym, ev->data.key.edit);
	if (slen > 0)
		rnd_hid_cfg_keys_action(mctx->hidlib, &rnd_mbtk_keymap);

	if (rnd_app.adjust_attached_objects != NULL)
		rnd_app.adjust_attached_objects(mctx->hidlib);
	else
		rnd_tool_adjust_attached(mctx->hidlib);

TODO("not available yet as we are not rendering yet");
/*	rnd_gui->invalidate_all(rnd_gui); */

TODO("do we need this?");
/*	g_idle_add(rnd_mbtk_idle_cb, tw);*/

	return MBTK_EVENT_HANDLED;
}
