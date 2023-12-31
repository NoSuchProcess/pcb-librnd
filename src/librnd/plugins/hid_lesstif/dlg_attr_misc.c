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
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Boxes and group widgets */

#define PB_SCALE_UP 10000.0

#include <Xm/PanedW.h>
#include "wt_xpm.h"
#include "wt_colorbtn.h"

#include <librnd/hid/hid_dad.h>

#include <librnd/plugins/lib_hid_common/dad_markup.h>

static void ltf_progress_set(lesstif_attr_dlg_t *ctx, int idx, double val)
{
	Widget bar = ctx->wl[idx];

	if (val < 0.0) val = 0.0;
	else if (val > 1.0) val = 1.0;
	
	if ((val >= 0.0) && (val <= 1.0)) { /* extra case for NaN */
		stdarg_n = 0;
		stdarg(XmNsliderSize, (val * PB_SCALE_UP)+1);
		XtSetValues(bar, stdarg_args, stdarg_n);
	}
}

static Widget ltf_progress_create(lesstif_attr_dlg_t *ctx, Widget parent, rnd_hatt_compflags_t flags)
{
	Widget bar;

	stdarg_n = 0;
	stdarg(XmNminimum, 0);
	stdarg(XmNvalue, 0);
	stdarg(XmNmaximum, (int)PB_SCALE_UP+1);
	stdarg(XmNsliderSize, 1);
	stdarg(XmNorientation, XmHORIZONTAL);
	stdarg(XmNshowArrows, rnd_false);
	stdarg_do_color_str("#000099", XmNforeground);
	stdarg(XmNsliderVisual, XmFOREGROUND_COLOR);
	if (flags & RND_HATF_PRG_SMALL) {
		stdarg(PxmNfillBoxMinSize, 4);
		stdarg(PxmNfillBoxFill, 1);
	}

	bar = XmCreateScrollBar(parent, XmStrCast("scale"), stdarg_args, stdarg_n);
	XtManageChild(bar);
	return bar;
}


#include "wt_preview.h"

/* called back from core (which is called from wt_preview) to get the user
   expose function called */
static void ltf_preview_expose(rnd_hid_gc_t gc, rnd_hid_expose_ctx_t *e)
{
	rnd_ltf_preview_t *pd = e->draw_data;
	rnd_hid_attribute_t *attr = pd->attr;
	rnd_hid_preview_t *prv = attr->wdata;
	prv->user_expose_cb(attr, prv, gc, e);
}

static void ltf_preview_set(lesstif_attr_dlg_t *ctx, int idx, double val)
{
	Widget pw = ctx->wl[idx];
	rnd_ltf_preview_t *pd;

	stdarg_n = 0;
	stdarg(XmNuserData, &pd);
	XtGetValues(pw, stdarg_args, stdarg_n);

	rnd_ltf_preview_redraw(pd);
}

static void ltf_preview_zoomto(rnd_hid_attribute_t *attr, void *hid_ctx, const rnd_box_t *view)
{
	rnd_hid_preview_t *prv = attr->wdata;
	rnd_ltf_preview_t *pd = prv->hid_wdata;

	if (view != NULL) {
		pd->x1 = view->X1;
		pd->y1 = view->Y1;
		pd->x2 = view->X2;
		pd->y2 = view->Y2;
	}

	rnd_ltf_preview_zoom_update(pd);
	rnd_ltf_preview_redraw(pd);
}

static void ltf_preview_motion_callback(Widget w, XtPointer pd_, XEvent *e, Boolean *ctd)
{
	rnd_ltf_preview_t *pd = pd_;
	rnd_hid_attribute_t *attr = pd->attr;
	rnd_hid_preview_t *prv = attr->wdata;
	rnd_coord_t x, y;
	Window root, child;
	unsigned int keys_buttons;
	int root_x, root_y, pos_x, pos_y;

	if (prv->user_mouse_cb == NULL)
		return;

	XQueryPointer(display, e->xmotion.window, &root, &child, &root_x, &root_y, &pos_x, &pos_y, &keys_buttons);
	rnd_ltf_preview_getxy(pd, pos_x, pos_y, &x, &y);

	if (prv->user_mouse_cb(attr, prv, RND_HID_MOUSE_MOTION, x, y))
		rnd_ltf_preview_redraw(pd);
}

