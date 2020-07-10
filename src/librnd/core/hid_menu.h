#ifndef RND_HID_MENU_H
#define RND_HID_MENU_H

#include <librnd/core/hid_cfg.h>
/* Search and load the menu res for hidname; if not found, parse
   embedded_fallback instead (if it is NULL, use the application default
   instead; for explicit empty embedded: use ""). Returns NULL on error */
rnd_hid_cfg_t *rnd_hid_menu_load(rnd_hidlib_t *hidlib, const char *fn, int exact_fn, const char *embedded_fallback);

lht_node_t *rnd_hid_cfg_get_menu(rnd_hid_cfg_t *hr, const char *menu_path);
lht_node_t *rnd_hid_cfg_get_menu_at(rnd_hid_cfg_t *hr, lht_node_t *at, const char *menu_path, lht_node_t *(*cb)(void *ctx, lht_node_t *node, const char *path, int rel_level), void *ctx);


/* search all instances of an @anchor menu in the currently active GUI and
   call cb on the lihata node; path has 128 extra bytes available at the end */
void rnd_hid_cfg_map_anchor_menus(const char *name, void (*cb)(void *ctx, rnd_hid_cfg_t *cfg, lht_node_t *n, char *path), void *ctx);

/* remove all adjacent anchor menus with matching cookie below anode, the
   anchor node */
int rnd_hid_cfg_del_anchor_menus(lht_node_t *anode, const char *cookie);


#endif
