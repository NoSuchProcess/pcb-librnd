/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
#include "config.h"
#include "board.h"
#include "data.h"
#include "conf_core.h"
#include "plug_io.h"
#include "compat_misc.h"
#include "hid_actions.h"
#include "paths.h"
#include "rtree.h"
#include "undo.h"
#include "draw.h"
#include "event.h"

pcb_board_t *PCB;

void pcb_board_free(pcb_board_t * pcb)
{
	int i;

	if (pcb == NULL)
		return;

	free(pcb->Name);
	free(pcb->Filename);
	free(pcb->PrintFilename);
	pcb_ratspatch_destroy(pcb);
	pcb_data_free(pcb->Data);
	free(pcb->Data);
	/* release font symbols */
	pcb_fontkit_free(&pcb->fontkit);
	for (i = 0; i < PCB_NUM_NETLISTS; i++)
		pcb_lib_free(&(pcb->NetlistLib[i]));
	vtroutestyle_uninit(&pcb->RouteStyle);
	pcb_attribute_free(&pcb->Attributes);
	/* clear struct */
	memset(pcb, 0, sizeof(pcb_board_t));
}

/* creates a new PCB */
pcb_board_t *pcb_board_new_(pcb_bool SetDefaultNames)
{
	pcb_board_t *ptr, *save;
	int i;

	/* allocate memory, switch all layers on and copy resources */
	ptr = calloc(1, sizeof(pcb_board_t));
	ptr->Data = pcb_buffer_new(ptr);

	ptr->ThermStyle = 4;
	ptr->IsleArea = 2.e8;
	ptr->SilkActive = pcb_false;
	ptr->RatDraw = pcb_false;

	/* NOTE: we used to set all the pcb flags on ptr here, but we don't need to do that anymore due to the new conf system */
						/* this is the most useful starting point for now */

	ptr->Grid = conf_core.editor.grid;
	save = PCB;
	PCB = ptr;
	pcb_layer_parse_group_string(ptr, conf_core.design.groups, PCB_MAX_LAYER, 0);
	pcb_event(PCB_EVENT_ROUTE_STYLES_CHANGED, NULL);
	PCB = save;

	ptr->Zoom = conf_core.editor.zoom;
	ptr->MaxWidth = conf_core.design.max_width;
	ptr->MaxHeight = conf_core.design.max_height;
	ptr->ID = pcb_create_ID_get();
	ptr->ThermScale = 0.5;

	ptr->Bloat = conf_core.design.bloat;
	ptr->Shrink = conf_core.design.shrink;
	ptr->minWid = conf_core.design.min_wid;
	ptr->minSlk = conf_core.design.min_slk;
	ptr->minDrill = conf_core.design.min_drill;
	ptr->minRing = conf_core.design.min_ring;

	for (i = 0; i < PCB_MAX_LAYER; i++)
		ptr->Data->Layer[i].Name = pcb_strdup(conf_core.design.default_layer_name[i]);

	pcb_font_create_default(ptr);

	return (ptr);
}

#include "defpcb_internal.c"

pcb_board_t *pcb_board_new(int inhibit_events)
{
	pcb_board_t *old, *nw = NULL;
	int dpcb;
	unsigned int inh = inhibit_events ? PCB_INHIBIT_BOARD_CHANGED : 0;

	old = PCB;
	PCB = NULL;

	dpcb = -1;
	pcb_io_err_inhibit_inc();
	conf_list_foreach_path_first(dpcb, &conf_core.rc.default_pcb_file, pcb_load_pcb(__path__, NULL, pcb_false, 1 | 0x10 | inh));
	pcb_io_err_inhibit_dec();

	if (dpcb != 0) { /* no default PCB in file, use embedded version */
		FILE *f;
		const char *tmp_fn = ".pcb-rnd.default.pcb";

		/* We can parse from file only, make a temp file */
		f = fopen(tmp_fn, "wb");
		if (f != NULL) {
			fwrite(default_pcb_internal, strlen(default_pcb_internal), 1, f);
			fclose(f);
			dpcb = pcb_load_pcb(tmp_fn, NULL, pcb_false, 1 | 0x10);
			if (dpcb == 0)
				pcb_message(PCB_MSG_WARNING, "Couldn't find default.pcb - using the embedded fallback\n");
			else
				pcb_message(PCB_MSG_ERROR, "Couldn't find default.pcb and failed to load the embedded fallback\n");
			remove(tmp_fn);
		}
	}

	if (dpcb == 0) {
		nw = PCB;
		if (nw->Filename != NULL) {
			/* make sure the new PCB doesn't inherit the name and loader of the default pcb */
			free(nw->Filename);
			nw->Filename = NULL;
			nw->Data->loader = NULL;
		}
	}

	PCB = old;
	return nw;
}

int pcb_board_new_postproc(pcb_board_t *pcb, int use_defaults)
{
	/* copy default settings */
	pcb_colors_from_settings(pcb);

	return 0;
}

#warning TODO: indeed, remove this and all the board *color fields
void pcb_colors_from_settings(pcb_board_t *ptr)
{
	int i;

	/* copy default settings */
	for (i = 0; i < PCB_MAX_LAYER; i++) {
		ptr->Data->Layer[i].Color = conf_core.appearance.color.layer[i];
		ptr->Data->Layer[i].SelectedColor = conf_core.appearance.color.layer_selected[i];
	}
}