static void ltf_preview_input_mouse(Widget w, XtPointer pd_, XmDrawingAreaCallbackStruct *cbs)
{
	rnd_ltf_preview_t *pd = pd_;
	rnd_hid_attribute_t *attr = pd->attr;
	rnd_hid_preview_t *prv = attr->wdata;
	rnd_coord_t x, y;
	rnd_hid_mouse_ev_t kind = -1;

	if (prv->user_mouse_cb == NULL)
		return;

	if (cbs->event->xbutton.button == 1) {
		if (cbs->event->type == ButtonPress) kind = RND_HID_MOUSE_PRESS;
		if (cbs->event->type == ButtonRelease) kind = RND_HID_MOUSE_RELEASE;
	}
	else if (cbs->event->xbutton.button == 3) {
		if (cbs->event->type == ButtonRelease) kind = RND_HID_MOUSE_POPUP;
	}

	if (kind < 0)
		return;

	rnd_ltf_preview_getxy(pd, cbs->event->xbutton.x, cbs->event->xbutton.y, &x, &y);

	if (prv->user_mouse_cb(attr, prv, kind, x, y))
		rnd_ltf_preview_redraw(pd);
}

extern int lesstif_key_translate(XKeyEvent *e, int *out_mods, KeySym *out_sym);

static void ltf_preview_input_key(Widget w, XtPointer pd_, XmDrawingAreaCallbackStruct *cbs)
{
	rnd_ltf_preview_t *pd = pd_;
	rnd_hid_attribute_t *attr = pd->attr;
	rnd_hid_preview_t *prv = attr->wdata;
	int mods, release;
	KeySym sym;

	release = (cbs->event->type == KeyRelease);

	if (lesstif_key_translate(&cbs->event->xkey, &mods, &sym) != 0)
		return;

	if (pd->flip_local && !release) {
		if (sym == XK_Tab) {
			pd->flip_y = !pd->flip_y;
			rnd_ltf_preview_redraw(pd);
		}
	}

	if ((prv->user_key_cb != NULL) && (prv->user_key_cb(attr, prv, release, mods, sym, sym)))
		rnd_ltf_preview_redraw(pd);
}

static void ltf_preview_input_callback(Widget w, XtPointer pd_, XmDrawingAreaCallbackStruct *cbs)
{
	if ((cbs->event->type == KeyPress) || (cbs->event->type == KeyRelease))
		ltf_preview_input_key(w, pd_, cbs);
	else
		ltf_preview_input_mouse(w, pd_, cbs);
}


static void ltf_preview_destroy(Widget w, XtPointer pd_, XmDrawingAreaCallbackStruct *cbs)
{
	rnd_ltf_preview_t *pd = pd_;
	rnd_ltf_preview_del(pd);
}

static Widget ltf_preview_create(lesstif_attr_dlg_t *ctx, Widget parent, rnd_hid_attribute_t *attr)
{
	Widget pw;
	rnd_ltf_preview_t *pd;
	rnd_hid_preview_t *prv = attr->wdata;

	pd = calloc(1, sizeof(rnd_ltf_preview_t));
	prv->hid_wdata = pd;

	pd->attr = attr;
	memset(&pd->exp_ctx, 0, sizeof(pd->exp_ctx));
	pd->exp_ctx.draw_data = pd;
	pd->exp_ctx.expose_cb = ltf_preview_expose;
	pd->flip_global = !!(attr->rnd_hatt_flags & RND_HATF_PRV_GFLIP);
	pd->flip_local = !!(attr->rnd_hatt_flags & RND_HATF_PRV_LFLIP);
	pd->flip_x = pd->flip_y = 0;

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
		pd->x2 = RND_MM_TO_COORD(100);
		pd->y2 = RND_MM_TO_COORD(100);
	}

	stdarg(XmNwidth, prv->min_sizex_px);
	stdarg(XmNheight, prv->min_sizey_px);
	stdarg(XmNresizePolicy, XmRESIZE_ANY);
	stdarg(XmNuserData, pd);
	pw = XmCreateDrawingArea(parent, XmStrCast("dad_preview"), stdarg_args, stdarg_n);
	XtManageChild(pw);

	pd->window = XtWindow(pw);
	pd->pw = pw;
	pd->redraw_with_design = !!(attr->hatt_flags & RND_HATF_PRV_DESIGN);

	XtAddCallback(pw, XmNexposeCallback, (XtCallbackProc)rnd_ltf_preview_callback, (XtPointer)pd);
	XtAddCallback(pw, XmNresizeCallback, (XtCallbackProc)rnd_ltf_preview_callback, (XtPointer)pd);
	XtAddCallback(pw, XmNinputCallback, (XtCallbackProc)ltf_preview_input_callback, (XtPointer)pd);
	XtAddCallback(pw, XmNdestroyCallback, (XtCallbackProc)ltf_preview_destroy, (XtPointer)pd);
	XtAddEventHandler(pw, PointerMotionMask | PointerMotionHintMask | EnterWindowMask | LeaveWindowMask, 0, ltf_preview_motion_callback, (XtPointer)pd);


	XtManageChild(pw);
	rnd_ltf_preview_add(pd);
	return pw;
}


