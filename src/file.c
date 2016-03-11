/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,2005,2006 Thomas Nau
 *  Copyright (C) 2015 Tibor 'Igor2' Palinkas
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

/* file save, load, merge ... routines
 * getpid() needs a cast to (int) to get rid of compiler warnings
 * on several architectures
 */

#include "config.h"

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "global.h"

#include <dirent.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include <time.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <sys/stat.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


#include "buffer.h"
#include "change.h"
#include "create.h"
#include "crosshair.h"
#include "data.h"
#include "error.h"
#include "file.h"
#include "hid.h"
#include "misc.h"
#include "move.h"
#include "mymem.h"
#include "parse_l.h"
#include "pcb-printf.h"
#include "polygon.h"
#include "rats.h"
#include "remove.h"
#include "set.h"
#include "strflags.h"
#include "portability.h"
#include "libpcb_fp.h"
#include "paths.h"
#include "rats_patch.h"
#include "stub_edif.h"


RCSID("$Id$");

#if !defined(HAS_ATEXIT) && !defined(HAS_ON_EXIT)
/* ---------------------------------------------------------------------------
 * some local identifiers for OS without an atexit() or on_exit()
 * call
 */
static char TMPFilename[80];
#endif

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void PrintQuotedString(FILE *, char *);
static void WritePCBInfoHeader(FILE *);
static void WritePCBDataHeader(FILE *);
static void WritePCBFontData(FILE *);
static void WriteViaData(FILE *, DataTypePtr);
static void WritePCBRatData(FILE *);
static void WriteElementData(FILE *, DataTypePtr);
static void WriteLayerData(FILE *, Cardinal, LayerTypePtr);
static int WritePCB(FILE *);
static int WritePCBFile(char *);
static int WritePipe(char *, bool);
static int ParseLibraryTree(void);
static int LoadNewlibFootprintsFromDir(const char *subdir, const char *toppath, int is_root);
static const char *pcb_basename(const char *p);

/* ---------------------------------------------------------------------------
 * Flag helper functions
 */

#define F2S(OBJ, TYPE) flags_to_string ((OBJ)->Flags, TYPE)

/* --------------------------------------------------------------------------- */

/* The idea here is to avoid gratuitously breaking backwards
   compatibility due to a new but rarely used feature.  The first such
   case, for example, was the polygon Hole - if your design included
   polygon holes, you needed a newer PCB to read it, but if your
   design didn't include holes, PCB would produce a file that older
   PCBs could read, if only it had the correct version number in it.

   If, however, you have to add or change a feature that really does
   require a new PCB version all the time, it's time to remove all the
   tests below and just always output the new version.

   Note: Best practices here is to add support for a feature *first*
   (and bump PCB_FILE_VERSION in file.h), and note the version that
   added that support below, and *later* update the file format to
   need that version (which may then be older than PCB_FILE_VERSION).
   Hopefully, that allows for one release between adding support and
   needing it, which should minimize breakage.  Of course, that's not
   *always* possible, practical, or desirable.

*/

/* Hole[] in Polygon.  */
#define PCB_FILE_VERSION_HOLES 20100606
/* First version ever saved.  */
#define PCB_FILE_VERSION_BASELINE 20070407

int PCBFileVersionNeeded(void)
{
	ALLPOLYGON_LOOP(PCB->Data);
	{
		if (polygon->HoleIndexN > 0)
			return PCB_FILE_VERSION_HOLES;
	}
	ENDALL_LOOP;

	return PCB_FILE_VERSION_BASELINE;
}

/* --------------------------------------------------------------------------- */

static int string_cmp(const char *a, const char *b)
{
	while (*a && *b) {
		if (isdigit((int) *a) && isdigit((int) *b)) {
			int ia = atoi(a);
			int ib = atoi(b);
			if (ia != ib)
				return ia - ib;
			while (isdigit((int) *a) && *(a + 1))
				a++;
			while (isdigit((int) *b) && *(b + 1))
				b++;
		}
		else if (tolower((int) *a) != tolower((int) *b))
			return tolower((int) *a) - tolower((int) *b);
		a++;
		b++;
	}
	if (*a)
		return 1;
	if (*b)
		return -1;
	return 0;
}

static int netlist_sort_offset = 0;

static int netlist_sort(const void *va, const void *vb)
{
	LibraryMenuType *am = (LibraryMenuType *) va;
	LibraryMenuType *bm = (LibraryMenuType *) vb;
	char *a = am->Name;
	char *b = bm->Name;
	if (*a == '~')
		a++;
	if (*b == '~')
		b++;
	return string_cmp(a, b);
}

static int netnode_sort(const void *va, const void *vb)
{
	LibraryEntryType *am = (LibraryEntryType *) va;
	LibraryEntryType *bm = (LibraryEntryType *) vb;
	char *a = am->ListEntry;
	char *b = bm->ListEntry;
	return string_cmp(a, b);
}

static void sort_library(LibraryTypePtr lib)
{
	int i;
	qsort(lib->Menu, lib->MenuN, sizeof(lib->Menu[0]), netlist_sort);
	for (i = 0; i < lib->MenuN; i++)
		qsort(lib->Menu[i].Entry, lib->Menu[i].EntryN, sizeof(lib->Menu[i].Entry[0]), netnode_sort);
}

void sort_netlist()
{
	netlist_sort_offset = 2;
	sort_library(&(PCB->NetlistLib[NETLIST_INPUT]));
	netlist_sort_offset = 0;
}

