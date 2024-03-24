/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2024)
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
 *  This file is new in librnd an implements a more efficient special case
 *  solution to the no-hole dicing problem. Special casing rationale: this
 *  runs for rendering at every frame at least in sw-render.
 *
 *  The code is not reentrant: it temporary modifies pline flags of the input.
 *
 *  This file is the new API in polybool, not part of the original poly lib.
 *  It is based on callbacks instead of allocating new polylines.
 */

typedef struct pa_dic_ctx_s pa_dic_ctx_t;

typedef enum pa_dic_side_s {
	PA_DIC_H1 = 0, /* in pcb-rnd/svg coord system: north; in doc coord system: south */
	PA_DIC_V1,     /* in pcb-rnd/svg coord system: east; in doc coord system: east */
	PA_DIC_H2,     /* in pcb-rnd/svg coord system: south; in doc coord system: north */
	PA_DIC_V2,     /* in pcb-rnd/svg coord system: west; in doc coord system: west */
	PA_DIC_sides
} pa_dic_side_t;

typedef struct pa_dic_isc_s pa_dic_isc_t;

struct pa_dic_ctx_s {
	/* configuration and user input */
	rnd_box_t clip;
	void (*begin_pline)(pa_dic_ctx_t *ctx);
	void (*append_coord)(pa_dic_ctx_t *ctx, rnd_coord_t x, rnd_coord_t y);
	void (*end_pline)(pa_dic_ctx_t *ctx);
	void *user_data;

	/* private: per pa cache */
	vtp0_t side[PA_DIC_sides];
	pa_dic_isc_t *head;
	pa_dic_isc_t *corner[4];
	rnd_coord_t first_x, first_y;
	rnd_coord_t last_x, last_y;
	long num_emits;
	unsigned first:1;
	unsigned has_coord:1; /* last_* is waiting to be printed out */
};

/* Clip a the rectangular area defined by ctx->clip out of multi-island pa
   calling the emit callbacks defined in ctx */
void rnd_polyarea_clip_box_emit(pa_dic_ctx_t *ctx, rnd_polyarea_t *pa);

