/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1998,1999,2000,2001 harry eaton
 *
 *  this file, heap.h, was written and is
 *  Copyright (c) 2001 C. Scott Ananian.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  harry eaton, 6697 Buttonhole Ct, Columbia, MD 21044 USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

/* Heap used by the polygon code and autoroute */

#ifndef PCB_HEAP_H
#define PCB_HEAP_H

#include "config.h"

/* type of heap costs */
typedef double pcb_cost_t;
/* what a heap looks like */
typedef struct pcb_heap_s pcb_heap_t;

/* create an empty heap */
pcb_heap_t *pcb_heap_create();
/* destroy a heap */
void pcb_heap_destroy(pcb_heap_t ** heap);
/* free all elements in a heap */
void pcb_heap_free(pcb_heap_t * heap, void (*funcfree) (void *));

/* -- mutation -- */
void pcb_heap_insert(pcb_heap_t * heap, pcb_cost_t cost, void *data);
void *pcb_heap_remove_smallest(pcb_heap_t * heap);
/* replace the smallest item with a new item and return the smallest item.
 * (if the new item is the smallest, than return it, instead.) */
void *pcb_heap_replace(pcb_heap_t * heap, pcb_cost_t cost, void *data);

/* -- interrogation -- */
int pcb_heap_is_empty(pcb_heap_t * heap);
int pcb_heap_size(pcb_heap_t * heap);

#endif /* PCB_HEAP_H */
