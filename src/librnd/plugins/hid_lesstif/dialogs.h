#ifndef LIBRND_HID_LESSTIF_DIALOGS_H
#define LIBRND_HID_LESSTIF_DIALOGS_H

#include <genlist/gendlist.h>

typedef struct {
	void *caller_data; /* WARNING: for now, this must be the first field (see core spinbox enter_cb) */
	rnd_design_t *hidlib; /* the hidlib that was active at the moment the dialog was created */
	rnd_hid_attribute_t *attrs;
	int n_attrs;
	Widget *wl;   /* content widget */
	Widget *wltop;/* the parent widget, which is different from wl if reparenting (extra boxes, e.g. for framing or scrolling) was needed */
	Widget **btn; /* enum value buttons */
	Widget dialog;
	rnd_hid_attr_val_t property[RND_HATP_max];
	Dimension minw, minh;
	void (*close_cb)(void *caller_data, rnd_hid_attr_ev_t ev);
	char *id;
	unsigned close_cb_called:1;
	unsigned already_closing:1;
	unsigned inhibit_valchg:1;
	unsigned widget_destroyed:1;
	unsigned set_ok:1;
	unsigned creating:1;
	gdl_elem_t link; /* in ltf_dad_dialogs  */
} lesstif_attr_dlg_t;

void lesstif_attr_sub_update_hidlib(void *hid_ctx, rnd_design_t *new_dsg);
void lesstif_attr_dlg_free_all(void);

#endif
