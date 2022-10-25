#include <librnd/core/hid.h>
#include <librnd/plugins/lib_mbtk_common/mbtk_common.h>

void rnd_mbtk_glue_hid_init(rnd_hid_t *dst, int (*init_backend)(rnd_mbtk_t *mctx, int *argc, char **argv[]));
int rnd_mbtk_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv);

