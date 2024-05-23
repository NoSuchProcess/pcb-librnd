#include "htvlist.h"
#define HT(x) htvlist_ ## x
#include <genht/ht.c>
#undef HT


#include <genht/hash.h>

unsigned int htvlist_keyhash(htvlist_key_t k)
{
	rnd_coord_t c = k.x ^ k.y;

	if (sizeof(rnd_coord_t) <= sizeof(unsigned int))
		return c;

	return murmurhash(&c, sizeof(c));
}


int htvlist_keyeq(htvlist_key_t a, htvlist_key_t b)
{
	return (a.x == b.x) && (a.y == b.y);
}
