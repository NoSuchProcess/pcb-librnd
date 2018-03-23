/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *
 *  IPC-D-356 Netlist export.
 *
 *  Copyright (C) 2012 Jerome Marchand (Jerome.Marchand@gmail.com)
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "board.h"
#include "data.h"
#include "rats.h"
#include "error.h"
#include "find.h"
#include "pcb-printf.h"
#include "netlist.h"
#include "conf_core.h"
#include "obj_pinvia.h"
#include "compat_misc.h"
#include "safe_fs.h"
#include "obj_elem.h"

#include "hid.h"
#include "hid_nogui.h"
#include "hid_helper.h"
#include "hid_attrib.h"
#include "hid_init.h"
#include "plugins.h"

static const char *ipcd356_cookie = "ipcd356 exporter";

static pcb_hid_attribute_t IPCD356_options[] = {
/* %start-doc options "8 IPC-D-356 Netlist Export"
@ftable @code
@item --netlist-file <string>
Name of the IPC-D-356 Netlist output file.
@end ftable
%end-doc
*/
	{
	 "netlistfile",
	 "Name of the IPC-D-356 Netlist output file",
	 PCB_HATT_STRING,
	 0, 0, {0, 0, 0}, 0, 0},
#define HA_IPCD356_filename 0
};

#define NUM_OPTIONS (sizeof(IPCD356_options)/sizeof(IPCD356_options[0]))

static pcb_hid_attr_val_t IPCD356_values[NUM_OPTIONS];

const char *IPCD356_filename;

typedef struct {
	char NName[11];
	char NetName[256];
} IPCD356_Alias;

typedef struct {
	int AliasN;										/*!< Number of entries. */
	IPCD356_Alias *Alias;
} IPCD356_AliasList;

void IPCD356_WriteNet(FILE *, char *);
void IPCD356_WriteHeader(FILE *);
void IPCD356_End(FILE *);
int IPCD356_Netlist(void);
int IPCD356_WriteAliases(FILE *, IPCD356_AliasList *);
void ResetVisitPinsViasAndPads(void);
void CheckNetLength(char *, IPCD356_AliasList *);
IPCD356_AliasList *CreateAliasList(void);
IPCD356_AliasList *AddAliasToList(IPCD356_AliasList *);
int IPCD356_SanityCheck(void);

static pcb_hid_attribute_t *IPCD356_get_export_options(int *n)
{
	static char *last_IPCD356_filename = 0;

	if (PCB) {
		pcb_derive_default_filename(PCB->Filename, &IPCD356_options[HA_IPCD356_filename], ".net", &last_IPCD356_filename);
	}

	if (n)
		*n = NUM_OPTIONS;

	return IPCD356_options;
}

/* Writes the IPC-D-356 Header to the file provided.
 *
 * The JOB name is the PCB Name (if set), otherwise the filename
 * (including the path) is used.
 *
 * The units used for the netlist depends on what is set (mils or mm).
 */
void IPCD356_WriteHeader(FILE * fd)
{
	char utcTime[64];

	pcb_print_utc(utcTime, sizeof utcTime, 0);

	fprintf(fd, "C  IPC-D-356 Netlist generated by pcb-rnd " PCB_VERSION "\nC  \n");
	fprintf(fd, "C  File created on %s\nC  \n", utcTime);
	if (PCB->Name == NULL) {
		fprintf(fd, "P  JOB   %s\n", PCB->Filename);	/* Use the file name if the PCB name in not set. */
	}
	else {
		fprintf(fd, "P  JOB   %s\n", PCB->Name);
	}
	fprintf(fd, "P  CODE  00\n");
	if (strcmp(conf_core.editor.grid_unit->suffix, "mil") == 0) {	/* Use whatever unit is currently in use (mil or mm). */
		fprintf(fd, "P  UNITS CUST 0\n");
	}
	else {
		fprintf(fd, "P  UNITS CUST 1\n");
	}
	fprintf(fd, "P  DIM   N\n");
	fprintf(fd, "P  VER   IPC-D-356\n");
	fprintf(fd, "P  IMAGE PRIMARY\nC  \n");
}


