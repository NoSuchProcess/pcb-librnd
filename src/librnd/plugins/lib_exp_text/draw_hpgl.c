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
	path_obj_type_t type;
	rnd_coord_t x, y; /* end coords */
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

