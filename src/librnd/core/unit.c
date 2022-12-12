/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2011 Andrew Poelstra
 *  Copyright (C) 2016,2017,2019,2022 Tibor 'Igor2' Palinkas
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
#include <librnd/rnd_config.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/unit.h>

vtp0_t rnd_units;

/* Should be kept in order of smallest scale_factor to largest -- the code
   uses this ordering for finding the best scale to use for a group of units */
static rnd_unit_t rnd_unit_tbl[] = {
	{"km",   'k', 0.000001, RND_UNIT_METRIC,   RND_UNIT_ALLOW_KM, 5, 0},
	{"m",    'f', 0.001,    RND_UNIT_METRIC,   RND_UNIT_ALLOW_M, 5, 0},
	{"cm",   'e', 0.1,      RND_UNIT_METRIC,   RND_UNIT_ALLOW_CM, 5, 0},
	{"mm",   'm', 1,        RND_UNIT_METRIC,   RND_UNIT_ALLOW_MM, 4, 0},
	{"um",   'u', 1000,     RND_UNIT_METRIC,   RND_UNIT_ALLOW_UM, 2, 0},
	{"du",   'd', 10000,    RND_UNIT_METRIC,   RND_UNIT_ALLOW_DU, 2, 0},  /* eagle bin decimicron */
	{"nm",   'n', 1000000,  RND_UNIT_METRIC,   RND_UNIT_ALLOW_NM, 0, 0},

	{"in",   'i', 0.001,    RND_UNIT_IMPERIAL, RND_UNIT_ALLOW_IN, 5, 0},
	{"mil",  'l', 1,        RND_UNIT_IMPERIAL, RND_UNIT_ALLOW_MIL, 2, 0},
	{"dmil", 'k', 10,       RND_UNIT_IMPERIAL, RND_UNIT_ALLOW_DMIL, 1, 0},/* kicad legacy decimil unit */
	{"cmil", 'c', 100,      RND_UNIT_IMPERIAL, RND_UNIT_ALLOW_CMIL, 0, 0},

	{"Hz",   'z', 1,           RND_UNIT_FREQ,  RND_UNIT_ALLOW_HZ, 3, 0},
	{"kHz",  'Z', 0.001,       RND_UNIT_FREQ,  RND_UNIT_ALLOW_KHZ, 6, 0},
	{"MHz",  'M', 0.000001,    RND_UNIT_FREQ,  RND_UNIT_ALLOW_MHZ, 6, 0},
	{"GHz",  'G', 0.000000001, RND_UNIT_FREQ,  RND_UNIT_ALLOW_GHZ, 6, 0},

	/* aliases */
	{"inch",  0,  0.001,    RND_UNIT_IMPERIAL, RND_UNIT_ALLOW_IN, 5, 1},
	{"pcb",   0,  100,      RND_UNIT_IMPERIAL, RND_UNIT_ALLOW_CMIL, 0, 1} /* old io_pcb unit */
};

#define UNIT_TBL_LEN ((int) (sizeof(rnd_unit_tbl) / sizeof(rnd_unit_tbl[0])))

void rnd_units_init(void)
{
	int i;

/* Initial allocation to avoid reallocs. Since rnd_unit_allow_t may
   be 32 bit bitfield, generally there would be at most 32 types and their
   aliases, so this buffer doesn't need to be too large. */
	vtp0_enlarge(&rnd_units, 128);
	rnd_units.used = 0;

	for (i = 0; i < UNIT_TBL_LEN; ++i) {
		rnd_unit_tbl[i].index = i;
		vtp0_append(&rnd_units, &rnd_unit_tbl[i]);
	}
}

void rnd_units_uninit(void)
{
	vtp0_uninit(&rnd_units);
}

