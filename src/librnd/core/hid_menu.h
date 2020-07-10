#ifndef RND_HID_MENU_H
#define RND_HID_MENU_H

#include <librnd/core/hid_cfg.h>
/* Search and load the menu res for hidname; if not found, parse
   embedded_fallback instead (if it is NULL, use the application default
   instead; for explicit empty embedded: use ""). Returns NULL on error */
rnd_hid_cfg_t *rnd_hid_cfg_load(rnd_hidlib_t *hidlib, const char *fn, int exact_fn, const char *embedded_fallback);


#endif
