#include <librnd/config.h>

#include <stdlib.h>
#include <string.h>

#include <genvector/vtp0.h>
#include <genht/hash.h>
#include <librnd/core/global_typedefs.h>

typedef enum {
	POT_MOVE,
	POT_LINE,
	POT_ARC
} path_obj_type_t;

typedef struct {
	rnd_coord_t x, y; /* end coords; must be the first field for the hash */
	path_obj_type_t type;
	union {
		struct {
			rnd_coord_t r, cx, cy; /* radius and center */
			double da;
		} arc;
	} data;
} path_obj_t;


/* Elem=path_obj_t */
#define GVT(x) vtph_ ## x
#define GVT_ELEM_TYPE path_obj_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 4096
#define GVT_START_SIZE 16
#define GVT_FUNC RND_INLINE
#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
#include <genvector/genvector_impl.c>
#include <genvector/genvector_undef.h>

/* endpoint coord[2] -> vtph* hash */
typedef rnd_coord_t *htph_key_t;                /* array of [2] for x;y */
typedef vtph_t *htph_value_t;
#define HT(x) htph_ ## x
#include <genht/ht.h>
#include <genht/ht.c>
#undef HT

typedef struct rnd_hpgl_paths_s {
	vtp0_t paths; /* all paths (value: vtph_t *) */
	htph_t start; /* same paths hashed by their start point */
	htph_t end; /* same paths hashed by their end point */
} ctx_t;

struct rnd_hpgl_path_s { vtph_t path; }; /* rename for the public API */

#include "draw_hpgl.h"

static void path_reverse(ctx_t *ctx, vtph_t *path)
{

}

/* src's start is the same as dst's end; append src to dst, free src */
static void path_append_path(ctx_t *ctx, vtph_t *dst, vtph_t *src)
{
	
}

RND_INLINE unsigned hash_coord(rnd_coord_t c)
{
	return murmurhash(&(c), sizeof(rnd_coord_t));
}

static unsigned htph_keyhash(const htph_key_t key)
{
	rnd_coord_t *crd = key;
	return hash_coord(crd[0]) ^ hash_coord(crd[1]);
}

static int htph_keyeq(const htph_key_t keya, const htph_key_t keyb)
{
	rnd_coord_t *crda = keya, *crdb = keya;
	return (crda[0] == crdb[0]) && (crda[1] == crdb[1]);
}

rnd_hpgl_ctx_t *rnd_hpgl_alloc(void)
{
	ctx_t *ctx = calloc(sizeof(ctx_t), 1);

	htph_init(&ctx->start, htph_keyhash, htph_keyeq);
	htph_init(&ctx->end, htph_keyhash, htph_keyeq);

	return ctx;
}

rnd_hpgl_path_t *rnd_hpgl_path_new(rnd_hpgl_ctx_t *ctx, rnd_coord_t x, rnd_coord_t y)
{
	vtph_t *path = calloc(sizeof(vtph_t), 1);
	path_obj_t *po = vtph_alloc_append(path, 1);
	po->type = POT_MOVE;
	po->x = x;
	po->y = y;
	vtp0_append(&ctx->paths, path);
	htph_set(&ctx->start, (htph_key_t)po, path);
	htph_set(&ctx->end, (htph_key_t)po, path);
	return (rnd_hpgl_path_t *)path;
}

rnd_hpgl_path_t *rnd_hpgl_path_append_line(rnd_hpgl_ctx_t *ctx, rnd_hpgl_path_t *path, rnd_coord_t x, rnd_coord_t y)
{
	return NULL;
}

