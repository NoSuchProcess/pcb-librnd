#ifndef RND_HID_COMMON_HIDNOGUI_H
#define RND_HID_COMMON_HIDNOGUI_H

void rnd_hid_nogui_init(rnd_hid_t * hid);
rnd_hid_t *rnd_hid_nogui_get_hid(void);

/* For checking if attr dialogs are not available: */
void rnd_nogui_attr_dlg_new(rnd_hid_t *hid, const char *id, rnd_hid_attribute_t *attrs, int n_attrs, const char *title, void *caller_data, rnd_bool modal, void (*button_cb)(void *caller_data, rnd_hid_attr_ev_t ev), int defx, int defy, int minx, int miny, void **hid_ctx_out);

int rnd_nogui_progress(long so_far, long total, const char *message);

/* Return a line of user input text, stripped of any newline characters.
   Returns NULL if the user simply presses enter, or otherwise gives no input. */
char *rnd_nogui_read_stdin_line(void);

#endif