static Widget ltf_picture_create(lesstif_attr_dlg_t *ctx, Widget parent, rnd_hid_attribute_t *attr)
{
	Widget pic = rnd_ltf_xpm_label(display, parent, XmStrCast("dad_picture"), attr->wdata);
	XtManageChild(pic);
	return pic;
}

static Widget ltf_picbutton_create(lesstif_attr_dlg_t *ctx, Widget parent, rnd_hid_attribute_t *attr)
{
	Widget pic = rnd_ltf_xpm_button(display, parent, XmStrCast("dad_picture"), attr->wdata);
	XtManageChild(pic);
	return pic;
}

static void ltf_colorbtn_set(lesstif_attr_dlg_t *ctx, int idx, const rnd_color_t *clr)
{
	Widget btn = ctx->wl[idx];
	ctx->attrs[idx].val.clr = *clr;
	rnd_ltf_color_button_recolor(display, btn, clr);
}

#define CPACT "gui_FallbackColorPick"

static void ltf_colorbtn_valchg(Widget w, XtPointer dlg_widget_, XtPointer call_data)
{
	fgw_error_t rs;
	fgw_arg_t res, argv[2];
	lesstif_attr_dlg_t *ctx;
	const rnd_color_t *clr;
	rnd_color_t nclr;
	int r, widx = attr_get_idx(w, &ctx);
	if (widx < 0)
		return;

	clr = &ctx->attrs[widx].val.clr;

	argv[0].type = FGW_VOID;
	argv[1].type = FGW_STR | FGW_DYN;
	argv[1].val.str = rnd_strdup_printf("#%02x%02x%02x", clr->r, clr->g, clr->b);
	rs = rnd_actionv_bin(ltf_hidlib, CPACT, &res, 2, argv);
	if (rs != 0)
		return;

	if (!(res.type & FGW_STR)) {
		rnd_message(RND_MSG_ERROR, CPACT " returned non-string\n");
		fgw_arg_free(&rnd_fgw, &res);
		return;
	}

	r = rnd_color_load_str(&nclr, res.val.str);
	fgw_arg_free(&rnd_fgw, &res);
	if (r != 0) {
		rnd_message(RND_MSG_ERROR, CPACT " returned invalid color string\n");
		return;
	}

	fgw_arg_free(&rnd_fgw, &res);
	rnd_ltf_color_button_recolor(display, w, &nclr);
	ctx->attrs[widx].val.clr = nclr;
	valchg(w, dlg_widget_, w);
}


static Widget ltf_colorbtn_create(lesstif_attr_dlg_t *ctx, Widget parent, rnd_hid_attribute_t *attr, int readonly)
{
	Widget pic = rnd_ltf_color_button(display, parent, XmStrCast("dad_picture"), &attr->val.clr);
	if (!readonly)
		XtAddCallback(pic, XmNactivateCallback, ltf_colorbtn_valchg, NULL);
	XtManageChild(pic);
	return pic;
}

static void ltf_txt_changed_callback(Widget w, XtPointer attr_, XEvent *e, Boolean *ctd)
{
	valchg(w, w, NULL);
}

static long ltf_text_get_offs(rnd_hid_attribute_t *attrib, void *hid_ctx)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	Widget wtxt = ctx->wl[idx];
	XmTextPosition pos;

	stdarg_n = 0;
	stdarg(XmNcursorPosition, &pos);
	XtGetValues(wtxt, stdarg_args, stdarg_n);
	return pos;
}

void ltf_text_set_offs(rnd_hid_attribute_t *attrib, void *hid_ctx, long offs)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	Widget wtxt = ctx->wl[idx];
	XmTextSetInsertionPosition(wtxt, offs);
}

static void ltf_text_set_text_(Widget wtxt, unsigned how, const char *txt)
{
	XmTextPosition pos;

	switch(how & 0x0F) { /* ignore flags - no markup support */
		case RND_HID_TEXT_INSERT:
			stdarg_n = 0;
			stdarg(XmNcursorPosition, &pos);
			XtGetValues(wtxt, stdarg_args, stdarg_n);
			XmTextInsert(wtxt, pos, XmStrCast(txt));
			break;
		case RND_HID_TEXT_REPLACE:
			XmTextSetString(wtxt, XmStrCast(txt));
			break;
		case RND_HID_TEXT_APPEND:
			pos = 1<<30;
			XmTextInsert(wtxt, pos, XmStrCast(txt));
			break;
	}
}