/* Writes a net to the file provided.
 *
 * The net name is passed through the "net" and should be 14 characters
 * max.
 * The function scans through pads, pins and vias  and looks for the
 * PCB_FLAG_FOUND.
 * Once the object has been added to the net list the \c PCB_FLAG_VISIT is
 * set on that object.
 *
 * TODO 1) The bottom layer is always written as layer #2 (A02).
 *         It could output the actual layer number (example: A06 on a
 *         6 layer board).
 *         But I could not find an easy way to do this...
 *
 * TODO 2) Objects with mutiple connections could have the "M"
 *         (column 32) field written to indicate a Mid Net Point.
 */
void IPCD356_WriteNet(FILE * fd, char *net)
{
	int padx, pady, tmp;

	PCB_ELEMENT_LOOP(PCB->Data);
	PCB_PAD_LOOP(element);
	if (PCB_FLAG_TEST(PCB_FLAG_FOUND, pad)) {
		fprintf(fd, "327%-17.14s", net);	/* Net Name. */
		fprintf(fd, "%-6.6s", element->Name[1].TextString);	/* Refdes. */
		fprintf(fd, "-%-4.4s", pad->Number);	/* pin number. */
		fprintf(fd, " ");						/*! \todo Midpoint indicator (M). */
		fprintf(fd, "      ");			/* Drilled hole Id (blank for pads). */
		if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) == pcb_true) {
			fprintf(fd, "A02");				/*! \todo Put actual layer # for bottom side. */
		}
		else {
			fprintf(fd, "A01");				/* Top side. */
		}
		padx = (pad->Point1.X + pad->Point2.X) / 2;	/* X location in PCB units. */
		pady = (PCB->MaxHeight - ((pad->Point1.Y + pad->Point2.Y) / 2));	/* Y location in PCB units. */

		if (strcmp(conf_core.editor.grid_unit->suffix, "mil") == 0) {
			padx = padx / 2540;				/* X location in 0.0001". */
			pady = pady / 2540;				/* Y location in 0.0001". */
		}
		else {
			padx = padx / 1000;				/* X location in 0.001 mm. */
			pady = pady / 1000;				/* Y location in 0.001 mm. */
		}
		fprintf(fd, "X%+6.6d", padx);	/* X Pad center. */
		fprintf(fd, "Y%+6.6d", pady);	/* Y pad center. */

		padx = (pad->Thickness + (pad->Point2.X - pad->Point1.X));	/* Pad dimension X in PCB units. */
		pady = (pad->Thickness + (pad->Point2.Y - pad->Point1.Y));	/* Pad dimension Y in PCB units. */

		if (strcmp(conf_core.editor.grid_unit->suffix, "mil") == 0) {
			padx = padx / 2540;				/* X location in 0.0001". */
			pady = pady / 2540;				/* Y location in 0.0001". */
		}
		else {
			padx = padx / 1000;				/* X location in 0.001mm */
			pady = pady / 1000;				/* Y location in 0.001mm */
		}

		fprintf(fd, "X%4.4d", padx);
		fprintf(fd, "Y%4.4d", pady);
		fprintf(fd, "R000");				/* Rotation (0 degrees). */
		fprintf(fd, " ");						/* Column 72 should be left blank. */
		if (pad->Mask > 0) {
			if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) == pcb_true) {
				fprintf(fd, "S2");			/* Soldermask on bottom side. */
			}
			else {
				fprintf(fd, "S1");			/* SolderMask on top side. */
			}
		}
		else {
			fprintf(fd, "S3");				/* No soldermask. */
		}
		fprintf(fd, "      ");			/* Padding. */
		fprintf(fd, "\n");
		PCB_FLAG_SET(PCB_FLAG_VISIT, pad);
	}

	PCB_END_LOOP;											/* Pad. */
	PCB_PIN_LOOP(element);
	if (PCB_FLAG_TEST(PCB_FLAG_FOUND, pin)) {
		if (PCB_FLAG_TEST(PCB_FLAG_HOLE, pin)) {	/* Non plated? */
			fprintf(fd, "367%-17.14s", net);	/* Net Name. */
		}
		else {
			fprintf(fd, "317%-17.14s", net);	/* Net Name. */
		}
		fprintf(fd, "%-6.6s", element->Name[1].TextString);	/* Refdes. */
		fprintf(fd, "-%-4.4s", pin->Number);	/* Pin number. */
		fprintf(fd, " ");						/*! \todo Midpoint indicator (M). */
		tmp = pin->DrillingHole;
		if (strcmp(conf_core.editor.grid_unit->suffix, "mil") == 0) {
			tmp = tmp / 2540;					/* 0.0001". */
		}
		else {
			tmp = tmp / 1000;					/* 0.001 mm. */
		}

		if (PCB_FLAG_TEST(PCB_FLAG_HOLE, pin)) {
			fprintf(fd, "D%-4.4dU", tmp);	/* Unplated Drilled hole Id. */
		}
		else {
			fprintf(fd, "D%-4.4dP", tmp);	/* Plated drill hole. */
		}
		fprintf(fd, "A00");					/* Accessible from both sides. */
		padx = pin->X;							/* X location in PCB units. */
		pady = (PCB->MaxHeight - pin->Y);	/* Y location in PCB units. */

		if (strcmp(conf_core.editor.grid_unit->suffix, "mil") == 0) {
			padx = padx / 2540;				/* X location in 0.0001". */
			pady = pady / 2540;				/* Y location in 0.0001". */
		}
		else {
			padx = padx / 1000;				/* X location in 0.001 mm. */
			pady = pady / 1000;				/* Y location in 0.001 mm. */
		}

		fprintf(fd, "X%+6.6d", padx);	/* X Pad center. */
		fprintf(fd, "Y%+6.6d", pady);	/* Y pad center. */

		padx = pin->Thickness;

		if (strcmp(conf_core.editor.grid_unit->suffix, "mil") == 0) {
			padx = padx / 2540;				/* X location in 0.0001". */
		}
		else {
			padx = padx / 1000;				/* X location in 0.001 mm. */
		}

		fprintf(fd, "X%4.4d", padx);	/* Pad dimension X. */
		if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pin)) {
			fprintf(fd, "Y%4.4d", padx);	/* Pad dimension Y. */
		}
		else {
			fprintf(fd, "Y0000");			/*  Y is 0 for round pins. */
		}
		fprintf(fd, "R000");				/* Rotation (0 degrees). */
		fprintf(fd, " ");						/* Column 72 should be left blank. */
		if (pin->Mask > 0) {
			fprintf(fd, "S0");				/* No Soldermask. */
		}
		else {
			fprintf(fd, "S3");				/* Soldermask on both sides. */
		}
		fprintf(fd, "      ");			/* Padding. */

		fprintf(fd, "\n");

		PCB_FLAG_SET(PCB_FLAG_VISIT, pin);

	}

	PCB_END_LOOP;											/* Pin. */
	PCB_END_LOOP;											/* Element */

	PCB_VIA_LOOP(PCB->Data);
	if (PCB_FLAG_TEST(PCB_FLAG_FOUND, via)) {
		if (PCB_FLAG_TEST(PCB_FLAG_HOLE, via)) {	/* Non plated ? */
			fprintf(fd, "367%-17.14s", net);	/* Net Name. */
		}
		else {
			fprintf(fd, "317%-17.14s", net);	/* Net Name. */
		}
		fprintf(fd, "VIA   ");			/* Refdes. */
		fprintf(fd, "-    ");				/* Pin number. */
		fprintf(fd, " ");						/*! \todo Midpoint indicator (M). */
		tmp = via->DrillingHole;
		if (strcmp(conf_core.editor.grid_unit->suffix, "mil") == 0) {
			tmp = tmp / 2540;					/* 0.0001". */
		}
		else {
			tmp = tmp / 1000;					/* 0.001 mm. */
		}

		if (PCB_FLAG_TEST(PCB_FLAG_HOLE, via)) {
			fprintf(fd, "D%-4.4dU", tmp);	/* Unplated Drilled hole Id. */
		}
		else {
			fprintf(fd, "D%-4.4dP", tmp);	/* Plated drill hole. */
		}
		fprintf(fd, "A00");					/* Accessible from both sides. */
		padx = via->X;							/* X location in PCB units. */
		pady = (PCB->MaxHeight - via->Y);	/* Y location in PCB units. */

		if (strcmp(conf_core.editor.grid_unit->suffix, "mil") == 0) {
			padx = padx / 2540;				/* X location in 0.0001". */
			pady = pady / 2540;				/* Y location in 0.0001". */
		}
		else {
			padx = padx / 1000;				/* X location in 0.001 mm. */
			pady = pady / 1000;				/* Y location in 0.001 mm. */
		}

		fprintf(fd, "X%+6.6d", padx);	/* X Pad center. */
		fprintf(fd, "Y%+6.6d", pady);	/* Y pad center. */

		padx = via->Thickness;

		if (strcmp(conf_core.editor.grid_unit->suffix, "mil") == 0) {
			padx = padx / 2540;				/* X location in 0.0001". */
		}
		else {
			padx = padx / 1000;				/* X location in 0.001 mm. */
		}

		fprintf(fd, "X%4.4d", padx);	/* Pad dimension X. */
		fprintf(fd, "Y0000");				/* Y is 0 for round pins (vias always round?). */
		fprintf(fd, "R000");				/* Rotation (0 degrees). */
		fprintf(fd, " ");						/* Column 72 should be left blank. */
		if (via->Mask > 0) {
			fprintf(fd, "S0");				/* No Soldermask. */
		}
		else {
			fprintf(fd, "S3");				/* Soldermask on both sides. */
		}
		fprintf(fd, "      ");			/* Padding. */
		fprintf(fd, "\n");
		PCB_FLAG_SET(PCB_FLAG_VISIT, via);
	}

	PCB_END_LOOP;											/* Via. */
}


