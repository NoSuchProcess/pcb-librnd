/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - stackup import/export
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#include "config.h"
#include <stdio.h>
#include <genht/htsp.h>
#include <genht/hash.h>
#include "layer.h"
#include "safe_fs.h"
#include "error.h"
#include "parse.h"
#include "compat_misc.h"
#include "obj_line.h"
#include "obj_arc.h"
#include "vtc0.h"


int tedax_layer_fsave(pcb_board_t *pcb, pcb_layergrp_id_t gid, const char *layname, FILE *f)
{
	char lntmp[64];
	int lno;
	pcb_pline_t *pl;
	long plid;
	pcb_layergrp_t *g = pcb_get_layergrp(pcb, gid);

	if (g == NULL)
		return -1;

	if (layname == NULL)
		layname = g->name;
	if (layname == NULL) {
		layname = lntmp;
		sprintf(lntmp, "anon_%ld", gid);
	}

	for(lno = 0; lno < g->len; lno++) {
		pcb_layer_t *ly = pcb_get_layer(pcb->Data, g->lid[lno]);
		if (ly == NULL)
			continue;

		PCB_POLY_LOOP(ly) {
			pcb_pline_t *pl;
			if (!polygon->NoHolesValid)
				pcb_poly_compute_no_holes(polygon);

			for(pl = polygon->NoHoles, plid = 0; pl != NULL; pl = pl->next, plid++) {
				pcb_vnode_t *v;
				long i, n;
				fprintf(f, "begin polyline v1 pllay_%ld_%ld_%ld\n", gid, polygon->ID, plid);
				n = pl->Count;
				for(v = &pl->head, i = 0; i < n; v = v->next, i++)
					pcb_fprintf(f, " v %.06mm %.06mm\n", v->point[0], v->point[1]);
				fprintf(f, "end polyline\n");
			}
		} PCB_END_LOOP;
	}

	fprintf(f, "begin layer v1 %s\n", layname);
	for(lno = 0; lno < g->len; lno++) {
		pcb_layer_t *ly = pcb_get_layer(pcb->Data, g->lid[lno]);
		if (ly == NULL)
			continue;
		PCB_LINE_LOOP(ly) {
			pcb_fprintf(f, " line %.06mm %.06mm %.06mm %.06mm %.06mm %.06mm\n",
				line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y,
				line->Thickness, PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, line) ? pcb_round(line->Clearance/2) : 0);
		} PCB_END_LOOP;
		PCB_ARC_LOOP(ly) {
			pcb_coord_t sx, sy, ex, ey, clr;

			if (arc->Width != arc->Height)
				pcb_io_incompat_save(pcb->Data, arc, "arc", "Elliptical arc", "Saving as circular arc instead - geometry will differ");

			pcb_arc_get_end(arc, 0, &sx, &sy);
			pcb_arc_get_end(arc, 1, &ex, &ey);
			clr = PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, arc) ? pcb_round(arc->Clearance/2) : 0;

			pcb_fprintf(f, " arc %.06mm %.06mm %.06mm %f %f %.06mm %.06mm ",
				arc->X, arc->Y, arc->Width, arc->StartAngle, arc->Delta, arc->Thickness, clr);
			pcb_fprintf(f, "%.06mm %.06mm %.06mm %.06mm\n", sx, sy, ex, ey);
		} PCB_END_LOOP;
		PCB_TEXT_LOOP(ly) {
			pcb_fprintf(f, " text %.06mm %.06mm %.06mm %.06mm %d %f %.06mm ",
				text->bbox_naked.X1, text->bbox_naked.Y1, text->bbox_naked.X2, text->bbox_naked.Y2,
				text->Scale, text->rot, PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, text) ? 1 : 0);
			tedax_fprint_escape(f, text->TextString);
			fputc('\n', f);
		} PCB_END_LOOP;

		PCB_POLY_LOOP(ly) {
			for(pl = polygon->NoHoles, plid = 0; pl != NULL; pl = pl->next, plid++)
				pcb_fprintf(f, " pllay_%ld_%ld_%ld 0 0\n", gid, polygon->ID, plid);
		} PCB_END_LOOP;

	}
	fprintf(f, "end layer\n");
	return 0;
}

