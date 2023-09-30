#include <genvector/gds_char.h>
#include <librnd/core/hidlib.h>

/* [4.1.0] Safe temp dir creation in /tmp */
int rnd_mktempdir(rnd_design_t *dsg, gds_t *dst, const char *prefix);
int rnd_rmtempdir(rnd_design_t *dsg, gds_t *dst);