/* ---------------------------------------------------------------------------
 * opens a file and check if it exists
 */
FILE *CheckAndOpenFile(char *Filename, bool Confirm, bool AllButton, bool * WasAllButton, bool * WasCancelButton)
{
	FILE *fp = NULL;
	struct stat buffer;
	char message[MAXPATHLEN + 80];
	int response;

	if (Filename && *Filename) {
		if (!stat(Filename, &buffer) && Confirm) {
			sprintf(message, _("File '%s' exists, use anyway?"), Filename);
			if (WasAllButton)
				*WasAllButton = false;
			if (WasCancelButton)
				*WasCancelButton = false;
			if (AllButton)
				response = gui->confirm_dialog(message, "Cancel", "Ok", AllButton ? "Sequence OK" : 0);
			else
				response = gui->confirm_dialog(message, "Cancel", "Ok", "Sequence OK");

			switch (response) {
			case 2:
				if (WasAllButton)
					*WasAllButton = true;
				break;
			case 0:
				if (WasCancelButton)
					*WasCancelButton = true;
			}
		}
		if ((fp = fopen(Filename, "w")) == NULL)
			OpenErrorMessage(Filename);
	}
	return (fp);
}

/* ---------------------------------------------------------------------------
 * opens a file for saving connection data
 */
FILE *OpenConnectionDataFile(void)
{
	char *fname;
	FILE *fp;
	static char *default_file = NULL;
	bool result;									/* not used */

	/* CheckAndOpenFile deals with the case where fname already exists */
	fname = gui->fileselect(_("Save Connection Data As ..."),
													_("Choose a file to save all connection data to."), default_file, ".net", "connection_data", 0);
	if (fname == NULL)
		return NULL;

	if (default_file != NULL) {
		free(default_file);
		default_file = NULL;
	}

	if (fname && *fname)
		default_file = strdup(fname);

	fp = CheckAndOpenFile(fname, true, false, &result, NULL);
	free(fname);

	return fp;
}

/* ---------------------------------------------------------------------------
 * save elements in the current buffer
 */
int SaveBufferElements(char *Filename)
{
	int result;

	if (SWAP_IDENT)
		SwapBuffers();
	result = WritePipe(Filename, false);
	if (SWAP_IDENT)
		SwapBuffers();
	return (result);
}

/* ---------------------------------------------------------------------------
 * save PCB
 */
int SavePCB(char *file)
{
	int retcode;

	if (gui->notify_save_pcb == NULL)
		return WritePipe(file, true);

	gui->notify_save_pcb(file, false);
	retcode = WritePipe(file, true);
	gui->notify_save_pcb(file, true);

	return retcode;
}

/* ---------------------------------------------------------------------------
 * set the route style to the first one, if the current one doesn't
 * happen to match any.  This way, "revert" won't change the route
 * style.
 */
static void set_some_route_style()
{
	if (hid_get_flag("style"))
		return;
	SetLineSize(PCB->RouteStyle[0].Thick);
	SetViaSize(PCB->RouteStyle[0].Diameter, true);
	SetViaDrillingHole(PCB->RouteStyle[0].Hole, true);
	SetKeepawayWidth(PCB->RouteStyle[0].Keepaway);
}

/* ---------------------------------------------------------------------------
 * load PCB
 * parse the file with enabled 'PCB mode' (see parser)
 * if successful, update some other stuff
 *
 * If revert is true, we pass "revert" as a parameter
 * to the HID's PCBChanged action.
 */
static int real_load_pcb(char *Filename, bool revert, bool require_font, bool is_misc)
{
	const char *unit_suffix;
	char *new_filename;
	PCBTypePtr newPCB = CreateNewPCB_(false);
	PCBTypePtr oldPCB;
#ifdef DEBUG
	double elapsed;
	clock_t start, end;

	start = clock();
#endif

	resolve_path(Filename, &new_filename);

	oldPCB = PCB;
	PCB = newPCB;

	/* mark the default font invalid to know if the file has one */
	newPCB->Font.Valid = false;

	/* new data isn't added to the undo list */
	if (!ParsePCB(PCB, new_filename)) {
		RemovePCB(oldPCB);

		CreateNewPCBPost(PCB, 0);
		ResetStackAndVisibility();

		if (!is_misc) {
			/* update cursor location */
			Crosshair.X = CLAMP(PCB->CursorX, 0, PCB->MaxWidth);
			Crosshair.Y = CLAMP(PCB->CursorY, 0, PCB->MaxHeight);

			/* update cursor confinement and output area (scrollbars) */
			ChangePCBSize(PCB->MaxWidth, PCB->MaxHeight);
		}

		/* enable default font if necessary */
		if (!PCB->Font.Valid) {
			if (require_font)
				Message(_("File '%s' has no font information, using default font\n"), new_filename);
			PCB->Font.Valid = true;
		}

		/* clear 'changed flag' */
		SetChangedFlag(false);
		PCB->Filename = new_filename;
		/* just in case a bad file saved file is loaded */

		/* Use attribute PCB::grid::unit as unit, if we can */
		unit_suffix = AttributeGet(PCB, "PCB::grid::unit");
		if (unit_suffix && *unit_suffix) {
			const Unit *new_unit = get_unit_struct(unit_suffix);
			if (new_unit)
				Settings.grid_unit = new_unit;
		}
		AttributePut(PCB, "PCB::grid::unit", Settings.grid_unit->suffix);

		sort_netlist();
		rats_patch_make_edited(PCB);

		set_some_route_style();

		if (!is_misc) {
			if (revert)
				hid_actionl("PCBChanged", "revert", NULL);
			else
				hid_action("PCBChanged");
		}

#ifdef DEBUG
		end = clock();
		elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
		gui->log("Loading file %s took %f seconds of CPU time\n", new_filename, elapsed);
#endif

		return (0);
	}
	PCB = oldPCB;
	hid_action("PCBChanged");

	/* release unused memory */
	RemovePCB(newPCB);
	return (1);
}

