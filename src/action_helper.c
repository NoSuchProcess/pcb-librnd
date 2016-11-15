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

/* action routines for output window */

#include "config.h"

#include "conf_core.h"

#include "action_helper.h"
#include "board.h"
#include "change.h"
#include "copy.h"
#include "data.h"
#include "draw.h"
#include "find.h"
#include "insert.h"
#include "polygon.h"
#include "remove.h"
#include "rotate.h"
#include "search.h"
#include "select.h"
#include "set.h"
#include "undo.h"
#include "stub_stroke.h"
#include "funchash_core.h"
#include "hid_actions.h"
#include "compat_misc.h"
#include "compat_nls.h"

#include "obj_pinvia_draw.h"
#include "obj_pad_draw.h"
#include "obj_line_draw.h"
#include "obj_arc_draw.h"
#include "obj_elem_draw.h"
#include "obj_text_draw.h"
#include "obj_rat_draw.h"
#include "obj_poly_draw.h"


static void GetGridLockCoordinates(int type, void *ptr1, void *ptr2, void *ptr3, pcb_coord_t * x, pcb_coord_t * y)
{
	switch (type) {
	case PCB_TYPE_VIA:
		*x = ((pcb_pin_t *) ptr2)->X;
		*y = ((pcb_pin_t *) ptr2)->Y;
		break;
	case PCB_TYPE_LINE:
		*x = ((pcb_line_t *) ptr2)->Point1.X;
		*y = ((pcb_line_t *) ptr2)->Point1.Y;
		break;
	case PCB_TYPE_TEXT:
	case PCB_TYPE_ELEMENT_NAME:
		*x = ((pcb_text_t *) ptr2)->X;
		*y = ((pcb_text_t *) ptr2)->Y;
		break;
	case PCB_TYPE_ELEMENT:
		*x = ((pcb_element_t *) ptr2)->MarkX;
		*y = ((pcb_element_t *) ptr2)->MarkY;
		break;
	case PCB_TYPE_POLYGON:
		*x = ((pcb_polygon_t *) ptr2)->Points[0].X;
		*y = ((pcb_polygon_t *) ptr2)->Points[0].Y;
		break;

	case PCB_TYPE_LINE_POINT:
	case PCB_TYPE_POLYGON_POINT:
		*x = ((pcb_point_t *) ptr3)->X;
		*y = ((pcb_point_t *) ptr3)->Y;
		break;
	case PCB_TYPE_ARC:
		{
			pcb_box_t *box;

			box = pcb_arc_get_ends((pcb_arc_t *) ptr2);
			*x = box->X1;
			*y = box->Y1;
			break;
		}
	}
}

static void AttachForCopy(pcb_coord_t PlaceX, pcb_coord_t PlaceY)
{
	pcb_box_t *box;
	pcb_coord_t mx = 0, my = 0;

	Crosshair.AttachedObject.RubberbandN = 0;
	if (!conf_core.editor.snap_pin) {
		/* dither the grab point so that the mark, center, etc
		 * will end up on a grid coordinate
		 */
		GetGridLockCoordinates(Crosshair.AttachedObject.Type,
													 Crosshair.AttachedObject.Ptr1,
													 Crosshair.AttachedObject.Ptr2, Crosshair.AttachedObject.Ptr3, &mx, &my);
		mx = pcb_grid_fit(mx, PCB->Grid, PCB->GridOffsetX) - mx;
		my = pcb_grid_fit(my, PCB->Grid, PCB->GridOffsetY) - my;
	}
	Crosshair.AttachedObject.X = PlaceX - mx;
	Crosshair.AttachedObject.Y = PlaceY - my;
	if (!Marked.status || conf_core.editor.local_ref)
		SetLocalRef(PlaceX - mx, PlaceY - my, pcb_true);
	Crosshair.AttachedObject.State = STATE_SECOND;

	/* get boundingbox of object and set cursor range */
	box = GetObjectBoundingBox(Crosshair.AttachedObject.Type,
														 Crosshair.AttachedObject.Ptr1, Crosshair.AttachedObject.Ptr2, Crosshair.AttachedObject.Ptr3);
	pcb_crosshair_set_range(Crosshair.AttachedObject.X - box->X1,
										Crosshair.AttachedObject.Y - box->Y1,
										PCB->MaxWidth - (box->X2 - Crosshair.AttachedObject.X),
										PCB->MaxHeight - (box->Y2 - Crosshair.AttachedObject.Y));

	/* get all attached objects if necessary */
	if ((conf_core.editor.mode != PCB_MODE_COPY) && conf_core.editor.rubber_band_mode)
		LookupRubberbandLines(Crosshair.AttachedObject.Type,
													Crosshair.AttachedObject.Ptr1, Crosshair.AttachedObject.Ptr2, Crosshair.AttachedObject.Ptr3);
	if (conf_core.editor.mode != PCB_MODE_COPY &&
			(Crosshair.AttachedObject.Type == PCB_TYPE_ELEMENT ||
			 Crosshair.AttachedObject.Type == PCB_TYPE_VIA ||
			 Crosshair.AttachedObject.Type == PCB_TYPE_LINE || Crosshair.AttachedObject.Type == PCB_TYPE_LINE_POINT))
		LookupRatLines(Crosshair.AttachedObject.Type,
									 Crosshair.AttachedObject.Ptr1, Crosshair.AttachedObject.Ptr2, Crosshair.AttachedObject.Ptr3);
}


/* --------------------------------------------------------------------------- */

/* %start-doc actions 00delta

Many actions take a @code{delta} parameter as the last parameter,
which is an amount to change something.  That @code{delta} may include
units, as an additional parameter, such as @code{Action(Object,5,mm)}.
If no units are specified, the default is PCB's native units
(currently 1/100 mil).  Also, if the delta is prefixed by @code{+} or
@code{-}, the size is increased or decreased by that amount.
Otherwise, the size size is set to the given amount.

@example
Action(Object,5,mil)
Action(Object,+0.5,mm)
Action(Object,-1)
@end example

Actions which take a @code{delta} parameter which do not accept all
these options will specify what they do take.

%end-doc */

