/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2005 DJ Delorie
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  DJ Delorie, 334 North Road, Deerfield NH 03037-1110, USA
 *  dj@delorie.com
 *
 */

#ifndef PCB_STRFLAGS_H
#define PCB_STRFLAGS_H

#include "flag.h"

typedef struct {

	/* This is the bit that we're setting.  */
	pcb_flag_values_t mask;
	const char *mask_name;

	/* The name used in the output file.  */
	const char *name;
	int nlen;

	/* If set, this entry won't be output unless the object type is one
	   of these.  */
	int object_types;

	char *help;
} pcb_flag_bits_t;

/* All flags natively known by the core */
extern pcb_flag_bits_t pcb_object_flagbits[];
extern const int pcb_object_flagbits_len;

/* The purpose of this interface is to make the file format able to
   handle more than 32 flags, and to hide the internal details of
   flags from the file format.  */

/* When passed a string, parse it and return an appropriate set of
   flags.  Errors cause error() to be called with a suitable message;
   if error is NULL, errors are ignored.  */
pcb_flag_t pcb_strflg_s2f(const char *flagstring, int (*error) (const char *msg), unsigned char *intconn);

/* Given a set of flags for a given object type, return a string which
   can be output to a file.  The returned pointer must not be
   freed.  */
char *pcb_strflg_f2s(pcb_flag_t flags, int object_type, unsigned char *intconn);

/* same as above, for pcb level flags */
char *pcb_strflg_board_f2s(pcb_flag_t flags);
pcb_flag_t pcb_strflg_board_s2f(const char *flagstring, int (*error) (const char *msg));

void pcb_strflg_uninit_buf(void);
void pcb_strflg_uninit_layerlist(void);

/* low level */
pcb_flag_t pcb_strflg_common_s2f(const char *flagstring, int (*error) (const char *msg), pcb_flag_bits_t * flagbits, int n_flagbits, unsigned char *intconn);
char *pcb_strflg_common_f2s(pcb_flag_t flags, int object_type, pcb_flag_bits_t * flagbits, int n_flagbits, unsigned char *intconn);

#endif
