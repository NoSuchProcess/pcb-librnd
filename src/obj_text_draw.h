/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/*** Standard draw of text ***/


/* Include rtree.h for these */
#ifdef PCB_RTREE_H
pcb_r_dir_t pcb_text_draw_callback(const pcb_box_t * b, void *cl);
#endif

void pcb_text_draw_(pcb_text_t *Text, pcb_coord_t min_line_width);
void pcb_text_invalidate_erase(pcb_layer_t *Layer, pcb_text_t *Text);
void pcb_text_invalidate_draw(pcb_layer_t *Layer, pcb_text_t *Text);
void pcb_text_draw_xor(pcb_text_t *text, pcb_coord_t x, pcb_coord_t y);