/* ---------------------------------------------------------------------------
 * Load PCB
 */
int LoadPCB(char *file, bool require_font, bool is_misc)
{
	return real_load_pcb(file, false, require_font, is_misc);
}

/* ---------------------------------------------------------------------------
 * Revert PCB
 */
int RevertPCB(void)
{
	return real_load_pcb(PCB->Filename, true, true, true);
}

/* ---------------------------------------------------------------------------
 * functions for loading elements-as-pcb
 */

extern PCBTypePtr yyPCB;
extern DataTypePtr yyData;
extern FontTypePtr yyFont;

void PreLoadElementPCB()
{

	if (!yyPCB)
		return;

	yyFont = &yyPCB->Font;
	yyData = yyPCB->Data;
	yyData->pcb = yyPCB;
	yyData->LayerN = 0;
}

void PostLoadElementPCB()
{
	PCBTypePtr pcb_save = PCB;
	ElementTypePtr e;

	if (!yyPCB)
		return;

	CreateNewPCBPost(yyPCB, 0);
	ParseGroupString("1,c:2,s", &yyPCB->LayerGroups, yyData->LayerN);
	e = elementlist_first(&yyPCB->Data->Element);	/* we know there's only one */
	PCB = yyPCB;
	MoveElementLowLevel(yyPCB->Data, e, -e->BoundingBox.X1, -e->BoundingBox.Y1);
	PCB = pcb_save;
	yyPCB->MaxWidth = e->BoundingBox.X2;
	yyPCB->MaxHeight = e->BoundingBox.Y2;
	yyPCB->is_footprint = 1;
}

/* ---------------------------------------------------------------------------
 * writes the quoted string created by another subroutine
 */
static void PrintQuotedString(FILE * FP, char *S)
{
	static DynamicStringType ds;

	CreateQuotedString(&ds, S);
	fputs(ds.Data, FP);
}

/* ---------------------------------------------------------------------------
 * writes out an attribute list
 */
static void WriteAttributeList(FILE * FP, AttributeListTypePtr list, char *prefix)
{
	int i;

	for (i = 0; i < list->Number; i++)
		fprintf(FP, "%sAttribute(\"%s\" \"%s\")\n", prefix, list->List[i].name, list->List[i].value);
}

/* ---------------------------------------------------------------------------
 * writes layout header information
 */
static void WritePCBInfoHeader(FILE * FP)
{
	/* write some useful comments */
	fprintf(FP, "# release: %s " VERSION "\n", Progname);

	/* avoid writing things like user name or date, as these cause merge
	 * conflicts in collaborative environments using version control systems
	 */
}

/* ---------------------------------------------------------------------------
 * writes data header
 * the name of the PCB, cursor location, zoom and grid
 * layergroups and some flags
 */
static void WritePCBDataHeader(FILE * FP)
{
	Cardinal group;

	/*
	 * ************************** README *******************
	 * ************************** README *******************
	 *
	 * If the file format is modified in any way, update
	 * PCB_FILE_VERSION in file.h as well as PCBFileVersionNeeded()
	 * at the top of this file.
	 *  
	 * ************************** README *******************
	 * ************************** README *******************
	 */

	fprintf(FP, "\n# To read pcb files, the pcb version (or the git source date) must be >= the file version\n");
	fprintf(FP, "FileVersion[%i]\n", PCBFileVersionNeeded());

	fputs("\nPCB[", FP);
	PrintQuotedString(FP, (char *) EMPTY(PCB->Name));
	pcb_fprintf(FP, " %mr %mr]\n\n", PCB->MaxWidth, PCB->MaxHeight);
	pcb_fprintf(FP, "Grid[%.1mr %mr %mr %d]\n", PCB->Grid, PCB->GridOffsetX, PCB->GridOffsetY, Settings.DrawGrid);
	pcb_fprintf(FP, "Cursor[%mr %mr %s]\n", Crosshair.X, Crosshair.Y, c_dtostr(PCB->Zoom));
	/* PolyArea should be output in square cmils, no suffix */
	fprintf(FP, "PolyArea[%s]\n", c_dtostr(COORD_TO_MIL(COORD_TO_MIL(PCB->IsleArea) * 100) * 100));
	pcb_fprintf(FP, "Thermal[%s]\n", c_dtostr(PCB->ThermScale));
	pcb_fprintf(FP, "DRC[%mr %mr %mr %mr %mr %mr]\n", PCB->Bloat, PCB->Shrink,
							PCB->minWid, PCB->minSlk, PCB->minDrill, PCB->minRing);
	fprintf(FP, "Flags(%s)\n", pcbflags_to_string(PCB->Flags));
	fprintf(FP, "Groups(\"%s\")\n", LayerGroupsToString(&PCB->LayerGroups));
	fputs("Styles[\"", FP);
	for (group = 0; group < NUM_STYLES - 1; group++)
		pcb_fprintf(FP, "%s,%mr,%mr,%mr,%mr:", PCB->RouteStyle[group].Name,
								PCB->RouteStyle[group].Thick,
								PCB->RouteStyle[group].Diameter, PCB->RouteStyle[group].Hole, PCB->RouteStyle[group].Keepaway);
	pcb_fprintf(FP, "%s,%mr,%mr,%mr,%mr\"]\n\n", PCB->RouteStyle[group].Name,
							PCB->RouteStyle[group].Thick,
							PCB->RouteStyle[group].Diameter, PCB->RouteStyle[group].Hole, PCB->RouteStyle[group].Keepaway);
}