/* The main IPC-D-356 function; gets the filename for the netlist from the dialog */
int IPCD356_Netlist(void)
{
	FILE *fp;
	char nodename[256];
	char net[256];
	pcb_lib_menu_t *netname;
	IPCD356_AliasList *aliaslist;

	if (IPCD356_SanityCheck()) {	/* Check for invalid names + numbers. */
		pcb_message(PCB_MSG_ERROR, "IPCD356: aborting on the sanity check.\n");
		return 1;
	}

	sprintf(net, "%s.ipc", PCB->Name);
	if (IPCD356_filename == NULL)
		return 1;

	fp = pcb_fopen(IPCD356_filename, "w+");
	if (fp == NULL) {
		pcb_message(PCB_MSG_ERROR, "error opening %s\n", IPCD356_filename);
		return 1;
	}
/*   free (IPCD356_filename); */


	IPCD356_WriteHeader(fp);

	aliaslist = CreateAliasList();
	if (aliaslist == NULL) {
		pcb_message(PCB_MSG_ERROR, "Error Aloccating memory for IPC-D-356 AliasList\n");
		return 1;
	}

	if (IPCD356_WriteAliases(fp, aliaslist)) {
		pcb_message(PCB_MSG_ERROR, "Error Writing IPC-D-356 AliasList\n");
		return 1;
	}


	PCB_ELEMENT_LOOP(PCB->Data);
	PCB_PIN_LOOP(element);
	if (!PCB_FLAG_TEST(PCB_FLAG_VISIT, pin)) {
		pcb_clear_flag_on_lines_polys(pcb_true, PCB_FLAG_FOUND);
		pcb_clear_flag_on_pins_vias_pads(pcb_true, PCB_FLAG_FOUND);
		pcb_lookup_conn_by_pin(PCB_TYPE_PIN, pin);
		sprintf(nodename, "%s-%s", element->Name[1].TextString, pin->Number);
		netname = pcb_netnode_to_netname(nodename);
/*      pcb_message(PCB_MSG_INFO, "Netname: %s\n", netname->Name +2); */
		if (netname) {
			strcpy(net, &netname->Name[2]);
			CheckNetLength(net, aliaslist);
		}
		else {
			strcpy(net, "N/C");
		}
		IPCD356_WriteNet(fp, net);
	}
	PCB_END_LOOP;											/* Pin. */
	PCB_PAD_LOOP(element);
	if (!PCB_FLAG_TEST(PCB_FLAG_VISIT, pad)) {
		pcb_clear_flag_on_lines_polys(pcb_true, PCB_FLAG_FOUND);
		pcb_clear_flag_on_pins_vias_pads(pcb_true, PCB_FLAG_FOUND);
		pcb_lookup_conn_by_pin(PCB_TYPE_PAD, pad);
		sprintf(nodename, "%s-%s", element->Name[1].TextString, pad->Number);
		netname = pcb_netnode_to_netname(nodename);
/*      pcb_message(PCB_MSG_INFO, "Netname: %s\n", netname->Name +2); */
		if (netname) {
			strcpy(net, &netname->Name[2]);
			CheckNetLength(net, aliaslist);
		}
		else {
			strcpy(net, "N/C");
		}
		IPCD356_WriteNet(fp, net);
	}
	PCB_END_LOOP;											/* Pad. */

	PCB_END_LOOP;											/* Element. */

	PCB_VIA_LOOP(PCB->Data);
	if (!PCB_FLAG_TEST(PCB_FLAG_VISIT, via)) {
		pcb_clear_flag_on_lines_polys(pcb_true, PCB_FLAG_FOUND);
		pcb_clear_flag_on_pins_vias_pads(pcb_true, PCB_FLAG_FOUND);
		pcb_lookup_conn_by_pin(PCB_TYPE_PIN, via);
		strcpy(net, "N/C");
		IPCD356_WriteNet(fp, net);
	}
	PCB_END_LOOP;											/* Via. */

	IPCD356_End(fp);
	fclose(fp);
	free(aliaslist);
	ResetVisitPinsViasAndPads();
	pcb_clear_flag_on_lines_polys(pcb_true, PCB_FLAG_FOUND);
	pcb_clear_flag_on_pins_vias_pads(pcb_true, PCB_FLAG_FOUND);
	return 0;
}

