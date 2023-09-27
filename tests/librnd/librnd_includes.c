#define RND_APP_PREFIX(x)  rndtest_ ## x

/* font */
#include <librnd/core/global_typedefs.h>
#include <librnd/src_3rd/liblihata/lihata.h>
#include <librnd/src_3rd/liblihata/dom.h>

static int PARSE_COORD(rnd_coord_t *dst, lht_node_t *src) { return 0; }
static int PARSE_DOUBLE(double *dst, lht_node_t *src) { return 0; }
static lht_node_t *HASH_GET(lht_node_t *hash, const char *name) { return NULL; }
static lht_node_t *HASH_GET_OPT(lht_node_t *hash, const char *name) { return NULL; }
static int RND_LHT_ERROR(lht_node_t *nd, char *fmt, ...) { return 0; }


/* header integrity test: nothing should be included from src/ that is
   not part of librnd */
#include "inc_all.h"

#include "glue.c"

int main(int argc, char *argv[])
{
	return 0;
}
