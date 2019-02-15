/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
 */

#include <genht/htsp.h>
#include <genlist/gendlist.h>
#include "obj_common.h"

typedef htsp_t pcb_netlist_t;

typedef struct pcb_net_term_s pcb_net_term_t;


struct pcb_net_term_s {
	PCB_ANY_OBJ_FIELDS;
	char *refdes;
	char *term;
	gdl_elem_t link; /* a net is mainly an ordered list of terminals */
};

/* List of refdes-terminals */
#define TDL(x)      pcb_termlist_ ## x
#define TDL_LIST_T  pcb_termlist_t
#define TDL_ITEM_T  pcb_net_term_t
#define TDL_FIELD   link
#define TDL_SIZE_T  size_t
#define TDL_FUNC

#define pcb_termlist_foreach(list, iterator, loop_elem) \
	gdl_foreach_((&((list)->lst)), (iterator), (loop_elem))


#include <genlist/gentdlist_impl.h>
#include <genlist/gentdlist_undef.h>


struct pcb_net_s {
	PCB_ANY_OBJ_FIELDS;
	char *name;
	pcb_termlist_t conns;
};