void IPCD356_End(FILE * fd)
{
	fprintf(fd, "999\n");
}

void ResetVisitPinsViasAndPads()
{
	PCB_VIA_LOOP(PCB->Data);
	PCB_FLAG_CLEAR(PCB_FLAG_VISIT, via);
	PCB_END_LOOP;											/* Via. */
	PCB_ELEMENT_LOOP(PCB->Data);
	PCB_PIN_LOOP(element);
	PCB_FLAG_CLEAR(PCB_FLAG_VISIT, pin);
	PCB_END_LOOP;											/* Pin. */
	PCB_PAD_LOOP(element);
	PCB_FLAG_CLEAR(PCB_FLAG_VISIT, pad);
	PCB_END_LOOP;											/* Pad. */
	PCB_END_LOOP;											/* Element. */
}

int IPCD356_WriteAliases(FILE * fd, IPCD356_AliasList * aliaslist)
{
	int index;
	int i;

	index = 1;

	for (i = 0; i < PCB->NetlistLib[PCB_NETLIST_EDITED].MenuN; i++) {
		if (strlen(PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[i].Name + 2) > 14) {
			if (index == 1) {
				fprintf(fd, "C  Netname Aliases Section\n");
			}
			aliaslist = AddAliasToList(aliaslist);
			if (aliaslist == NULL) {
				return 1;
			}
			sprintf(aliaslist->Alias[index].NName, "NNAME%-5.5d", index);
			strcpy(aliaslist->Alias[index].NetName, PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[i].Name + 2);

			fprintf(fd, "P  %s  %-58.58s\n", aliaslist->Alias[index].NName, aliaslist->Alias[index].NetName);
			index++;
		}
	}
	if (index > 1) {
		fprintf(fd, "C  End Netname Aliases Section\nC  \n");
	}
	return 0;
}

