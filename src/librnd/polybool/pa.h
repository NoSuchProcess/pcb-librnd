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

#ifdef PB_RATIONAL_ISC
#include "isc_rational.h"
#define pa_big_copy(dst, src)  memcpy(&(dst), &(src), sizeof(pa_big_vector_t));
#else
typedef rnd_vector_t pa_big_vector_t;
#define pa_big_copy(dst, src)  memcpy((dst), (src), sizeof(pa_big_vector_t));

#endif

/* A "descriptor" that makes up a "(cross vertex) connectivity list" in the paper */
struct pa_conn_desc_s {
	double angle;
	rnd_vnode_t *parent;                 /* the point this descriptor is for */
	pa_conn_desc_t *prev, *next, *head;  /* "connectivity list": a list of all the related descriptors */
	char poly;                           /* 'A' or 'B' */
	char side;                           /* 'P' for previous 'N' for next (the other endpoint of edge of interest from ->parent) */
	pa_big_vector_t isc;                 /* (precise) coords of the intersection */
	unsigned prelim:1;                   /* preliminary allocation: isc is set but other fields are blank */
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

