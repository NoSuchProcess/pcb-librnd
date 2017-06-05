/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
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
 */

#include "config.h"

#include "buffer.h"
#include "board.h"
#include "crosshair.h"
#include "data.h"
#include "error.h"
#include "obj_subc.h"
#include "obj_subc_op.h"

pcb_subc_t *pcb_subc_alloc(void)
{
	pcb_subc_t *sc;
	sc = calloc(sizeof(pcb_subc_t), 1);
	sc->data = pcb_data_new(NULL);
	PCB_SET_PARENT(sc->data, subc, sc);
	return sc;
}

pcb_layer_t *pcb_subc_layer_create(pcb_subc_t *sc, pcb_layer_t *src)
{
	pcb_layer_t *dst = &sc->data->Layer[sc->data->LayerN++];
	dst->parent = sc->data;
	dst->grp = src->grp;
	dst->comb = src->comb;
	
	dst->line_tree = src->line_tree;
	dst->text_tree = src->text_tree;
	dst->polygon_tree = src->polygon_tree;
	dst->subc_tree = src->subc_tree;


	dst->meta.bound.real = src;
	dst->meta.bound.type = pcb_layergrp_flags(PCB, src->grp);

	if (dst->meta.bound.type & PCB_LYT_INTERN) {
#warning TODO: calculate inner layer stack offset - needs a stack
		dst->meta.bound.stack_offs = 0;
	}
	else
		dst->meta.bound.stack_offs = 0;
	return dst;
}

int pcb_subc_convert_from_buffer(pcb_buffer_t *buffer)
{
	pcb_subc_t *sc;
	int n;

	sc = pcb_subc_alloc();
	PCB_SET_PARENT(sc->data, data, buffer->Data);
	pcb_subclist_append(&buffer->Data->subc, sc);

	/* create layer matches and copy objects */
	for(n = 0; n < PCB_MAX_LAYER; n++) {
		pcb_layer_t *dst, *src;
		pcb_line_t *line;
		pcb_text_t *text;
		pcb_polygon_t *poly;
		pcb_arc_t *arc;

		if (pcb_layer_is_pure_empty(&buffer->Data->Layer[n]))
			continue;

		src = &buffer->Data->Layer[n];
		dst = pcb_subc_layer_create(sc, src);

		while((line = linelist_first(&src->Line)) != NULL) {
			linelist_remove(line);
			linelist_append(&dst->Line, line);
			PCB_SET_PARENT(line, subc, sc);
		}

		while((arc = arclist_first(&src->Arc)) != NULL) {
			arclist_remove(arc);
			arclist_append(&dst->Arc, arc);
			PCB_SET_PARENT(arc, subc, sc);
		}

		while((text = textlist_first(&src->Text)) != NULL) {
			textlist_remove(text);
			textlist_append(&dst->Text, text);
			PCB_SET_PARENT(text, subc, sc);
		}

		while((poly = polylist_first(&src->Polygon)) != NULL) {
			polylist_remove(poly);
			polylist_append(&dst->Polygon, poly);
			PCB_SET_PARENT(poly, subc, sc);
		}
	}

	pcb_message(PCB_MSG_ERROR, "pcb_subc_convert_from_buffer(): not yet implemented\n");
	return -1;
}

