/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2023, 2024 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2023,2024)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.*
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* poly bool internals */

#ifndef POLYBOOL_PA_H
#define POLYBOOL_PA_H

#include "pa_config.h"

typedef double pa_angle_t;
#define pa_angle_equ(a, b)   ((a) == (b))
#define pa_angle_lt(a, b)    ((a) < (b))
#define pa_angle_gt(a, b)    ((a) > (b))
#define pa_angle_gte(a, b)   ((a) >= (b))
#define pa_angle_lte(a, b)   ((a) <= (b))
#define pa_angle_valid(a)    (((a) >= 0.0) && ((a) <= 4.0))

#define pa_vnode_equ(vna, vnb) Vequ2((vna)->point, (vnb)->point)

typedef enum pa_pline_label_e {
	PA_PLL_UNKNWN  = 0
} pa_pline_label_t;

typedef enum pa_plinept_label_e {
	PA_PTL_UNKNWN  = 0
} pa_plinept_label_t;

#define PA_ISC_TOUCHES 99


enum {
	pa_err_ok = 0
};

/* A segment is an object of the pline between ->v and ->v->next, upgraded
   with a bounding box so that it can be added to an rtree. Also remembers
   its parent pline, plus random temporary fields used in the calculations */
typedef struct pa_seg_s {
	rnd_box_t box;
	rnd_vnode_t *v;
	rnd_pline_t *p;
} pa_seg_t;

void rnd_poly_copy_edge_tree(rnd_pline_t *dst, const rnd_pline_t *src);


/* Returns whether pl intersects with the line segment specified in lx;ly; if
   it does, and cx and cy are not NULL, fill in cx;cy with the first
   intersection point */
rnd_bool rnd_pline_isect_line(rnd_pline_t *pl, rnd_coord_t lx1, rnd_coord_t ly1, rnd_coord_t lx2, rnd_coord_t ly2, rnd_coord_t *cx, rnd_coord_t *cy);

/* Update the bbox of pl using pt's coords */
RND_INLINE void pa_pline_box_bump(rnd_pline_t *pl, const rnd_vector_t pt)
{
	TODO("arc: bbox: the code below works only for lines");
	if (pt[0]     < pl->xmin) pl->xmin = pt[0];
	if ((pt[0]+1) > pl->xmax) pl->xmax = pt[0]+1;
	if (pt[1]     < pl->ymin) pl->ymin = pt[1];
	if ((pt[1]+1) > pl->ymax) pl->ymax = pt[1]+1;
}

int rnd_polyarea_boolean_free_nochk(rnd_polyarea_t *a_, rnd_polyarea_t *b_, rnd_polyarea_t **res, int op);

int pa_pline_is_point_on_seg(pa_seg_t *s, rnd_vector_t pt);

void pa_calc_angle_nn(pa_angle_t *dst, rnd_vector_t PT, rnd_vector_t OTHER);

#define pa_swap(type,a,b) \
do { \
	type __tmp__ = a; \
	a = b; \
	b = __tmp__; \
} while(0)

#endif