/* ---------------------------------------------------------------------------
 * writes font data of non empty symbols
 */
static void WritePCBFontData(FILE * FP)
{
	Cardinal i, j;
	LineTypePtr line;
	FontTypePtr font;

	for (font = &PCB->Font, i = 0; i <= MAX_FONTPOSITION; i++) {
		if (!font->Symbol[i].Valid)
			continue;

		if (isprint(i))
			pcb_fprintf(FP, "Symbol['%c' %mr]\n(\n", i, font->Symbol[i].Delta);
		else
			pcb_fprintf(FP, "Symbol[%i %mr]\n(\n", i, font->Symbol[i].Delta);

		line = font->Symbol[i].Line;
		for (j = font->Symbol[i].LineN; j; j--, line++)
			pcb_fprintf(FP, "\tSymbolLine[%mr %mr %mr %mr %mr]\n",
									line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y, line->Thickness);
		fputs(")\n", FP);
	}
}

/* ---------------------------------------------------------------------------
 * writes via data
 */
static void WriteViaData(FILE * FP, DataTypePtr Data)
{
	gdl_iterator_t it;
	PinType *via;

	/* write information about vias */
	pinlist_foreach(&Data->Via, &it, via) {
		pcb_fprintf(FP, "Via[%mr %mr %mr %mr %mr %mr ", via->X, via->Y,
								via->Thickness, via->Clearance, via->Mask, via->DrillingHole);
		PrintQuotedString(FP, (char *) EMPTY(via->Name));
		fprintf(FP, " %s]\n", F2S(via, VIA_TYPE));
	}
}

/* ---------------------------------------------------------------------------
 * writes rat-line data
 */
static void WritePCBRatData(FILE * FP)
{
	gdl_iterator_t it;
	RatType *line;

	/* write information about rats */
	ratlist_foreach(&PCB->Data->Rat, &it, line) {
		pcb_fprintf(FP, "Rat[%mr %mr %d %mr %mr %d ",
								line->Point1.X, line->Point1.Y, line->group1, line->Point2.X, line->Point2.Y, line->group2);
		fprintf(FP, " %s]\n", F2S(line, RATLINE_TYPE));
	}
}

/* ---------------------------------------------------------------------------
 * writes netlist data
 */
static void WritePCBNetlistData(FILE * FP)
{
	/* write out the netlist if it exists */
	if (PCB->NetlistLib[NETLIST_INPUT].MenuN) {
		int n, p;
		fprintf(FP, "NetList()\n(\n");

		for (n = 0; n < PCB->NetlistLib[NETLIST_INPUT].MenuN; n++) {
			LibraryMenuTypePtr menu = &PCB->NetlistLib[NETLIST_INPUT].Menu[n];
			fprintf(FP, "\tNet(");
			PrintQuotedString(FP, &menu->Name[2]);
			fprintf(FP, " ");
			PrintQuotedString(FP, (char *) UNKNOWN(menu->Style));
			fprintf(FP, ")\n\t(\n");
			for (p = 0; p < menu->EntryN; p++) {
				LibraryEntryTypePtr entry = &menu->Entry[p];
				fprintf(FP, "\t\tConnect(");
				PrintQuotedString(FP, entry->ListEntry);
				fprintf(FP, ")\n");
			}
			fprintf(FP, "\t)\n");
		}
		fprintf(FP, ")\n");
	}
}

/* ---------------------------------------------------------------------------
 * writes netlist patch data
 */
static void WritePCBNetlistPatchData(FILE * FP)
{
	if (PCB->NetlistPatches != NULL) {
		fprintf(FP, "NetListPatch()\n(\n");
		rats_patch_fexport(PCB, FP, 1);
		fprintf(FP, ")\n");
	}
}

/* ---------------------------------------------------------------------------
 * writes element data
 */
