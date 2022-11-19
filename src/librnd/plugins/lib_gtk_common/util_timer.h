#include <librnd/hid/hid.h>
#include "rnd_gtk.h"

rnd_hidval_t rnd_gtk_add_timer(struct rnd_gtk_s *gctx, void (*func)(rnd_hidval_t user_data), unsigned long milliseconds, rnd_hidval_t user_data);
void rnd_gtk_stop_timer(rnd_hid_t *hid, rnd_hidval_t timer);
