static rnd_hidval_t rnd_mbtk_add_timer(rnd_hid_t *hid, void (*func)(rnd_hidval_t user_data), unsigned long milliseconds, rnd_hidval_t user_data)
{
	TODO("implement");
}

static void rnd_mbtk_stop_timer(rnd_hid_t *hid, rnd_hidval_t timer)
{
	TODO("implement");
}

static rnd_hidval_t rnd_mbtk_watch_file(rnd_hid_t *hid, int fd, unsigned int condition,
	rnd_bool (*func)(rnd_hidval_t, int, unsigned int, rnd_hidval_t), rnd_hidval_t user_data)
{
	TODO("implemet");
}

static void rnd_mbtk_unwatch_file(rnd_hid_t *hid, rnd_hidval_t data)
{
	TODO("implement");
}

static void rnd_mbtk_beep(rnd_hid_t *hid)
{
	TODO("remove me with librnd4");
}
