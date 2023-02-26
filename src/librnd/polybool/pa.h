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

/* A "descriptor" that makes up a "connectivity list" in the paper */
struct pa_conn_desc_s {
	double angle;
	rnd_vnode_t *parent;                 /* the point this descriptor is for */
	pa_conn_desc_t *prev, *next, *head;  /* "connectivity list": a list of all the related descriptors */
	char poly;                           /* 'A' or 'B' */
	char side;                           /* 'P' for previous 'N' for next */
};

#define PA_CONN_DESC_INVALID ((pa_conn_desc_t *)-1)


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