/* %start-doc actions 00objects

Many actions act on indicated objects on the board.  They will have
parameters like @code{ToggleObject} or @code{SelectedVias} to indicate
what group of objects they act on.  Unless otherwise specified, these
parameters are defined as follows:

@table @code

@item Object
@itemx ToggleObject
Affects the object under the mouse pointer.  If this action is invoked
from a menu or script, the user will be prompted to click on an
object, which is then the object affected.

@item Selected
@itemx SelectedObjects

Affects all objects which are currently selected.  At least, all
selected objects for which the given action makes sense.

@item SelectedPins
@itemx SelectedVias
@itemx Selected@var{Type}
@itemx @i{etc}
Affects all objects which are both selected and of the @var{Type} specified.

@end table

%end-doc */

/*  %start-doc actions 00macros

@macro pinshapes

Pins, pads, and vias can have various shapes.  All may be round.  Pins
and pads may be square (obviously "square" pads are usually
rectangular).  Pins and vias may be octagonal.  When you change a
shape flag of an element, you actually change all of its pins and
pads.

Note that the square flag takes precedence over the octagon flag,
thus, if both the square and octagon flags are set, the object is
square.  When the square flag is cleared, the pins and pads will be
either round or, if the octagon flag is set, octagonal.

@end macro

%end-doc */

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static pcb_point_t InsertedPoint;
pcb_layer_t *lastLayer;
static struct {
	pcb_polygon_t *poly;
	pcb_line_t line;
} fake;

pcb_action_note_t Note;
int defer_updates = 0;
int defer_needs_update = 0;


static pcb_cardinal_t polyIndex = 0;
pcb_bool saved_mode = pcb_false;

static void AdjustAttachedBox(void);

void pcb_clear_warnings()
{
	conf_core.temp.rat_warn = pcb_false;
	ALLPIN_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_WARN, pin)) {
			PCB_FLAG_CLEAR(PCB_FLAG_WARN, pin);
			DrawPin(pin);
		}
	}
	ENDALL_LOOP;
	VIA_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_WARN, via)) {
			PCB_FLAG_CLEAR(PCB_FLAG_WARN, via);
			DrawVia(via);
		}
	}
	END_LOOP;
	ALLPAD_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_WARN, pad)) {
			PCB_FLAG_CLEAR(PCB_FLAG_WARN, pad);
			DrawPad(pad);
		}
	}
	ENDALL_LOOP;
	ALLLINE_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_WARN, line)) {
			PCB_FLAG_CLEAR(PCB_FLAG_WARN, line);
			DrawLine(layer, line);
		}
	}
	ENDALL_LOOP;
	ALLARC_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_WARN, arc)) {
			PCB_FLAG_CLEAR(PCB_FLAG_WARN, arc);
			DrawArc(layer, arc);
		}
	}
	ENDALL_LOOP;
	ALLPOLYGON_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_WARN, polygon)) {
			PCB_FLAG_CLEAR(PCB_FLAG_WARN, polygon);
			DrawPolygon(layer, polygon);
		}
	}
	ENDALL_LOOP;

	pcb_draw();
}

static void click_cb(pcb_hidval_t hv)
{
	if (Note.Click) {
		pcb_notify_crosshair_change(pcb_false);
		Note.Click = pcb_false;
		if (Note.Moving && !gui->shift_is_pressed()) {
			Note.Buffer = conf_core.editor.buffer_number;
			SetBufferNumber(MAX_BUFFER - 1);
			pcb_buffer_clear(PCB_PASTEBUFFER);
			pcb_buffer_add_selected(PCB_PASTEBUFFER, Note.X, Note.Y, pcb_true);
			SaveUndoSerialNumber();
			RemoveSelected();
			SaveMode();
			saved_mode = pcb_true;
			SetMode(PCB_MODE_PASTE_BUFFER);
		}
		else if (Note.Hit && !gui->shift_is_pressed()) {
			pcb_box_t box;

			SaveMode();
			saved_mode = pcb_true;
			SetMode(gui->control_is_pressed()? PCB_MODE_COPY : PCB_MODE_MOVE);
			Crosshair.AttachedObject.Ptr1 = Note.ptr1;
			Crosshair.AttachedObject.Ptr2 = Note.ptr2;
			Crosshair.AttachedObject.Ptr3 = Note.ptr3;
			Crosshair.AttachedObject.Type = Note.Hit;

			if (Crosshair.drags != NULL) {
				free(Crosshair.drags);
				Crosshair.drags = NULL;
			}
			Crosshair.dragx = Note.X;
			Crosshair.dragy = Note.Y;
			box.X1 = Note.X + SLOP * pixel_slop;
			box.X2 = Note.X - SLOP * pixel_slop;
			box.Y1 = Note.Y + SLOP * pixel_slop;
			box.Y2 = Note.Y - SLOP * pixel_slop;
			Crosshair.drags = ListBlock(&box, &Crosshair.drags_len);
			Crosshair.drags_current = 0;
			AttachForCopy(Note.X, Note.Y);
		}
		else {
			pcb_box_t box;

			Note.Hit = 0;
			Note.Moving = pcb_false;
			SaveUndoSerialNumber();
			box.X1 = -MAX_COORD;
			box.Y1 = -MAX_COORD;
			box.X2 = MAX_COORD;
			box.Y2 = MAX_COORD;
			/* unselect first if shift key not down */
			if (!gui->shift_is_pressed() && SelectBlock(&box, pcb_false))
				SetChangedFlag(pcb_true);
			pcb_notify_block();
			Crosshair.AttachedBox.Point1.X = Note.X;
			Crosshair.AttachedBox.Point1.Y = Note.Y;
		}
		pcb_notify_crosshair_change(pcb_true);
	}
}

