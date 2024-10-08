#ifndef GENHT_HTVLIST_H
#define GENHT_HTVLIST_H

#include <genlist/gendlist.h>

#define HT_INVALID_VALUE htvlist_invalid_value

#ifdef RND_HAS_INLINE
#define GENHT_WANT_INLINE
#endif

typedef struct {
	rnd_coord_t x, y;
} htvlist_key_t;

typedef struct {
	struct pb2_seg_s *heads; /* head of list for seg starts */
	struct pb2_seg_s *heade; /* head of list for seg ends */
} htvlist_value_t;
static const htvlist_value_t htvlist_invalid_value = {0};
#define HT(x) htvlist_ ## x
#include <genht/ht.h>
#undef HT

unsigned int htvlist_keyhash(htvlist_key_t k);
int htvlist_keyeq(htvlist_key_t a, htvlist_key_t b);

#endif
