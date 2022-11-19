#include <librnd/hid/hid.h>
#include "rnd_gtk.h"

rnd_hidval_t rnd_gtk_watch_file(rnd_gtk_t *gctx, int fd, unsigned int condition,
	rnd_bool (*func)(rnd_hidval_t watch, int fd, unsigned int condition, rnd_hidval_t user_data),
	rnd_hidval_t user_data);

void rnd_gtk_unwatch_file(rnd_hid_t *hid, rnd_hidval_t data);

