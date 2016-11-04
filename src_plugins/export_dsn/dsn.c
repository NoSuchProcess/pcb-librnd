/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *
 *  Specctra .dsn export HID
 *  Copyright (C) 2008, 2011 Josh Jordan, Dan McMahill, and Jared Casper
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

/*
This program exports specctra .dsn files from geda .pcb files.
By Josh Jordan and Dan McMahill, modified from bom.c
  -- Updated to use Coord and other fixes by Jared Casper 16 Sep 2011
*/

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>

#include "board.h"
#include "data.h"
#include "error.h"
#include "misc.h"
#include "rats.h"
#include "buffer.h"
#include "change.h"
#include "draw.h"
#include "set.h"
#include "undo.h"
#include "pcb-printf.h"
#include "create.h"
#include "polygon.h"
#include "compat_misc.h"
#include "layer.h"

#include "hid.h"
#include "hid_draw_helpers.h"
#include "hid_nogui.h"
#include "hid_actions.h"
#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_helper.h"
#include "plugins.h"
#include "obj_line.h"

static const char *dsn_cookie = "dsn exporter";

static Coord trackwidth = 8;  /* user options defined in export dialog */
static Coord clearance = 8;
static Coord viawidth = 45;
static Coord viadrill = 25;

static HID dsn_hid;

