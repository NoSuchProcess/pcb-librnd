#ifndef SPOKE_H
#define SPOKE_H

#include "sktypedefs.h"
#include "obj_common.h"
#include "cdt/point.h"

#include <genvector/vtp0.h>


typedef enum {
  SPOKE_DIR_1PI4 = 0,
  SPOKE_DIR_3PI4 = 1,
  SPOKE_DIR_5PI4 = 2,
  SPOKE_DIR_7PI4 = 3
} spoke_dir_t;

struct spoke_s {
  spoke_dir_t dir;
	pcb_box_t bbox;
	vtp0_t slots;
  point_t *p;
};


void spoke_pos_at_wire_node(spoke_t *sp, wirelist_node_t *w_node, pcb_coord_t *x, pcb_coord_t *y);

#endif