void pcb_release_mode(void)
{
	pcb_box_t box;

	if (Note.Click) {
		pcb_box_t box;

		box.X1 = -MAX_COORD;
		box.Y1 = -MAX_COORD;
		box.X2 = MAX_COORD;
		box.Y2 = MAX_COORD;

		Note.Click = pcb_false;					/* inhibit timer action */
		SaveUndoSerialNumber();
		/* unselect first if shift key not down */
		if (!gui->shift_is_pressed()) {
			if (SelectBlock(&box, pcb_false))
				SetChangedFlag(pcb_true);
			if (Note.Moving) {
				Note.Moving = 0;
				Note.Hit = 0;
				return;
			}
		}
		/* Restore the SN so that if we select something the deselect/select combo
		   gets the same SN. */
		RestoreUndoSerialNumber();
		if (SelectObject())
			SetChangedFlag(pcb_true);
		else
			IncrementUndoSerialNumber(); /* We didn't select anything new, so, the deselection should get its  own SN. */
		Note.Hit = 0;
		Note.Moving = 0;
	}
	else if (Note.Moving) {
		RestoreUndoSerialNumber();
		pcb_notify_mode();
		pcb_buffer_clear(PCB_PASTEBUFFER);
		SetBufferNumber(Note.Buffer);
		Note.Moving = pcb_false;
		Note.Hit = 0;
	}
	else if (Note.Hit) {
		pcb_notify_mode();
		Note.Hit = 0;
	}
	else if (conf_core.editor.mode == PCB_MODE_ARROW) {
		box.X1 = Crosshair.AttachedBox.Point1.X;
		box.Y1 = Crosshair.AttachedBox.Point1.Y;
		box.X2 = Crosshair.AttachedBox.Point2.X;
		box.Y2 = Crosshair.AttachedBox.Point2.Y;

		RestoreUndoSerialNumber();
		if (SelectBlock(&box, pcb_true))
			SetChangedFlag(pcb_true);
		else if (Bumped)
			IncrementUndoSerialNumber();
		Crosshair.AttachedBox.State = STATE_FIRST;
	}
	if (saved_mode)
		RestoreMode();
	saved_mode = pcb_false;
}

/* ---------------------------------------------------------------------------
 * set new coordinates if in 'RECTANGLE' mode
 * the cursor shape is also adjusted
 */
static void AdjustAttachedBox(void)
{
	if (conf_core.editor.mode == PCB_MODE_ARC) {
		Crosshair.AttachedBox.otherway = gui->shift_is_pressed();
		return;
	}
	switch (Crosshair.AttachedBox.State) {
	case STATE_SECOND:						/* one corner is selected */
		{
			/* update coordinates */
			Crosshair.AttachedBox.Point2.X = Crosshair.X;
			Crosshair.AttachedBox.Point2.Y = Crosshair.Y;
			break;
		}
	}
}

void pcb_adjust_attached_objects(void)
{
	pcb_point_t *pnt;
	switch (conf_core.editor.mode) {
		/* update at least an attached block (selection) */
	case PCB_MODE_NO:
	case PCB_MODE_ARROW:
		if (Crosshair.AttachedBox.State) {
			Crosshair.AttachedBox.Point2.X = Crosshair.X;
			Crosshair.AttachedBox.Point2.Y = Crosshair.Y;
		}
		break;

		/* rectangle creation mode */
	case PCB_MODE_RECTANGLE:
	case PCB_MODE_ARC:
		AdjustAttachedBox();
		break;

		/* polygon creation mode */
	case PCB_MODE_POLYGON:
	case PCB_MODE_POLYGON_HOLE:
		AdjustAttachedLine();
		break;
		/* line creation mode */
	case PCB_MODE_LINE:
		if (PCB->RatDraw || conf_core.editor.line_refraction == 0)
			AdjustAttachedLine();
		else
			AdjustTwoLine(conf_core.editor.line_refraction - 1);
		break;
		/* point insertion mode */
	case PCB_MODE_INSERT_POINT:
		pnt = pcb_adjust_insert_point();
		if (pnt)
			InsertedPoint = *pnt;
		break;
	case PCB_MODE_ROTATE:
		break;
	}
}

void pcb_notify_line(void)
{
	int type = PCB_TYPE_NONE;
	void *ptr1, *ptr2, *ptr3;

	if (!Marked.status || conf_core.editor.local_ref)
		SetLocalRef(Crosshair.X, Crosshair.Y, pcb_true);
	switch (Crosshair.AttachedLine.State) {
	case STATE_FIRST:						/* first point */
		if (PCB->RatDraw && SearchScreen(Crosshair.X, Crosshair.Y, PCB_TYPE_PAD | PCB_TYPE_PIN, &ptr1, &ptr1, &ptr1) == PCB_TYPE_NONE) {
			gui->beep();
			break;
		}
		if (conf_core.editor.auto_drc && conf_core.editor.mode == PCB_MODE_LINE) {
			type = SearchScreen(Crosshair.X, Crosshair.Y, PCB_TYPE_PIN | PCB_TYPE_PAD | PCB_TYPE_VIA, &ptr1, &ptr2, &ptr3);
			pcb_lookup_conn(Crosshair.X, Crosshair.Y, pcb_true, 1, PCB_FLAG_FOUND);
		}
		if (type == PCB_TYPE_PIN || type == PCB_TYPE_VIA) {
			Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X = ((pcb_pin_t *) ptr2)->X;
			Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y = ((pcb_pin_t *) ptr2)->Y;
		}
		else if (type == PCB_TYPE_PAD) {
			pcb_pad_t *pad = (pcb_pad_t *) ptr2;
			double d1 = pcb_distance(Crosshair.X, Crosshair.Y, pad->Point1.X, pad->Point1.Y);
			double d2 = pcb_distance(Crosshair.X, Crosshair.Y, pad->Point2.X, pad->Point2.Y);
			double dm = pcb_distance(Crosshair.X, Crosshair.Y, (pad->Point1.X + pad->Point2.X) / 2, (pad->Point1.Y + pad->Point2.Y)/2);
			if ((dm <= d1) && (dm <= d2)) { /* prefer to snap to the middle of a pin if that's the closest */
				Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X = Crosshair.X;
				Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y = Crosshair.Y;
			}
			else if (d2 < d1) { /* else select the closest endpoint */
				Crosshair.AttachedLine.Point1 = Crosshair.AttachedLine.Point2 = pad->Point2;
			}
			else {
				Crosshair.AttachedLine.Point1 = Crosshair.AttachedLine.Point2 = pad->Point1;
			}
		}
		else {
			Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X = Crosshair.X;
			Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y = Crosshair.Y;
		}
		Crosshair.AttachedLine.State = STATE_SECOND;
		break;

	case STATE_SECOND:
		/* fall through to third state too */
		lastLayer = CURRENT;
	default:											/* all following points */
		Crosshair.AttachedLine.State = STATE_THIRD;
		break;
	}
}

void pcb_notify_block(void)
{
	pcb_notify_crosshair_change(pcb_false);
	switch (Crosshair.AttachedBox.State) {
	case STATE_FIRST:						/* setup first point */
		Crosshair.AttachedBox.Point1.X = Crosshair.AttachedBox.Point2.X = Crosshair.X;
		Crosshair.AttachedBox.Point1.Y = Crosshair.AttachedBox.Point2.Y = Crosshair.Y;
		Crosshair.AttachedBox.State = STATE_SECOND;
		break;

	case STATE_SECOND:						/* setup second point */
		Crosshair.AttachedBox.State = STATE_THIRD;
		break;
	}
	pcb_notify_crosshair_change(pcb_true);
}

