#include <librnd/hid/hid.h>
#include <librnd/core/hidlib.h>
#include <librnd/core/rnd_bool.h>

void rnd_mbtk_attr_dlg_new(rnd_hid_t *hid, rnd_mbtk_t *gctx, const char *id, rnd_hid_attribute_t *attrs, int n_attrs, const char *title, void *caller_data, rnd_bool modal, void (*button_cb)(void *caller_data, rnd_hid_attr_ev_t ev), int defx, int defy, int minx, int miny, void **hid_ctx_out);
int rnd_mbtk_attr_dlg_run(void *hid_ctx);
void rnd_mbtk_attr_dlg_raise(void *hid_ctx);
void rnd_mbtk_attr_dlg_close(void *hid_ctx);
void rnd_mbtk_attr_dlg_free(void *hid_ctx);
void rnd_mbtk_attr_dlg_property(void *hid_ctx, rnd_hat_property_t prop, const rnd_hid_attr_val_t *val);
int rnd_mbtk_attr_dlg_widget_state(void *hid_ctx, int idx, int enabled);
int rnd_mbtk_attr_dlg_widget_hide(void *hid_ctx, int idx, rnd_bool hide);
int rnd_mbtk_attr_dlg_widget_poke(void *hid_ctx, int idx, int argc, fgw_arg_t argv[]);
int rnd_mbtk_attr_dlg_set_value(void *hid_ctx, int idx, const rnd_hid_attr_val_t *val);
void rnd_mbtk_attr_dlg_set_help(void *hid_ctx, int idx, const char *val);

/* Create an interacgive DAD subdialog under parent_vbox */
void *rnd_mbtk_attr_sub_new(rnd_mbtk_t *gctx, mbtk_widget_t *parent_box, rnd_hid_attribute_t *attrs, int n_attrs, void *caller_data);

/* Report new window coords to the central window placement code
   emitting an event */
int rnd_mbtk_winplace_cfg(rnd_design_t *hidlib, mbtk_widget_t *widget, void *ctx, const char *id);


rnd_design_t *rnd_mbtk_attr_get_dad_hidlib(void *hid_ctx);

/* Close and free all open DAD dialogs */
void rnd_mbtk_attr_dlg_free_all(rnd_mbtk_t *gctx);





