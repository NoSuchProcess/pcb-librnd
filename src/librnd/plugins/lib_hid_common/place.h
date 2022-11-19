#include <librnd/core/conf.h>
void rnd_wplc_load(rnd_conf_role_t role);

void rnd_wplc_save_to_role(rnd_design_t *hidlib, rnd_conf_role_t role);
int rnd_wplc_save_to_file(rnd_design_t *hidlib, const char *fn);


/*** for internal use ***/
void rnd_dialog_place_uninit(void);
void rnd_dialog_place_init(void);
void rnd_dialog_resize(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[]);
void rnd_dialog_place(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[]);