void pcb_notify_mode(void)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	if (conf_core.temp.rat_warn)
		pcb_clear_warnings();
	switch (conf_core.editor.mode) {
	case PCB_MODE_ARROW:
		{
			int test;
			pcb_hidval_t hv;

			Note.Click = pcb_true;
			/* do something after click time */
			gui->add_timer(click_cb, conf_core.editor.click_time, hv);

			/* see if we clicked on something already selected
			 * (Note.Moving) or clicked on a MOVE_TYPE
			 * (Note.Hit)
			 */
			for (test = (SELECT_TYPES | MOVE_TYPES) & ~PCB_TYPE_RATLINE; test; test &= ~type) {
				type = SearchScreen(Note.X, Note.Y, test, &ptr1, &ptr2, &ptr3);
				if (!Note.Hit && (type & MOVE_TYPES) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_pin_t *) ptr2)) {
					Note.Hit = type;
					Note.ptr1 = ptr1;
					Note.ptr2 = ptr2;
					Note.ptr3 = ptr3;
				}
				if (!Note.Moving && (type & SELECT_TYPES) && PCB_FLAG_TEST(PCB_FLAG_SELECTED, (pcb_pin_t *) ptr2))
					Note.Moving = pcb_true;
				if ((Note.Hit && Note.Moving) || type == PCB_TYPE_NONE)
					break;
			}
			break;
		}

	case PCB_MODE_VIA:
		{
			pcb_pin_t *via;

			if (!PCB->ViaOn) {
				pcb_message(PCB_MSG_DEFAULT, _("You must turn via visibility on before\n" "you can place vias\n"));
				break;
			}
			if ((via = pcb_via_new(PCB->Data, Note.X, Note.Y,
															conf_core.design.via_thickness, 2 * conf_core.design.clearance,
															0, conf_core.design.via_drilling_hole, NULL, pcb_no_flags())) != NULL) {
				AddObjectToCreateUndoList(PCB_TYPE_VIA, via, via, via);
				if (gui->shift_is_pressed())
					pcb_chg_obj_thermal(PCB_TYPE_VIA, via, via, via, PCB->ThermStyle);
				IncrementUndoSerialNumber();
				DrawVia(via);
				pcb_draw();
			}
			break;
		}

	case PCB_MODE_ARC:
		{
			switch (Crosshair.AttachedBox.State) {
			case STATE_FIRST:
				Crosshair.AttachedBox.Point1.X = Crosshair.AttachedBox.Point2.X = Note.X;
				Crosshair.AttachedBox.Point1.Y = Crosshair.AttachedBox.Point2.Y = Note.Y;
				Crosshair.AttachedBox.State = STATE_SECOND;
				break;

			case STATE_SECOND:
			case STATE_THIRD:
				{
					pcb_arc_t *arc;
					pcb_coord_t wx, wy;
					pcb_angle_t sa, dir;

					wx = Note.X - Crosshair.AttachedBox.Point1.X;
					wy = Note.Y - Crosshair.AttachedBox.Point1.Y;
					if (PCB_XOR(Crosshair.AttachedBox.otherway, coord_abs(wy) > coord_abs(wx))) {
						Crosshair.AttachedBox.Point2.X = Crosshair.AttachedBox.Point1.X + coord_abs(wy) * PCB_SGNZ(wx);
						sa = (wx >= 0) ? 0 : 180;
#ifdef ARC45
						if (abs(wy) / 2 >= abs(wx))
							dir = (PCB_SGNZ(wx) == PCB_SGNZ(wy)) ? 45 : -45;
						else
#endif
							dir = (PCB_SGNZ(wx) == PCB_SGNZ(wy)) ? 90 : -90;
					}
					else {
						Crosshair.AttachedBox.Point2.Y = Crosshair.AttachedBox.Point1.Y + coord_abs(wx) * PCB_SGNZ(wy);
						sa = (wy >= 0) ? -90 : 90;
#ifdef ARC45
						if (abs(wx) / 2 >= abs(wy))
							dir = (PCB_SGNZ(wx) == PCB_SGNZ(wy)) ? -45 : 45;
						else
#endif
							dir = (PCB_SGNZ(wx) == PCB_SGNZ(wy)) ? -90 : 90;
						wy = wx;
					}
					if (coord_abs(wy) > 0 && (arc = pcb_arc_new(CURRENT,
																												Crosshair.AttachedBox.Point2.X,
																												Crosshair.AttachedBox.Point2.Y,
																												coord_abs(wy),
																												coord_abs(wy),
																												sa,
																												dir,
																												conf_core.design.line_thickness,
																												2 * conf_core.design.clearance,
																												pcb_flag_make(conf_core.editor.clear_line ? PCB_FLAG_CLEARLINE : 0)))) {
						pcb_box_t *bx;

						bx = pcb_arc_get_ends(arc);
						Crosshair.AttachedBox.Point1.X = Crosshair.AttachedBox.Point2.X = bx->X2;
						Crosshair.AttachedBox.Point1.Y = Crosshair.AttachedBox.Point2.Y = bx->Y2;
						AddObjectToCreateUndoList(PCB_TYPE_ARC, CURRENT, arc, arc);
						IncrementUndoSerialNumber();
						addedLines++;
						DrawArc(CURRENT, arc);
						pcb_draw();
						Crosshair.AttachedBox.State = STATE_THIRD;
					}
					break;
				}
			}
			break;
		}
	case PCB_MODE_LOCK:
		{
			type = SearchScreen(Note.X, Note.Y, PCB_TYPEMASK_LOCK, &ptr1, &ptr2, &ptr3);
			if (type == PCB_TYPE_ELEMENT) {
				pcb_element_t *element = (pcb_element_t *) ptr2;

				PCB_FLAG_TOGGLE(PCB_FLAG_LOCK, element);
				PIN_LOOP(element);
				{
					PCB_FLAG_TOGGLE(PCB_FLAG_LOCK, pin);
					PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, pin);
				}
				END_LOOP;
				PAD_LOOP(element);
				{
					PCB_FLAG_TOGGLE(PCB_FLAG_LOCK, pad);
					PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, pad);
				}
				END_LOOP;
				PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, element);
				/* always re-draw it since I'm too lazy
				 * to tell if a selected flag changed
				 */
				DrawElement(element);
				pcb_draw();
				pcb_hid_actionl("Report", "Object", NULL);
			}
			else if (type != PCB_TYPE_NONE) {
				pcb_text_t *thing = (pcb_text_t *) ptr3;
				PCB_FLAG_TOGGLE(PCB_FLAG_LOCK, thing);
				if (PCB_FLAG_TEST(PCB_FLAG_LOCK, thing)
						&& PCB_FLAG_TEST(PCB_FLAG_SELECTED, thing)) {
					/* this is not un-doable since LOCK isn't */
					PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, thing);
					pcb_draw_obj(type, ptr1, ptr2);
					pcb_draw();
				}
				pcb_hid_actionl("Report", "Object", NULL);
			}
			break;
		}
	case PCB_MODE_THERMAL:
		{
			if (((type = SearchScreen(Note.X, Note.Y, PCB_TYPEMASK_PIN, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE)
					&& !PCB_FLAG_TEST(PCB_FLAG_HOLE, (pcb_pin_t *) ptr3)) {
				if (gui->shift_is_pressed()) {
					int tstyle = PCB_FLAG_THERM_GET(INDEXOFCURRENT, (pcb_pin_t *) ptr3);
					tstyle++;
					if (tstyle > 5)
						tstyle = 1;
					pcb_chg_obj_thermal(type, ptr1, ptr2, ptr3, tstyle);
				}
				else if (PCB_FLAG_THERM_GET(INDEXOFCURRENT, (pcb_pin_t *) ptr3))
					pcb_chg_obj_thermal(type, ptr1, ptr2, ptr3, 0);
				else
					pcb_chg_obj_thermal(type, ptr1, ptr2, ptr3, PCB->ThermStyle);
			}
			break;
		}

	case PCB_MODE_LINE:
		/* do update of position */
		pcb_notify_line();
		if (Crosshair.AttachedLine.State != STATE_THIRD)
			break;

		/* Remove anchor if clicking on start point;
		 * this means we can't paint 0 length lines
		 * which could be used for square SMD pads.
		 * Instead use a very small delta, or change
		 * the file after saving.
		 */
		if (Crosshair.X == Crosshair.AttachedLine.Point1.X && Crosshair.Y == Crosshair.AttachedLine.Point1.Y) {
			SetMode(PCB_MODE_LINE);
			break;
		}

		if (PCB->RatDraw) {
			pcb_rat_t *line;
			if ((line = AddNet())) {
				addedLines++;
				AddObjectToCreateUndoList(PCB_TYPE_RATLINE, line, line, line);
				IncrementUndoSerialNumber();
				DrawRat(line);
				Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X;
				Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y;
				pcb_draw();
			}
			break;
		}
		else
			/* create line if both ends are determined && length != 0 */
		{
			pcb_line_t *line;
			int maybe_found_flag;

			if (conf_core.editor.line_refraction
					&& Crosshair.AttachedLine.Point1.X ==
					Crosshair.AttachedLine.Point2.X
					&& Crosshair.AttachedLine.Point1.Y ==
					Crosshair.AttachedLine.Point2.Y
					&& (Crosshair.AttachedLine.Point2.X != Note.X || Crosshair.AttachedLine.Point2.Y != Note.Y)) {
				/* We will only need to paint the second line segment.
				   Since we only check for vias on the first segment,
				   swap them so the non-empty segment is the first segment. */
				Crosshair.AttachedLine.Point2.X = Note.X;
				Crosshair.AttachedLine.Point2.Y = Note.Y;
			}

			if (conf_core.editor.auto_drc
					&& !TEST_SILK_LAYER(CURRENT))
				maybe_found_flag = PCB_FLAG_FOUND;
			else
				maybe_found_flag = 0;

			if ((Crosshair.AttachedLine.Point1.X !=
					 Crosshair.AttachedLine.Point2.X || Crosshair.AttachedLine.Point1.Y != Crosshair.AttachedLine.Point2.Y)
					&& (line =
							pcb_line_new_merge(CURRENT,
																		 Crosshair.AttachedLine.Point1.X,
																		 Crosshair.AttachedLine.Point1.Y,
																		 Crosshair.AttachedLine.Point2.X,
																		 Crosshair.AttachedLine.Point2.Y,
																		 conf_core.design.line_thickness,
																		 2 * conf_core.design.clearance,
																		 pcb_flag_make(maybe_found_flag |
																							 (conf_core.editor.clear_line ? PCB_FLAG_CLEARLINE : 0)))) != NULL) {
				pcb_pin_t *via;

				addedLines++;
				AddObjectToCreateUndoList(PCB_TYPE_LINE, CURRENT, line, line);
				DrawLine(CURRENT, line);
				/* place a via if vias are visible, the layer is
				   in a new group since the last line and there
				   isn't a pin already here */
				if (PCB->ViaOn && GetLayerGroupNumberByPointer(CURRENT) !=
						GetLayerGroupNumberByPointer(lastLayer) &&
						SearchObjectByLocation(PCB_TYPEMASK_PIN, &ptr1, &ptr2, &ptr3,
																	 Crosshair.AttachedLine.Point1.X,
																	 Crosshair.AttachedLine.Point1.Y,
																	 conf_core.design.via_thickness / 2) ==
						PCB_TYPE_NONE
						&& (via =
								pcb_via_new(PCB->Data,
														 Crosshair.AttachedLine.Point1.X,
														 Crosshair.AttachedLine.Point1.Y,
														 conf_core.design.via_thickness,
														 2 * conf_core.design.clearance, 0, conf_core.design.via_drilling_hole, NULL, pcb_no_flags())) != NULL) {
					AddObjectToCreateUndoList(PCB_TYPE_VIA, via, via, via);
					DrawVia(via);
				}
				/* copy the coordinates */
				Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X;
				Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y;
				IncrementUndoSerialNumber();
				lastLayer = CURRENT;
			}
			if (conf_core.editor.line_refraction && (Note.X != Crosshair.AttachedLine.Point2.X || Note.Y != Crosshair.AttachedLine.Point2.Y)
					&& (line =
							pcb_line_new_merge(CURRENT,
																		 Crosshair.AttachedLine.Point2.X,
																		 Crosshair.AttachedLine.Point2.Y,
																		 Note.X, Note.Y,
																		 conf_core.design.line_thickness,
																		 2 * conf_core.design.clearance,
																		 pcb_flag_make((conf_core.editor.auto_drc ? PCB_FLAG_FOUND : 0) |
																							 (conf_core.editor.clear_line ? PCB_FLAG_CLEARLINE : 0)))) != NULL) {
				addedLines++;
				AddObjectToCreateUndoList(PCB_TYPE_LINE, CURRENT, line, line);
				IncrementUndoSerialNumber();
				DrawLine(CURRENT, line);
				/* move to new start point */
				Crosshair.AttachedLine.Point1.X = Note.X;
				Crosshair.AttachedLine.Point1.Y = Note.Y;
				Crosshair.AttachedLine.Point2.X = Note.X;
				Crosshair.AttachedLine.Point2.Y = Note.Y;


				if (conf_core.editor.swap_start_direction) {
					conf_setf(CFR_DESIGN,"editor/line_refraction", -1, "%d",conf_core.editor.line_refraction ^ 3);
				}
			}
			if (conf_core.editor.orthogonal_moves) {
				/* set the mark to the new starting point so ortho works as expected and we can draw a perpendicular line from here */
				Marked.X = Note.X;
				Marked.Y = Note.Y;
			}
			pcb_draw();
		}
		break;

	case PCB_MODE_RECTANGLE:
		/* do update of position */
		pcb_notify_block();

		/* create rectangle if both corners are determined
		 * and width, height are != 0
		 */
		if (Crosshair.AttachedBox.State == STATE_THIRD &&
				Crosshair.AttachedBox.Point1.X != Crosshair.AttachedBox.Point2.X &&
				Crosshair.AttachedBox.Point1.Y != Crosshair.AttachedBox.Point2.Y) {
			pcb_polygon_t *polygon;

			int flags = PCB_FLAG_CLEARPOLY;
			if (conf_core.editor.full_poly)
				flags |= PCB_FLAG_FULLPOLY;
			if ((polygon = pcb_poly_new_from_rectangle(CURRENT,
																									 Crosshair.AttachedBox.Point1.X,
																									 Crosshair.AttachedBox.Point1.Y,
																									 Crosshair.AttachedBox.Point2.X,
																									 Crosshair.AttachedBox.Point2.Y, pcb_flag_make(flags))) != NULL) {
				AddObjectToCreateUndoList(PCB_TYPE_POLYGON, CURRENT, polygon, polygon);
				IncrementUndoSerialNumber();
				DrawPolygon(CURRENT, polygon);
				pcb_draw();
			}

			/* reset state to 'first corner' */
			Crosshair.AttachedBox.State = STATE_FIRST;
		}
		break;

	case PCB_MODE_TEXT:
		{
			char *string;

			if ((string = gui->prompt_for(_("Enter text:"), "")) != NULL) {
				if (strlen(string) > 0) {
					pcb_text_t *text;
					int flag = PCB_FLAG_CLEARLINE;

					if (GetLayerGroupNumberByNumber(INDEXOFCURRENT) == GetLayerGroupNumberByNumber(solder_silk_layer))
						flag |= PCB_FLAG_ONSOLDER;
					if ((text = pcb_text_new(CURRENT, &PCB->Font, Note.X,
																		Note.Y, 0, conf_core.design.text_scale, string, pcb_flag_make(flag))) != NULL) {
						AddObjectToCreateUndoList(PCB_TYPE_TEXT, CURRENT, text, text);
						IncrementUndoSerialNumber();
						DrawText(CURRENT, text);
						pcb_draw();
					}
				}
				free(string);
			}
			break;
		}

	case PCB_MODE_POLYGON:
		{
			pcb_point_t *points = Crosshair.AttachedPolygon.Points;
			pcb_cardinal_t n = Crosshair.AttachedPolygon.PointN;

			/* do update of position; use the 'PCB_MODE_LINE' mechanism */
			pcb_notify_line();

			/* check if this is the last point of a polygon */
			if (n >= 3 && points[0].X == Crosshair.AttachedLine.Point2.X && points[0].Y == Crosshair.AttachedLine.Point2.Y) {
				pcb_hid_actionl("Polygon", "Close", NULL);
				ClosePolygon();
				break;
			}

			/* Someone clicking twice on the same point ('doubleclick'): close polygon */
			if (n >= 3 && points[n - 1].X == Crosshair.AttachedLine.Point2.X && points[n - 1].Y == Crosshair.AttachedLine.Point2.Y) {
				pcb_hid_actionl("Polygon", "Close", NULL);
				break;
			}

			/* create new point if it's the first one or if it's
			 * different to the last one
			 */
			if (!n || points[n - 1].X != Crosshair.AttachedLine.Point2.X || points[n - 1].Y != Crosshair.AttachedLine.Point2.Y) {
				pcb_poly_point_new(&Crosshair.AttachedPolygon, Crosshair.AttachedLine.Point2.X, Crosshair.AttachedLine.Point2.Y);

				/* copy the coordinates */
				Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X;
				Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y;
			}

			if (conf_core.editor.orthogonal_moves) {
				/* set the mark to the new starting point so ortho works */
				Marked.X = Note.X;
				Marked.Y = Note.Y;
			}

			break;
		}

	case PCB_MODE_POLYGON_HOLE:
		{
			switch (Crosshair.AttachedObject.State) {
				/* first notify, lookup object */
			case STATE_FIRST:
				Crosshair.AttachedObject.Type =
					SearchScreen(Note.X, Note.Y, PCB_TYPE_POLYGON,
											 &Crosshair.AttachedObject.Ptr1, &Crosshair.AttachedObject.Ptr2, &Crosshair.AttachedObject.Ptr3);

				if (Crosshair.AttachedObject.Type == PCB_TYPE_NONE) {
					pcb_message(PCB_MSG_DEFAULT, "The first point of a polygon hole must be on a polygon.\n");
					break; /* don't start doing anything if clicket out of polys */
				}

				if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_polygon_t *)
											Crosshair.AttachedObject.Ptr2)) {
					pcb_message(PCB_MSG_DEFAULT, _("Sorry, the object is locked\n"));
					Crosshair.AttachedObject.Type = PCB_TYPE_NONE;
					break;
				}
				else
					Crosshair.AttachedObject.State = STATE_SECOND;
			/* fall thru: first click is also the first point of the poly hole */

				/* second notify, insert new point into object */
			case STATE_SECOND:
				{
					pcb_point_t *points = Crosshair.AttachedPolygon.Points;
					pcb_cardinal_t n = Crosshair.AttachedPolygon.PointN;
					pcb_polyarea_t *original, *new_hole, *result;
					pcb_flag_t Flags;

					/* do update of position; use the 'PCB_MODE_LINE' mechanism */
					pcb_notify_line();

					if (conf_core.editor.orthogonal_moves) {
						/* set the mark to the new starting point so ortho works */
						Marked.X = Note.X;
						Marked.Y = Note.Y;
					}

					/* check if this is the last point of a polygon */
					if (n >= 3 && points[0].X == Crosshair.AttachedLine.Point2.X && points[0].Y == Crosshair.AttachedLine.Point2.Y) {
						/* Create pcb_polyarea_ts from the original polygon
						 * and the new hole polygon */
						original = PolygonToPoly((pcb_polygon_t *) Crosshair.AttachedObject.Ptr2);
						new_hole = PolygonToPoly(&Crosshair.AttachedPolygon);

						/* Subtract the hole from the original polygon shape */
						poly_Boolean_free(original, new_hole, &result, PBO_SUB);

						/* Convert the resulting polygon(s) into a new set of nodes
						 * and place them on the page. Delete the original polygon.
						 */
						SaveUndoSerialNumber();
						Flags = ((pcb_polygon_t *) Crosshair.AttachedObject.Ptr2)->Flags;
						PolyToPolygonsOnLayer(PCB->Data, (pcb_layer_t *) Crosshair.AttachedObject.Ptr1, result, Flags);
						RemoveObject(PCB_TYPE_POLYGON,
												 Crosshair.AttachedObject.Ptr1, Crosshair.AttachedObject.Ptr2, Crosshair.AttachedObject.Ptr3);
						RestoreUndoSerialNumber();
						IncrementUndoSerialNumber();
						pcb_draw();

						/* reset state of attached line */
						memset(&Crosshair.AttachedPolygon, 0, sizeof(pcb_polygon_t));
						Crosshair.AttachedLine.State = STATE_FIRST;
						Crosshair.AttachedObject.State = STATE_FIRST;
						addedLines = 0;

						break;
					}

					/* create new point if it's the first one or if it's
					 * different to the last one
					 */
					if (!n || points[n - 1].X != Crosshair.AttachedLine.Point2.X || points[n - 1].Y != Crosshair.AttachedLine.Point2.Y) {
						pcb_poly_point_new(&Crosshair.AttachedPolygon,
																		Crosshair.AttachedLine.Point2.X, Crosshair.AttachedLine.Point2.Y);

						/* copy the coordinates */
						Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X;
						Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y;
					}
					break;
				}
			}
			break;
		}

	case PCB_MODE_PASTE_BUFFER:
		{
			pcb_text_t estr[MAX_ELEMENTNAMES];
			pcb_element_t *e = 0;

			if (gui->shift_is_pressed()) {
				int type = SearchScreen(Note.X, Note.Y, PCB_TYPE_ELEMENT, &ptr1, &ptr2,
																&ptr3);
				if (type == PCB_TYPE_ELEMENT) {
					e = (pcb_element_t *) ptr1;
					if (e) {
						int i;

						memcpy(estr, e->Name, MAX_ELEMENTNAMES * sizeof(pcb_text_t));
						for (i = 0; i < MAX_ELEMENTNAMES; ++i)
							estr[i].TextString = estr[i].TextString ? pcb_strdup(estr[i].TextString) : NULL;
						RemoveElement(e);
					}
				}
			}
			if (pcb_buffer_copy_to_layout(Note.X, Note.Y))
				SetChangedFlag(pcb_true);
			if (e) {
				int type = SearchScreen(Note.X, Note.Y, PCB_TYPE_ELEMENT, &ptr1, &ptr2,
																&ptr3);
				if (type == PCB_TYPE_ELEMENT && ptr1) {
					int i, save_n;
					e = (pcb_element_t *) ptr1;

					save_n = NAME_INDEX();

					for (i = 0; i < MAX_ELEMENTNAMES; i++) {
						if (i == save_n)
							EraseElementName(e);
						r_delete_entry(PCB->Data->name_tree[i], (pcb_box_t *) & (e->Name[i]));
						memcpy(&(e->Name[i]), &(estr[i]), sizeof(pcb_text_t));
						e->Name[i].Element = e;
						pcb_text_bbox(&PCB->Font, &(e->Name[i]));
						r_insert_entry(PCB->Data->name_tree[i], (pcb_box_t *) & (e->Name[i]), 0);
						if (i == save_n)
							DrawElementName(e);
					}
				}
			}
			break;
		}

	case PCB_MODE_REMOVE:
		if ((type = SearchScreen(Note.X, Note.Y, REMOVE_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE) {
			if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_line_t *) ptr2)) {
				pcb_message(PCB_MSG_DEFAULT, _("Sorry, the object is locked\n"));
				break;
			}
			if (type == PCB_TYPE_ELEMENT) {
				pcb_rubberband_t *ptr;
				int i;

				Crosshair.AttachedObject.RubberbandN = 0;
				LookupRatLines(type, ptr1, ptr2, ptr3);
				ptr = Crosshair.AttachedObject.Rubberband;
				for (i = 0; i < Crosshair.AttachedObject.RubberbandN; i++) {
					if (PCB->RatOn)
						EraseRat((pcb_rat_t *) ptr->Line);
					if (PCB_FLAG_TEST(PCB_FLAG_RUBBEREND, ptr->Line))
						MoveObjectToRemoveUndoList(PCB_TYPE_RATLINE, ptr->Line, ptr->Line, ptr->Line);
					else
						PCB_FLAG_TOGGLE(PCB_FLAG_RUBBEREND, ptr->Line);	/* only remove line once */
					ptr++;
				}
			}
			RemoveObject(type, ptr1, ptr2, ptr3);
			IncrementUndoSerialNumber();
			SetChangedFlag(pcb_true);
		}
		break;

	case PCB_MODE_ROTATE:
		RotateScreenObject(Note.X, Note.Y, gui->shift_is_pressed()? (SWAP_IDENT ? 1 : 3)
											 : (SWAP_IDENT ? 3 : 1));
		break;

		/* both are almost the same */
	case PCB_MODE_COPY:
	case PCB_MODE_MOVE:
		switch (Crosshair.AttachedObject.State) {
			/* first notify, lookup object */
		case STATE_FIRST:
			{
				int types = (conf_core.editor.mode == PCB_MODE_COPY) ? COPY_TYPES : MOVE_TYPES;

				Crosshair.AttachedObject.Type =
					SearchScreen(Note.X, Note.Y, types,
											 &Crosshair.AttachedObject.Ptr1, &Crosshair.AttachedObject.Ptr2, &Crosshair.AttachedObject.Ptr3);
				if (Crosshair.AttachedObject.Type != PCB_TYPE_NONE) {
					if (conf_core.editor.mode == PCB_MODE_MOVE && PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_pin_t *)
																											Crosshair.AttachedObject.Ptr2)) {
						pcb_message(PCB_MSG_DEFAULT, _("Sorry, the object is locked\n"));
						Crosshair.AttachedObject.Type = PCB_TYPE_NONE;
					}
					else
						AttachForCopy(Note.X, Note.Y);
				}
				break;
			}

			/* second notify, move or copy object */
		case STATE_SECOND:
			if (conf_core.editor.mode == PCB_MODE_COPY)
				pcb_copy_obj(Crosshair.AttachedObject.Type,
									 Crosshair.AttachedObject.Ptr1,
									 Crosshair.AttachedObject.Ptr2,
									 Crosshair.AttachedObject.Ptr3, Note.X - Crosshair.AttachedObject.X, Note.Y - Crosshair.AttachedObject.Y);
			else {
				pcb_move_obj_and_rubberband(Crosshair.AttachedObject.Type,
																Crosshair.AttachedObject.Ptr1,
																Crosshair.AttachedObject.Ptr2,
																Crosshair.AttachedObject.Ptr3,
																Note.X - Crosshair.AttachedObject.X, Note.Y - Crosshair.AttachedObject.Y);
				SetLocalRef(0, 0, pcb_false);
			}
			SetChangedFlag(pcb_true);

			/* reset identifiers */
			Crosshair.AttachedObject.Type = PCB_TYPE_NONE;
			Crosshair.AttachedObject.State = STATE_FIRST;
			break;
		}
		break;

		/* insert a point into a polygon/line/... */
	case PCB_MODE_INSERT_POINT:
		switch (Crosshair.AttachedObject.State) {
			/* first notify, lookup object */
		case STATE_FIRST:
			Crosshair.AttachedObject.Type =
				SearchScreen(Note.X, Note.Y, INSERT_TYPES,
										 &Crosshair.AttachedObject.Ptr1, &Crosshair.AttachedObject.Ptr2, &Crosshair.AttachedObject.Ptr3);

			if (Crosshair.AttachedObject.Type != PCB_TYPE_NONE) {
				if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_polygon_t *)
											Crosshair.AttachedObject.Ptr2)) {
					pcb_message(PCB_MSG_DEFAULT, _("Sorry, the object is locked\n"));
					Crosshair.AttachedObject.Type = PCB_TYPE_NONE;
					break;
				}
				else {
					/* get starting point of nearest segment */
					if (Crosshair.AttachedObject.Type == PCB_TYPE_POLYGON) {
						fake.poly = (pcb_polygon_t *) Crosshair.AttachedObject.Ptr2;
						polyIndex = GetLowestDistancePolygonPoint(fake.poly, Note.X, Note.Y);
						fake.line.Point1 = fake.poly->Points[polyIndex];
						fake.line.Point2 = fake.poly->Points[prev_contour_point(fake.poly, polyIndex)];
						Crosshair.AttachedObject.Ptr2 = &fake.line;

					}
					Crosshair.AttachedObject.State = STATE_SECOND;
					InsertedPoint = *pcb_adjust_insert_point();
				}
			}
			break;

			/* second notify, insert new point into object */
		case STATE_SECOND:
			if (Crosshair.AttachedObject.Type == PCB_TYPE_POLYGON)
				pcb_insert_point_in_object(PCB_TYPE_POLYGON,
															Crosshair.AttachedObject.Ptr1, fake.poly,
															&polyIndex, InsertedPoint.X, InsertedPoint.Y, pcb_false, pcb_false);
			else
				pcb_insert_point_in_object(Crosshair.AttachedObject.Type,
															Crosshair.AttachedObject.Ptr1,
															Crosshair.AttachedObject.Ptr2, &polyIndex, InsertedPoint.X, InsertedPoint.Y, pcb_false, pcb_false);
			SetChangedFlag(pcb_true);

			/* reset identifiers */
			Crosshair.AttachedObject.Type = PCB_TYPE_NONE;
			Crosshair.AttachedObject.State = STATE_FIRST;
			break;
		}
		break;
	}
}

