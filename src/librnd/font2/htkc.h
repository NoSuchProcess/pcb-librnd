#ifndef GENHT_HTKC_H
#define GENHT_HTKC_H

/* Kerning to coord;
   key: a pair of characters for the kerning table;
   val: a signed coordinate offset (on top of normal advance
*/

#include <librnd/config.h>

typedef struct htkc_key_s {
	char left, right;
} htkc_key_t;
typedef rnd_coord_t htkc_value_t;
#define HT(x) htkc_ ## x
#include <genht/ht.h>
#undef HT

int htkc_keyeq(htkc_key_t a, htkc_key_t b);

#endif
