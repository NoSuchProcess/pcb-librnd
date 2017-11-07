/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2003, 2004 Thomas Nau
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

/* Local functions to draw a layer group as a composite of logical layers
   using positive and negative draw operations. Included from draw.c. */

typedef struct comp_ctx_s {
	pcb_board_t *pcb;
	const pcb_box_t *screen;
	pcb_layergrp_t *grp;
	pcb_layergrp_id_t gid;
	const char *color;

	unsigned thin:1;
	unsigned invert:1;
} comp_ctx_t;

static void comp_fill_board(comp_ctx_t *ctx)
{
	pcb_gui->set_color(Output.fgGC, ctx->color);
	if (ctx->screen == NULL)
		pcb_gui->fill_rect(Output.fgGC, 0, 0, ctx->pcb->MaxWidth, ctx->pcb->MaxHeight);
	else
		pcb_gui->fill_rect(Output.fgGC, ctx->screen->X1, ctx->screen->Y1, ctx->screen->X2, ctx->screen->Y2);
}

static void comp_start_sub_(comp_ctx_t *ctx)
{
	pcb_gui->set_drawing_mode(PCB_HID_COMP_NEGATIVE, Output.direct, ctx->screen);
}

static void comp_start_add_(comp_ctx_t *ctx)
{
	pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, Output.direct, ctx->screen);
}

static void comp_start_sub(comp_ctx_t *ctx)
{
	if (ctx->thin) {
		pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, Output.direct, ctx->screen);
		pcb_gui->set_color(Output.pmGC, ctx->color);
		return;
	}

	if (ctx->invert)
		comp_start_add_(ctx);
	else
		comp_start_sub_(ctx);
}

static void comp_start_add(comp_ctx_t *ctx)
{
	if (ctx->thin) {
		pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, Output.direct, ctx->screen);
		return;
	}

	if (ctx->invert)
		comp_start_sub_(ctx);
	else
		comp_start_add_(ctx);
}

static void comp_finish(comp_ctx_t *ctx)
{
	if (ctx->thin) {
		pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, Output.direct, ctx->screen);
		return;
	}

	pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, Output.direct, ctx->screen);
}

static void comp_init(comp_ctx_t *ctx, int negative)
{
	pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, Output.direct, ctx->screen);

	if (ctx->thin)
		return;

	if (ctx->invert)
		negative = !negative;

	if ((!ctx->thin) && (negative)) {
		/* drawing the big poly for the negative */
		pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, Output.direct, ctx->screen);
		comp_fill_board(ctx);
	}
}

/* Real composite draw: if any layer is negative, we have to use the HID API's
   composite layer draw mechanism */
static void comp_draw_layer_real(comp_ctx_t *ctx, void (*draw_auto)(comp_ctx_t *ctx, void *data), void *auto_data)
{ /* generic multi-layer rendering */
	int n, adding = -1;
	pcb_layer_t *l = pcb_get_layer(PCB->Data, ctx->grp->lid[0]);
	comp_init(ctx, (l->comb & PCB_LYC_SUB));

	for(n = 0; n < ctx->grp->len; n++) {
		int want_add;
		l = pcb_get_layer(PCB->Data, ctx->grp->lid[n]);

		want_add = ctx->thin || !(l->comb & PCB_LYC_SUB);
		if (want_add != adding) {
			if (want_add)
				comp_start_add(ctx);
			else
				comp_start_sub(ctx);
			adding = want_add;
		}

		{
			const char *old_color = l->meta.real.color;
			pcb_hid_gc_t old_fg = Output.fgGC;
			Output.fgGC = Output.pmGC;
			l->meta.real.color = ctx->color;
			if (!want_add)
				l->meta.real.color = "erase";
			if (l->comb & PCB_LYC_AUTO)
				draw_auto(ctx, auto_data);
			pcb_draw_layer(l, ctx->screen);
			l->meta.real.color = old_color;
			Output.fgGC = old_fg;
		}
	}
	if (!adding)
		comp_start_add(ctx);
}

int pcb_draw_layergrp_is_comp(pcb_layergrp_t *g)
{
	int n;

	/* as long as we are doing only positive compositing, we can go without compositing */
	for(n = 0; n < g->len; n++)
		if (PCB->Data->Layer[g->lid[n]].comb & (PCB_LYC_SUB))
			return 1;

	return 0;
}

int pcb_draw_layer_is_comp(pcb_layer_id_t id)
{
	pcb_layergrp_t *g = pcb_get_layergrp(PCB, PCB->Data->Layer[id].meta.real.grp);
	if (g == NULL) return 0;
	return pcb_draw_layergrp_is_comp(g);
}

/* Draw a layer group with fake or real compositing */
static void comp_draw_layer(comp_ctx_t *ctx, void (*draw_auto)(comp_ctx_t *ctx, void *data), void *auto_data)
{
	int is_comp = pcb_draw_layergrp_is_comp(ctx->grp);

	if (is_comp)
		Output.direct = 0;

	comp_draw_layer_real(ctx, draw_auto, auto_data);
}