static void WriteElementData(FILE * FP, DataTypePtr Data)
{
	gdl_iterator_t eit;
	LineType *line;
	ArcType *arc;
	ElementType *element;

	elementlist_foreach(&Data->Element, &eit, element) {
		gdl_iterator_t it;
		PinType *pin;
		PadType *pad;

		/* only non empty elements */
		if (!linelist_length(&element->Line) && !pinlist_length(&element->Pin) && !arclist_length(&element->Arc) && !padlist_length(&element->Pad))
			continue;
		/* the coordinates and text-flags are the same for
		 * both names of an element
		 */
		fprintf(FP, "\nElement[%s ", F2S(element, ELEMENT_TYPE));
		PrintQuotedString(FP, (char *) EMPTY(DESCRIPTION_NAME(element)));
		fputc(' ', FP);
		PrintQuotedString(FP, (char *) EMPTY(NAMEONPCB_NAME(element)));
		fputc(' ', FP);
		PrintQuotedString(FP, (char *) EMPTY(VALUE_NAME(element)));
		pcb_fprintf(FP, " %mr %mr %mr %mr %d %d %s]\n(\n",
								element->MarkX, element->MarkY,
								DESCRIPTION_TEXT(element).X - element->MarkX,
								DESCRIPTION_TEXT(element).Y - element->MarkY,
								DESCRIPTION_TEXT(element).Direction,
								DESCRIPTION_TEXT(element).Scale, F2S(&(DESCRIPTION_TEXT(element)), ELEMENTNAME_TYPE));
		WriteAttributeList(FP, &element->Attributes, "\t");
		pinlist_foreach(&element->Pin, &it, pin) {
			pcb_fprintf(FP, "\tPin[%mr %mr %mr %mr %mr %mr ",
									pin->X - element->MarkX,
									pin->Y - element->MarkY, pin->Thickness, pin->Clearance, pin->Mask, pin->DrillingHole);
			PrintQuotedString(FP, (char *) EMPTY(pin->Name));
			fprintf(FP, " ");
			PrintQuotedString(FP, (char *) EMPTY(pin->Number));
			fprintf(FP, " %s]\n", F2S(pin, PIN_TYPE));
		}
		pinlist_foreach(&element->Pad, &it, pad) {
			pcb_fprintf(FP, "\tPad[%mr %mr %mr %mr %mr %mr %mr ",
									pad->Point1.X - element->MarkX,
									pad->Point1.Y - element->MarkY,
									pad->Point2.X - element->MarkX, pad->Point2.Y - element->MarkY, pad->Thickness, pad->Clearance, pad->Mask);
			PrintQuotedString(FP, (char *) EMPTY(pad->Name));
			fprintf(FP, " ");
			PrintQuotedString(FP, (char *) EMPTY(pad->Number));
			fprintf(FP, " %s]\n", F2S(pad, PAD_TYPE));
		}
		linelist_foreach(&element->Line, &it, line) {
			pcb_fprintf(FP, "\tElementLine [%mr %mr %mr %mr %mr]\n",
									line->Point1.X - element->MarkX,
									line->Point1.Y - element->MarkY,
									line->Point2.X - element->MarkX, line->Point2.Y - element->MarkY, line->Thickness);
		}
		linelist_foreach(&element->Arc, &it, arc) {
			pcb_fprintf(FP, "\tElementArc [%mr %mr %mr %mr %ma %ma %mr]\n",
									arc->X - element->MarkX,
									arc->Y - element->MarkY, arc->Width, arc->Height, arc->StartAngle, arc->Delta, arc->Thickness);
		}
		fputs("\n\t)\n", FP);
	}
}

/* ---------------------------------------------------------------------------
 * writes layer data
 */
static void WriteLayerData(FILE * FP, Cardinal Number, LayerTypePtr layer)
{
	gdl_iterator_t it;
	LineType *line;
	ArcType *arc;
	TextType *text;
	PolygonType *polygon;

	/* write information about non empty layers */
	if (!LAYER_IS_EMPTY(layer) || (layer->Name && *layer->Name)) {
		fprintf(FP, "Layer(%i ", (int) Number + 1);
		PrintQuotedString(FP, (char *) EMPTY(layer->Name));
		fputs(")\n(\n", FP);
		WriteAttributeList(FP, &layer->Attributes, "\t");

		linelist_foreach(&layer->Line, &it, line) {
			pcb_fprintf(FP, "\tLine[%mr %mr %mr %mr %mr %mr %s]\n",
									line->Point1.X, line->Point1.Y,
									line->Point2.X, line->Point2.Y, line->Thickness, line->Clearance, F2S(line, LINE_TYPE));
		}
		arclist_foreach(&layer->Arc, &it, arc) {
			pcb_fprintf(FP, "\tArc[%mr %mr %mr %mr %mr %mr %ma %ma %s]\n",
									arc->X, arc->Y, arc->Width,
									arc->Height, arc->Thickness, arc->Clearance, arc->StartAngle, arc->Delta, F2S(arc, ARC_TYPE));
		}
		textlist_foreach(&layer->Text, &it, text) {
			pcb_fprintf(FP, "\tText[%mr %mr %d %d ", text->X, text->Y, text->Direction, text->Scale);
			PrintQuotedString(FP, (char *) EMPTY(text->TextString));
			fprintf(FP, " %s]\n", F2S(text, TEXT_TYPE));
		}
		textlist_foreach(&layer->Polygon, &it, polygon) {
			int p, i = 0;
			Cardinal hole = 0;
			fprintf(FP, "\tPolygon(%s)\n\t(", F2S(polygon, POLYGON_TYPE));
			for (p = 0; p < polygon->PointN; p++) {
				PointTypePtr point = &polygon->Points[p];

				if (hole < polygon->HoleIndexN && p == polygon->HoleIndex[hole]) {
					if (hole > 0)
						fputs("\n\t\t)", FP);
					fputs("\n\t\tHole (", FP);
					hole++;
					i = 0;
				}

				if (i++ % 5 == 0) {
					fputs("\n\t\t", FP);
					if (hole)
						fputs("\t", FP);
				}
				pcb_fprintf(FP, "[%mr %mr] ", point->X, point->Y);
			}
			if (hole > 0)
				fputs("\n\t\t)", FP);
			fputs("\n\t)\n", FP);
		}
		fputs(")\n", FP);
	}
}

/* ---------------------------------------------------------------------------
 * writes just the elements in the buffer to file
 */
static int WriteBuffer(FILE * FP)
{
	Cardinal i;

	WriteViaData(FP, PASTEBUFFER->Data);
	WriteElementData(FP, PASTEBUFFER->Data);
	for (i = 0; i < max_copper_layer + 2; i++)
		WriteLayerData(FP, i, &(PASTEBUFFER->Data->Layer[i]));
	return (STATUS_OK);
}

