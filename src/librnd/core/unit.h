/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2011 Andrew Poelstra
 *  Copyright (C) 2016,2019,2022 Tibor 'Igor2' Palinkas
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
 *
 */

#ifndef RND_UNIT_H
#define RND_UNIT_H

#include <librnd/core/global_typedefs.h>
#include <genvector/vtp0.h>

/*** Static units - these are hardwired because they are needed by most apps ***/
enum rnd_unit_allow_e {
	RND_UNIT_NO_PRINT = 0, /* suffixes we can read but not print (i.e., "inch") */
	RND_UNIT_ALLOW_NM = 1,
	RND_UNIT_ALLOW_UM = 2,
	RND_UNIT_ALLOW_MM = 4,
	RND_UNIT_ALLOW_CM = 8,
	RND_UNIT_ALLOW_M = 16,
	RND_UNIT_ALLOW_KM = 32,

	RND_UNIT_ALLOW_HZ = 64,
	RND_UNIT_ALLOW_KHZ = 128,
	RND_UNIT_ALLOW_MHZ = 256,
	RND_UNIT_ALLOW_GHZ = 512,

	RND_UNIT_ALLOW_CMIL = 1024, /* pcb-rnd: old gEDA/pcb format */
	RND_UNIT_ALLOW_MIL = 2048,

	/* unusual units, avoid using these */
	RND_UNIT_ALLOW_IN = 4096,
	RND_UNIT_ALLOW_DMIL = 8192, /* for kicad legacy decimil units */
	RND_UNIT_ALLOW_DU = 16384,  /* "du" = 0.1 micron "decimicron" units, for eagle bin */

	RND_UNIT_ALLOW_dyn = 32768, /* first bit available for dynamic registration */

	RND_UNIT_ALLOW_METRIC = RND_UNIT_ALLOW_NM  | RND_UNIT_ALLOW_UM | RND_UNIT_ALLOW_MM | RND_UNIT_ALLOW_CM | RND_UNIT_ALLOW_M | RND_UNIT_ALLOW_KM,
	RND_UNIT_ALLOW_IMPERIAL = RND_UNIT_ALLOW_DMIL | RND_UNIT_ALLOW_CMIL | RND_UNIT_ALLOW_MIL | RND_UNIT_ALLOW_IN,
	RND_UNIT_ALLOW_FREQ = RND_UNIT_ALLOW_HZ | RND_UNIT_ALLOW_KHZ | RND_UNIT_ALLOW_MHZ | RND_UNIT_ALLOW_GHZ,

	/* DO NOT USE - this is all units allowed in %mr and pcb-rnd's io_pcb old format */
	RND_UNIT_ALLOW_READABLE = RND_UNIT_ALLOW_CMIL,

	/* Used for rnd_printf %mS - should not include unusual units like km, cmil and dmil */
	RND_UNIT_ALLOW_NATURAL = RND_UNIT_ALLOW_NM | RND_UNIT_ALLOW_UM | RND_UNIT_ALLOW_MM | RND_UNIT_ALLOW_M | RND_UNIT_ALLOW_MIL | RND_UNIT_ALLOW_IN,

	/* Allow all but the most exotic: anything up to IN, minus the freqs */
	RND_UNIT_ALLOW_ALL_SANE = (RND_UNIT_ALLOW_DMIL-1) & (~(RND_UNIT_ALLOW_HZ | RND_UNIT_ALLOW_KHZ | RND_UNIT_ALLOW_MHZ | RND_UNIT_ALLOW_GHZ))
}; /* used as rnd_unit_allow_t, which is unsigned long to guarantee enough bits for dynamic units */

typedef unsigned long rnd_unit_allow_t;