int tedax_layer_save(pcb_board_t *pcb, pcb_layergrp_id_t gid, const char *layname, const char *fn)
{
	int res;
	FILE *f;

	f = pcb_fopen(fn, "w");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "tedax_layer_save(): can't open %s for writing\n", fn);
		return -1;
	}
	fprintf(f, "tEDAx v1\n");
	res = tedax_layer_fsave(pcb, gid, layname, f);
	fclose(f);
	return res;
}

int tedax_layers_fload(pcb_data_t *data, FILE *f)
{
	long start, n;
	int argc, res = 0;
	char line[520];
	char *argv[16];
	htsp_t plines;
	htsp_entry_t *e;
	vtc0_t *coords;

	if (tedax_seek_hdr(f, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
		return -1;

	start = ftell(f);

	htsp_init(&plines, strhash, strkeyeq);
	while((argc = tedax_seek_block(f, "polyline", "v1", 1, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]))) > 1) {
		char *pname;
		if (htsp_has(&plines, argv[3])) {
			pcb_message(PCB_MSG_ERROR, "duplicate polyline: %s\n", argv[3]);
			res = -1;
			goto error;
		}
		pname = pcb_strdup(argv[3]);
		coords = malloc(sizeof(vtc0_t));
		vtc0_init(coords);

		while((argc = tedax_getline(f, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]))) >= 0) {

			if ((argc == 3) && (strcmp(argv[0], "v") == 0)) {
				pcb_bool s1, s2;
				vtc0_append(coords, pcb_get_value(argv[1], "mm", NULL, &s1));
				vtc0_append(coords, pcb_get_value(argv[2], "mm", NULL, &s2));
				if (!s1 || !s2) {
					pcb_message(PCB_MSG_ERROR, "invalid coords in polyline %s: %s;%s\n", pname, argv[1], argv[2]);
					res = -1;
					free(pname);
					goto error;
				}
			}
			else if ((argc == 2) && (strcmp(argv[0], "end") == 0) && (strcmp(argv[1], "polyline") == 0)) {
				break;
			}
			else {
				pcb_message(PCB_MSG_ERROR, "invalid command in polyline %s: %s\n", pname, argv[0]);
				res = -1;
				free(pname);
				goto error;
			}
		}

		htsp_insert(&plines, pname, coords);
	}

	fseek(f, start, SEEK_SET);

	while((argc = tedax_seek_block(f, "layer", "v1", 1, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]))) > 1) {
		pcb_trace("layer %s at %ld!\n", argv[3], ftell(f));

		while((argc = tedax_getline(f, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]))) >= 0) {
			if ((argc == 4) && (strcmp(argv[0], "poly") == 0)) {
				pcb_bool s1, s2;
				pcb_coord_t ox, oy;
				ox = pcb_get_value(argv[2], "mm", NULL, &s1);
				oy = pcb_get_value(argv[3], "mm", NULL, &s2);
				if (!s1 || !s2) {
					pcb_message(PCB_MSG_ERROR, "invalid coords in poly %s;%s\n", argv[2], argv[3]);
					res = -1;
					goto error;
				}
				coords = htsp_get(&plines, argv[1]);
				if (coords == NULL) {
					pcb_message(PCB_MSG_ERROR, "invalid polyline referecnce %s\n", argv[1]);
					res = -1;
					goto error;
				}
				pcb_trace("POLY: %mm %mm %s\n", ox, oy, argv[1]);
				for(n = 0; n < coords->used; n+=2)	{
					pcb_trace("  %mm %mm\n", ox+coords->array[n], oy+coords->array[n+1]);
				}
			}
		}
	}


	error:;
	for(e = htsp_first(&plines); e != NULL; e = htsp_next(&plines, e)) {
		free(e->key);
		vtc0_uninit(e->value);
		free(e->value);
	}
	htsp_uninit(&plines);
	return res;
}

int tedax_layers_load(pcb_data_t *data, const char *fn)
{
	int res;
	FILE *f;

	f = pcb_fopen(fn, "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "tedax_layers_load(): can't open %s for reading\n", fn);
		return -1;
	}
	res = tedax_layers_fload(data, f);
	fclose(f);
	return res;
}

