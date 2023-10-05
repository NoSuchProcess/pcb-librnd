#include <librnd/rnd_config.h>
#include <librnd/hid/hid.h>
#include <librnd/core/actions.h>

/* Normally returns a char *path for the path selected, or NULL on error/cancel.
   If flags contains both RND_HID_FSD_READ and RND_HID_FSD_MULTI, multiple
   paths may be selected; these are returned as concatenated \0 terminated
   strings with an extra \0 appended at the end (so that the last string of
   the list is an empty string). */
char *rnd_dlg_fileselect(rnd_hid_t *hid, const char *title, const char *descr, const char *default_file, const char *default_ext, const rnd_hid_fsd_filter_t *flt, const char *history_tag, rnd_hid_fsd_flags_t flags, rnd_hid_dad_subdialog_t *sub);

extern const char rnd_acts_FsdTest[];
extern const char rnd_acth_FsdTest[];
fgw_error_t rnd_act_FsdTest(fgw_arg_t *res, int argc, fgw_arg_t *argv);

extern const char rnd_acts_FsdSimple[];
extern const char rnd_acth_FsdSimple[];
fgw_error_t rnd_act_FsdSimple(fgw_arg_t *res, int argc, fgw_arg_t *argv);

