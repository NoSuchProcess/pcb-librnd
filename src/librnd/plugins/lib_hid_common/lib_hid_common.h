#include "dialogs_conf.h"

extern conf_dialogs_t dialogs_conf;

/* HID implementations shall call this to announce GUI init - this
   will send out the event and try to minimize the noise by setting
   up inhibits */
void rnd_hid_announce_gui_init(rnd_hidlib_t *hidlib);

void rnd_hidcore_crosshair_move_to(rnd_hidlib_t *hidlib, rnd_coord_t abs_x, rnd_coord_t abs_y, int mouse_mot);
