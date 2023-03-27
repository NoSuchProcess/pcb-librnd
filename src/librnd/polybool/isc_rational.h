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

typedef struct pa_isc_s {
	rnd_bcr_t x, y;  /* precise x;y coords of the isc expressed as rationals */
	rnd_big_coord_t qx, qy, rx, ry; /* quotient and remainder of x;y rationals, for quick comparison */
} pa_isc_t;

/* Single intersection between non-parallel lines x1;y1->x2;y2 and x3;y3->x4;y4.
   Returns 0 on success and loads x;y with the coords of the intersection */
int rnd_big_coord_isc(rnd_bcr_t *x, rnd_bcr_t *y, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t x3, rnd_coord_t y3, rnd_coord_t x4, rnd_coord_t y4);
