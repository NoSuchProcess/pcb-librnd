/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Boxes and group widgets */

#define PB_SCALE_UP 10000.0

#include <Xm/PanedW.h>
#include "wt_xpm.h"
#include "wt_colorbtn.h"

static void ltf_progress_set(lesstif_attr_dlg_t *ctx, int idx, double val)
{
	Widget bar = ctx->wl[idx];

	if (val < 0.0) val = 0.0;
	else if (val > 1.0) val = 1.0;
	
	stdarg_n = 0;
	stdarg(XmNsliderSize, (val * PB_SCALE_UP)+1);
	XtSetValues(bar, stdarg_args, stdarg_n);
}

static Widget ltf_progress_create(lesstif_attr_dlg_t *ctx, Widget parent)
{
	Widget bar;

	stdarg_n = 0;
	stdarg(XmNminimum, 0);
	stdarg(XmNvalue, 0);
	stdarg(XmNmaximum, (int)PB_SCALE_UP+1);
	stdarg(XmNsliderSize, 1);
	stdarg(XmNorientation, XmHORIZONTAL);
	stdarg(XmNshowArrows, pcb_false);
	stdarg_do_color_str("#000099", XmNforeground);
	stdarg(XmNsliderVisual, XmFOREGROUND_COLOR);
	
	bar = XmCreateScrollBar(parent, XmStrCast("scale"), stdarg_args, stdarg_n);
	XtManageChild(bar);
	return bar;
}


#include "wt_preview.h"

/* called back from core (which is called from wt_preview) to get the user
   expose function called */
static void ltf_preview_expose(pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	pcb_ltf_preview_t *pd = e->draw_data;
	pcb_hid_attribute_t *attr = pd->attr;
	pcb_hid_preview_t *prv = (pcb_hid_preview_t *)attr->enumerations;
	prv->user_expose_cb(attr, prv, gc, e);
}

static void ltf_preview_set(lesstif_attr_dlg_t *ctx, int idx, double val)
{
	Widget pw = ctx->wl[idx];
	pcb_ltf_preview_t *pd;

	stdarg_n = 0;
	stdarg(XmNuserData, &pd);
	XtGetValues(pw, stdarg_args, stdarg_n);

	pcb_ltf_preview_redraw(pd);
}

static void ltf_preview_zoomto(pcb_hid_attribute_t *attr, void *hid_ctx, const pcb_box_t *view)
{
	pcb_hid_preview_t *prv = (pcb_hid_preview_t *)attr->enumerations;
	pcb_ltf_preview_t *pd = prv->hid_ctx;

	pd->x1 = view->X1;
	pd->y1 = view->Y1;
	pd->x2 = view->X2;
	pd->y2 = view->Y2;

	pcb_ltf_preview_zoom_update(pd);
	pcb_ltf_preview_redraw(pd);
}

static void ltf_preview_motion_callback(Widget w, XtPointer pd_, XEvent *e, Boolean *ctd)
{
	pcb_ltf_preview_t *pd = pd_;
	pcb_hid_attribute_t *attr = pd->attr;
	pcb_hid_preview_t *prv = (pcb_hid_preview_t *)attr->enumerations;
	pcb_coord_t x, y;
	Window root, child;
	unsigned int keys_buttons;
	int root_x, root_y, pos_x, pos_y;

	if (prv->user_mouse_cb == NULL)
		return;

	XQueryPointer(display, e->xmotion.window, &root, &child, &root_x, &root_y, &pos_x, &pos_y, &keys_buttons);
	pcb_ltf_preview_getxy(pd, pos_x, pos_y, &x, &y);

	if (prv->user_mouse_cb(attr, prv, PCB_HID_MOUSE_MOTION, x, y))
		pcb_ltf_preview_redraw(pd);
}

