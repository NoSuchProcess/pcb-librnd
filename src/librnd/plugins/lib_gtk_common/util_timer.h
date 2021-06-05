#include <librnd/core/hid.h>
#include "rnd_gtk.h"

rnd_hidval_t rnd_gtk_add_timer(struct rnd_gtk_s *gctx, void (*func)(rnd_hidval_t user_data), unsigned long milliseconds, rnd_hidval_t user_data);
void ghid_stop_timer(rnd_hid_t *hid, rnd_hidval_t timer);