/* ---------------------------------------------------------------------------
 * writes PCB to file
 */
static int WritePCB(FILE * FP)
{
	Cardinal i;

	WritePCBInfoHeader(FP);
	WritePCBDataHeader(FP);
	WritePCBFontData(FP);
	WriteAttributeList(FP, &PCB->Attributes, "");
	WriteViaData(FP, PCB->Data);
	WriteElementData(FP, PCB->Data);
	WritePCBRatData(FP);
	for (i = 0; i < max_copper_layer + 2; i++)
		WriteLayerData(FP, i, &(PCB->Data->Layer[i]));
	WritePCBNetlistData(FP);
	WritePCBNetlistPatchData(FP);

	return (STATUS_OK);
}

/* ---------------------------------------------------------------------------
 * writes PCB to file
 */
static int WritePCBFile(char *Filename)
{
	FILE *fp;
	int result;

	if ((fp = fopen(Filename, "w")) == NULL) {
		OpenErrorMessage(Filename);
		return (STATUS_ERROR);
	}
	result = WritePCB(fp);
	fclose(fp);
	return (result);
}

/* ---------------------------------------------------------------------------
 * writes to pipe using the command defined by Settings.SaveCommand
 * %f are replaced by the passed filename
 */
static int WritePipe(char *Filename, bool thePcb)
{
	FILE *fp;
	int result;
	char *p;
	static DynamicStringType command;
	int used_popen = 0;

	if (EMPTY_STRING_P(Settings.SaveCommand)) {
		fp = fopen(Filename, "w");
		if (fp == 0) {
			Message("Unable to write to file %s\n", Filename);
			return STATUS_ERROR;
		}
	}
	else {
		used_popen = 1;
		/* setup commandline */
		DSClearString(&command);
		for (p = Settings.SaveCommand; *p; p++) {
			/* copy character if not special or add string to command */
			if (!(*p == '%' && *(p + 1) == 'f'))
				DSAddCharacter(&command, *p);
			else {
				DSAddString(&command, Filename);

				/* skip the character */
				p++;
			}
		}
		DSAddCharacter(&command, '\0');
		printf("write to pipe \"%s\"\n", command.Data);
		if ((fp = popen(command.Data, "w")) == NULL) {
			PopenErrorMessage(command.Data);
			return (STATUS_ERROR);
		}
	}
	if (thePcb) {
		if (PCB->is_footprint) {
			WriteElementData(fp, PCB->Data);
			result = 0;
		}
		else
			result = WritePCB(fp);
	}
	else
		result = WriteBuffer(fp);

	if (used_popen)
		return (pclose(fp) ? STATUS_ERROR : result);
	return (fclose(fp) ? STATUS_ERROR : result);
}

/* ---------------------------------------------------------------------------
 * saves the layout in a temporary file
 * this is used for fatal errors and does not call the program specified
 * in 'saveCommand' for safety reasons
 */
void SaveInTMP(void)
{
	char filename[80];

	/* memory might have been released before this function is called */
	if (PCB && PCB->Changed) {
		sprintf(filename, EMERGENCY_NAME, (int) getpid());
		Message(_("Trying to save your layout in '%s'\n"), filename);
		WritePCBFile(filename);
	}
}

/* ---------------------------------------------------------------------------
 * front-end for 'SaveInTMP()'
 * just makes sure that the routine is only called once
 */
static bool dont_save_any_more = false;
void EmergencySave(void)
{

	if (!dont_save_any_more) {
		SaveInTMP();
		dont_save_any_more = true;
	}
}

void DisableEmergencySave(void)
{
	dont_save_any_more = true;
}

/* ----------------------------------------------------------------------
 * Callback for the autosave
 */

static hidval backup_timer;

/*  
 * If the backup interval is > 0 then set another timer.  Otherwise
 * we do nothing and it is up to the GUI to call EnableAutosave()
 * after setting Settings.BackupInterval > 0 again.
 */
static void backup_cb(hidval data)
{
	backup_timer.ptr = NULL;
	Backup();
	if (Settings.BackupInterval > 0 && gui->add_timer)
		backup_timer = gui->add_timer(backup_cb, 1000 * Settings.BackupInterval, data);
}

void EnableAutosave(void)
{
	hidval x;

	x.ptr = NULL;

	/* If we already have a timer going, then cancel it out */
	if (backup_timer.ptr != NULL && gui->stop_timer)
		gui->stop_timer(backup_timer);

	backup_timer.ptr = NULL;
	/* Start up a new timer */
	if (Settings.BackupInterval > 0 && gui->add_timer)
		backup_timer = gui->add_timer(backup_cb, 1000 * Settings.BackupInterval, x);
}

/* ---------------------------------------------------------------------------
 * creates backup file.  The default is to use the pcb file name with
 * a "-" appended (like "foo.pcb-") and if we don't have a pcb file name
 * then use the template in BACKUP_NAME
 */
void Backup(void)
{
	char *filename = NULL;

	if (PCB && PCB->Filename) {
		filename = (char *) malloc(sizeof(char) * (strlen(PCB->Filename) + 2));
		if (filename == NULL) {
			fprintf(stderr, "Backup():  malloc failed\n");
			exit(1);
		}
		sprintf(filename, "%s-", PCB->Filename);
	}
	else {
		/* BACKUP_NAME has %.8i which  will be replaced by the process ID */
		filename = (char *) malloc(sizeof(char) * (strlen(BACKUP_NAME) + 8));
		if (filename == NULL) {
			fprintf(stderr, "Backup():  malloc failed\n");
			exit(1);
		}
		sprintf(filename, BACKUP_NAME, (int) getpid());
	}

	WritePCBFile(filename);
	free(filename);
}

