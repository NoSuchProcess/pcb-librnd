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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#ifndef	PCB_FILE_H
#define	PCB_FILE_H

#include <stdio.h>							/* needed to define 'FILE *' */
#include "config.h"
#include "board.h"
#include "plug_io.h"

/* This is used by the lexer/parser */
typedef struct {
	int ival;
	pcb_coord_t bval;
	double dval;
	char has_units;
} PLMeasure;

typedef enum {
	PCB_USTY_CMIL = 1,       /* unitless centimil */
	PCB_USTY_NANOMETER = 2,
	PCB_USTY_UNITS = 4       /* using various units */
} pcb_unit_style_t;

extern pcb_unit_style_t pcb_io_pcb_usty_seen;

typedef struct {
	const char *write_coord_fmt;
} io_pcb_ctx_t;

extern pcb_plug_io_t *pcb_preferred_io_pcb, *pcb_nanometer_io_pcb, *pcb_centimil_io_pcb;


int io_pcb_WriteBuffer(pcb_plug_io_t *ctx, FILE *f, pcb_buffer_t *buff, pcb_bool elem_only);
int io_pcb_WriteElementData(pcb_plug_io_t *ctx, FILE *f, pcb_data_t *);
int io_pcb_WritePCB(pcb_plug_io_t *ctx, FILE *f, const char *old_filename, const char *new_filename, pcb_bool emergency);

void PreLoadElementPCB(void);
void PostLoadElementPCB(void);

int io_pcb_test_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, FILE *f);

/*
 * Whenever the pcb file format is modified, this version number
 * should be updated to the date when the new code is committed.
 * It will be written out to the file and also used by pcb to give
 * guidance to the user as to what the minimum version of pcb required
 * is.
 */

/* This is the version needed by the file we're saving.  */
int PCBFileVersionNeeded(void);

/* Improvise layers and groups for a partial input file that lacks layer groups (and maybe even some layers) */
int pcb_layer_improvise(pcb_board_t *pcb);

/* This is the version we support.  */
#define PCB_FILE_VERSION 20110603

#endif