IPCD356_AliasList *CreateAliasList()
{
	IPCD356_AliasList *aliaslist;

	aliaslist = malloc(sizeof(IPCD356_AliasList));	/* Create an alias list. */
	aliaslist->AliasN = 0;				/* Initialize Number of Alias. */
	return aliaslist;
}

IPCD356_AliasList *AddAliasToList(IPCD356_AliasList * aliaslist)
{
	aliaslist->AliasN++;
	aliaslist->Alias = realloc(aliaslist->Alias, sizeof(IPCD356_Alias) * (aliaslist->AliasN + 1));
	if (aliaslist->Alias == NULL) {
		return NULL;
	}
	return aliaslist;
}

void CheckNetLength(char *net, IPCD356_AliasList * aliaslist)
{
	int i;

	if (strlen(net) > 14) {
		for (i = 1; i <= aliaslist->AliasN; i++) {
			if (strcmp(net, aliaslist->Alias[i].NetName) == 0) {
				strcpy(net, aliaslist->Alias[i].NName);
			}
		}
	}
}

int IPCD356_SanityCheck()
{
	PCB_ELEMENT_LOOP(PCB->Data);
	if (element->Name[1].TextString == '\0') {
		pcb_message(PCB_MSG_ERROR, "Error: Found unnamed element. All elements need to be named to create an IPC-D-356 netlist.\n");
		return 1;
	}
	PCB_END_LOOP;											/* Element. */
	return 0;
}