#if !defined(HAS_ATEXIT) && !defined(HAS_ON_EXIT)
/* ---------------------------------------------------------------------------
 * makes a temporary copy of the data. This is useful for systems which
 * doesn't support calling functions on exit. We use this to save the data
 * before LEX and YACC functions are called because they are able to abort
 * the program.
 */
void SaveTMPData(void)
{
	sprintf(TMPFilename, EMERGENCY_NAME, (int) getpid());
	WritePCBFile(TMPFilename);
}

/* ---------------------------------------------------------------------------
 * removes the temporary copy of the data file
 */
void RemoveTMPData(void)
{
	unlink(TMPFilename);
}
#endif

/* ---------------------------------------------------------------------------
 * Parse the directory tree where newlib footprints are found
 */

/* Helper function for ParseLibraryTree */
static const char *pcb_basename(const char *p)
{
	char *rv = strrchr(p, '/');
	if (rv)
		return rv + 1;
	return p;
}

typedef struct list_dir_s list_dir_t;

struct list_dir_s {
	char *parent;
	char *subdir;
	list_dir_t *next;
};

typedef struct {
	LibraryMenuTypePtr menu;
	list_dir_t *subdirs;
	int children;
} list_st_t;


static int list_cb(void *cookie, const char *subdir, const char *name, pcb_fp_type_t type, void *tags[])
{
	list_st_t *l = (list_st_t *) cookie;
	LibraryEntryTypePtr entry;		/* Pointer to individual menu entry */
	size_t len;

	if (type == PCB_FP_DIR) {
		list_dir_t *d;
		/* can not recurse directly from here because that would ruin the menu
		   pointer: GetLibraryMenuMemory (&Library) calls realloc()! 
		   Build a list of directories to be visited later, instead. */
		d = malloc(sizeof(list_dir_t));
		d->subdir = strdup(name);
		d->parent = strdup(subdir);
		d->next = l->subdirs;
		l->subdirs = d;
		return 0;
	}

	l->children++;
	entry = GetLibraryEntryMemory(l->menu);

	/* 
	 * entry->AllocatedMemory points to abs path to the footprint.
	 * entry->ListEntry points to fp name itself.
	 */
	len = strlen(subdir) + strlen("/") + strlen(name) + 8;
	entry->AllocatedMemory = (char *) calloc(1, len);
	strcat(entry->AllocatedMemory, subdir);
	strcat(entry->AllocatedMemory, PCB_DIR_SEPARATOR_S);

	/* store pointer to start of footprint name */
	entry->ListEntry = entry->AllocatedMemory + strlen(entry->AllocatedMemory);

	/* Now place footprint name into AllocatedMemory */
	strcat(entry->AllocatedMemory, name);

	if (type == PCB_FP_PARAMETRIC)
		strcat(entry->AllocatedMemory, "()");

	entry->Type = type;

	entry->Tags = tags;

	return 0;
}

static int LoadNewlibFootprintsFromDir(const char *subdir, const char *toppath, int is_root)
{
	LibraryMenuTypePtr menu = NULL;	/* Pointer to PCB's library menu structure */
	list_st_t l;
	list_dir_t *d, *nextd;
	char working_[MAXPATHLEN + 1];
	char *working;								/* String holding abs path to working dir */
	int menuidx;

	sprintf(working_, "%s%c%s", toppath, PCB_DIR_SEPARATOR_C, subdir);
	resolve_path(working_, &working);

	/* Get pointer to memory holding menu */
	menu = GetLibraryMenuMemory(&Library, &menuidx);


	/* Populate menuname and path vars */
	menu->Name = strdup(pcb_basename(subdir));
	menu->directory = strdup(pcb_basename(toppath));

	l.menu = menu;
	l.subdirs = NULL;
	l.children = 0;

	pcb_fp_list(working, 0, list_cb, &l, is_root, 1);

	/* now recurse to each subdirectory mapped in the previous call;
	   by now we don't care if menu is ruined by the realloc() in GetLibraryMenuMemory() */
	for (d = l.subdirs; d != NULL; d = nextd) {
		l.children += LoadNewlibFootprintsFromDir(d->subdir, d->parent, 0);
		nextd = d->next;
		free(d->subdir);
		free(d->parent);
		free(d);
	}
	if (l.children == 0) {
		DeleteLibraryMenuMemory(&Library, menuidx);
	}
	free(working);
	return l.children;
}



/* This function loads the newlib footprints into the Library.
 * It examines all directories pointed to by Settings.LibraryTree.
 * In each directory specified there, it looks both in that directory,
 * as well as *one* level down.  It calls the subfunction 
 * LoadNewlibFootprintsFromDir to put the footprints into PCB's internal
 * datastructures.
 */
