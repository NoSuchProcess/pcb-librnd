#include <librnd/rnd_config.h>
#include <librnd/core/hid.h>
#include <librnd/core/actions.h>

char *rnd_dlg_fileselect(rnd_hid_t *hid, const char *title, const char *descr, const char *default_file, const char *default_ext, const rnd_hid_fsd_filter_t *flt, const char *history_tag, rnd_hid_fsd_flags_t flags, rnd_hid_dad_subdialog_t *sub);

extern const char rnd_acts_FsdTest[];
extern const char rnd_acth_FsdTest[];
fgw_error_t rnd_act_FsdTest(fgw_arg_t *res, int argc, fgw_arg_t *argv);
