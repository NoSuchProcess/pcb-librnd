#ifndef PCB_CONF_HID_H
#define PCB_CONF_HID_H

#include "conf.h"

typedef struct conf_hid_callbacks_s {
	/* Called before/after a value of a config item is updated - this doesn't necessarily mean the value actually changes */
	void (*val_change_pre)(conf_native_t *cfg);
	void (*val_change_post)(conf_native_t *cfg);

	/* Called when a new config item is added to the database */
	void (*new_item_post)(conf_native_t *cfg);

	/* Called during conf_hid_unreg to get hid-data cleaned up */
	void (*unreg_item)(conf_native_t *cfg);
} conf_hid_callbacks_t;

typedef int conf_hid_id_t;

/* Set local hid data in a native item; returns the previous value set or NULL */
void *conf_hid_set_data(conf_native_t *cfg, conf_hid_id_t id, void *data);

/* Returns local hid data in a native item */
void *conf_hid_get_data(conf_native_t *cfg, conf_hid_id_t id);

/* Set local callbacks in a native item; returns the previous callbacks set or NULL */
const conf_hid_callbacks_t *conf_hid_set_cb(conf_native_t *cfg, conf_hid_id_t id, const conf_hid_callbacks_t *cbs);


/* register a hid with a cookie; this is necessary only if:
     - the HID wants to store per-config-item hid_data with the above calls
     - the HID wants to get notified about changes in the config tree using callback functions
   NOTE: cookie is taken by pointer, the string value does not matter. One pointer
         can be registered only once.
   cb holds the global notification callbacks - called when anything changed; it can be NULL.
   Returns a new HID id that can be used to access hid data, or -1 on error.
*/
conf_hid_id_t conf_hid_reg(const char *cookie, const conf_hid_callbacks_t *cb);

/* Unregister a hid; if unreg_item cb is specified, call it on each config item */
void conf_hid_unreg(const char *cookie);

void conf_hid_uninit(void);

/* Call the local callback of a native item */
#define conf_hid_cb(native, cb) \
do { \
	int __n__; \
	for(__n__ = 0; __n__ < vtp0_len(&((native)->hid_callbacks)); __n__++) { \
		const conf_hid_callbacks_t *cbs = (native)->hid_callbacks.array[__n__]; \
		if (cbs != NULL) \
			cbs->cb(native); \
	} \
} while(0)

#endif
