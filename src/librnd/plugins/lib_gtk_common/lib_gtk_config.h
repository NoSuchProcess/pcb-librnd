#include <librnd/core/hid.h>
#include <librnd/core/event.h>
#include <librnd/core/conf_hid.h>

extern rnd_conf_hid_id_t ghid_conf_id;

void rnd_gtk_conf_init(void);
void rnd_gtk_conf_uninit(void);

/* Parses string_path to expand and select the corresponding path in tree view. */
void rnd_gtk_config_set_cursor(const char *string_path);