void XORDrawSubc(pcb_subc_t *sc, pcb_coord_t DX, pcb_coord_t DY)
{
	int n;

	for(n = 0; n < sc->data->LayerN; n++) {
		pcb_layer_t *ly = sc->data->Layer + n;
		pcb_line_t *line;
		pcb_text_t *text;
		pcb_polygon_t *poly;
		pcb_arc_t *arc;
		gdl_iterator_t it;

		linelist_foreach(&ly->Line, &it, line)
			pcb_gui->draw_line(pcb_crosshair.GC, DX + line->Point1.X, DY + line->Point1.Y, DX + line->Point2.X, DY + line->Point2.Y);

		arclist_foreach(&ly->Arc, &it, arc)
			pcb_gui->draw_arc(pcb_crosshair.GC, DX + arc->X, DY + arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta);

		polylist_foreach(&ly->Polygon, &it, poly)
			XORPolygon(poly, DX, DY, 0);

		textlist_foreach(&ly->Text, &it, text)
			XORDrawText(text, DX, DY);
	}

	/* mark */
	pcb_gui->draw_line(pcb_crosshair.GC, DX - PCB_EMARK_SIZE, DY, DX, DY - PCB_EMARK_SIZE);
	pcb_gui->draw_line(pcb_crosshair.GC, DX + PCB_EMARK_SIZE, DY, DX, DY - PCB_EMARK_SIZE);
	pcb_gui->draw_line(pcb_crosshair.GC, DX - PCB_EMARK_SIZE, DY, DX, DY + PCB_EMARK_SIZE);
	pcb_gui->draw_line(pcb_crosshair.GC, DX + PCB_EMARK_SIZE, DY, DX, DY + PCB_EMARK_SIZE);
}


pcb_subc_t *pcb_subc_dup(pcb_board_t *pcb, pcb_data_t *dst, pcb_subc_t *src)
{
	int n;
	pcb_subc_t *sc = pcb_subc_alloc();
	PCB_SET_PARENT(sc->data, data, dst);
	pcb_subclist_append(&dst->subc, sc);

	/* make a copy */
	for(n = 0; n < src->data->LayerN; n++) {
		pcb_layer_t *sl = src->data->Layer + n;
		pcb_layer_t *dl = sc->data->Layer + n;
		pcb_line_t *line, *nline;
		pcb_text_t *text;
		pcb_polygon_t *poly;
		pcb_arc_t *arc, *narc;

		memcpy(&dl->meta.bound, &sl->meta.bound, sizeof(sl->meta.bound));


		dl->meta.bound.real = NULL;
		/* bind layer to the stack provided by pcb/dst */
		if (pcb != NULL) {
			int l;
			pcb_layergrp_id_t gid;
			if (pcb_layergrp_list(pcb, dl->meta.bound.type, &gid, 1) == 1) {
				pcb_layergrp_t *grp = pcb->LayerGroups.grp+gid;
				for(l = 0; l < grp->len; l++) {
					pcb_layer_t *ly = pcb_get_layer(grp->lid[l]);
					if (ly->comb == sl->meta.bound.comb) {
						dl->meta.bound.real = ly;
						break;
					}
				}
			}
#warning TODO: calculate inner layer stack offset
		}

		if (dl->meta.bound.real == NULL)
			pcb_message(PCB_MSG_WARNING, "Couldn't bind a layer of subcricuit TODO while placing it\n");

		while((line = linelist_first(&sl->Line)) != NULL) {
			nline = pcb_line_dup(dl, line);
			if (nline != NULL)
				PCB_SET_PARENT(nline, layer, dl);
		}

		while((arc = arclist_first(&sl->Arc)) != NULL) {
			narc = pcb_arc_dup(dl, arc);
			if (narc != NULL)
			PCB_SET_PARENT(arc, layer, dl);
		}

		while((text = textlist_first(&sl->Text)) != NULL) {
/*			PCB_SET_PARENT(text, layer, dl);*/
		}

		while((poly = polylist_first(&sl->Polygon)) != NULL) {
/*			PCB_SET_PARENT(poly, layer, dl);*/
		}

	}
}

/* copies a subcircuit onto the PCB.  Then does a draw. */
void *CopySubc(pcb_opctx_t *ctx, pcb_subc_t *src)
{
	pcb_subc_t *sc;

	sc = pcb_subc_dup(PCB, PCB->Data, src);

	/* this call clears the polygons */
/*	pcb_undo_add_obj_to_create(PCB_TYPE_ELEMENT, element, element, element);*/

/*	DrawElementPinsAndPads(element);*/
	return (sc);
}

