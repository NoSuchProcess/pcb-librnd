/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

/* action routines for output window
 */

#include "config.h"
#include "conf_core.h"

#include "data.h"
#include "funchash_core.h"

#include "board.h"
#include "action_helper.h"
#include "hid_actions.h"
#include "change.h"
#include "draw.h"
#include "search.h"
#include "set.h"
#include "undo.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "obj_rat_draw.h"

static void ChangeFlag(const char *, const char *, int, const char *);
static int ActionChangeSize(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);
static int ActionChange2ndSize(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);

/* --------------------------------------------------------------------------- */

static const char changeclearsize_syntax[] =
	"ChangeClearSize(Object, delta|style)\n"
	"ChangeClearSize(SelectedPins|SelectedPads|SelectedVias, delta|style)\n"
	"ChangeClearSize(SelectedLines|SelectedArcs, delta|style)\n" "ChangeClearSize(Selected|SelectedObjects, delta|style)";

static const char changeclearsize_help[] = "Changes the clearance size of objects.";

/* %start-doc actions ChangeClearSize

If the solder mask is currently showing, this action changes the
solder mask clearance.  If the mask is not showing, this action
changes the polygon clearance.

%end-doc */

static int ActionChangeClearSize(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	const char *delta = PCB_ACTION_ARG(1);
	const char *units = PCB_ACTION_ARG(2);
	pcb_bool absolute;
	pcb_coord_t value;
	int type = PCB_TYPE_NONE;
	void *ptr1, *ptr2, *ptr3;

	if (function && delta) {
		int funcid = pcb_funchash_get(function, NULL);

		if (funcid == F_Object) {
			gui->get_coords(_("Select an Object"), &x, &y);
			type = pcb_search_screen(x, y, CHANGECLEARSIZE_TYPES, &ptr1, &ptr2, &ptr3);
		}

		if (strcmp(argv[1], "style") == 0) {
			if ((type == PCB_TYPE_NONE) || (type == PCB_TYPE_POLYGON))	/* workaround: pcb_search_screen(CHANGECLEARSIZE_TYPES) wouldn't return elements */
				type = pcb_search_screen(x, y, CHANGE2NDSIZE_TYPES, &ptr1, &ptr2, &ptr3);
			if (pcb_get_style_size(funcid, &value, type, 2) != 0)
				return 1;
			absolute = 1;
			value *= 2;
		}
		else
			value = 2 * pcb_get_value(delta, units, &absolute, NULL);
		switch (pcb_funchash_get(function, NULL)) {
		case F_Object:
			{
				if (type != PCB_TYPE_NONE)
					if (pcb_chg_obj_clear_size(type, ptr1, ptr2, ptr3, value, absolute))
						pcb_board_set_changed_flag(pcb_true);
				break;
			}
		case F_SelectedVias:
			if (pcb_chg_selected_clear_size(PCB_TYPE_VIA, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		case F_SelectedPads:
			if (pcb_chg_selected_clear_size(PCB_TYPE_PAD, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		case F_SelectedPins:
			if (pcb_chg_selected_clear_size(PCB_TYPE_PIN, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		case F_SelectedLines:
			if (pcb_chg_selected_clear_size(PCB_TYPE_LINE, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		case F_SelectedArcs:
			if (pcb_chg_selected_clear_size(PCB_TYPE_ARC, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		case F_Selected:
		case F_SelectedObjects:
			if (pcb_chg_selected_clear_size(CHANGECLEARSIZE_TYPES, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char changeflag_syntax[] =
	"ChangeFlag(Object|Selected|SelectedObjects, flag, value)\n"
	"ChangeFlag(SelectedLines|SelectedPins|SelectedVias, flag, value)\n"
	"ChangeFlag(SelectedPads|SelectedTexts|SelectedNames, flag, value)\n"
	"ChangeFlag(SelectedElements, flag, value)\n" "flag = square | octagon | thermal | join\n" "value = 0 | 1";

static const char changeflag_help[] = "Sets or clears flags on objects.";

/* %start-doc actions ChangeFlag

Toggles the given flag on the indicated object(s).  The flag may be
one of the flags listed above (square, octagon, thermal, join).  The
value may be the number 0 or 1.  If the value is 0, the flag is
cleared.  If the value is 1, the flag is set.

%end-doc */

static int ActionChangeFlag(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	const char *flag = PCB_ACTION_ARG(1);
	int value = argc > 2 ? atoi(argv[2]) : -1;
	if (value != 0 && value != 1)
		PCB_AFAIL(changeflag);

	ChangeFlag(function, flag, value, "ChangeFlag");
	return 0;
}


static void ChangeFlag(const char *what, const char *flag_name, int value,
											 const char *cmd_name)
{
	pcb_bool(*set_object) (int, void *, void *, void *);
	pcb_bool(*set_selected) (int);

	if (PCB_NSTRCMP(flag_name, "square") == 0) {
		set_object = value ? pcb_set_obj_square : pcb_clr_obj_square;
		set_selected = value ? pcb_set_selected_square : pcb_clr_selected_square;
	}
	else if (PCB_NSTRCMP(flag_name, "octagon") == 0) {
		set_object = value ? pcb_set_obj_octagon : pcb_clr_obj_octagon;
		set_selected = value ? pcb_set_selected_octagon : pcb_clr_selected_octagon;
	}
	else if (PCB_NSTRCMP(flag_name, "join") == 0) {
		/* Note: these are backwards, because the flag is "clear" but
		   the command is "join".  */
		set_object = value ? pcb_clr_obj_join : pcb_set_obj_join;
		set_selected = value ? pcb_clr_selected_join : pcb_set_selected_join;
	}
	else {
		pcb_message(PCB_MSG_DEFAULT, _("%s():  Flag \"%s\" is not valid\n"), cmd_name, flag_name);
		return;
	}

	switch (pcb_funchash_get(what, NULL)) {
	case F_Object:
		{
			int type;
			void *ptr1, *ptr2, *ptr3;

			if ((type = pcb_search_screen(Crosshair.X, Crosshair.Y, CHANGESIZE_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE)
				if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_pin_t *) ptr2))
					pcb_message(PCB_MSG_DEFAULT, _("Sorry, the object is locked\n"));
			if (set_object(type, ptr1, ptr2, ptr3))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}

	case F_SelectedVias:
		if (set_selected(PCB_TYPE_VIA))
			pcb_board_set_changed_flag(pcb_true);
		break;

	case F_SelectedPins:
		if (set_selected(PCB_TYPE_PIN))
			pcb_board_set_changed_flag(pcb_true);
		break;

	case F_SelectedPads:
		if (set_selected(PCB_TYPE_PAD))
			pcb_board_set_changed_flag(pcb_true);
		break;

	case F_SelectedLines:
		if (set_selected(PCB_TYPE_LINE))
			pcb_board_set_changed_flag(pcb_true);
		break;

	case F_SelectedTexts:
		if (set_selected(PCB_TYPE_TEXT))
			pcb_board_set_changed_flag(pcb_true);
		break;

	case F_SelectedNames:
		if (set_selected(PCB_TYPE_ELEMENT_NAME))
			pcb_board_set_changed_flag(pcb_true);
		break;

	case F_SelectedElements:
		if (set_selected(PCB_TYPE_ELEMENT))
			pcb_board_set_changed_flag(pcb_true);
		break;

	case F_Selected:
	case F_SelectedObjects:
		if (set_selected(CHANGESIZE_TYPES))
			pcb_board_set_changed_flag(pcb_true);
		break;
	}
}

/* --------------------------------------------------------------------------- */

static const char changehold_syntax[] = "ChangeHole(ToggleObject|Object|SelectedVias|Selected)";

static const char changehold_help[] = "Changes the hole flag of objects.";

/* %start-doc actions ChangeHole

The "hole flag" of a via determines whether the via is a
plated-through hole (not set), or an unplated hole (set).

%end-doc */

static int ActionChangeHole(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_ToggleObject:
		case F_Object:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;

				gui->get_coords(_("Select an Object"), &x, &y);
				if ((type = pcb_search_screen(x, y, PCB_TYPE_VIA, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE && pcb_pin_change_hole((pcb_pin_t *) ptr3))
					IncrementUndoSerialNumber();
				break;
			}

		case F_SelectedVias:
		case F_Selected:
			if (pcb_chg_selected_hole())
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char changepaste_syntax[] = "ChangePaste(ToggleObject|Object|SelectedPads|Selected)";

static const char changepaste_help[] = "Changes the no paste flag of objects.";

/* %start-doc actions ChangePaste

The "no paste flag" of a pad determines whether the solderpaste
 stencil will have an opening for the pad (no set) or if there will be
 no solderpaste on the pad (set).  This is used for things such as
 fiducial pads.

%end-doc */

static int ActionChangePaste(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_ToggleObject:
		case F_Object:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;

				gui->get_coords(_("Select an Object"), &x, &y);
				if ((type = pcb_search_screen(x, y, PCB_TYPE_PAD, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE && pcb_pad_change_paste((pcb_pad_t *) ptr3))
					IncrementUndoSerialNumber();
				break;
			}

		case F_SelectedPads:
		case F_Selected:
			if (pcb_chg_selected_paste())
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
}


/* --------------------------------------------------------------------------- */

static const char changesizes_syntax[] =
	"ChangeSizes(Object, delta|style)\n"
	"ChangeSizes(SelectedObjects|Selected, delta|style)\n"
	"ChangeSizes(SelectedLines|SelectedPins|SelectedVias, delta|style)\n"
	"ChangeSizes(SelectedPads|SelectedTexts|SelectedNames, delta|style)\n" "ChangeSizes(SelectedElements, delta|style)";

static const char changesizes_help[] = "Changes all sizes of objects.";

/* %start-doc actions ChangeSize

Call ActionChangeSize, ActionChangeDrillSize and ActionChangeClearSize
with the same arguments. If any of them did not fail, return success.
%end-doc */

static int ActionChangeSizes(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int a, b, c;
	SaveUndoSerialNumber();
	a = ActionChangeSize(argc, argv, x, y);
	RestoreUndoSerialNumber();
	b = ActionChange2ndSize(argc, argv, x, y);
	RestoreUndoSerialNumber();
	c = ActionChangeClearSize(argc, argv, x, y);
	RestoreUndoSerialNumber();
	IncrementUndoSerialNumber();
	return !(!a || !b || !c);
}

/* --------------------------------------------------------------------------- */

static const char changesize_syntax[] =
	"ChangeSize(Object, delta|style)\n"
	"ChangeSize(SelectedObjects|Selected, delta|style)\n"
	"ChangeSize(SelectedLines|SelectedPins|SelectedVias, delta|style)\n"
	"ChangeSize(SelectedPads|SelectedTexts|SelectedNames, delta|style)\n" "ChangeSize(SelectedElements, delta|style)";

static const char changesize_help[] = "Changes the size of objects.";

/* %start-doc actions ChangeSize

For lines and arcs, this changes the width.  For pins and vias, this
changes the overall diameter of the copper annulus.  For pads, this
changes the width and, indirectly, the length.  For texts and names,
this changes the scaling factor.  For elements, this changes the width
of the silk layer lines and arcs for this element.

%end-doc */

static int ActionChangeSize(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	const char *delta = PCB_ACTION_ARG(1);
	const char *units = PCB_ACTION_ARG(2);
	pcb_bool absolute;								/* indicates if absolute size is given */
	pcb_coord_t value;
	int type = PCB_TYPE_NONE, tostyle = 0;
	void *ptr1, *ptr2, *ptr3;


	if (function && delta) {
		int funcid = pcb_funchash_get(function, NULL);

		if (funcid == F_Object)
			type = pcb_search_screen(Crosshair.X, Crosshair.Y, CHANGESIZE_TYPES, &ptr1, &ptr2, &ptr3);

		if (strcmp(argv[1], "style") == 0) {
			if (pcb_get_style_size(funcid, &value, type, 0) != 0)
				return 1;
			absolute = 1;
			tostyle = 1;
		}
		else
			value = pcb_get_value(delta, units, &absolute, NULL);
		switch (funcid) {
		case F_Object:
			{
				if (type != PCB_TYPE_NONE)
					if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_pin_t *) ptr2))
						pcb_message(PCB_MSG_DEFAULT, _("Sorry, the object is locked\n"));
				if (tostyle) {
					if (pcb_chg_obj_1st_size(type, ptr1, ptr2, ptr3, value, absolute))
						pcb_board_set_changed_flag(pcb_true);
				}
				else {
					if (pcb_chg_obj_size(type, ptr1, ptr2, ptr3, value, absolute))
						pcb_board_set_changed_flag(pcb_true);
				}
				break;
			}
		case F_SelectedVias:
			if (pcb_chg_selected_size(PCB_TYPE_VIA, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedPins:
			if (pcb_chg_selected_size(PCB_TYPE_PIN, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedPads:
			if (pcb_chg_selected_size(PCB_TYPE_PAD, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedArcs:
			if (pcb_chg_selected_size(PCB_TYPE_ARC, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedLines:
			if (pcb_chg_selected_size(PCB_TYPE_LINE, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedTexts:
			if (pcb_chg_selected_size(PCB_TYPE_TEXT, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedNames:
			if (pcb_chg_selected_size(PCB_TYPE_ELEMENT_NAME, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedElements:
			if (pcb_chg_selected_size(PCB_TYPE_ELEMENT, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_Selected:
		case F_SelectedObjects:
			if (pcb_chg_selected_size(CHANGESIZE_TYPES, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char changedrillsize_syntax[] =
	"ChangeDrillSize(Object, delta|style)\n" "ChangeDrillSize(SelectedPins|SelectedVias|Selected|SelectedObjects, delta|style)";

static const char changedrillsize_help[] = "Changes the drilling hole size of objects.";

/* %start-doc actions ChangeDrillSize

%end-doc */

static int ActionChange2ndSize(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	const char *delta = PCB_ACTION_ARG(1);
	const char *units = PCB_ACTION_ARG(2);
	int type = PCB_TYPE_NONE;
	void *ptr1, *ptr2, *ptr3;

	pcb_bool absolute;
	pcb_coord_t value;

	if (function && delta) {
		int funcid = pcb_funchash_get(function, NULL);

		if (funcid == F_Object) {
			gui->get_coords(_("Select an Object"), &x, &y);
			type = pcb_search_screen(x, y, CHANGE2NDSIZE_TYPES, &ptr1, &ptr2, &ptr3);
		}

		if (strcmp(argv[1], "style") == 0) {
			if (pcb_get_style_size(funcid, &value, type, 1) != 0)
				return 1;
			absolute = 1;
		}
		else
			value = pcb_get_value(delta, units, &absolute, NULL);

		switch (pcb_funchash_get(function, NULL)) {
		case F_Object:
			{

				if (type != PCB_TYPE_NONE)
					if (pcb_chg_obj_2nd_size(type, ptr1, ptr2, ptr3, value, absolute, pcb_true))
						pcb_board_set_changed_flag(pcb_true);
				break;
			}

		case F_SelectedVias:
			if (pcb_chg_selected_2nd_size(PCB_TYPE_VIA, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedPins:
			if (pcb_chg_selected_2nd_size(PCB_TYPE_PIN, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		case F_Selected:
		case F_SelectedObjects:
			if (pcb_chg_selected_2nd_size(PCB_TYPEMASK_PIN, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
}

/* ---------------------------------------------------------------------------  */

static const char changepinname_syntax[] = "ChangePinName(ElementName,PinNumber,PinName)";

static const char changepinname_help[] = "Sets the name of a specific pin on a specific element.";

/* %start-doc actions ChangePinName

This can be especially useful for annotating pin names from a
schematic to the layout without requiring knowledge of the pcb file
format.

@example
ChangePinName(U3, 7, VCC)
@end example

%end-doc */

static int ActionChangePinName(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int changed = 0;
	const char *refdes, *pinnum, *pinname;

	if (argc != 3) {
		PCB_AFAIL(changepinname);
	}

	refdes = argv[0];
	pinnum = argv[1];
	pinname = argv[2];

	PCB_ELEMENT_LOOP(PCB->Data);
	{
		if (PCB_NSTRCMP(refdes, NAMEONPCB_NAME(element)) == 0) {
			PCB_PIN_LOOP(element);
			{
				if (PCB_NSTRCMP(pinnum, pin->Number) == 0) {
					AddObjectToChangeNameUndoList(PCB_TYPE_PIN, NULL, NULL, pin, pin->Name);
					/*
					 * Note:  we can't free() pin->Name first because
					 * it is used in the undo list
					 */
					pin->Name = pcb_strdup(pinname);
					pcb_board_set_changed_flag(pcb_true);
					changed = 1;
				}
			}
			END_LOOP;

			PCB_PAD_LOOP(element);
			{
				if (PCB_NSTRCMP(pinnum, pad->Number) == 0) {
					AddObjectToChangeNameUndoList(PCB_TYPE_PAD, NULL, NULL, pad, pad->Name);
					/*
					 * Note:  we can't free() pad->Name first because
					 * it is used in the undo list
					 */
					pad->Name = pcb_strdup(pinname);
					pcb_board_set_changed_flag(pcb_true);
					changed = 1;
				}
			}
			END_LOOP;
		}
	}
	END_LOOP;
	/*
	 * done with our action so increment the undo # if we actually
	 * changed anything
	 */
	if (changed) {
		if (defer_updates)
			defer_needs_update = 1;
		else {
			IncrementUndoSerialNumber();
			gui->invalidate_all();
		}
	}

	return 0;
}

/* --------------------------------------------------------------------------- */

static const char changename_syntax[] = "ChangeName(Object)\n" "ChangeName(Object|\"Number\")\n" "ChangeName(Layout|Layer)";

static const char changename_help[] = "Sets the name (or pin number) of objects.";

/* %start-doc actions ChangeName

@table @code

@item Object
Changes the name of the element under the cursor.

@item Layout
Changes the name of the layout.  This is printed on the fab drawings.

@item Layer
Changes the name of the currently active layer.

@end table

%end-doc */

int ActionChangeName(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	const char *pinnums = PCB_ACTION_ARG(1);
	char *name;
	int pinnum;

	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
			/* change the name of an object */
		case F_Object:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;

				gui->get_coords(_("Select an Object"), &x, &y);
				if ((type = pcb_search_screen(x, y, CHANGENAME_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE) {
					SaveUndoSerialNumber();
					if ((pinnums != NULL) && (strcasecmp(pinnums, "Number") == 0))
						pinnum = 1;
					else
						pinnum = 0;
					if (pcb_chg_obj_name_query(type, ptr1, ptr2, ptr3, pinnum)) {
						pcb_board_set_changed_flag(pcb_true);
						if (type == PCB_TYPE_ELEMENT) {
							pcb_rubberband_t *ptr;
							int i;

							RestoreUndoSerialNumber();
							Crosshair.AttachedObject.RubberbandN = 0;
							pcb_rubber_band_lookup_rat_lines(type, ptr1, ptr2, ptr3);
							ptr = Crosshair.AttachedObject.Rubberband;
							for (i = 0; i < Crosshair.AttachedObject.RubberbandN; i++, ptr++) {
								if (PCB->RatOn)
									EraseRat((pcb_rat_t *) ptr->Line);
								MoveObjectToRemoveUndoList(PCB_TYPE_RATLINE, ptr->Line, ptr->Line, ptr->Line);
							}
							IncrementUndoSerialNumber();
							pcb_draw();
						}
					}
				}
				break;
			}

			/* change the layout's name */
		case F_Layout:
			name = gui->prompt_for(_("Enter the layout name:"), PCB_EMPTY(PCB->Name));
			/* NB: ChangeLayoutName takes ownership of the passed memory */
			if (name && pcb_board_change_name(name))
				pcb_board_set_changed_flag(pcb_true);
			break;

			/* change the name of the active layer */
		case F_Layer:
			name = gui->prompt_for(_("Enter the layer name:"), PCB_EMPTY(CURRENT->Name));
			/* NB: ChangeLayerName takes ownership of the passed memory */
			if (name && pcb_layer_change_name(CURRENT, name))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char changejoin_syntax[] = "ChangeJoin(ToggleObject|SelectedLines|SelectedArcs|Selected)";

static const char changejoin_help[] = "Changes the join (clearance through polygons) of objects.";

/* %start-doc actions ChangeJoin

The join flag determines whether a line or arc, drawn to intersect a
polygon, electrically connects to the polygon or not.  When joined,
the line/arc is simply drawn over the polygon, making an electrical
connection.  When not joined, a gap is drawn between the line and the
polygon, insulating them from each other.

%end-doc */

static int ActionChangeJoin(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_ToggleObject:
		case F_Object:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;

				gui->get_coords(_("Select an Object"), &x, &y);
				if ((type = pcb_search_screen(x, y, CHANGEJOIN_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE)
					if (pcb_chg_obj_join(type, ptr1, ptr2, ptr3))
						pcb_board_set_changed_flag(pcb_true);
				break;
			}

		case F_SelectedLines:
			if (pcb_chg_selected_join(PCB_TYPE_LINE))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedArcs:
			if (pcb_chg_selected_join(PCB_TYPE_ARC))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_Selected:
		case F_SelectedObjects:
			if (pcb_chg_selected_join(CHANGEJOIN_TYPES))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char changenonetlist_syntax[] =
	"ChangeNonetlist(ToggleObject)\n" "ChangeNonetlist(SelectedElements)\n" "ChangeNonetlist(Selected|SelectedObjects)";

static const char changenonetlist_help[] = "Changes the nonetlist flag of elements.";

/* %start-doc actions ChangeNonetlist

Note that @code{Pins} means both pins and pads.

@pinshapes

%end-doc */

static int ActionChangeNonetlist(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_ToggleObject:
		case F_Object:
		case F_Element:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;
				gui->get_coords(_("Select an Element"), &x, &y);

				ptr3 = NULL;
				type = pcb_search_screen(x, y, CHANGENONETLIST_TYPES, &ptr1, &ptr2, &ptr3);
				if (pcb_chg_obj_nonetlist(type, ptr1, ptr2, ptr3))
					pcb_board_set_changed_flag(pcb_true);
				break;
			}
		case F_SelectedElements:
		case F_Selected:
		case F_SelectedObjects:
			if (pcb_chg_selected_nonetlist(PCB_TYPE_ELEMENT))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
}


/* --------------------------------------------------------------------------- */

static const char changesquare_syntax[] =
	"ChangeSquare(ToggleObject)\n" "ChangeSquare(SelectedElements|SelectedPins)\n" "ChangeSquare(Selected|SelectedObjects)";

static const char changesquare_help[] = "Changes the square flag of pins and pads.";

/* %start-doc actions ChangeSquare

Note that @code{Pins} means both pins and pads.

@pinshapes

%end-doc */

static int ActionChangeSquare(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_ToggleObject:
		case F_Object:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;

				gui->get_coords(_("Select an Object"), &x, &y);

				ptr3 = NULL;
				type = pcb_search_screen(x, y, CHANGESQUARE_TYPES, &ptr1, &ptr2, &ptr3);

				if (ptr3 != NULL) {
					int qstyle = PCB_FLAG_SQUARE_GET((pcb_pin_t *) ptr3);
					qstyle++;
					if (qstyle > 17)
						qstyle = 0;
					if (type != PCB_TYPE_NONE)
						if (pcb_chg_obj_square(type, ptr1, ptr2, ptr3, qstyle))
							pcb_board_set_changed_flag(pcb_true);
				}
				break;
			}

		case F_SelectedElements:
			if (pcb_chg_selected_square(PCB_TYPE_ELEMENT))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedPins:
			if (pcb_chg_selected_square(PCB_TYPE_PIN | PCB_TYPE_PAD))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_Selected:
		case F_SelectedObjects:
			if (pcb_chg_selected_square(PCB_TYPE_PIN | PCB_TYPE_PAD))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char setsquare_syntax[] = "SetSquare(ToggleObject|SelectedElements|SelectedPins)";

static const char setsquare_help[] = "sets the square-flag of objects.";

/* %start-doc actions SetSquare

Note that @code{Pins} means pins and pads.

@pinshapes

%end-doc */

static int ActionSetSquare(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (function && *function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_ToggleObject:
		case F_Object:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;

				gui->get_coords(_("Select an Object"), &x, &y);
				if ((type = pcb_search_screen(x, y, CHANGESQUARE_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE)
					if (pcb_set_obj_square(type, ptr1, ptr2, ptr3))
						pcb_board_set_changed_flag(pcb_true);
				break;
			}

		case F_SelectedElements:
			if (pcb_set_selected_square(PCB_TYPE_ELEMENT))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedPins:
			if (pcb_set_selected_square(PCB_TYPE_PIN | PCB_TYPE_PAD))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_Selected:
		case F_SelectedObjects:
			if (pcb_set_selected_square(PCB_TYPE_PIN | PCB_TYPE_PAD))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char clearsquare_syntax[] = "ClearSquare(ToggleObject|SelectedElements|SelectedPins)";

static const char clearsquare_help[] = "Clears the square-flag of pins and pads.";

/* %start-doc actions ClearSquare

Note that @code{Pins} means pins and pads.

@pinshapes

%end-doc */

static int ActionClearSquare(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (function && *function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_ToggleObject:
		case F_Object:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;

				gui->get_coords(_("Select an Object"), &x, &y);
				if ((type = pcb_search_screen(x, y, CHANGESQUARE_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE)
					if (pcb_clr_obj_square(type, ptr1, ptr2, ptr3))
						pcb_board_set_changed_flag(pcb_true);
				break;
			}

		case F_SelectedElements:
			if (pcb_clr_selected_square(PCB_TYPE_ELEMENT))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedPins:
			if (pcb_clr_selected_square(PCB_TYPE_PIN | PCB_TYPE_PAD))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_Selected:
		case F_SelectedObjects:
			if (pcb_clr_selected_square(PCB_TYPE_PIN | PCB_TYPE_PAD))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char changeoctagon_syntax[] =
	"ChangeOctagon(Object|ToggleObject|SelectedObjects|Selected)\n" "ChangeOctagon(SelectedElements|SelectedPins|SelectedVias)";

static const char changeoctagon_help[] = "Changes the octagon-flag of pins and vias.";

/* %start-doc actions ChangeOctagon

@pinshapes

%end-doc */

static int ActionChangeOctagon(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_ToggleObject:
		case F_Object:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;

				gui->get_coords(_("Select an Object"), &x, &y);
				if ((type = pcb_search_screen(x, y, CHANGEOCTAGON_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE)
					if (pcb_chg_obj_octagon(type, ptr1, ptr2, ptr3))
						pcb_board_set_changed_flag(pcb_true);
				break;
			}

		case F_SelectedElements:
			if (pcb_chg_selected_octagon(PCB_TYPE_ELEMENT))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedPins:
			if (pcb_chg_selected_octagon(PCB_TYPE_PIN))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedVias:
			if (pcb_chg_selected_octagon(PCB_TYPE_VIA))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_Selected:
		case F_SelectedObjects:
			if (pcb_chg_selected_octagon(PCB_TYPEMASK_PIN))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char setoctagon_syntax[] = "SetOctagon(Object|ToggleObject|SelectedElements|Selected)";

static const char setoctagon_help[] = "Sets the octagon-flag of objects.";

/* %start-doc actions SetOctagon

@pinshapes

%end-doc */

static int ActionSetOctagon(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_ToggleObject:
		case F_Object:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;

				gui->get_coords(_("Select an Object"), &x, &y);
				if ((type = pcb_search_screen(x, y, CHANGEOCTAGON_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE)
					if (pcb_set_obj_octagon(type, ptr1, ptr2, ptr3))
						pcb_board_set_changed_flag(pcb_true);
				break;
			}

		case F_SelectedElements:
			if (pcb_set_selected_octagon(PCB_TYPE_ELEMENT))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedPins:
			if (pcb_set_selected_octagon(PCB_TYPE_PIN))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedVias:
			if (pcb_set_selected_octagon(PCB_TYPE_VIA))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_Selected:
		case F_SelectedObjects:
			if (pcb_set_selected_octagon(PCB_TYPEMASK_PIN))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char clearoctagon_syntax[] =
	"ClearOctagon(ToggleObject|Object|SelectedObjects|Selected)\n" "ClearOctagon(SelectedElements|SelectedPins|SelectedVias)";

static const char clearoctagon_help[] = "Clears the octagon-flag of pins and vias.";

/* %start-doc actions ClearOctagon

@pinshapes

%end-doc */

static int ActionClearOctagon(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_ToggleObject:
		case F_Object:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;

				gui->get_coords(_("Select an Object"), &x, &y);
				if ((type = pcb_search_screen(Crosshair.X, Crosshair.Y, CHANGEOCTAGON_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE)
					if (pcb_clr_obj_octagon(type, ptr1, ptr2, ptr3))
						pcb_board_set_changed_flag(pcb_true);
				break;
			}

		case F_SelectedElements:
			if (pcb_clr_selected_octagon(PCB_TYPE_ELEMENT))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedPins:
			if (pcb_clr_selected_octagon(PCB_TYPE_PIN))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_SelectedVias:
			if (pcb_clr_selected_octagon(PCB_TYPE_VIA))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_Selected:
		case F_SelectedObjects:
			if (pcb_clr_selected_octagon(PCB_TYPEMASK_PIN))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
}

/* -------------------------------------------------------------------------- */

static const char setthermal_syntax[] = "SetThermal(Object|SelectedPins|SelectedVias|Selected, Style)";

static const char setthermal_help[] =
	"Set the thermal (on the current layer) of pins or vias to the given style.\n"
	"Style = 0 means no thermal.\n"
	"Style = 1 has diagonal fingers with sharp edges.\n"
	"Style = 2 has horizontal and vertical fingers with sharp edges.\n"
	"Style = 3 is a solid connection to the plane."
	"Style = 4 has diagonal fingers with rounded edges.\n" "Style = 5 has horizontal and vertical fingers with rounded edges.\n";

/* %start-doc actions SetThermal

This changes how/whether pins or vias connect to any rectangle or polygon
on the current layer. The first argument can specify one object, or all
selected pins, or all selected vias, or all selected pins and vias.
The second argument specifies the style of connection.
There are 5 possibilities:
0 - no connection,
1 - 45 degree fingers with sharp edges,
2 - horizontal & vertical fingers with sharp edges,
3 - solid connection,
4 - 45 degree fingers with rounded corners,
5 - horizontal & vertical fingers with rounded corners.

Pins and Vias may have thermals whether or not there is a polygon available
to connect with. However, they will have no effect without the polygon.
%end-doc */

static int ActionSetThermal(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	const char *style = PCB_ACTION_ARG(1);
	void *ptr1, *ptr2, *ptr3;
	int type, kind;
	int err = 0;

	if (function && *function && style && *style) {
		pcb_bool absolute;

		kind = pcb_get_value(style, NULL, &absolute, NULL);
		if (absolute)
			switch (pcb_funchash_get(function, NULL)) {
			case F_Object:
				if ((type = pcb_search_screen(Crosshair.X, Crosshair.Y, CHANGETHERMAL_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE) {
					pcb_chg_obj_thermal(type, ptr1, ptr2, ptr3, kind);
					IncrementUndoSerialNumber();
					pcb_draw();
				}
				break;
			case F_SelectedPins:
				pcb_chg_selected_thermals(PCB_TYPE_PIN, kind);
				break;
			case F_SelectedVias:
				pcb_chg_selected_thermals(PCB_TYPE_VIA, kind);
				break;
			case F_Selected:
			case F_SelectedElements:
				pcb_chg_selected_thermals(CHANGETHERMAL_TYPES, kind);
				break;
			default:
				err = 1;
				break;
			}
		else
			err = 1;
		if (!err)
			return 0;
	}

	PCB_AFAIL(setthermal);
}


/* --------------------------------------------------------------------------- */

static const char setflag_syntax[] =
	"SetFlag(Object|Selected|SelectedObjects, flag)\n"
	"SetFlag(SelectedLines|SelectedPins|SelectedVias, flag)\n"
	"SetFlag(SelectedPads|SelectedTexts|SelectedNames, flag)\n"
	"SetFlag(SelectedElements, flag)\n" "flag = square | octagon | thermal | join";

static const char setflag_help[] = "Sets flags on objects.";

/* %start-doc actions SetFlag

Turns the given flag on, regardless of its previous setting.  See
@code{ChangeFlag}.

@example
SetFlag(SelectedPins,thermal)
@end example

%end-doc */

static int ActionSetFlag(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	const char *flag = PCB_ACTION_ARG(1);
	ChangeFlag(function, flag, 1, "SetFlag");
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char clrflag_syntax[] =
	"ClrFlag(Object|Selected|SelectedObjects, flag)\n"
	"ClrFlag(SelectedLines|SelectedPins|SelectedVias, flag)\n"
	"ClrFlag(SelectedPads|SelectedTexts|SelectedNames, flag)\n"
	"ClrFlag(SelectedElements, flag)\n" "flag = square | octagon | thermal | join";

static const char clrflag_help[] = "Clears flags on objects.";

/* %start-doc actions ClrFlag

Turns the given flag off, regardless of its previous setting.  See
@code{ChangeFlag}.

@example
ClrFlag(SelectedLines,join)
@end example

%end-doc */

static int ActionClrFlag(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	const char *flag = PCB_ACTION_ARG(1);
	ChangeFlag(function, flag, 0, "ClrFlag");
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char setvalue_syntax[] = "SetValue(Grid|Line|LineSize|Text|TextScale|ViaDrillingHole|Via|ViaSize, delta)";

static const char setvalue_help[] = "Change various board-wide values and sizes.";

/* %start-doc actions SetValue

@table @code

@item ViaDrillingHole
Changes the diameter of the drill for new vias.

@item Grid
Sets the grid spacing.

@item Line
@item LineSize
Changes the thickness of new lines.

@item Via
@item ViaSize
Changes the diameter of new vias.

@item Text
@item TextScale
Changes the size of new text.

@end table

%end-doc */

static int ActionSetValue(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	const char *val = PCB_ACTION_ARG(1);
	const char *units = PCB_ACTION_ARG(2);
	pcb_bool absolute;								/* flag for 'absolute' value */
	double value;
	int err = 0;

	if (function && val) {
		value = pcb_get_value(val, units, &absolute, NULL);
		switch (pcb_funchash_get(function, NULL)) {
		case F_ViaDrillingHole:
			pcb_board_set_via_drilling_hole(absolute ? value : value + conf_core.design.via_drilling_hole, pcb_false);
			pcb_hid_action("RouteStylesChanged");
			break;

		case F_Grid:
			if (absolute)
				pcb_board_set_grid(value, pcb_false);
			else {
				/* On the way down, short against the minimum
				 * PCB drawing unit */
				if ((value + PCB->Grid) < 1)
					pcb_board_set_grid(1, pcb_false);
				else if (PCB->Grid == 1)
					pcb_board_set_grid(value, pcb_false);
				else
					pcb_board_set_grid(value + PCB->Grid, pcb_false);
			}
			break;

		case F_LineSize:
		case F_Line:
			pcb_board_set_line_width(absolute ? value : value + conf_core.design.line_thickness);
			pcb_hid_action("RouteStylesChanged");
			break;

		case F_Via:
		case F_ViaSize:
			pcb_board_set_via_size(absolute ? value : value + conf_core.design.via_thickness, pcb_false);
			pcb_hid_action("RouteStylesChanged");
			break;

		case F_Text:
		case F_TextScale:
			value /= 45;
			pcb_board_set_text_scale(absolute ? value : value + conf_core.design.text_scale);
			break;
		default:
			err = 1;
			break;
		}
		if (!err)
			return 0;
	}

	PCB_AFAIL(setvalue);
}

/* --------------------------------------------------------------------------- */

static const char changeangle_syntax[] =
	"ChangeAngle(Object, start|delta|both, delta)\n"
	"ChangeAngle(SelectedObjects|Selected, start|delta|both, delta)\n"
	"ChangeAngle(SelectedArcs, start|delta|both, delta)\n";
static const char changeangle_help[] = "Changes the start angle, delta angle or both angles of an arc.";
static int ActionChangeAngle(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	const char *prim  = PCB_ACTION_ARG(1);
	const char *delta = PCB_ACTION_ARG(2);
	pcb_bool absolute;								/* indicates if absolute size is given */
	double value;
	int type = PCB_TYPE_NONE, which;
	void *ptr1, *ptr2, *ptr3;

	if (strcasecmp(prim, "start") == 0) which = 0;
	else if (strcasecmp(prim, "delta") == 0) which = 1;
	else if (strcasecmp(prim, "both") == 0) which = 2;
	else {
		pcb_message(PCB_MSG_ERROR, "Second argument of ChangeAngle must be start, delta or both\n");
		return -1;
	}

	if (function && delta) {
		int funcid = pcb_funchash_get(function, NULL);

		if (funcid == F_Object)
			type = pcb_search_screen(Crosshair.X, Crosshair.Y, CHANGESIZE_TYPES, &ptr1, &ptr2, &ptr3);

		{ /* convert angle from string */
			char *end;
			while(isspace(*delta)) delta++;
			value = strtod(delta, &end);
			if (*end != '\0') {
				pcb_message(PCB_MSG_ERROR, "Invalid numeric (in angle)\n");
				return -1;
			}
			absolute = ((*delta != '-') && (*delta != '+'));
		}
		
		switch (funcid) {
		case F_Object:
			{
				if (type != PCB_TYPE_NONE) {
					if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_pin_t *) ptr2))
						pcb_message(PCB_MSG_DEFAULT, _("Sorry, the object is locked\n"));
					else {
						if (pcb_chg_obj_angle(type, ptr1, ptr2, ptr3, which, value, absolute))
							pcb_board_set_changed_flag(pcb_true);
					}
				}
				break;
			}

		case F_SelectedArcs:
			if (pcb_chg_selected_angle(PCB_TYPE_ARC, which, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_Selected:
		case F_SelectedObjects:
			if (pcb_chg_selected_angle(CHANGESIZE_TYPES, which, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char changeradius_syntax[] =
	"ChangeRadius(Object, width|x|height|y|both, delta)\n"
	"ChangeRadius(SelectedObjects|Selected, width|x|height|y|both, delta)\n"
	"ChangeRadius(SelectedArcs, width|x|height|y|both, delta)\n";
static const char changeradius_help[] = "Changes the width or height (radius) of an arc.";
static int ActionChangeRadius(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	const char *prim  = PCB_ACTION_ARG(1);
	const char *delta = PCB_ACTION_ARG(2);
	const char *units = PCB_ACTION_ARG(3);
	pcb_bool absolute;								/* indicates if absolute size is given */
	double value;
	int type = PCB_TYPE_NONE, which;
	void *ptr1, *ptr2, *ptr3;

	if ((strcasecmp(prim, "width") == 0) || (strcasecmp(prim, "x") == 0)) which = 0;
	else if ((strcasecmp(prim, "height") == 0) || (strcasecmp(prim, "y") == 0)) which = 1;
	else if (strcasecmp(prim, "both") == 0) which = 2;
	else {
		pcb_message(PCB_MSG_ERROR, "Second argument of ChangeRadius must be width, x, height, y or both\n");
		return -1;
	}

	if (function && delta) {
		int funcid = pcb_funchash_get(function, NULL);

		if (funcid == F_Object)
			type = pcb_search_screen(Crosshair.X, Crosshair.Y, CHANGESIZE_TYPES, &ptr1, &ptr2, &ptr3);

		value = pcb_get_value(delta, units, &absolute, NULL);

		switch (funcid) {
		case F_Object:
			{
				if (type != PCB_TYPE_NONE) {
					if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_pin_t *) ptr2))
						pcb_message(PCB_MSG_DEFAULT, _("Sorry, the object is locked\n"));
					else {
						if (pcb_chg_obj_radius(type, ptr1, ptr2, ptr3, which, value, absolute))
							pcb_board_set_changed_flag(pcb_true);
					}
				}
				break;
			}

		case F_SelectedArcs:
			if (pcb_chg_selected_radius(PCB_TYPE_ARC, which, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;

		case F_Selected:
		case F_SelectedObjects:
			if (pcb_chg_selected_radius(CHANGESIZE_TYPES, which, value, absolute))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
}


/* --------------------------------------------------------------------------- */

pcb_hid_action_t change_action_list[] = {
	{"ChangeAngle", 0, ActionChangeAngle,
	 changeangle_help, changeangle_syntax}
	,
	{"ChangeClearSize", 0, ActionChangeClearSize,
	 changeclearsize_help, changeclearsize_syntax}
	,
	{"ChangeDrillSize", 0, ActionChange2ndSize,
	 changedrillsize_help, changedrillsize_syntax}
	,
	{"ChangeHole", 0, ActionChangeHole,
	 changehold_help, changehold_syntax}
	,
	{"ChangeJoin", 0, ActionChangeJoin,
	 changejoin_help, changejoin_syntax}
	,
	{"ChangeName", 0, ActionChangeName,
	 changename_help, changename_syntax}
	,
	{"ChangePaste", 0, ActionChangePaste,
	 changepaste_help, changepaste_syntax}
	,
	{"ChangePinName", 0, ActionChangePinName,
	 changepinname_help, changepinname_syntax}
	,
	{"ChangeRadius", 0, ActionChangeRadius,
	 changeradius_help, changeradius_syntax}
	,
	{"ChangeSize", 0, ActionChangeSize,
	 changesize_help, changesize_syntax}
	,
	{"ChangeSizes", 0, ActionChangeSizes,
	 changesizes_help, changesizes_syntax}
	,
	{"ChangeNonetlist", 0, ActionChangeNonetlist,
	 changenonetlist_help, changenonetlist_syntax}
	,
	{"ChangeSquare", 0, ActionChangeSquare,
	 changesquare_help, changesquare_syntax}
	,
	{"ChangeOctagon", 0, ActionChangeOctagon,
	 changeoctagon_help, changeoctagon_syntax}
	,
	{"ChangeFlag", 0, ActionChangeFlag,
	 changeflag_help, changeflag_syntax}
	,
	{"ClearSquare", 0, ActionClearSquare,
	 clearsquare_help, clearsquare_syntax}
	,
	{"ClearOctagon", 0, ActionClearOctagon,
	 clearoctagon_help, clearoctagon_syntax}
	,
	{"SetSquare", 0, ActionSetSquare,
	 setsquare_help, setsquare_syntax}
	,
	{"SetOctagon", 0, ActionSetOctagon,
	 setoctagon_help, setoctagon_syntax}
	,
	{"SetThermal", 0, ActionSetThermal,
	 setthermal_help, setthermal_syntax}
	,
	{"SetValue", 0, ActionSetValue,
	 setvalue_help, setvalue_syntax}
	,
	{"SetFlag", 0, ActionSetFlag,
	 setflag_help, setflag_syntax}
	,
	{"ClrFlag", 0, ActionClrFlag,
	 clrflag_help, clrflag_syntax}
};

PCB_REGISTER_ACTIONS(change_action_list, NULL)