static int ParseLibraryTree(void)
{
	char toppath[MAXPATHLEN + 1];	/* String holding abs path to top level library dir */
	char *libpaths;								/* String holding list of library paths to search */
	char *p;											/* Helper string used in iteration */
	DIR *dirobj;									/* Iterable directory object */
	struct dirent *direntry = NULL;	/* Object holding individual directory entries */
	struct stat buffer;						/* buffer used in stat */
	int n_footprints = 0;					/* Running count of footprints found */

	/* Initialize path, working by writing 0 into every byte. */
	memset(toppath, 0, sizeof toppath);

	/* Additional loop to allow for multiple 'newlib' style library directories 
	 * called out in Settings.LibraryTree
	 */
	libpaths = strdup(Settings.LibrarySearchPaths);
	for (p = strtok(libpaths, PCB_PATH_DELIMETER); p && *p; p = strtok(NULL, PCB_PATH_DELIMETER)) {
		/* remove trailing path delimeter */
		strncpy(toppath, p, sizeof(toppath) - 1);

#ifdef DEBUG
		printf("In ParseLibraryTree, looking for newlib footprints inside top level directory %s ... \n", toppath);
#endif

		/* Next read in any footprints in the top level dir */
		n_footprints += LoadNewlibFootprintsFromDir(".", toppath, 1);
	}

#ifdef DEBUG
	printf("Leaving ParseLibraryTree, found %d footprints.\n", n_footprints);
#endif

	return n_footprints;
}

int ReadLibraryContents(void)
{
	static char *command = NULL;
	char inputline[MAX_LIBRARY_LINE_LENGTH + 1];
	FILE *resultFP = NULL;
	LibraryMenuTypePtr menu = NULL;
	LibraryEntryTypePtr entry;

	/* List all footprint libraries.  Then sort the whole
	 * library.
	 */
	if (ParseLibraryTree() > 0 || resultFP != NULL) {
		sort_library(&Library);
		return 0;
	}

	return (1);
}

#define BLANK(x) ((x) == ' ' || (x) == '\t' || (x) == '\n' \
		|| (x) == '\0')

/* ---------------------------------------------------------------------------
 * Read in a netlist and store it in the netlist menu 
 */

int ReadNetlist(char *filename)
{
	static char *command = NULL;
	char inputline[MAX_NETLIST_LINE_LENGTH + 1];
	char temp[MAX_NETLIST_LINE_LENGTH + 1];
	FILE *fp;
	LibraryMenuTypePtr menu = NULL;
	LibraryEntryTypePtr entry;
	int i, j, lines, kind;
	bool continued;
	int used_popen = 0;

	if (!filename)
		return (1);									/* nothing to do */

	Message(_("Importing PCB netlist %s\n"), filename);

	if (EMPTY_STRING_P(Settings.RatCommand)) {
		fp = fopen(filename, "r");
		if (!fp) {
			Message("Cannot open %s for reading", filename);
			return 1;
		}
	}
	else {
		used_popen = 1;
		free(command);
		command = EvaluateFilename(Settings.RatCommand, Settings.RatPath, filename, NULL);

		/* open pipe to stdout of command */
		if (*command == '\0' || (fp = popen(command, "r")) == NULL) {
			PopenErrorMessage(command);
			return (1);
		}
	}
	lines = 0;
	/* kind = 0  is net name
	 * kind = 1  is route style name
	 * kind = 2  is connection
	 */
	kind = 0;
	while (fgets(inputline, MAX_NETLIST_LINE_LENGTH, fp)) {
		size_t len = strlen(inputline);
		/* check for maximum length line */
		if (len) {
			if (inputline[--len] != '\n')
				Message(_("Line length (%i) exceeded in netlist file.\n"
									"additional characters will be ignored.\n"), MAX_NETLIST_LINE_LENGTH);
			else
				inputline[len] = '\0';
		}
		continued = (inputline[len - 1] == '\\') ? true : false;
		if (continued)
			inputline[len - 1] = '\0';
		lines++;
		i = 0;
		while (inputline[i] != '\0') {
			j = 0;
			/* skip leading blanks */
			while (inputline[i] != '\0' && BLANK(inputline[i]))
				i++;
			if (kind == 0) {
				/* add two spaces for included/unincluded */
				temp[j++] = ' ';
				temp[j++] = ' ';
			}
			while (!BLANK(inputline[i]))
				temp[j++] = inputline[i++];
			temp[j] = '\0';
			while (inputline[i] != '\0' && BLANK(inputline[i]))
				i++;
			if (kind == 0) {
				menu = GetLibraryMenuMemory(&PCB->NetlistLib[NETLIST_INPUT], NULL);
				menu->Name = strdup(temp);
				menu->flag = 1;
				kind++;
			}
			else {
				if (kind == 1 && strchr(temp, '-') == NULL) {
					kind++;
					menu->Style = strdup(temp);
				}
				else {
					entry = GetLibraryEntryMemory(menu);
					entry->ListEntry = strdup(temp);
				}
			}
		}
		if (!continued)
			kind = 0;
	}
	if (!lines) {
		Message(_("Empty netlist file!\n"));
		pclose(fp);
		return (1);
	}
	if (used_popen)
		pclose(fp);
	else
		fclose(fp);
	sort_netlist();
	rats_patch_make_edited(PCB);
	return (0);
}

int ImportNetlist(char *filename)
{
	FILE *fp;
	char buf[16];
	int i;
	char *p;


	if (!filename)
		return (1);									/* nothing to do */
	fp = fopen(filename, "r");
	if (!fp)
		return (1);									/* bad filename */
	i = fread(buf, 1, sizeof(buf) - 1, fp);
	fclose(fp);
	buf[i] = '\0';
	p = buf;
	while (*p) {
		*p = tolower((int) *p);
		p++;
	}
	p = strstr(buf, "edif");
	if (!p)
		return ReadNetlist(filename);
	else
		return stub_ReadEdifNetlist(filename);
}
