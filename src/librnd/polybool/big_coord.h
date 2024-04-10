#	if RND_COORD_MAX == ((1UL<<31)-1)
#		define BIG_BITS 32
#		define BIG_DECIMAL_DIGITS 9
#		define BIG_DECIMAL_BASE 1000000000UL
#		define BIG_NEG_BASE 0x80000000UL
#		define BIG_UMAX 0xFFFFFFFFUL
#		define BIG_DBL_MULT ((double)4294967296.0)
#	elif RND_COORD_MAX == ((1ULL<<63)-1)
#		define BIG_BITS 64
#		define BIG_DECIMAL_DIGITS 19
#		define BIG_DECIMAL_BASE 10000000000000000000UL
#		define BIG_NEG_BASE 0x8000000000000000UL
#		define BIG_UMAX 0xFFFFFFFFFFFFFFFFUL
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

int pa_angle_equ(const pa_big_angle_t a, const pa_big_angle_t b);
int pa_angle_lt(const pa_big_angle_t a, const pa_big_angle_t b);
int pa_angle_gt(const pa_big_angle_t a, const pa_big_angle_t b);
int pa_angle_gte(const pa_big_angle_t a, const pa_big_angle_t b);
int pa_angle_lte(const pa_big_angle_t a, const pa_big_angle_t b);
void pa_angle_sub(pa_big_angle_t res, const pa_big_angle_t a, const pa_big_angle_t b); /* res = a-b */
int pa_angle_valid(const pa_big_angle_t a);


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

/* Returns non-zero if v1 is exactly on line v2a..v2b */
int pa_big_is_node_on_line(rnd_vnode_t *v1, rnd_vnode_t *v2a, rnd_vnode_t *v2b);


/* Return node for a point next to dst. Returns existing node on coord
   match else allocates a new node. Returns NULL if new node can't be
   allocated. */
rnd_vnode_t *pa_big_node_add_single(rnd_vnode_t *dst, pa_big_vector_t ptv);

/* Fill ion cd->angle assuming big coord */
void pa_big_calc_angle(pa_conn_desc_t *cd, rnd_vnode_t *pt, char poly, char side);
void pa_big_calc_angle_nn(pa_big_angle_t *dst, rnd_vnode_t *nfrom, rnd_vnode_t *nto);


void pa_big_load(pa_big_coord_t dst, rnd_coord_t src);

/* returns 1 if coordinates of a conn desc is the same as the coordinates of
   a node (or another conn desc) */
int pa_big_desc_node_incident(pa_conn_desc_t *d, rnd_vnode_t *n);
int pa_big_desc_desc_incident(pa_conn_desc_t *a, pa_conn_desc_t *b);

/* Convert big coord to native coord with rounding */
rnd_coord_t pa_big_to_coord(pa_big_coord_t crd);

/* returns whether a big coord is integer (fraction part is zero) */
RND_INLINE int pa_big_is_int(pa_big_coord_t crd) { return (crd[0] == 0) && (crd[1] == 0) && (crd[2] == 0); }
RND_INLINE int pa_big_vect_is_int(pa_big_vector_t pt) { return pa_big_is_int(pt.x) && pa_big_is_int(pt.y); }


/* Compare the coords of vna and vnb and return true of they are equal. Use
   high precision coords where available */
int pa_big_vnode_vnode_equ(rnd_vnode_t *vna, rnd_vnode_t *vnb);

/* Approximation of big coords in double, for debug prints */
double pa_big_double(pa_big_coord_t crd);
double pa_big2_double(pa_big2_coord_t crd);
double pa_big_vnxd(rnd_vnode_t *vn);
double pa_big_vnyd(rnd_vnode_t *vn);

/* Contour postprocessing; see pa_bool_postproc() for explanation. Set
   papa_touch_risk to 1 if there were overlapping islands within A or B
   during a non-SUB operation */
void pa_big_bool_postproc(rnd_polyarea_t **pa, int from_selfisc, int papa_touch_risk);


void rnd_pa_big_load_cvc(pa_big_vector_t *dst, rnd_vnode_t *src);

/* signed and unsigned distance^2 */
void rnd_vect_m_dist2_big(pa_big2_coord_t dst, pa_big_vector_t v1, pa_big_vector_t v2);
void rnd_vect_u_dist2_big(pa_big2_coord_t dst, pa_big_vector_t v1, pa_big_vector_t v2);


int pa_big_coord_cmp(pa_big_coord_t a, pa_big_coord_t b);
int pa_big2_coord_cmp(pa_big2_coord_t a, pa_big2_coord_t b);

int pa_small_big_xy_eq(rnd_coord_t smallx, rnd_coord_t smally, pa_big_coord_t bigx, pa_big_coord_t bigy);


/* Return 1 if a and vnode are very close; when they are close and copy is 1,
   modify a to be the same as vnode's coord */
int pa_big_vmandist_small(pa_big_vector_t *a, rnd_vnode_t *vnode, int copy);


/* Round to nearest integer */
void pa_big_round(pa_big_coord_t big);


/* For collect/gather */
void pa_big_area_incremental(pa_big2_coord_t big_area, rnd_vnode_t *big_curr, rnd_vnode_t *big_prev);
int pa_big_area2ori(pa_big2_coord_t big_area);

