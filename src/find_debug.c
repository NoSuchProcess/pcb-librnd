/*
 *
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* find-related debug functions */

/* ---------------------------------------------------------------------------
 * writes the several names of an element to a file
 */
static void PrintElementNameList(pcb_element_t *Element, FILE * FP)
{
	fputc('(', FP);
	PrintQuotedString(FP, (char *) PCB_EMPTY(DESCRIPTION_NAME(Element)));
	fputc(' ', FP);
	PrintQuotedString(FP, (char *) PCB_EMPTY(NAMEONPCB_NAME(Element)));
	fputc(' ', FP);
	PrintQuotedString(FP, (char *) PCB_EMPTY(VALUE_NAME(Element)));
	fputc(')', FP);
	fputc('\n', FP);
}

/* ---------------------------------------------------------------------------
 * writes the several names of an element to a file
 */
static void PrintConnectionElementName(pcb_element_t *Element, FILE * FP)
{
	fputs("Element", FP);
	PrintElementNameList(Element, FP);
	fputs("{\n", FP);
}

/* ---------------------------------------------------------------------------
 * prints one {pin,pad,via}/element entry of connection lists
 */
static void PrintConnectionListEntry(char *ObjName, pcb_element_t *Element, pcb_bool FirstOne, FILE * FP)
{
	if (FirstOne) {
		fputc('\t', FP);
		PrintQuotedString(FP, ObjName);
		fprintf(FP, "\n\t{\n");
	}
	else {
		fprintf(FP, "\t\t");
		PrintQuotedString(FP, ObjName);
		fputc(' ', FP);
		if (Element)
			PrintElementNameList(Element, FP);
		else
			fputs("(__VIA__)\n", FP);
	}
}

/* ---------------------------------------------------------------------------
 * prints all found connections of a pads to file FP
 * the connections are stacked in 'PadList'
 */
static void PrintPadConnections(pcb_cardinal_t Layer, FILE * FP, pcb_bool IsFirst)
{
	pcb_cardinal_t i;
	pcb_pad_t *ptr;

	if (!PadList[Layer].Number)
		return;

	/* the starting pad */
	if (IsFirst) {
		ptr = PADLIST_ENTRY(Layer, 0);
		if (ptr != NULL)
			PrintConnectionListEntry((char *) PCB_UNKNOWN(ptr->Name), NULL, pcb_true, FP);
		else
			printf("Skipping NULL ptr in 1st part of PrintPadConnections\n");
	}

	/* we maybe have to start with i=1 if we are handling the
	 * starting-pad itself
	 */
	for (i = IsFirst ? 1 : 0; i < PadList[Layer].Number; i++) {
		ptr = PADLIST_ENTRY(Layer, i);
		if (ptr != NULL)
			PrintConnectionListEntry((char *) PCB_EMPTY(ptr->Name), (pcb_element_t *) ptr->Element, pcb_false, FP);
		else
			printf("Skipping NULL ptr in 2nd part of PrintPadConnections\n");
	}
}

/* ---------------------------------------------------------------------------
 * prints all found connections of a pin to file FP
 * the connections are stacked in 'PVList'
 */
static void PrintPinConnections(FILE * FP, pcb_bool IsFirst)
{
	pcb_cardinal_t i;
	pcb_pin_t *pv;

	if (!PVList.Number)
		return;

	if (IsFirst) {
		/* the starting pin */
		pv = PVLIST_ENTRY(0);
		PrintConnectionListEntry((char *) PCB_EMPTY(pv->Name), NULL, pcb_true, FP);
	}

	/* we maybe have to start with i=1 if we are handling the
	 * starting-pin itself
	 */
	for (i = IsFirst ? 1 : 0; i < PVList.Number; i++) {
		/* get the elements name or assume that its a via */
		pv = PVLIST_ENTRY(i);
		PrintConnectionListEntry((char *) PCB_EMPTY(pv->Name), (pcb_element_t *) pv->Element, pcb_false, FP);
	}
}
