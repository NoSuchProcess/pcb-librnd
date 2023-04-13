#	if RND_COORD_MAX == ((1UL<<31)-1)
#		define BIG_BITS 32
#		define BIG_DECIMAL_DIGITS 9
#		define BIG_DECIMAL_BASE 1000000000UL
#		define BIG_NEG_BASE 0x80000000UL
#		define BIG_DBL_MULT ((double)4294967296.0)
#	elif RND_COORD_MAX == ((1ULL<<63)-1)
#		define BIG_BITS 64
#		define BIG_DECIMAL_DIGITS 19
#		define BIG_DECIMAL_BASE 10000000000000000000UL
#		define BIG_NEG_BASE 0x8000000000000000UL
#		define BIG_DBL_MULT ((double)18446744073709551616.0)
#	else
#		error "unsupported system: rnd_coord has to be 32 or 64 bits wide (checked: RND_COORD_MAX)"
#	endif

#define PA_BIGCRD_WIDTH 6
typedef rnd_ucoord_t pa_big_coord_t[PA_BIGCRD_WIDTH];  /* signed PA_BIGCRD_WIDTH.PA_BIGCRD_WIDTH */

typedef rnd_ucoord_t pa_big2_coord_t[PA_BIGCRD_WIDTH*2]; /* for internal calculations */
typedef rnd_ucoord_t pa_big3_coord_t[PA_BIGCRD_WIDTH*3]; /* for internal calculations */

#define PA_BIGCOORD_SIZEOF (sizeof(rnd_ucoord_t) * PA_BIGCRD_WIDTH)

typedef pa_big_coord_t pa_big_angle_t;

int pa_angle_equ(pa_big_angle_t a, pa_big_angle_t b);
int pa_angle_lt(pa_big_angle_t a, pa_big_angle_t b);
int pa_angle_gte(pa_big_angle_t a, pa_big_angle_t b);
int pa_angle_lte(pa_big_angle_t a, pa_big_angle_t b);
int pa_angle_valid(pa_big_angle_t a);


#include <librnd/core/math_helper.h>
#include <librnd/core/box.h>
#include "polyarea.h"

typedef struct pa_big_vector_s {
	pa_big_coord_t x, y; /*  fixed point representation */
} pa_big_vector_t;

/* Intersection(s) between lines p1->p2 and q1->q2.
   Returns number of intersections (0, 1 or 2) and loads x;y with the
   coords of the intersection */
int rnd_big_coord_isc(pa_big_vector_t res[2], pa_big_vector_t p1, pa_big_vector_t p2, pa_big_vector_t q1, pa_big_vector_t q2);

/* Intersection(s) between lines v1a->v1b and v2a->v2b.
   Returns number of intersections (0, 1 or 2) and loads isc1 and isc2 with the
   high resolution coords of the intersection */
int pa_big_inters2(rnd_vnode_t *v1a, rnd_vnode_t *v1b, rnd_vnode_t *v2a, rnd_vnode_t *v2b, pa_big_vector_t *isc1, pa_big_vector_t *isc2);


/* Return node for a point next to dst. Returns existing node on coord
   match else allocates a new node. Returns NULL if new node can't be
   allocated. */
rnd_vnode_t *pa_big_node_add_single(rnd_vnode_t *dst, pa_big_vector_t ptv);

/* Fill ion cd->angle assuming big coord */
void pa_big_calc_angle(pa_conn_desc_t *cd, rnd_vnode_t *pt, char poly, char side);


void pa_big_load(pa_big_coord_t dst, rnd_coord_t src);

/* returns 1 if coordinates of a conn desc is the same as the coordinates of
   a node (or another conn desc) */
int pa_big_desc_node_incident(pa_conn_desc_t *d, rnd_vnode_t *n);
int pa_big_desc_desc_incident(pa_conn_desc_t *a, pa_conn_desc_t *b);

/* Convert big coord to native coord with rounding */
rnd_coord_t pa_big_to_coord(pa_big_coord_t crd);

/* returns whether a big coord is integer (fraction part is zero) */
RND_INLINE int pa_big_is_int(pa_big_coord_t crd) { return (crd[0] == 0) && (crd[1] == 0) && (crd[2] == 0); }

/* Compare the coords of vna and vnb and return true of they are equal. Use
   high precision coords where available */
int pa_big_vnode_vnode_equ(rnd_vnode_t *vna, rnd_vnode_t *vnb);

/* Approximation of big coords in double, for debug prints */
double pa_big_double(pa_big_coord_t crd);
double pa_big_vnxd(rnd_vnode_t *vn);
double pa_big_vnyd(rnd_vnode_t *vn);
