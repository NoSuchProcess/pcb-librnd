/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2023 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2023)
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
 *  This is a full rewrite of pcb-rnd's (and PCB's) polygon lib originally
 *  written by Harry Eaton in 2006, in turn building on "poly_Boolean: a
 *  polygon clip library" by Alexey Nikitin, Michael Leonov from 1997 and
 *  "nclip: a polygon clip library" Klamer Schutte from 1993.
 *
 *  English translation of the original paper the lib is largely based on:
 *  https://web.archive.org/web/20160418014630/http://www.complex-a5.ru/polyboolean/downloads/polybool_eng.pdf
 *
 */

/* poly bool internals */

#include <librnd/polybool/pa_config.h>

#ifdef PA_BIGCOORD_ISC

#include "big_coord.h"
#define pa_big_copy(dst, src)  memcpy(&(dst), &(src), sizeof(pa_big_vector_t));
#define pa_vnode_equ(vna, vnb) pa_big_vnode_vnode_equ(vna, vnb)

/* Calculates the cross product and returns its sign (+1, 0 or -1)
   See also (same as): PA_CIN_CROSS_SMALL */
int pa_big_vnode_vnode_cross_sgn(pa_big_vector_t sv, pa_big_vector_t sv_next, pa_big_vector_t pt);

#else

typedef double pa_big_angle_t;
#define pa_angle_equ(a, b)   ((a) == (b))
#define pa_angle_lt(a, b)    ((a) < (b))
#define pa_angle_gte(a, b)   ((a) >= (b))
#define pa_angle_lte(a, b)   ((a) <= (b))
#define pa_angle_valid(a)    (((a) >= 0.0) && ((a) <= 4.0))

typedef rnd_vector_t pa_big_vector_t;
#define pa_big_copy(dst, src)  memcpy((dst), (src), sizeof(pa_big_vector_t));
#define pa_big_double(crd)  ((double)(crd))
#define pa_big_vnxd(vn)     pa_big_double((vn)->point[0])
#define pa_big_vnyd(vn)     pa_big_double((vn)->point[0])

#define pa_vnode_equ(vna, vnb) Vequ2((vna)->point, (vnb)->point)

#endif

/* A "descriptor" that makes up a "(cross vertex) connectivity list" in the paper */
struct pa_conn_desc_s {
	pa_big_angle_t angle;
	rnd_vnode_t *parent;                 /* the point this descriptor is for */
	pa_conn_desc_t *prev, *next, *head;  /* "connectivity list": a list of all the related descriptors */
	char poly;                           /* 'A' or 'B' */
	char side;                           /* 'P' for previous 'N' for next (the other endpoint of edge of interest from ->parent) */
	pa_big_vector_t isc;                 /* (precise) coords of the intersection */
	unsigned prelim:1;                   /* preliminary allocation: isc is set but other fields are blank */
	unsigned ignore:1;                   /* do not consider this when jumping to the next/prev cvc - e.g. this is a stub */

	rnd_vnode_t *PP_OTHER;               /* used only in big_postproc to link point-point overlap nodes */
};


typedef enum pa_pline_label_e {
	PA_PLL_UNKNWN  = 0,
	PA_PLL_INSIDE  = 1,
	PA_PLL_OUTSIDE = 2,
	PA_PLL_ISECTED = 3
} pa_pline_label_t;

typedef enum pa_plinept_label_e {
	PA_PTL_UNKNWN  = 0,
	PA_PTL_INSIDE  = 1,
	PA_PTL_OUTSIDE = 2,
	PA_PTL_SHARED  = 3,
	PA_PTL_SHARED2 = 4
} pa_plinept_label_t;

#define PA_ISC_TOUCHES 99


/* ugly hack: long-jump out to the error handler e (assumed to exist in
   current scope, normally received as a function argument) with a
   non-zero error code */
#define pa_error(code)  longjmp(*(e), code)

/* standard error codes for pa_error() */
enum {
	pa_err_no_memory = 2,
	pa_err_bad_parm = 3,
	pa_err_ok = 0
};

/* A segment is an object of the pline between ->v and ->v->next, upgraded
   with a bounding box so that it can be added to an rtree. Also remembers
   its parent pline, plus random temporary fields used in the calculations */
typedef struct pa_seg_s {
	rnd_box_t box;
	rnd_vnode_t *v;
	rnd_pline_t *p;
	int intersected;
} pa_seg_t;

void rnd_poly_copy_edge_tree(rnd_pline_t *dst, const rnd_pline_t *src);


/* Returns whether pl intersects with the line segment specified in lx;ly; if
   it does, and cx and cy are not NULL, fill in cx;cy with the first
   intersection point */
rnd_bool rnd_pline_isect_line(rnd_pline_t *pl, rnd_coord_t lx1, rnd_coord_t ly1, rnd_coord_t lx2, rnd_coord_t ly2, rnd_coord_t *cx, rnd_coord_t *cy);

/* Update the bbox of pl using pt's coords */
RND_INLINE void pa_pline_box_bump(rnd_pline_t *pl, const rnd_vector_t pt)
{
	if (pt[0]     < pl->xmin) pl->xmin = pt[0];
	if ((pt[0]+1) > pl->xmax) pl->xmax = pt[0]+1;
	if (pt[1]     < pl->ymin) pl->ymin = pt[1];
	if ((pt[1]+1) > pl->ymax) pl->ymax = pt[1]+1;
}

int pa_cvc_crossing_at_node(rnd_vnode_t *nd);
int pa_cvc_line_line_overlap(rnd_vnode_t *na, rnd_vnode_t *nb);

pa_conn_desc_t *pa_add_conn_desc(rnd_pline_t *pl, char poly, pa_conn_desc_t *list);
pa_conn_desc_t *pa_add_conn_desc_at(rnd_vnode_t *node, char poly, pa_conn_desc_t *list);
pa_conn_desc_t *pa_prealloc_conn_desc(pa_big_vector_t isc);

int rnd_polyarea_boolean_free_nochk(rnd_polyarea_t *a_, rnd_polyarea_t *b_, rnd_polyarea_t **res, int op);
