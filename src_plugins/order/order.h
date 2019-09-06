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

#ifndef PCB_ORDER_H
#define PCB_ORDER_H

#include <genvector/vtp0.h>
#include "hid_dad.h"

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
	vtp0_t names;
} order_ctx_t;

/* order implementation - registered by an order plugin */
typedef struct pcb_order_imp_s pcb_order_imp_t;
struct pcb_order_imp_s {
	const char *name;
	int (*enabled)(pcb_order_imp_t *imp);          /* returns 1 if the plugin is enabled */
	void (*populate_dad)(pcb_order_imp_t *imp, order_ctx_t *octx);
};

extern vtp0_t pcb_order_imps; /* of (pcb_order_imp_t *) items */

void pcb_order_reg(const pcb_order_imp_t *imp);

#endif
