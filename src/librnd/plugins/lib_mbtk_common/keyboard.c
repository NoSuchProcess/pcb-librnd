static rnd_hid_cfg_keys_t rnd_mbtk_keymap;

static unsigned short int rnd_mbtk_translate_key(const char *desc, int len)
{
	unsigned int key = 0;

TODO("implement this");
#if 0
	if (rnd_strcasecmp(desc, "enter") == 0)
		desc = "Return";

	key = gdk_keyval_from_name(desc);
	if (key > 0xffff) {
		rnd_message(RND_MSG_WARNING, "Ignoring invalid/exotic key sym: '%s'\n", desc);
		return 0;
	}
#endif
	return key;
}

static int rnd_mbtk_key_name(unsigned short int key_char, char *out, int out_len)
{
	const char *name = /*keyval_name(key_char);*/ NULL;
	if (name == NULL)
		return -1;
	strncpy(out, name, out_len);
	out[out_len - 1] = '\0';
	return 0;
}