static void ltf_text_set_text(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_hid_text_set_t how, const char *txt)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	Widget wtxt = ctx->wl[idx];

	if (how & RND_HID_TEXT_MARKUP) {
		char *orig, *tmp = rnd_strdup(txt);
		rnd_markup_state_t st = 0;
		char *seg;
		long seglen;

		orig = tmp;
		while((seg = (char *)rnd_markup_next(&st, (const char **)&tmp, &seglen)) != NULL) {
			char save = seg[seglen];
			seg[seglen] = '\0';
			ltf_text_set_text_(wtxt, how, seg);
			seg[seglen] = save;
		}
		free(orig);
	}
	else
		ltf_text_set_text_(wtxt, how, txt);
}


static void ltf_text_set(lesstif_attr_dlg_t *ctx, int idx, const char *val)
{
	ltf_text_set_text(&ctx->attrs[idx], ctx, RND_HID_TEXT_REPLACE, val);
}


static char *ltf_text_get_text_(rnd_hid_attribute_t *attrib, void *hid_ctx)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	Widget wtxt = ctx->wl[idx];
	return XmTextGetString(wtxt);
}

char *ltf_text_get_text(rnd_hid_attribute_t *attrib, void *hid_ctx)
{
	char *orig = ltf_text_get_text_(attrib, hid_ctx);
	char *s = rnd_strdup(orig);
	XtFree(orig);
	return s;
}

static void ltf_text_get_xy(rnd_hid_attribute_t *attrib, void *hid_ctx, long *x, long *y)
{
	char *orig, *s = ltf_text_get_text_(attrib, hid_ctx);
	long to, n, lines = 0, cols = 0;

	if (s == NULL) {
		*x = *y = 0;
		return;
	}

	orig = s;
	to = ltf_text_get_offs(attrib, hid_ctx);
	for(n = 0; n < to; n++,s++) {
		if (*s == '\n') {
			lines++;
			cols = 0;
		}
		else
			cols++;
	}

	XtFree(orig);
	*x = cols;
	*y = lines;
}

void ltf_text_set_xy(rnd_hid_attribute_t *attrib, void *hid_ctx, long x, long y)
{
	char *orig, *s = ltf_text_get_text_(attrib, hid_ctx);
	long offs;

	if (s == NULL)
		return;

	orig = s;
	for(offs = 0; *s != '\0'; s++,offs++) {
		if (*s == '\n') {
			y--;
			if (y < 0) {
				offs--;
				break;
			}
		}
		else if (y == 0) {
			if (x == 0)
				break;
			x--;
		}
	}
	ltf_text_set_offs(attrib, hid_ctx, offs);
	XtFree(orig);
}

void ltf_text_set_readonly(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_bool readonly)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	Widget wtxt = ctx->wl[idx];

	stdarg_n = 0;
	stdarg(XmNeditable, !readonly);
	XtSetValues(wtxt, stdarg_args, stdarg_n);
}

static void ltf_text_scroll_to_bottom(rnd_hid_attribute_t *attrib, void *hid_ctx)
{
	lesstif_attr_dlg_t *ctx = hid_ctx;
	int idx = attrib - ctx->attrs;
	Widget wtxt = ctx->wl[idx];
	char *buf;
	int offs, len;

	buf = XmTextGetString(wtxt);
	len = strlen(buf);

	if (len < 3)
		return;

	for(offs = len-2; offs > 0; offs--) {
		if (buf[offs] == '\n') {
			offs++;
			break;
		}
	}
	XmTextSetCursorPosition(wtxt, offs);
	free(buf);
}

static Widget ltf_text_create(lesstif_attr_dlg_t *ctx, Widget parent, rnd_hid_attribute_t *attr)
{
	Widget wtxt;
	rnd_hid_text_t *txt = attr->wdata;

	stdarg(XmNresizePolicy, XmRESIZE_GROW);
	stdarg(XmNeditMode, XmMULTI_LINE_EDIT);
	stdarg(XmNuserData, ctx);
	if (attr->rnd_hatt_flags & RND_HATF_SCROLL)
		wtxt = XmCreateScrolledText(parent, XmStrCast("dad_text"), stdarg_args, stdarg_n);
	else
		wtxt = XmCreateText(parent, XmStrCast("dad_text"), stdarg_args, stdarg_n);
	XtManageChild(wtxt);
	XtAddCallback(wtxt, XmNvalueChangedCallback, (XtCallbackProc)ltf_txt_changed_callback, (XtPointer)attr);


	txt->hid_get_xy = ltf_text_get_xy;
	txt->hid_get_offs = ltf_text_get_offs;
	txt->hid_set_xy = ltf_text_set_xy;
	txt->hid_set_offs = ltf_text_set_offs;
	txt->hid_scroll_to_bottom = ltf_text_scroll_to_bottom;
	txt->hid_get_text = ltf_text_get_text;
	txt->hid_set_text = ltf_text_set_text;
	txt->hid_set_readonly = ltf_text_set_readonly;

	return wtxt;
}

