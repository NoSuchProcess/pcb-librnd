#include <stdlib.h>
#include <string.h>
#include <librnd/config.h>
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
#define HT_HAS_CONST_KEY
typedef rnd_coord_t *htph_key_t;                /* array of [2] for x;y */
typedef const rnd_coord_t *htph_const_key_t;    /* array of [2] for x;y */
typedef vtph_t *htph_value_t;
#define HT(x) htph_ ## x
#include <genht/ht.h>
#include <genht/ht.c>
#undef HT