typedef struct {
	int nplated;
	int nunplated;
} HoleCountStruct;

static pcb_r_dir_t hole_counting_callback(const pcb_box_t * b, void *cl)
{
	pcb_pin_t *pin = (pcb_pin_t *) b;
	HoleCountStruct *hcs = (HoleCountStruct *) cl;
	if (PCB_FLAG_TEST(PCB_FLAG_HOLE, pin))
		hcs->nunplated++;
	else
		hcs->nplated++;
	return PCB_R_DIR_FOUND_CONTINUE;
}

void pcb_board_count_holes(int *plated, int *unplated, const pcb_box_t * within_area)
{
	HoleCountStruct hcs = { 0, 0 };

	pcb_r_search(PCB->Data->pin_tree, within_area, NULL, hole_counting_callback, &hcs, NULL);
	pcb_r_search(PCB->Data->via_tree, within_area, NULL, hole_counting_callback, &hcs, NULL);

	if (plated != NULL)
		*plated = hcs.nplated;
	if (unplated != NULL)
		*unplated = hcs.nunplated;
}

const char *pcb_board_get_filename(void)
{
	return PCB->Filename;
}

const char *pcb_board_get_name(void)
{
	return PCB->Name;
}

pcb_bool pcb_board_change_name(char *Name)
{
	free(PCB->Name);
	PCB->Name = Name;
	pcb_board_changed(0);
	return (pcb_true);
}

void pcb_board_resize(pcb_coord_t Width, pcb_coord_t Height)
{
	PCB->MaxWidth = Width;
	PCB->MaxHeight = Height;

	/* crosshair range is different if pastebuffer-mode
	 * is enabled
	 */
	if (conf_core.editor.mode == PCB_MODE_PASTE_BUFFER)
		pcb_crosshair_set_range(PCB_PASTEBUFFER->X - PCB_PASTEBUFFER->BoundingBox.X1,
											PCB_PASTEBUFFER->Y - PCB_PASTEBUFFER->BoundingBox.Y1,
											MAX(0,
													Width - (PCB_PASTEBUFFER->BoundingBox.X2 -
																	 PCB_PASTEBUFFER->X)), MAX(0, Height - (PCB_PASTEBUFFER->BoundingBox.Y2 - PCB_PASTEBUFFER->Y)));
	else
		pcb_crosshair_set_range(0, 0, Width, Height);

	pcb_board_changed(0);
}

void pcb_board_remove(pcb_board_t *Ptr)
{
	pcb_undo_clear_list(pcb_true);
	pcb_board_free(Ptr);
	free(Ptr);
}

/* sets cursor grid with respect to grid offset values */
void pcb_board_set_grid(pcb_coord_t Grid, pcb_bool align)
{
	if (Grid >= 1 && Grid <= PCB_MAX_GRID) {
		if (align) {
			PCB->GridOffsetX = pcb_crosshair.X % Grid;
			PCB->GridOffsetY = pcb_crosshair.Y % Grid;
		}
		PCB->Grid = Grid;
		conf_set_design("editor/grid", "%$mS", Grid);
		if (conf_core.editor.draw_grid)
			pcb_redraw();
	}
}

/* sets a new line thickness */
void pcb_board_set_line_width(pcb_coord_t Size)
{
	if (Size >= PCB_MIN_LINESIZE && Size <= PCB_MAX_LINESIZE) {
		conf_set_design("design/line_thickness", "%$mS", Size);
		if (conf_core.editor.auto_drc)
			pcb_crosshair_grid_fit(pcb_crosshair.X, pcb_crosshair.Y);
	}
}

/* sets a new via thickness */
void pcb_board_set_via_size(pcb_coord_t Size, pcb_bool Force)
{
	if (Force || (Size <= PCB_MAX_PINORVIASIZE && Size >= PCB_MIN_PINORVIASIZE && Size >= conf_core.design.via_drilling_hole + PCB_MIN_PINORVIACOPPER)) {
		conf_set_design("design/via_thickness", "%$mS", Size);
	}
}

/* sets a new via drilling hole */
void pcb_board_set_via_drilling_hole(pcb_coord_t Size, pcb_bool Force)
{
	if (Force || (Size <= PCB_MAX_PINORVIASIZE && Size >= PCB_MIN_PINORVIAHOLE && Size <= conf_core.design.via_thickness - PCB_MIN_PINORVIACOPPER)) {
		conf_set_design("design/via_drilling_hole", "%$mS", Size);
	}
}

/* sets a clearance width */
void pcb_board_set_clearance(pcb_coord_t Width)
{
	if (Width <= PCB_MAX_LINESIZE) {
		conf_set_design("design/clearance", "%$mS", Width);
	}
}

/* sets a text scaling */
void pcb_board_set_text_scale(int Scale)
{
	if (Scale <= PCB_MAX_TEXTSCALE && Scale >= PCB_MIN_TEXTSCALE) {
		conf_set_design("design/text_scale", "%d", Scale);
	}
}

/* sets or resets changed flag and redraws status */
void pcb_board_set_changed_flag(pcb_bool New)
{
	PCB->Changed = New;
}


void pcb_board_changed(int reverted)
{
	pcb_event(PCB_EVENT_BOARD_CHANGED, "i", reverted);
}