static void ltf_preview_input_callback(Widget w, XtPointer pd_, XmDrawingAreaCallbackStruct *cbs)
{
	pcb_ltf_preview_t *pd = pd_;
	pcb_hid_attribute_t *attr = pd->attr;
	pcb_hid_preview_t *prv = (pcb_hid_preview_t *)attr->enumerations;
	pcb_coord_t x, y;
	pcb_hid_mouse_ev_t kind = -1;

	if (prv->user_mouse_cb == NULL)
		return;

	if (cbs->event->xbutton.button == 1) {
		if (cbs->event->type == ButtonPress) kind = PCB_HID_MOUSE_PRESS;
		if (cbs->event->type == ButtonRelease) kind = PCB_HID_MOUSE_RELEASE;
	}
	else if (cbs->event->xbutton.button == 3) {
		if (cbs->event->type == ButtonRelease) kind = PCB_HID_MOUSE_POPUP;
	}

	if (kind < 0)
		return;

	pcb_ltf_preview_getxy(pd, cbs->event->xbutton.x, cbs->event->xbutton.y, &x, &y);

	if (prv->user_mouse_cb(attr, prv, kind, x, y))
		pcb_ltf_preview_redraw(pd);
}


static Widget ltf_preview_create(lesstif_attr_dlg_t *ctx, Widget parent, pcb_hid_attribute_t *attr)
{
	Widget pw;
	pcb_ltf_preview_t *pd;
	pcb_hid_preview_t *prv = (pcb_hid_preview_t *)attr->enumerations;

	pd = calloc(1, sizeof(pcb_ltf_preview_t));
	prv->hid_ctx = pd;

	pd->attr = attr;
	memset(&pd->exp_ctx, 0, sizeof(pd->exp_ctx));
	pd->exp_ctx.draw_data = pd;
	pd->exp_ctx.expose_cb = ltf_preview_expose;

	pd->hid_ctx = ctx;
	prv->hid_zoomto_cb = ltf_preview_zoomto;

	pd->resized = 0;
	if (prv->initial_view_valid) {
		pd->x1 = prv->initial_view.X1;
		pd->y1 = prv->initial_view.Y1;
		pd->x2 = prv->initial_view.X2;
		pd->y2 = prv->initial_view.Y2;
		prv->initial_view_valid = 0;
	}
	else {
		pd->x1 = 0;
		pd->y1 = 0;
		pd->x2 = PCB_MM_TO_COORD(100);
		pd->y2 = PCB_MM_TO_COORD(100);
	}

	stdarg_n = 0;
	stdarg(XmNwidth, prv->min_sizex_px);
	stdarg(XmNheight, prv->min_sizey_px);
	stdarg(XmNresizePolicy, XmRESIZE_GROW);
	stdarg(XmNleftAttachment, XmATTACH_FORM);
	stdarg(XmNrightAttachment, XmATTACH_FORM);
	stdarg(XmNtopAttachment, XmATTACH_FORM);
	stdarg(XmNbottomAttachment, XmATTACH_FORM);
	stdarg(XmNuserData, pd);
	pw = XmCreateDrawingArea(parent, XmStrCast("dad_preview"), stdarg_args, stdarg_n);
	XtManageChild(pw);

	pd->window = XtWindow(pw);
	pd->pw = pw;

	XtAddCallback(pw, XmNexposeCallback, (XtCallbackProc)pcb_ltf_preview_callback, (XtPointer)pd);
	XtAddCallback(pw, XmNresizeCallback, (XtCallbackProc)pcb_ltf_preview_callback, (XtPointer)pd);
	XtAddCallback(pw, XmNinputCallback, (XtCallbackProc)ltf_preview_input_callback, (XtPointer)pd);
	XtAddEventHandler(pw, PointerMotionMask | PointerMotionHintMask | EnterWindowMask | LeaveWindowMask, 0, ltf_preview_motion_callback, (XtPointer)pd);


	XtManageChild(pw);
	return pw;
}


static Widget ltf_picture_create(lesstif_attr_dlg_t *ctx, Widget parent, pcb_hid_attribute_t *attr)
{
	Widget pic = pcb_ltf_xpm_label(display, parent, XmStrCast("dad_picture"), attr->enumerations);
	XtManageChild(pic);
	return pic;
}

static Widget ltf_picbutton_create(lesstif_attr_dlg_t *ctx, Widget parent, pcb_hid_attribute_t *attr)
{
	Widget pic = pcb_ltf_xpm_button(display, parent, XmStrCast("dad_picture"), attr->enumerations);
	XtManageChild(pic);
	return pic;
}