const rnd_unit_t *rnd_get_unit_struct_(const char *suffix, int strict)
{
	int i;
	int s_len = 0;

	/* Determine bounds */
	while (isspace(*suffix))
		suffix++;
	while (isalnum(suffix[s_len]))
		s_len++;

	/* Also understand plural suffixes: "inches", "mils" */
	if (s_len > 2) {
		if (suffix[s_len - 2] == 'e' && suffix[s_len - 1] == 's')
			s_len -= 2;
		else if (suffix[s_len - 1] == 's')
			s_len -= 1;
	}

	/* Do lookup */
	if (s_len <= 0)
		return NULL;

	for (i = 0; i < rnd_units.used; ++i) {
		const rnd_unit_t *u = rnd_units.array[i];
		if (strcmp(suffix, u->suffix) == 0)
			return u;
	}

	if (!strict) {
		for (i = 0; i < rnd_units.used; ++i) {
			const rnd_unit_t *u = rnd_units.array[i];
			if (strncmp(suffix, u->suffix, s_len) == 0)
				return u;
		}
	}

	return NULL;
}

const rnd_unit_t *rnd_get_unit_struct(const char *suffix)
{
	return rnd_get_unit_struct_(suffix, 0);
}


const rnd_unit_t *rnd_get_unit_struct_by_allow(rnd_unit_allow_t allow)
{
	int i;
	for (i = 0; i < rnd_units.used; ++i) {
		const rnd_unit_t *u = rnd_units.array[i];
		if (u->allow == allow)
			return u;
	}

	return NULL;
}

const rnd_unit_t *rnd_unit_get_idx(int idx)
{
	if ((idx < 0) || (idx >= rnd_units.used))
		return NULL;
	return rnd_units.array[idx];
}

const rnd_unit_t *rnd_unit_get_suffix(const char *suffix)
{
	int i;
	for (i = 0; i < rnd_units.used; ++i) {
		const rnd_unit_t *u = rnd_units.array[i];
		if (strcmp(u->suffix, suffix) == 0)
			return u;
	}
	return NULL;
}

int rnd_get_n_units()
{
	return rnd_units.used;
}

double rnd_coord_to_unit(const rnd_unit_t *unit, rnd_coord_t x)
{
	double base;
	if (unit == NULL)
		return -1;
	switch(unit->family) {
		case RND_UNIT_METRIC: base = RND_COORD_TO_MM(1); break;
		case RND_UNIT_IMPERIAL: base = RND_COORD_TO_MIL(1); break;
		case RND_UNIT_FREQ: base = 1.0; break;
		default: base = 1.0; break;
	}
	return x * unit->scale_factor * base;
}

rnd_coord_t rnd_unit_to_coord(const rnd_unit_t *unit, double x)
{
	double base, res;

	if (unit == NULL)
		return -1;

	switch(unit->family) {
		case RND_UNIT_METRIC:   base = RND_MM_TO_COORD(x); break;
		case RND_UNIT_IMPERIAL: base = RND_MIL_TO_COORD(x); break;
		case RND_UNIT_FREQ:     base = x; break;
		default:                base = x; break;
	}
	res = rnd_round(base/unit->scale_factor);

	/* clamp */
	switch(unit->family) {
		case RND_UNIT_METRIC:
		case RND_UNIT_IMPERIAL:
			if (res >= (double)RND_COORD_MAX)
				return RND_COORD_MAX;
			if (res <= -1.0 * (double)RND_COORD_MAX)
				return -RND_COORD_MAX;
			break;
		case RND_UNIT_FREQ:
		default:
			break;
	}
	return res;
}

double rnd_unit_to_factor(const rnd_unit_t *unit)
{
	return 1.0 / rnd_coord_to_unit(unit, 1);
}

rnd_angle_t rnd_normalize_angle(rnd_angle_t a)
{
	while (a < 0)
		a += 360.0;
	while (a >= 360.0)
		a -= 360.0;
	return a;
}

/*** dynamic ***/
static unsigned long rnd_unit_next_family_bit = RND_UNIT_dyn;
unsigned long rnd_unit_reg_family(void)
{
	long id = rnd_unit_next_family_bit;
	rnd_unit_next_family_bit <<= 1;
	return id;
}


static unsigned long rnd_unit_next_allow_bit = RND_UNIT_ALLOW_dyn;
int rnd_unit_reg_units(rnd_unit_t *in, int num_in, unsigned long family_bit)
{
	int n;

	for(n = 0; n < num_in; n++) {
		in[n].index = rnd_units.used;
		in[n].family = family_bit;
		in[n].allow = rnd_unit_next_allow_bit;
		rnd_unit_next_allow_bit <<= 1;
		vtp0_append(&rnd_units, &in[n]);
	}

	return 0;
}

