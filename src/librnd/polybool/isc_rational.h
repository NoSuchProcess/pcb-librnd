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
typedef rnd_ucoord_t rnd_big_coord_t[RND_BIGCRD_WIDTH];

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

typedef struct pa_isc_s {
	rnd_bcr_t x, y;  /* precise x;y coords of the isc expressed as rationals */
	rnd_big_coord_t qx, qy, rx, ry; /* quotient and remainder of x;y rationals, for quick comparison */
} pa_isc_t;

/* Intersection(s) between lines p1->p2 and q1->q2.
   Returns number of intersections (0, 1 or 2) and loads x;y with the
   coords of the intersection */
int rnd_big_coord_isc(rnd_bcr_t x[2], rnd_bcr_t y[2], rnd_vector_t p1, rnd_vector_t p2, rnd_vector_t q1, rnd_vector_t q2);