static HID_Attribute dsn_options[] = {
	{"dsnfile", "SPECCTRA output file",
	 HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_dsnfile 0
	{"trackwidth", "track width in mils",
	 HID_Coord, PCB_MIL_TO_COORD(0), PCB_MIL_TO_COORD(100), {0, 0, 0, PCB_MIL_TO_COORD(8)}, 0, 0},
#define HA_trackwidth 1
	{"clearance", "clearance in mils",
	 HID_Coord, PCB_MIL_TO_COORD(0), PCB_MIL_TO_COORD(100), {0, 0, 0, PCB_MIL_TO_COORD(8)}, 0, 0},
#define HA_clearance 2
	{"viawidth", "via width in mils",
	 HID_Coord, PCB_MIL_TO_COORD(0), PCB_MIL_TO_COORD(100), {0, 0, 0, PCB_MIL_TO_COORD(27)}, 0,
	 0},
#define HA_viawidth 3
	{"viadrill", "via drill diameter in mils",
	 HID_Coord, PCB_MIL_TO_COORD(0), PCB_MIL_TO_COORD(100), {0, 0, 0, PCB_MIL_TO_COORD(15)}, 0,
	 0},
#define HA_viadrill 4
};

#define NUM_OPTIONS (sizeof(dsn_options)/sizeof(dsn_options[0]))
REGISTER_ATTRIBUTES(dsn_options, dsn_cookie)

static HID_Attr_Val dsn_values[NUM_OPTIONS];

static const char *dsn_filename;

static HID_Attribute *dsn_get_export_options(int *n)
{
	static char *last_dsn_filename = 0;
	if (PCB) {
		derive_default_filename(PCB->Filename, &dsn_options[HA_dsnfile], ".dsn", &last_dsn_filename);
	}
	if (n)
		*n = NUM_OPTIONS;
	return dsn_options;
}


/* this function is mostly ripped from bom.c */
static PointType get_centroid(ElementType * element)
{
	PointType centroid;
	double sumx = 0.0, sumy = 0.0;
	int pin_cnt = 0;

	PIN_LOOP(element);
	{
		sumx += (double) pin->X;
		sumy += (double) pin->Y;
		pin_cnt++;
	}
	END_LOOP;

	PAD_LOOP(element);
	{
		sumx += (pad->Point1.X + pad->Point2.X) / 2.0;
		sumy += (pad->Point1.Y + pad->Point2.Y) / 2.0;
		pin_cnt++;
	}
	END_LOOP;

	if (pin_cnt > 0) {
		centroid.X = sumx / (double) pin_cnt;
		centroid.Y = PCB->MaxHeight - (sumy / (double) pin_cnt);
	}
	else {
		centroid.X = 0;
		centroid.Y = 0;
	}
	return centroid;
}

static GList *layerlist = NULL;  /* contain routing layers */

static void print_structure(FILE * fp)
{
	int group, top_group, bot_group, top_layer, bot_layer;

	pcb_layer_group_list(PCB_LYT_TOP | PCB_LYT_SILK, &top_group, 1);
	pcb_layer_group_list(PCB_LYT_BOTTOM | PCB_LYT_SILK, &bot_group, 1);

	top_layer = PCB->LayerGroups.Entries[top_group][0];
	bot_layer = PCB->LayerGroups.Entries[bot_group][0];


	g_list_free(layerlist);				/* might be around from the last export */

	if (PCB->Data->Layer[top_layer].On) {
		layerlist = g_list_append(layerlist, &PCB->Data->Layer[top_layer]);
	}
	else {
		gui->log("WARNING! DSN export does not include the top layer. "
						 "Router will consider an inner layer to be the \"top\" layer.\n");
	}

	for (group = 0; group < max_group; group++) {
		LayerType *first_layer;
		if (group == top_group || group == bot_group)
			continue;

		if (PCB->LayerGroups.Number[group] < 1)
			continue;

		first_layer = &PCB->Data->Layer[PCB->LayerGroups.Entries[group][0]];
		if (!first_layer->On)
			continue;

		layerlist = g_list_append(layerlist, first_layer);

		if (group < top_group) {
			gui->log("WARNING! DSN export moved layer group with the \"%s\" layer "
							 "after the top layer group.  DSN files must have the top " "layer first.\n", first_layer->Name);
		}

		if (group > bot_group) {
			gui->log("WARNING! DSN export moved layer group with the \"%s\" layer "
							 "before the bottom layer group.  DSN files must have the " "bottom layer last.\n", first_layer->Name);
		}

		GROUP_LOOP(PCB->Data, group);
		{
			if (entry > 0) {
				gui->log("WARNING! DSN export squashed layer \"%s\" into layer "
								 "\"%s\", DSN files do not have layer groups.", layer->Name, first_layer->Name);
			}
		}
		END_LOOP;
	}

	if (PCB->Data->Layer[bot_layer].On) {
		layerlist = g_list_append(layerlist, &PCB->Data->Layer[bot_layer]);
	}
	else {
		gui->log("WARNING! DSN export does not include the bottom layer. "
						 "Router will consider an inner layer to be the \"bottom\" layer.\n");
	}

	fprintf(fp, "  (structure\n");

	for (GList * iter = layerlist; iter; iter = g_list_next(iter)) {
		LayerType *layer = iter->data;
		char *layeropts = pcb_strdup("(type signal)");
		/* see if layer has same name as a net and make it a power layer */
		/* loop thru all nets */
		for (int ni = 0; ni < PCB->NetlistLib[NETLIST_EDITED].MenuN; ni++) {
			char *nname;
			nname = PCB->NetlistLib[NETLIST_EDITED].Menu[ni].Name + 2;
			if (!strcmp(layer->Name, nname)) {
				g_free(layeropts);
				layeropts = pcb_strdup_printf("(type power) (use_net \"%s\")", layer->Name);
			}
		}
		fprintf(fp, "    (layer \"%s\"\n", layer->Name);
		fprintf(fp, "      %s\n", layeropts);
		fprintf(fp, "    )\n");
		g_free(layeropts);
	}

	/* PCB outline */
	pcb_fprintf(fp, "    (boundary\n");
	pcb_fprintf(fp, "      (rect pcb 0.0 0.0 %.6mm %.6mm)\n", PCB->MaxWidth, PCB->MaxHeight);
	pcb_fprintf(fp, "    )\n");
	pcb_fprintf(fp, "    (via via_%ld_%ld)\n", viawidth, viadrill);

	/* DRC rules */
	pcb_fprintf(fp, "    (rule\n");
	pcb_fprintf(fp, "      (width %mm)\n", trackwidth);
	pcb_fprintf(fp, "      (clear %mm)\n", clearance);
	pcb_fprintf(fp, "      (clear %mm (type wire_area))\n", clearance);
	pcb_fprintf(fp, "      (clear %mm (type via_smd via_pin))\n", clearance);
	pcb_fprintf(fp, "      (clear %mm (type smd_smd))\n", clearance);
	pcb_fprintf(fp, "      (clear %mm (type default_smd))\n", clearance);
	pcb_fprintf(fp, "    )\n  )\n");
}

static void print_placement(FILE * fp)
{
	fprintf(fp, "  (placement\n");
	ELEMENT_LOOP(PCB->Data);
	{
		char *ename;
		PointType ecentroid = get_centroid(element);
		char *side = TEST_FLAG(PCB_FLAG_ONSOLDER, element) ? "back" : "front";
		ename = NAMEONPCB_NAME(element);
		if (ename != NULL)
			ename = pcb_strdup(ename);
		else
			ename = pcb_strdup("null");
		pcb_fprintf(fp, "    (component %d\n", element->ID);
		pcb_fprintf(fp, "      (place \"%s\" %.6mm %.6mm %s 0 (PN 0))\n", ename, ecentroid.X, ecentroid.Y, side);
		pcb_fprintf(fp, "    )\n");
		g_free(ename);
	}
	END_LOOP;

	VIA_LOOP(PCB->Data);
	{ /* add mounting holes */
		pcb_fprintf(fp, "    (component %d\n", via->ID);
		pcb_fprintf(fp, "      (place %d %.6mm %.6mm %s 0 (PN 0))\n", via->ID, via->X, (PCB->MaxHeight - via->Y), "front");
		pcb_fprintf(fp, "    )\n");
	}
	END_LOOP;
	fprintf(fp, "  )\n");
}

static void print_library(FILE * fp)
{
	GList *pads = NULL, *iter; /* contain unique pad names */
	gchar *padstack;
	fprintf(fp, "  (library\n");
	ELEMENT_LOOP(PCB->Data);
	{
		int partside = TEST_FLAG(PCB_FLAG_ONSOLDER, element) ? g_list_length(layerlist) - 1 : 0;
		int partsidesign = TEST_FLAG(PCB_FLAG_ONSOLDER, element) ? -1 : 1;
		PointType centroid = get_centroid(element);
		fprintf(fp, "    (image %ld\n", element->ID); /* map every element by ID */
		/* loop thru pins and pads to add to image */
		PIN_LOOP(element);
		{
			Coord ty;
			Coord pinthickness;
			Coord lx, ly;  /* hold local pin coordinates */
			ty = PCB->MaxHeight - pin->Y;
			pinthickness = pin->Thickness;
			if (TEST_FLAG(PCB_FLAG_SQUARE, pin))
				padstack = pcb_strdup_printf("Th_square_%mI", pinthickness);
			else
				padstack = pcb_strdup_printf("Th_round_%mI", pinthickness);
			lx = (pin->X - centroid.X) * partsidesign;
			ly = (centroid.Y - ty) * -1;

			if (!pin->Number) { /* if pin is null just make it a keepout */
				for (GList * iter = layerlist; iter; iter = g_list_next(iter)) {
					LayerType *lay = iter->data;
					pcb_fprintf(fp, "      (keepout \"\" (circle \"%s\" %.6mm %.6mm %.6mm))\n", lay->Name, pinthickness, lx, ly);
				}
			}
			else {
				pcb_fprintf(fp, "      (pin %s \"%s\" %.6mm %.6mm)\n", padstack, pin->Number, lx, ly);
			}

			if (!g_list_find_custom(pads, padstack, (GCompareFunc) strcmp))
				pads = g_list_append(pads, padstack);
			else
				g_free(padstack);
		}
		END_LOOP;

		PAD_LOOP(element);
		{
			Coord xlen, ylen, xc, yc, p1y, p2y;
			Coord lx, ly;  /* store local coordinates for pins */
			p1y = PCB->MaxHeight - pad->Point1.Y;
			p2y = PCB->MaxHeight - pad->Point2.Y;
			/* pad dimensions are unusual-
			   the width is thickness and length is point difference plus thickness */
			xlen = ABS(pad->Point1.X - pad->Point2.X);
			if (xlen == 0) {
				xlen = pad->Thickness;
				ylen = ABS(p1y - p2y) + pad->Thickness;
			}
			else {
				ylen = pad->Thickness;
				xlen += pad->Thickness;
			}
			xc = (pad->Point1.X + pad->Point2.X) / 2;
			yc = (p1y + p2y) / 2;
			lx = (xc - centroid.X) * partsidesign;
			ly = (centroid.Y - yc) * -1;
			padstack = pcb_strdup_printf("Smd_rect_%mIx%mI", xlen, ylen);

			if (!pad->Number) {				/* if pad is null just make it a keepout */
				LayerType *lay;
				lay = g_list_nth_data(layerlist, partside);
				pcb_fprintf(fp, "      (keepout \"\" (rect \"%s\" %.6mm %.6mm %.6mm %.6mm))\n",
										lay->Name, lx - xlen / 2, ly - ylen / 2, lx + xlen / 2, ly + ylen / 2);
			}
			else {
				pcb_fprintf(fp, "      (pin %s \"%s\" %.6mm %.6mm)\n", padstack, pad->Number, lx, ly);
			}
			if (!g_list_find_custom(pads, padstack, (GCompareFunc) strcmp))
				pads = g_list_append(pads, padstack);
			else
				g_free(padstack);
		}
		END_LOOP;
		fprintf(fp, "    )\n");
	}
	END_LOOP;

	VIA_LOOP(PCB->Data);
	{ /* add mounting holes and vias */
		fprintf(fp, "    (image %ld\n", via->ID); /* map every via by ID */
		/* for mounting holes, clearance is added to thickness for higher total clearance */
		padstack = pcb_strdup_printf("Th_round_%mI", via->Thickness + via->Clearance);
		fprintf(fp, "      (pin %s 1 0 0)\n", padstack);	/* only 1 pin, 0,0 puts it right on component placement spot */
		fprintf(fp, "    )\n");
		if (!g_list_find_custom(pads, padstack, (GCompareFunc) strcmp))
			pads = g_list_append(pads, padstack);
		else
			g_free(padstack);
	}
	END_LOOP;

	/* loop thru padstacks and define them all */
	for (iter = pads; iter; iter = g_list_next(iter)) {
		Coord dim1, dim2;
		padstack = iter->data;
		fprintf(fp, "    (padstack %s\n", padstack);

		/* print info about pad here */
		if (sscanf(padstack, "Smd_rect_%ldx%ld", &dim1, &dim2) == 2) {	/* then pad is smd */
			pcb_fprintf(fp,
									"      (shape (rect \"%s\" %.6mm %.6mm %.6mm %.6mm))\n",
									((LayerType *) (g_list_first(layerlist)->data))->Name, dim1 / -2, dim2 / -2, dim1 / 2, dim2 / 2);
		}
		else if (sscanf(padstack, "Th_square_%ld", &dim1) == 1) {
			pcb_fprintf(fp, "      (shape (rect signal %.6mm %.6mm %.6mm %.6mm))\n", dim1 / -2, dim1 / -2, dim1 / 2, dim1 / 2);
		}
		else {
			sscanf(padstack, "Th_round_%ld", &dim1);
			pcb_fprintf(fp, "      (shape (circle signal %.6mm))\n", dim1);
		}
		fprintf(fp, "      (attach off)\n");
		fprintf(fp, "    )\n");
	}

	/* add padstack for via */
	pcb_fprintf(fp, "    (padstack via_%ld_%ld\n", viawidth, viadrill);
	pcb_fprintf(fp, "      (shape (circle signal %.6mm))\n", viawidth);
	pcb_fprintf(fp, "      (attach off)\n    )\n");
	pcb_fprintf(fp, "  )\n");
	g_list_foreach(pads, (GFunc) g_free, NULL);
	g_list_free(pads);
}

static void print_quoted_pin(FILE * fp, const char *s)
{
	char *hyphen_pos = strchr(s, '-');
	if (!hyphen_pos) {
		fprintf(fp, " %s", s);
	}
	else {
		char refdes_name[1024];
		int copy_len = hyphen_pos - s;
		if (copy_len >= sizeof(refdes_name))
			copy_len = sizeof(refdes_name) - 1;
		strncpy(refdes_name, s, copy_len);
		refdes_name[copy_len] = 0;
		fprintf(fp, " \"%s\"-\"%s\"", refdes_name, hyphen_pos + 1);
	}
}

static void print_network(FILE * fp)
{
	int ni, nei;
	fprintf(fp, "  (network\n");
	for (ni = 0; ni < PCB->NetlistLib[NETLIST_EDITED].MenuN; ni++) {
		fprintf(fp, "    (net \"%s\"\n", PCB->NetlistLib[NETLIST_EDITED].Menu[ni].Name + 2);
		fprintf(fp, "      (pins");
		for (nei = 0; nei < PCB->NetlistLib[NETLIST_EDITED].Menu[ni].EntryN; nei++)
			print_quoted_pin(fp, PCB->NetlistLib[NETLIST_EDITED].Menu[ni].Entry[nei].ListEntry);
		fprintf(fp, ")\n");
		fprintf(fp, "    )\n");
	}

	fprintf(fp, "    (class geda_default");
	for (ni = 0; ni < PCB->NetlistLib[NETLIST_EDITED].MenuN; ni++) {
		fprintf(fp, " \"%s\"", PCB->NetlistLib[NETLIST_EDITED].Menu[ni].Name + 2);
	}
	pcb_fprintf(fp, "\n");
	pcb_fprintf(fp, "      (circuit\n");
	pcb_fprintf(fp, "        (use_via via_%ld_%ld)\n", viawidth, viadrill);
	pcb_fprintf(fp, "      )\n");
	pcb_fprintf(fp, "      (rule (width %.6mm))\n    )\n  )\n", trackwidth);
}

static void print_wires(FILE * fp)
{
	GList *iter;
	LayerType *lay;
	fprintf(fp, "    (wiring\n");

	for (iter = layerlist; iter; iter = g_list_next(iter)) {
		lay = iter->data;
		LINE_LOOP(lay);
		{
			pcb_fprintf(fp,
									"        (wire (path %s %.6mm %.6mm %.6mm %.6mm %.6mm)\n",
									lay->Name, line->Thickness, line->Point1.X,
									(PCB->MaxHeight - line->Point1.Y), line->Point2.X, (PCB->MaxHeight - line->Point2.Y));
			fprintf(fp, "            (type protect))\n");
		}
		END_LOOP;
	}
	fprintf(fp, "\n    )\n)\n"); /* close all braces */
}

static int PrintSPECCTRA(void)
{
	FILE *fp;
	/* Print out the dsn .dsn file. */
	fp = fopen(dsn_filename, "w");
	if (!fp) {
		gui->log("Cannot open file %s for writing\n", dsn_filename);
		return 1;
	}

	/* pcb [required] */
	fprintf(fp, "(pcb %s\n", ((PCB->Name) && *(PCB->Name) ? (PCB->Name) : "notnamed"));

	/* parser descriptor [optional] */
	fprintf(fp, "  (parser\n");
	fprintf(fp, "    (string_quote \")\n");
	fprintf(fp, "    (space_in_quoted_tokens on)\n");
	fprintf(fp, "    (host_cad \"gEDA pcb-rnd\")\n");
	fprintf(fp, "    (host_version \"%s\")\n", VERSION);
	fprintf(fp, "  )\n");

	/* capacitance resolution descriptor [optional] */

	/* conductance resolution descriptor [optional] */

	/* current resolution descriptor [optional] */

	/* inductance resolution descriptor [optional] */

	/* resistance resolution descriptor [optional] */

	/* resolution descriptor [optional] */
	fprintf(fp, "  (resolution mm 1000000)\n");

	/* time resolution descriptor [optional] */

	/* voltage resolution descriptor [optional] */

	/* unit descriptor [optional] */

	/* structure descriptor [required] */
	print_structure(fp);

	/* placement descriptor [optional] */
	print_placement(fp);

	/* library descriptor [required] */
	print_library(fp);

	/* floor plan descriptor [optional] */

	/* part library descriptor [optional] */

	/* network descriptor [required] */
	print_network(fp);

	/* wiring descriptor [optional] */
	print_wires(fp);

	/* color descriptor [optional] */

	fclose(fp);

	return (0);
}


static void dsn_do_export(HID_Attr_Val * options)
{
	int i;
	if (!options) {
		dsn_get_export_options(0);
		for (i = 0; i < NUM_OPTIONS; i++)
			dsn_values[i] = dsn_options[i].default_val;
		options = dsn_values;
	}
	dsn_filename = options[HA_dsnfile].str_value;
	if (!dsn_filename)
		dsn_filename = "pcb-out.dsn";

	trackwidth = options[HA_trackwidth].coord_value;
	clearance = options[HA_clearance].coord_value;
	viawidth = options[HA_viawidth].coord_value;
	viadrill = options[HA_viadrill].coord_value;
	PrintSPECCTRA();
}

static void dsn_parse_arguments(int *argc, char ***argv)
{
	hid_parse_command_line(argc, argv);
}

static void hid_dsn_uninit()
{

}

#include "dolists.h"
pcb_uninit_t hid_export_dsn_init()
{
	memset(&dsn_hid, 0, sizeof(HID));
	common_nogui_init(&dsn_hid);

	dsn_hid.struct_size = sizeof(HID);
	dsn_hid.name = "dsn";
	dsn_hid.description = "Exports DSN format";
	dsn_hid.exporter = 1;
	dsn_hid.get_export_options = dsn_get_export_options;
	dsn_hid.do_export = dsn_do_export;
	dsn_hid.parse_arguments = dsn_parse_arguments;
	hid_register_hid(&dsn_hid);

	hid_register_attributes(dsn_options, sizeof(dsn_options) / sizeof(dsn_options[0]), dsn_cookie, 0);
	return hid_dsn_uninit;
}

