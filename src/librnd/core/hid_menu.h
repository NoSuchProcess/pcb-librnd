#ifndef RND_HID_MENU_H
#define RND_HID_MENU_H

#include <librnd/core/hid_cfg.h>
/* Search and load the menu file called from fn, using the menu search
   path (from the conf system) if not given by an absolute path; if NULL or
   not found, parse embedded_fallback instead (if it is not NULL).
   Returns 0 on success. */
int rnd_hid_menu_load(rnd_hid_t *hid, rnd_hidlib_t *hidlib, const char *cookie, int prio, const char *fn, int exact_fn, const char *embedded_fallback, const char *desc);

/* The GUI announces that it is ready for creating the menu; the initial
   merge should happen, but no modification callbacks are done */
void rnd_hid_menu_gui_ready_to_create(rnd_hid_t *hid);

/* The GUI announces that it is ready for executing menu modification callbacks */
void rnd_hid_menu_gui_ready_to_modify(rnd_hid_t *hid);


lht_node_t *rnd_hid_cfg_get_menu(rnd_hid_cfg_t *hr, const char *menu_path);
lht_node_t *rnd_hid_cfg_get_menu_at(rnd_hid_cfg_t *hr, lht_node_t *at, const char *menu_path, lht_node_t *(*cb)(void *ctx, lht_node_t *node, const char *path, int rel_level), void *ctx);


/* search all instances of an @anchor menu in the currently active GUI and
   call cb on the lihata node; path has 128 extra bytes available at the end */
void rnd_hid_cfg_map_anchor_menus(const char *name, void (*cb)(void *ctx, rnd_hid_cfg_t *cfg, lht_node_t *n, char *path), void *ctx);

/* remove all adjacent anchor menus with matching cookie below anode, the
   anchor node */
int rnd_hid_cfg_del_anchor_menus(lht_node_t *anode, const char *cookie);


#endif
