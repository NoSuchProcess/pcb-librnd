#	if RND_COORD_MAX == ((1UL<<31)-1)
#		define BIG_BITS 32
#		define BIG_DECIMAL_DIGITS 9
#		define BIG_DECIMAL_BASE 1000000000UL
#		define BIG_NEG_BASE 0x80000000UL
#	elif RND_COORD_MAX == ((1ULL<<63)-1)
#		define BIG_BITS 64
#		define BIG_DECIMAL_DIGITS 19
#		define BIG_DECIMAL_BASE 10000000000000000000UL
#		define BIG_NEG_BASE 0x8000000000000000UL
#	else
#		error "unsupported system: rnd_coord has to be 32 or 64 bits wide (checked: RND_COORD_MAX)"
#	endif

#define RND_BIGCRD_WIDTH 3
#define RND_BIGCRD_WIDTH2 (RND_BIGCRD_WIDTH*2)
typedef rnd_ucoord_t rnd_big_coord_t[RND_BIGCRD_WIDTH];
typedef rnd_ucoord_t rnd_big2_coord_t[RND_BIGCRD_WIDTH2];

#define RATIONAL(x) rnd_bcr_ ## x
#define RATIONAL_INT rnd_big_coord_t

#ifndef FROM_ISC_RATIONAL_C
#define RATIONAL_INHIBIT_IMPLEMENTATION
#else
#include <genfip/big.h>
#endif

#include <genfip/rational.h>

#include <librnd/core/math_helper.h>
#include <librnd/core/box.h>
#include "polyarea.h"

typedef struct rnd_bcr_coord_s {
	rnd_bcr_t r;         /* precise coord expressed as rationals */
	rnd_big2_coord_t f;  /* RND_BIGCRD_WIDTH.RND_BIGCRD_WIDTH fixed point representation for comparison */
} rnd_bcr_coord_t;

typedef struct pa_isc_s {
	rnd_bcr_coord_t x, y;
} pa_isc_t;

/* Intersection(s) between lines p1->p2 and q1->q2.
   Returns number of intersections (0, 1 or 2) and loads x;y with the
   coords of the intersection */
int rnd_big_coord_isc(pa_isc_t res[2], rnd_vector_t p1, rnd_vector_t p2, rnd_vector_t q1, rnd_vector_t q2);

/* Intersection(s) between lines v1a->v1b and v2a->v2b.
   Returns number of intersections (0, 1 or 2) and loads isc1 and isc2 with the
   high resolution coords of the intersection */
int rnd_bcr_inters2(rnd_vnode_t *v1a, rnd_vnode_t *v1b, rnd_vnode_t *v2a, rnd_vnode_t *v2b, pa_isc_t isc1, pa_isc_t isc2);


void pa_div_to_big2(rnd_big2_coord_t dst, rnd_big_coord_t n, rnd_big_coord_t d);