void pcb_event_move_crosshair(int ev_x, int ev_y)
{
	if (mid_stroke)
		stub_stroke_record(ev_x, ev_y);
	if (pcb_crosshair_move_absolute(ev_x, ev_y)) {
		/* update object position and cursor location */
		pcb_adjust_attached_objects();
		pcb_notify_crosshair_change(pcb_true);
	}
}

int pcb_get_style_size(int funcid, pcb_coord_t * out, int type, int size_id)
{
	switch (funcid) {
	case F_Object:
		switch (type) {
		case PCB_TYPE_ELEMENT:					/* we'd set pin/pad properties, so fall thru */
		case PCB_TYPE_VIA:
		case PCB_TYPE_PIN:
			return pcb_get_style_size(F_SelectedVias, out, 0, size_id);
		case PCB_TYPE_PAD:
			return pcb_get_style_size(F_SelectedPads, out, 0, size_id);
		case PCB_TYPE_LINE:
			return pcb_get_style_size(F_SelectedLines, out, 0, size_id);
		case PCB_TYPE_ARC:
			return pcb_get_style_size(F_SelectedArcs, out, 0, size_id);
		}
		pcb_message(PCB_MSG_DEFAULT, _("Sorry, can't fetch the style of that object type (%x)\n"), type);
		return -1;
	case F_SelectedPads:
		if (size_id != 2)						/* don't mess with pad size */
			return -1;
	case F_SelectedVias:
	case F_SelectedPins:
	case F_SelectedObjects:
	case F_Selected:
	case F_SelectedElements:
		if (size_id == 0)
			*out = conf_core.design.via_thickness;
		else if (size_id == 1)
			*out = conf_core.design.via_drilling_hole;
		else
			*out = conf_core.design.clearance;
		break;
	case F_SelectedArcs:
	case F_SelectedLines:
		if (size_id == 2)
			*out = conf_core.design.clearance;
		else
			*out = conf_core.design.line_thickness;
		return 0;
	case F_SelectedTexts:
	case F_SelectedNames:
		pcb_message(PCB_MSG_DEFAULT, _("Sorry, can't change style of every selected object\n"));
		return -1;
	}
	return 0;
}