/* bitfield */
enum rnd_family_e {
	RND_UNIT_METRIC   = 1,
	RND_UNIT_IMPERIAL = 2,
	RND_UNIT_FREQ     = 4,
	RND_UNIT_dyn      = 8   /* first bit available for dynamic registration */
}; /* used as rnd_unit_family_t, which is unsigned long to guarantee enough bits for dynamic units */
typedef unsigned long rnd_unit_family_t;

typedef enum rnd_unit_suffix_e {
	RND_UNIT_NO_SUFFIX,    /* do not print suffix */
	RND_UNIT_SUFFIX,       /* print suffix separated by a space from the value - human readable */
	RND_UNIT_FILE_MODE     /* print suffix without separation - keeps the value a single token */
} rnd_unit_suffix_t;

struct rnd_unit_s {
	const char *suffix;
	char printf_code;        /* 0 means the unit is an alias and should not be used in printf */
	double scale_factor;
	rnd_unit_family_t family;
	rnd_unit_allow_t allow;
	int default_prec;
	char is_alias;           /* if 1, this unit is a less favorable alias for another unit */

	/* autogenerated */
	int index; /* Index into the rnd_units vector */
};

extern vtp0_t rnd_units; /* of (const rnd_unit_t *) elements */

/* Look up a given suffix in the units array. Pluralized units are supported.
   The _ version allows strict (full name match) lookup if strict iz non-zero. */
const rnd_unit_t *rnd_get_unit_struct(const char *suffix);
const rnd_unit_t *rnd_get_unit_struct_(const char *suffix, int strict);

/* Return the furst unit that matches allow */
const rnd_unit_t *rnd_get_unit_struct_by_allow(rnd_unit_allow_t allow);

/* Return the number of units (use only when listing units) */
int rnd_get_n_units();

/* Return the idxth unit or NULL (bounds check) */
const rnd_unit_t *rnd_unit_get_idx(int idx);

/* Return the unit with (case sensitive) matching suffix */
const rnd_unit_t *rnd_unit_get_suffix(const char *suffix);

/* Convert x to the given unit */
double rnd_coord_to_unit(const rnd_unit_t *unit, rnd_coord_t x);

/* Return how many rnd-internal-coord-unit a unit translates to */
double rnd_unit_to_factor(const rnd_unit_t *unit);

/* Convert a given unit to librnd coords; clamp at the end of the ranges */
rnd_coord_t rnd_unit_to_coord(const rnd_unit_t *unit, double x);

/* Bring an angle into [0, 360) range */
rnd_angle_t rnd_normalize_angle(rnd_angle_t a);


/* Initialize non-static unit data: assigns each unit its index for
   quick access through the main units vector */
void rnd_units_init(void);
void rnd_units_uninit(void);

/* internal<->physical unit conversions */
#define RND_COORD_TO_MIL(n)      ((n) / 25400.0)
#define RND_MIL_TO_COORD(n)      ((n) * 25400.0)
#define RND_COORD_TO_MM(n)       ((n) / 1000000.0)
#define RND_MM_TO_COORD(n)       ((n) * 1000000.0)
#define RND_COORD_TO_INCH(n)     (RND_COORD_TO_MIL(n) / 1000.0)
#define RND_INCH_TO_COORD(n)     (RND_MIL_TO_COORD(n) * 1000.0)

#define RND_SWAP_ANGLE(a)        (-(a))
#define RND_SWAP_DELTA(d)        (-(d))

RND_INLINE rnd_coord_t rnd_coord_abs(rnd_coord_t c)
{
	if (c < 0) return -c;
	return c;
}

/*** dynamic unit system (for apps to register custom types) ***/

/* Register a new family and return a type bit representing it (as if it
   was part of the rnd_unit_family_t enum) */
unsigned long rnd_unit_reg_family(void);

/* Register a set of units of a family. Fills in the following fields:
    ->family (from the argument)
    ->allow (newly allocated bits)
    ->index
   Returns 0 on success.
*/
int rnd_unit_reg_units(rnd_unit_t *in, int num_in, unsigned long family_bit);



#endif
