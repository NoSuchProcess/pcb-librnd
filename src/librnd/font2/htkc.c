#include "htkc.h"
#define HT(x) htkc_ ## x
#include <genht/ht.c>
#include <genht/hash.h>

unsigned htkc_keyhash(htkc_key_t a)
{
	return longhash((((unsigned int)(a.left)) << 8) + (unsigned int)a.right);
}


int htkc_keyeq(htkc_key_t a, htkc_key_t b)
{
	return (a.left == b.left) && (a.right == b.right);
}

#undef HT
