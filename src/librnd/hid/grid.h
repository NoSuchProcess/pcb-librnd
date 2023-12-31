/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#ifndef RND_GRID_H
#define RND_GRID_H

#define RND_MAX_GRID         RND_MIL_TO_COORD(1000)

#include <genvector/gds_char.h>
#include <librnd/core/rnd_bool.h>
#include <librnd/core/global_typedefs.h>
#include <librnd/core/unit.h>

/* String packed syntax (bracket means optional):

     [name:]size[@offs][!unit]

  "!" means to switch the UI to the unit specified. */
typedef struct {
	char *name;
	rnd_coord_t size;
	rnd_coord_t ox, oy;
	const rnd_unit_t *unit; /* force switching to unit if not NULL */
} rnd_grid_t;

/* Returns the nearest grid-point to the given coord x */
rnd_coord_t rnd_grid_fit(rnd_coord_t x, rnd_coord_t grid_spacing, rnd_coord_t grid_offset);

/* Parse packed string format src into dst; allocat dst->name on success */
rnd_bool_t rnd_grid_parse(rnd_grid_t *dst, const char *src);

/* Free all allocated fields of a grid struct */
void rnd_grid_free(rnd_grid_t *dst);

/* Convert src into packed string format */
rnd_bool_t rnd_grid_append_print(gds_t *dst, const rnd_grid_t *src);
char *rnd_grid_print(const rnd_grid_t *src);

/* Apply grid settings from src to the design */
void rnd_grid_set(rnd_design_t *design, const rnd_grid_t *src);

/* Jump to grid index dst (clamped); absolute set */
rnd_bool_t rnd_grid_list_jump(rnd_design_t *design, int dst);

/* Step stp steps (can be 0) on the grids list and set the resulting grid; relative set */
rnd_bool_t rnd_grid_list_step(rnd_design_t *design, int stp);

/* invalidate the grid index; call this when changing the grid settings */
void rnd_grid_inval(void);

/* Reinstall grid menus */
void rnd_grid_install_menu(void);

void rnd_grid_init(void);
void rnd_grid_uninit(void);

/* sets cursor grid with respect to grid spacing, offset and unit values */
void rnd_hid_set_grid(rnd_design_t *design, rnd_coord_t Grid, rnd_bool align, rnd_coord_t ox, rnd_coord_t oy);
void rnd_hid_set_unit(rnd_design_t *design, const rnd_unit_t *new_unit);

#endif
