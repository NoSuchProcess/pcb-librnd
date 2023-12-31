#include <gtk/gtk.h>
#include <librnd/hid/hid.h>
#include "rnd_gtk.h"

void rnd_gtk_attr_dlg_new(rnd_hid_t *hid, rnd_gtk_t *gctx, const char *id, rnd_hid_attribute_t *attrs, int n_attrs, const char *title, void *caller_data, rnd_bool modal, void (*button_cb)(void *caller_data, rnd_hid_attr_ev_t ev), int defx, int defy, int minx, int miny, void **hid_ctx_out);
int rnd_gtk_attr_dlg_run(void *hid_ctx);
void rnd_gtk_attr_dlg_raise(void *hid_ctx);
void rnd_gtk_attr_dlg_close(void *hid_ctx);
void rnd_gtk_attr_dlg_free(void *hid_ctx);
void rnd_gtk_attr_dlg_property(void *hid_ctx, rnd_hat_property_t prop, const rnd_hid_attr_val_t *val);

int rnd_gtk_attr_dlg_widget_state(void *hid_ctx, int idx, int enabled);
int rnd_gtk_attr_dlg_widget_hide(void *hid_ctx, int idx, rnd_bool hide);
int rnd_gtk_attr_dlg_widget_poke(void *hid_ctx, int idx, int argc, fgw_arg_t argv[]);
int rnd_gtk_attr_dlg_widget_focus(void *hid_ctx, int idx);

int rnd_gtk_attr_dlg_set_value(void *hid_ctx, int idx, const rnd_hid_attr_val_t *val);
void rnd_gtk_attr_dlg_set_help(void *hid_ctx, int idx, const char *val);


/* Create an interacgive DAD subdialog under parent_vbox */
void *rnd_gtk_attr_sub_new(rnd_gtk_t *gctx, GtkWidget *parent_box, rnd_hid_attribute_t *attrs, int n_attrs, void *caller_data);

/* Fix up background color of various widgets - useful if the host dialog's background color is not the default */
void rnd_gtk_dad_fixcolor(void *hid_ctx, const rnd_gtk_color_t *color);

/* Report new window coords to the central window placement code
   emitting an event */
int rnd_gtk_winplace_cfg(rnd_design_t *hidlib, GtkWidget *widget, void *ctx, const char *id);

rnd_design_t *rnd_gtk_attr_get_dad_hidlib(void *hid_ctx);

/* Close and free all open DAD dialogs */
void rnd_gtk_attr_dlg_free_all(rnd_gtk_t *gctx);

/* Replace a subdialog's ->hidlib with new_dsg */
void rnd_gtk_attr_sub_update_hidlib(void *hid_ctx, rnd_design_t *new_dsg);
