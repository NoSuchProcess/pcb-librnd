#ifndef WIRE_H
#define WIRE_H

#include <stdlib.h>
#include <string.h>

#if defined(GVT_DONT_UNDEF) && defined(LST_DONT_UNDEF)
  #undef GVT_DONT_UNDEF
  #undef LST_DONT_UNDEF
  #include "cdt/point.h"
  #define GVT_DONT_UNDEF
  #define LST_DONT_UNDEF
#else
  #include "cdt/typedefs.h"
#endif

typedef struct wirelist_node_s wirelist_node_t;

typedef struct {
	point_t *p;
	enum {
		SIDE_LEFT = (1<<0),
		SIDE_RIGHT = (1<<1),
		SIDE_TERM = (1<<1)|(1<<0)
	} side;
  wirelist_node_t *wire_node;
} wire_point_t;

typedef struct {
	int point_num;
	int point_max;
	wire_point_t *points;
} wire_t;

typedef wire_t* wire_ptr_t;

void wire_init(wire_t *w);
void wire_uninit(wire_t *w);
void wire_push_point(wire_t *w, point_t *p, int side);
void wire_pop_point(wire_t *w);
void wire_copy(wire_t *dst, wire_t *src);
void wire_print(wire_t *w, const char *tab);


/* List */
#define LST(x) wirelist_ ## x
#define LST_ITEM_T wire_ptr_t
#define LST_DONT_TYPEDEF_NODE

#include "cdt/list/list.h"

#ifndef LST_DONT_UNDEF
	#undef LST
	#undef LST_ITEM_T
	#undef LST_DONT_TYPEDEF_NODE
#endif

#define WIRELIST_FOREACH(_loop_item_, _list_) do { \
	wirelist_node_t *_node_ = _list_; \
	while (_node_ != NULL) { \
		wire_t *_loop_item_ = _node_->item;

#define WIRELIST_FOREACH_END() \
		_node_ = _node_->next; \
	} \
} while(0)


/* Vector */
#define GVT(x) vtwire_ ## x
#define GVT_ELEM_TYPE wire_ptr_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 4096
#define GVT_START_SIZE 32
#define GVT_FUNC
#define GVT_SET_NEW_BYTES_TO 0
#define GVT_ELEM_CONSTRUCTOR
#define GVT_ELEM_DESTRUCTOR
#define GVT_ELEM_COPY

#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
int GVT(constructor)(GVT(t) *vect, GVT_ELEM_TYPE *elem);
void GVT(destructor)(GVT(t) *vect, GVT_ELEM_TYPE *elem);
#include <genvector/genvector_undef.h>

#define VTWIRE_FOREACH(_loop_item_, _vt_) do { \
	int _i_; \
	for (_i_ = 0; _i_ < vtwire_len(_vt_); _i_++) { \
		wire_t *_loop_item_ = (_vt_)->array[_i_];

#define VTWIRE_FOREACH_END() \
	} \
} while(0)

#endif