static void ltf_colorbtn_set(lesstif_attr_dlg_t *ctx, int idx, const pcb_color_t *clr)
{
	Widget btn = ctx->wl[idx];
	ctx->attrs[idx].default_val.clr_value = *clr;
	pcb_ltf_color_button_recolor(display, btn, clr);
}

#define CPACT "gui_FallbackColorPick"

static void ltf_colorbtn_valchg(Widget w, XtPointer dlg_widget_, XtPointer call_data)
{
	const char *new_color;
	fgw_error_t rs;
	fgw_arg_t res, argv[2];
	lesstif_attr_dlg_t *ctx;
	const pcb_color_t *clr;
	pcb_color_t nclr;
	int r, widx = attr_get_idx(w, &ctx);
	if (widx < 0)
		return;

	clr = &ctx->attrs[widx].default_val.clr_value;

	argv[0].type = FGW_VOID;
	argv[1].type = FGW_STR | FGW_DYN;
	argv[1].val.str = pcb_strdup_printf("#%02x%02x%02x", clr->r, clr->g, clr->b);
	rs = pcb_actionv_bin(CPACT, &res, 2, argv);
	if (rs != 0)
		return;

	if (!(res.type & FGW_STR)) {
		pcb_message(PCB_MSG_ERROR, CPACT " returned non-string\n");
		fgw_arg_free(&pcb_fgw, &res);
		return;
	}

	r = pcb_color_load_str(&nclr, res.val.str);
	fgw_arg_free(&pcb_fgw, &res);
	if (r != 0) {
		pcb_message(PCB_MSG_ERROR, CPACT " returned invalid color string\n");
		return;
	}

	fgw_arg_free(&pcb_fgw, &res);
	pcb_ltf_color_button_recolor(display, w, &nclr);
	ctx->attrs[widx].default_val.clr_value = nclr;
	valchg(w, dlg_widget_, w);
}


static Widget ltf_colorbtn_create(lesstif_attr_dlg_t *ctx, Widget parent, pcb_hid_attribute_t *attr, int readonly)
{
	Widget pic = pcb_ltf_color_button(display, parent, XmStrCast("dad_picture"), &attr->default_val.clr_value);
	if (!readonly)
		XtAddCallback(pic, XmNactivateCallback, ltf_colorbtn_valchg, NULL);
	XtManageChild(pic);
	return pic;
}

static void ltf_txt_changed_callback(Widget w, XtPointer attr_, XEvent *e, Boolean *ctd)
{
	pcb_hid_attribute_t *attr = attr_;
	pcb_hid_text_t *txt = (pcb_hid_text_t *)attr->enumerations;
	valchg(w, w, NULL);
}

static void ltf_text_set(lesstif_attr_dlg_t *ctx, int idx, const char *val)
{

}


static Widget ltf_text_create(lesstif_attr_dlg_t *ctx, Widget parent, pcb_hid_attribute_t *attr)
{
	Widget wtxt;
	pcb_hid_text_t *txt = (pcb_hid_text_t *)attr->enumerations;

	stdarg_n = 0;
	stdarg(XmNresizePolicy, XmRESIZE_GROW);
	stdarg(XmNleftAttachment, XmATTACH_FORM);
	stdarg(XmNrightAttachment, XmATTACH_FORM);
	stdarg(XmNtopAttachment, XmATTACH_FORM);
	stdarg(XmNbottomAttachment, XmATTACH_FORM);
	stdarg(XmNeditMode, XmMULTI_LINE_EDIT);
TODO("this should be removed once the EXPFILL bug is fixed");
	stdarg(XmNrows, 8);
	stdarg(XmNuserData, ctx);
	wtxt = XmCreateText(parent, XmStrCast("dad_text"), stdarg_args, stdarg_n);
	XtManageChild(wtxt);

	XtAddCallback(wtxt, XmNvalueChangedCallback, (XtCallbackProc)ltf_txt_changed_callback, (XtPointer)attr);

	return wtxt;
}