static void IPCD356_do_export(pcb_hid_attr_val_t * options)
{
	int i;

	if (!options) {
		IPCD356_get_export_options(0);

		for (i = 0; i < NUM_OPTIONS; i++)
			IPCD356_values[i] = IPCD356_options[i].default_val;

		options = IPCD356_values;
	}

	IPCD356_filename = options[HA_IPCD356_filename].str_value;
	if (!IPCD356_filename)
		IPCD356_filename = "pcb-out.net";

	IPCD356_Netlist();
}

static void IPCD356_parse_arguments(int *argc, char ***argv)
{
	pcb_hid_parse_command_line(argc, argv);
}

pcb_hid_t IPCD356_hid;

int pplg_check_ver_export_ipcd356(int ver_needed) { return 0; }

void pplg_uninit_export_ipcd356(void)
{
	pcb_hid_remove_attributes_by_cookie(ipcd356_cookie);
}

int pplg_init_export_ipcd356(void)
{
	memset(&IPCD356_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&IPCD356_hid);

	IPCD356_hid.struct_size = sizeof(pcb_hid_t);
	IPCD356_hid.name = "IPC-D-356";
	IPCD356_hid.description = "Exports a IPC-D-356 Netlist";
	IPCD356_hid.exporter = 1;

	IPCD356_hid.get_export_options = IPCD356_get_export_options;
	IPCD356_hid.do_export = IPCD356_do_export;
	IPCD356_hid.parse_arguments = IPCD356_parse_arguments;

	pcb_hid_register_hid(&IPCD356_hid);

	pcb_hid_register_attributes(IPCD356_options, sizeof(IPCD356_options) / sizeof(IPCD356_options[0]), ipcd356_cookie, 0);
	return 0;
}
