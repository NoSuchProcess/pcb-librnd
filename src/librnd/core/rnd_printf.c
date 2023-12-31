/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2011 Andrew Poelstra
 *  Copyright (C) 2016,2019,2022,2023 Tibor 'Igor2' Palinkas
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

/* Implementation of printf wrapper to output pcb coords and angles
 * For details of all supported specifiers, see the comment at the
 * top of rnd_printf.h */

#include <stdio.h>
#include <locale.h>
#include <ctype.h>
#include <math.h>
#include <librnd/rnd_config.h>
#include <librnd/core/rnd_printf.h>
#include "actions.h"
#include "hidlib.h"

const char *rnd_printf_slot[RND_PRINTF_SLOT_max] =
{
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 8 user formats */
	"%mr",      /* original unitless cmil file format coord */
	"%.07$$mS"  /* safe file format coord */
};

static int min_sig_figs(double d)
{
	char buf[50];
	int rv;

	if (d == 0)
		return 0;

	/* Normalize to x.xxxx... form */
	if (d < 0)
		d *= -1;
	while (d >= 10)
		d /= 10;
	while (d < 1)
		d *= 10;

	rv = sprintf(buf, "%g", d);
	return rv;
}

/* Truncate trailing 0's from str */
static int do_trunc0(char *str)
{
	char *end = str + strlen(str) - 1;
	while((end > str) && (*end == '0') && (end[-1] != '.')) {
		*end = '\0';
		end--;
	}
	return end-str+1;
}


/* Create sprintf specifier, using default_prec no precision is given */
RND_INLINE void make_printf_spec(char *printf_spec_new, const char *printf_spec, int precision, int *trunc0)
{
	int i = 0;

	*trunc0 = 0;

	while((printf_spec[i] == '%') || isdigit(printf_spec[i]) || (printf_spec[i] == '-') || (printf_spec[i] == '+') || (printf_spec[i] == '#') || (printf_spec[i] == '0'))
		++i;

	if (printf_spec[i] == '.') {
		if ((printf_spec[i+1] == '0') && (isdigit(printf_spec[i+2])))
			*trunc0 = 1;
		sprintf(printf_spec_new, ", %sf", printf_spec);
	}
	else
		sprintf(printf_spec_new, ", %s.%df", printf_spec, precision);
}

/* sprintf a value (used to change locale on is_file_mode) */
#define sprintf_lc_safe(is_file_mode, out, fmt, val) \
do { \
	sprintf(out, fmt, val); \
	if (trunc0) \
		do_trunc0(filemode_buff); \
} while(0)

/* append a suffix, with or without space */
RND_INLINE int append_suffix(gds_t *dest, rnd_unit_suffix_t suffix_type, const char *suffix)
{
	switch (suffix_type) {
	case RND_UNIT_NO_SUFFIX:
		break;
	case RND_UNIT_SUFFIX:
		if (gds_append(dest, ' ') != 0) return -1;
		/* deliberate fall-thru */
	case RND_UNIT_FILE_MODE:
		if (gds_append_str(dest, suffix) != 0) return -1;
		break;
	}
	return 0;
}


/* Internal coord-to-string converter for pcb-printf
 * Converts a (group of) measurement(s) to a comma-delimited
 * string, with appropriate units. If more than one coord is
 * given, the list is enclosed in parens to make the scope of
 * the unit suffix clear.
 *
 * dest         Append the output to this dynamic string
 * coord        Array of coords to convert
 * n_coords     Number of coords in array
 * printf_spec  printf sub-specifier to use with %f
 * e_allow      Bitmap of units the function may use
 * suffix_type  Whether to add a suffix
 *
 * return 0 on success, -1 on error
 */
static int CoordsToString(gds_t *dest, rnd_coord_t coord[], int n_coords, const gds_t *printf_spec_, rnd_unit_allow_t allow, rnd_unit_suffix_t suffix_type)
{
	char filemode_buff[128]; /* G_ASCII_DTOSTR_BUF_SIZE */
	char printf_spec_new_local[256];
	double *value, value_local[32];
	enum rnd_family_e family;
	const char *suffix;
	int i, n, retval = -1, trunc0 = 0;
	const char *printf_spec = printf_spec_->array;
	char *printf_spec_new;

	if (n_coords > (sizeof(value_local) / sizeof(value_local[0]))) {
		value = malloc(n_coords * sizeof *value);
		if (value == NULL)
			return -1;
	}
	else
		value = value_local;

	if (allow == 0)
		allow = RND_UNIT_ALLOW_ALL_SANE;

	i = printf_spec_->used + 64;
	if (i > sizeof(printf_spec_new_local))
		printf_spec_new = malloc(i);
	else
		printf_spec_new = printf_spec_new_local;

	/* Check our freedom in choosing units */
	if (((allow & RND_UNIT_ALLOW_IMPERIAL) == 0) && (allow < RND_UNIT_ALLOW_dyn))
		family = RND_UNIT_METRIC;
	else if (((allow & RND_UNIT_ALLOW_METRIC) == 0) && (allow < RND_UNIT_ALLOW_dyn))
		family = RND_UNIT_IMPERIAL;
	else {
		int met_votes = 0, imp_votes = 0;

		for (i = 0; i < n_coords; ++i)
			if (min_sig_figs(RND_COORD_TO_MIL(coord[i])) < min_sig_figs(RND_COORD_TO_MM(coord[i])))
				++imp_votes;
			else
				++met_votes;

		if (imp_votes > met_votes)
			family = RND_UNIT_IMPERIAL;
		else
			family = RND_UNIT_METRIC;
	}

	/* Set base unit */
	for (i = 0; i < n_coords; ++i) {
		switch (family) {
			case RND_UNIT_METRIC:
				value[i] = RND_COORD_TO_MM(coord[i]);
				break;
			case RND_UNIT_IMPERIAL:
				value[i] = RND_COORD_TO_MIL(coord[i]);
				break;
			default:
				value[i] = 0;
		}
	}

	/* Determine scale factor -- find smallest unit that brings
	 * the whole group above unity */
	for (n = 0; n < rnd_get_n_units(); ++n) {
		const rnd_unit_t *u = rnd_units.array[n];
		if (u->is_alias)
			continue;
		if (((u->allow & allow) != 0) && (u->family == family)) {
			int n_above_one = 0;

			for (i = 0; i < n_coords; ++i)
				if (fabs(value[i] * u->scale_factor) > 1)
					++n_above_one;
			if (n_above_one == n_coords)
				break;
		}
	}
	/* If nothing worked, wind back to the smallest allowable unit */
	if (n == rnd_get_n_units()) {
		const rnd_unit_t *u;
		do {
			--n;
			if (n < 0)
				break;
			u = rnd_units.array[n];
		} while (u->is_alias || ((u->allow & allow) == 0) || (u->family != family));
	}

	if (n < 0)
		return -1;

	/* Apply scale factor */
	{
		const rnd_unit_t *u = rnd_units.array[n];
		suffix = u->suffix;
		for(i = 0; i < n_coords; ++i)
			value[i] = value[i] * u->scale_factor;

		make_printf_spec(printf_spec_new, printf_spec, u->default_prec, &trunc0);
	}


	/* Actually sprintf the values in place
	 *  (+ 2 skips the ", " for first value) */
	if (n_coords > 1)
		if (gds_append(dest,  '(') != 0)
			goto err;

	sprintf_lc_safe((suffix_type == RND_UNIT_FILE_MODE), filemode_buff, printf_spec_new + 2, value[0]);
	if (gds_append_str(dest, filemode_buff) != 0)
		goto err;

	for (i = 1; i < n_coords; ++i) {
		sprintf_lc_safe((suffix_type == RND_UNIT_FILE_MODE), filemode_buff, printf_spec_new, value[i]);
		if (gds_append_str(dest, filemode_buff) != 0)
			goto err;
	}
	if (n_coords > 1)
		if (gds_append(dest, ')') != 0) goto err;


	/* Append suffix */
	if (value[0] != 0 || n_coords > 1)
		if (append_suffix(dest, suffix_type, suffix) != 0)
			goto err;

	retval = 0;
err:;
	if (printf_spec_new != printf_spec_new_local)
		free(printf_spec_new);

	if (value != value_local)
		free(value);
	return retval;
}

typedef struct {
	int           score_factor;

	rnd_unit_allow_t  base;
	double        down_limit;
	rnd_unit_allow_t  down;
	double        up_limit;
	rnd_unit_allow_t  up;

	/* persistent, calculated once */
	const rnd_unit_t    *base_unit, *down_unit, *up_unit;
} human_coord_t;

/* Conversion preference table */
static human_coord_t human_coord[] = {
	{2, RND_UNIT_ALLOW_MM,  0.001, RND_UNIT_ALLOW_UM, 1000.0, RND_UNIT_ALLOW_M     ,NULL,NULL,NULL},
	{1, RND_UNIT_ALLOW_MIL, 0,     0,        1000.0, RND_UNIT_ALLOW_IN    ,NULL,NULL,NULL}
};
#define NUM_HUMAN_COORD (sizeof(human_coord) / sizeof(human_coord[0]))

RND_INLINE int try_human_coord(rnd_coord_t coord, const rnd_unit_t *unit, double down_limit, double up_limit, int score_factor, double *value, unsigned int *best, const char **suffix)
{
	double v, frac, save;
	long int digs, zeros, nines;
	unsigned int score;
	int round_up = 0;

	/* convert the value to the proposed unit */
	if (unit->family == RND_UNIT_METRIC)
		v = RND_COORD_TO_MM(coord);
	else
		v = RND_COORD_TO_MIL(coord);
	save = v = v * unit->scale_factor;
	if (v < 0) v = -v;

	/* Check if neighbour units are better */
	if ((down_limit > 0) && (v < down_limit))
		return -1;
	if ((up_limit > 0) && (v > up_limit))
		return +1;

	/* count trailing zeros after the decimal point up to 8 digits */
	frac = v - floor(v);
	digs = frac * 100000000.0;

	if (digs != 0) {
		int in_ze = 1, in_ni = 1;

		/* count continous zeros and nines from the back in parallel */
		for(zeros = nines = 0; (digs > 0) ; digs /= 10)
		{
			int d = (digs % 10), ze = (d == 0), ni = (d == 9);
			if (ze && in_ze) zeros++;
			else in_ze = 0;
			if (ni && in_ni) nines++;
			else in_ni = 0;
		}
	}
	else {
		zeros = 8;
		nines = 0;
	}

/*	rnd_trace("^^ zeros=%d nines=%d\n", zeros, nines);*/

	/* ending to more nines than zeros */
	if (nines > zeros) {
		/* go for the nines and get the converter to round up */
		zeros = nines;
		round_up = 1;
	}

	/* score is higher for more zeroes or nines */
	score = score_factor + 8 * zeros;

/*	printf("try: %s '%.8f' -> %.8f %d score=%d\n", unit->suffix, v, frac, zeros, score);*/

	/* update the best score */
	if (score > *best) {
/*		rnd_trace("save= %.16f %d\n", save, round_up);*/
		if (round_up) {
			/* ends in 999999, tweak it just a little bit so it's rounded up */
			save += 0.0000000001;
/*			rnd_trace("save= %.16f %d\n", save, round_up);*/
		}
		*value = save;
		*best = score;
		*suffix = unit->suffix;
	}

	return 0;
}

/* Same as CoordsToString but take only one coord and print it in human readable format */
static int CoordsToHumanString(gds_t *dest, rnd_coord_t coord, const gds_t *printf_spec_, rnd_unit_suffix_t suffix_type)
{
	char filemode_buff[128]; /* G_ASCII_DTOSTR_BUF_SIZE */
	char printf_spec_new_local[256];
	char *printf_spec_new;
	int i, retval = -1, trunc0, from_app;
	const char *printf_spec = printf_spec_->array;
	const char *suffix;
	double value;
	unsigned int best_score = 0;

	i = printf_spec_->used + 64;
	if (i > sizeof(printf_spec_new_local))
		printf_spec_new = malloc(i);
	else
		printf_spec_new = printf_spec_new_local;

	/* cache unit lookup */
	if (human_coord[0].base_unit == NULL) {
		for(i = 0; i < NUM_HUMAN_COORD; i++) {
			human_coord[i].base_unit = rnd_get_unit_struct_by_allow(human_coord[i].base);
			human_coord[i].down_unit = rnd_get_unit_struct_by_allow(human_coord[i].down);
			human_coord[i].up_unit = rnd_get_unit_struct_by_allow(human_coord[i].up);
		}
	}

	if (rnd_app.human_coord != NULL)
		from_app = rnd_app.human_coord(coord, &value, &suffix);
	else
		from_app = 0;

	if (!from_app) {
		for(i = 0; i < NUM_HUMAN_COORD; i++) {
			int res;
			/* try the base units first */
			res = try_human_coord(coord, human_coord[i].base_unit, human_coord[i].down_limit, human_coord[i].up_limit, human_coord[i].score_factor,&value, &best_score, &suffix);
			if (res < 0)
				try_human_coord(coord, human_coord[i].down_unit, 0, 0, human_coord[i].score_factor, &value, &best_score, &suffix);
			else if (res > 0)
				try_human_coord(coord, human_coord[i].up_unit, 0, 0, human_coord[i].score_factor,&value, &best_score, &suffix);
		}
	}

	make_printf_spec(printf_spec_new, printf_spec, 8, &trunc0);
	sprintf_lc_safe((suffix_type == RND_UNIT_FILE_MODE), filemode_buff, printf_spec_new + 2, value);
	if (gds_append_str(dest, filemode_buff) != 0)
		goto err;

	if (value != 0) {
		if (suffix_type == RND_UNIT_NO_SUFFIX)
			suffix_type = RND_UNIT_SUFFIX;
		if (append_suffix(dest, suffix_type, suffix) != 0)
			goto err;
	}

	err:;
	if (printf_spec_new != printf_spec_new_local)
		free(printf_spec_new);
	return retval;
}

int QstringToString(gds_t *dest, const char *qstr, char q, char esc, const char *needq)
{
	const char *s;

	if ((qstr == NULL) || (*qstr == '\0')) {
		gds_append(dest, q);
		gds_append(dest, q);
		return 0;
	}

	/* check if quoting is needed */
	if (strpbrk(qstr, needq) == NULL) {
		gds_append_str(dest, qstr);
		return 0;
	}

	/* wrap in quotes and protect escape and quote chars */
	gds_append(dest, q);
	for(s = qstr; *s != '\0'; s++) {
		if ((*s == esc) || (*s == q))
			gds_append(dest, esc);
		gds_append(dest, *s);
	}
	gds_append(dest, q);
	return 0;
}

static void print_fgw_arg(gds_t *string, fgw_arg_t a, rnd_unit_allow_t mask)
{
	if ((a.type & FGW_STR) == FGW_STR) {
		gds_append_str(string, a.val.str);
		return;
	}

	if (a.type == FGW_COORD) {
		rnd_append_printf(string, "%m+%$ms", mask, fgw_coord(&a));
		return;
	}

	switch(a.type) {
		case FGW_CHAR:
		case FGW_UCHAR:
		case FGW_SCHAR:
			gds_append(string, a.val.nat_char);
			return;
		case FGW_SHORT:
			rnd_append_printf(string, "%d", a.val.nat_short);
			return;
		case FGW_USHORT:
			rnd_append_printf(string, "%u", a.val.nat_ushort);
			return;
		case FGW_INT:
			rnd_append_printf(string, "%d", a.val.nat_int);
			return;
		case FGW_UINT:
			rnd_append_printf(string, "%u", a.val.nat_uint);
			return;
		case FGW_LONG:
			rnd_append_printf(string, "%ld", a.val.nat_long);
			return;
		case FGW_ULONG:
			rnd_append_printf(string, "%lu", a.val.nat_ulong);
			return;
		case FGW_SIZE_T:
			rnd_append_printf(string, "%ld", (long)a.val.nat_size_t);
			return;
		case FGW_FLOAT:
			rnd_append_printf(string, "%f", a.val.nat_float);
			return;
		case FGW_DOUBLE:
			rnd_append_printf(string, "%f", a.val.nat_double);
			return;
		default: break;
	}
	rnd_append_printf(string, "<invalid fgw_arg_t %d>", a.type);
}

rnd_aft_ret_t (*rnd_printf_app_format)(gds_t *string, gds_t *spec, const char **fmt, rnd_unit_allow_t mask, rnd_unit_suffix_t suffix, rnd_aft_arg_t *arg, rnd_aft_arg_t *cookie) = NULL;

/* Main low level pcb-printf function
 * This is a printf wrapper that accepts new format specifiers to
 * output pcb coords as various units. See the comment at the top
 * of rnd_printf.h for full details.
 *
 * [in] string Append anything new at the end of this dynamic string (must be initialized before the call)
 * [in] fmt    Format specifier
 * [in] args   Arguments to specifier
 *
 * return 0 on success */
int rnd_safe_append_vprintf(gds_t *string, rnd_safe_printf_t safe, const char *fmt, va_list args)
{
	char spec_buff[128]; /* large enough for any reasonable format spec */
	gds_t spec;
	const char *qstr, *needsq;
	char tmp[128]; /* large enough for rendering a long long int */
	int tmplen, retval = -1, slot_recursion = 0, mq_has_spec;
	char *dot, *free_fmt = NULL;
	rnd_unit_allow_t mask = RND_UNIT_ALLOW_ALL_SANE;
	unsigned long maxfmts;

	/* set up static allocation for spec */
	spec.array = spec_buff;
	spec.no_realloc = 1;
	spec.used = 0;
	spec.alloced = sizeof(spec_buff);

	if ((safe & RND_SAFEPRINT_arg_max) > 0)
		maxfmts = (safe & RND_SAFEPRINT_arg_max);
	else
		maxfmts = 1UL<<31;

	new_fmt:;
	while (*fmt) {
		rnd_unit_suffix_t suffix = RND_UNIT_NO_SUFFIX;

		if (*fmt == '%') {
			const char *ext_unit = "";
			rnd_coord_t value[10];
			int count, i;

			gds_truncate(&spec, 0);
			mq_has_spec = 0;

			/* Get printf sub-specifiers */
			if (gds_append(&spec, *fmt++) != 0) goto err;
			while (isdigit(*fmt) || *fmt == '.' || *fmt == ' ' || *fmt == '*'
						 || *fmt == '#' || *fmt == 'l' || *fmt == 'L' || *fmt == 'h'
						 || *fmt == '+' || *fmt == '-' || *fmt == '[' || *fmt == '{') {
				if (*fmt == '*') {
					char itmp[32];
					int ilen;
					ilen = sprintf(itmp, "%d", va_arg(args, int));
					if (gds_append_len(&spec, itmp, ilen) != 0) goto err;
					fmt++;
				}
				else
					if (gds_append(&spec, *fmt++) != 0) goto err;
			}

			/* check if specifier is %[] */
			if ((gds_len(&spec) > 2) && (spec.array[1] == '[')) {
				int slot = atoi(spec.array+2);
				gds_t new_spec;
				if ((slot < 0) || (slot > RND_PRINTF_SLOT_max)) {
					fprintf(stderr, "Internal error: invalid printf slot addressed: '%s'\n", spec.array);
					abort();
				}
				slot_recursion++;
				if (slot_recursion > 16) {
					fprintf(stderr, "Internal error: printf slot recursion too deep on addressed: '%s'\n", spec.array);
					abort();
				}
				if (rnd_printf_slot[slot] == NULL) {
					fprintf(stderr, "Internal error: printf empty slot reference: '%s'\n", spec.array);
					abort();
				}
				gds_init(&new_spec);
				gds_append_str(&new_spec, rnd_printf_slot[slot]);
				if (*fmt == ']')
					fmt++;
				gds_append_str(&new_spec, fmt);
				if (free_fmt != NULL)
					free(free_fmt);
				fmt = free_fmt = new_spec.array;
				memset(&new_spec, 0, sizeof(new_spec));
				gds_truncate(&spec, 0);
				goto new_fmt;
			}
			if ((spec.array[0] == '%') && (spec.array[1] == '{')) {
				/* {} specifier for %mq */
				const char *end;
				gds_truncate(&spec, 0);
				for(end = fmt; *end != '\0'; end++) {
					if ((end[0] != '\\') && (end[1] == '}')) {
						end++;
						break;
					}
				}
				if (*end == '\0')
					goto err;
				gds_append_len(&spec, fmt, end-fmt);
				fmt = end+1;
				mq_has_spec = 1;
			}
			else
				slot_recursion = 0;

			/* Get our sub-specifiers */
			if (*fmt == '#') {
				mask = RND_UNIT_ALLOW_CMIL; /* This must be pcb's base unit */
				fmt++;
			}
			if (*fmt == '$') {
				suffix = RND_UNIT_SUFFIX;
				fmt++;
				if (*fmt == '$') {
					fmt++;
					suffix = RND_UNIT_FILE_MODE;
				}
			}

			if (maxfmts == 0)
				goto err;
			maxfmts--;

			/* Tack full specifier onto specifier */
			if ((*fmt != 'm') && (*fmt != 'r'))
				if (gds_append(&spec, *fmt) != 0)
					goto err;


			switch (*fmt) {

				/*** Standard printf specs ***/
				case 'o':
				case 'i':
				case 'd':
				case 'u':
				case 'x':
				case 'X':
					if (safe & RND_SAFEPRINT_COORD_ONLY)
						goto err;
					if (spec.array[1] == 'l')
						tmplen = sprintf(tmp, spec.array, va_arg(args, long));
					else
						tmplen = sprintf(tmp, spec.array, va_arg(args, int));
					if (gds_append_len(string, tmp, tmplen) != 0) goto err;
					break;
				case 'e':
				case 'E':
				case 'f':
				case 'g':
				case 'G':
					if (safe & RND_SAFEPRINT_COORD_ONLY)
						goto err;
					if (strchr(spec.array, '*')) {
						int prec = va_arg(args, int);
						tmplen = sprintf(tmp, spec.array, va_arg(args, double), prec);
					}
					else
						tmplen = sprintf(tmp, spec.array, va_arg(args, double));
					if (gds_append_len(string, tmp, tmplen) != 0) goto err;
					break;
				case 'c':
					if (safe & RND_SAFEPRINT_COORD_ONLY)
						goto err;
					if (spec.array[1] == 'l' && sizeof(int) <= sizeof(wchar_t))
						tmplen = sprintf(tmp, spec.array, va_arg(args, wchar_t));
					else
						tmplen = sprintf(tmp, spec.array, va_arg(args, int));
					if (gds_append_len(string, tmp, tmplen) != 0) goto err;
					break;
				case 's':
					if (safe & RND_SAFEPRINT_COORD_ONLY)
						goto err;
					if (spec.array[0] == 'l') {
						fprintf(stderr, "Internal error: appending %%ls is not supported\n");
						abort();
					}
					else {
						const char *s = va_arg(args, const char *);
						if (s == NULL) s = "(null)";
						if (gds_append_str(string, s) != 0) goto err;
					}
					break;
				case 'n':
					if (safe & RND_SAFEPRINT_COORD_ONLY)
						goto err;
					/* Depending on gcc settings, this will probably break with
					 *  some silly "can't put %n in writable data space" message */
					tmplen = sprintf(tmp, spec.array, va_arg(args, int *));
					if (gds_append_len(string, tmp, tmplen) != 0) goto err;
					break;
				case 'p':
					if (safe & RND_SAFEPRINT_COORD_ONLY)
						goto err;
					tmplen = sprintf(tmp, spec.array, va_arg(args, void *));
					if (gds_append_len(string, tmp, tmplen) != 0) goto err;
					break;
				case '%':
					if (gds_append(string, '%') != 0) goto err;
					break;

				/*** librnd (ex-pcb-rnd) custom spec ***/
				case 'm':
					++fmt;
					if (*fmt == '*') {
						if (safe & RND_SAFEPRINT_COORD_ONLY)
							goto err;
						ext_unit = va_arg(args, const char *);
					}
					if (*fmt != '+' && *fmt != 'a' && *fmt != 'A' && *fmt != 'f' && *fmt != 'q' && *fmt != 'w')
						value[0] = va_arg(args, rnd_coord_t);
					count = 1;
					switch (*fmt) {
						case 'q':
							if (safe & RND_SAFEPRINT_COORD_ONLY)
								goto err;
							qstr = va_arg(args, const char *);
							if (mq_has_spec)
								needsq = spec.array;
							else
								needsq = " \"\\";
							if (QstringToString(string, qstr, '"', '\\', needsq) != 0) goto err;
							break;
						case 'I':
							if (CoordsToString(string, value, 1, &spec, RND_UNIT_ALLOW_NM, RND_UNIT_NO_SUFFIX) != 0) goto err;
							break;
						case 's':
							if (CoordsToString(string, value, 1, &spec, RND_UNIT_ALLOW_MM | RND_UNIT_ALLOW_MIL, suffix) != 0) goto err;
							break;
						case 'S':
							if (CoordsToString(string, value, 1, &spec, mask & RND_UNIT_ALLOW_NATURAL, suffix) != 0) goto err;
							break;
						case 'N':
							if (CoordsToString(string, value, 1, &spec, mask, suffix) != 0) goto err;
							break;
						case 'H':
							if ((safe & RND_SAFEPRINT_COORD_ONLY) && (value[0] > 1))
								goto err;
							if (CoordsToHumanString(string, value[0], &spec, suffix) != 0) goto err;
							break;
						case 'M':
							if (CoordsToString(string, value, 1, &spec, mask & RND_UNIT_ALLOW_METRIC, suffix) != 0) goto err;
							break;
						case 'L':
							if (CoordsToString(string, value, 1, &spec, mask & RND_UNIT_ALLOW_IMPERIAL, suffix) != 0) goto err;
							break;
						case 'k':
							if (CoordsToString(string, value, 1, &spec, RND_UNIT_ALLOW_DMIL, suffix) != 0) goto err;
							break;
						case 'r':
							if (CoordsToString(string, value, 1, &spec, RND_UNIT_ALLOW_READABLE, RND_UNIT_NO_SUFFIX) != 0) goto err;
							break;
							/* All these fallthroughs are deliberate */
						case '9':
							if (safe & RND_SAFEPRINT_COORD_ONLY)
								goto err;
							value[count++] = va_arg(args, rnd_coord_t);
						case '8':
							if (safe & RND_SAFEPRINT_COORD_ONLY)
								goto err;
							value[count++] = va_arg(args, rnd_coord_t);
						case '7':
							if (safe & RND_SAFEPRINT_COORD_ONLY)
								goto err;
							value[count++] = va_arg(args, rnd_coord_t);
						case '6':
							if (safe & RND_SAFEPRINT_COORD_ONLY)
								goto err;
							value[count++] = va_arg(args, rnd_coord_t);
						case '5':
							if (safe & RND_SAFEPRINT_COORD_ONLY)
								goto err;
							value[count++] = va_arg(args, rnd_coord_t);
						case '4':
							if (safe & RND_SAFEPRINT_COORD_ONLY)
								goto err;
							value[count++] = va_arg(args, rnd_coord_t);
						case '3':
							if (safe & RND_SAFEPRINT_COORD_ONLY)
								goto err;
							value[count++] = va_arg(args, rnd_coord_t);
						case '2':
						case 'D':
							if (safe & RND_SAFEPRINT_COORD_ONLY)
								goto err;
							value[count++] = va_arg(args, rnd_coord_t);
							if (CoordsToString(string, value, count, &spec, mask & RND_UNIT_ALLOW_ALL_SANE, suffix) != 0) goto err;
							break;
						case 'd':
							if (safe & RND_SAFEPRINT_COORD_ONLY)
								goto err;
							value[1] = va_arg(args, rnd_coord_t);
							if (CoordsToString(string, value, 2, &spec, RND_UNIT_ALLOW_MM | RND_UNIT_ALLOW_MIL, suffix) != 0) goto err;
							break;
						case '*':
							{
								int found = 0;
								for (i = 0; i < rnd_get_n_units(); ++i) {
									const rnd_unit_t *u = rnd_units.array[i];
									if (strcmp(ext_unit, u->suffix) == 0) {
										if (CoordsToString(string, value, 1, &spec, u->allow, suffix) != 0) goto err;
										found = 1;
										break;
									}
								}
								if (!found)
									if (CoordsToString(string, value, 1, &spec, mask & RND_UNIT_ALLOW_ALL_SANE, suffix) != 0) goto err;
							}
							break;
						case 'a':
							if (safe & RND_SAFEPRINT_COORD_ONLY)
								goto err;
							if (gds_append_len(&spec, ".06f", 4) != 0) goto err;
							if (suffix == RND_UNIT_SUFFIX)
								if (gds_append_len(&spec, " deg", 4) != 0) goto err;
							tmplen = sprintf(tmp, spec.array, (double) va_arg(args, rnd_angle_t));
							if (gds_append_len(string, tmp, tmplen) != 0) goto err;
							break;
						case 'A':
							if (safe & RND_SAFEPRINT_COORD_ONLY)
								goto err;
							if (gds_append_len(&spec, ".0f", 3) != 0) goto err;
							/* if (suffix == RND_UNIT_SUFFIX)
								if (gds_append_len(&spec, " deg", 4) != 0) goto err;*/
							tmplen = sprintf(tmp, spec.array, 10*((double) va_arg(args, rnd_angle_t))); /* kicad legacy needs decidegrees */
							if (gds_append_len(string, tmp, tmplen) != 0) goto err;
							break;
						case 'f':
							if (safe & RND_SAFEPRINT_COORD_ONLY)
								goto err;
							gds_append_len(&spec, "f", 1);
							tmplen = sprintf(tmp, spec.array, va_arg(args, double));
							dot = strchr(spec.array, '.');
							if ((dot != NULL) && (dot[1] == '0'))
								tmplen = do_trunc0(tmp);
							if (gds_append_len(string, tmp, tmplen) != 0) goto err;
							break;
						case 'w':
							if (safe & RND_SAFEPRINT_COORD_ONLY)
								goto err;
							print_fgw_arg(string, va_arg(args, fgw_arg_t), mask);
							break;
						case '+':
							if (safe & RND_SAFEPRINT_COORD_ONLY)
								goto err;
							mask = va_arg(args, rnd_unit_allow_t);
							break;
						default:
							{
								int found = 0;
								for (i = 0; i < rnd_get_n_units(); ++i) {
									const rnd_unit_t *u = rnd_units.array[i];
									if (*fmt == u->printf_code) {
										if (CoordsToString(string, value, 1, &spec, u->allow, suffix) != 0) goto err;
										found = 1;
										break;
									}
								}
								if (!found)
									if (CoordsToString(string, value, 1, &spec, RND_UNIT_ALLOW_ALL_SANE, suffix) != 0) goto err;
							}
							break;
					}
					break;

				/*** ringdove app-specific custom spec ***/
				case 'r':
					{
						rnd_aft_ret_t r;
						rnd_aft_arg_t arg, cookie;

						if (rnd_printf_app_format == NULL)
							goto err;
						++fmt;
						memset(&cookie, 0, sizeof(cookie));
						r = rnd_printf_app_format(string, &spec, &fmt, mask, suffix, NULL, &cookie);
						switch(r) {
							case RND_AFT_INT:       arg.i = va_arg(args, int); break;
							case RND_AFT_LONG:      arg.l = va_arg(args, long); break;
							case RND_AFT_DOUBLE:    arg.d = va_arg(args, double); break;
							case RND_AFT_PTR:       arg.p = va_arg(args, void *); break;
							case RND_AFT_error:
							default:
								goto err;
						}
						r = rnd_printf_app_format(string, &spec, &fmt, mask, suffix, &arg, &cookie);
						if (r == RND_AFT_error)
							goto err;
					}
					break;
			}
		}
		else
			if (gds_append(string, *fmt) != 0) goto err;
		++fmt;
	}

	retval = 0;
err:;
	/* gds_uninit(&spec); - not needed with static allocation */
	if (free_fmt != NULL)
		free(free_fmt);

	return retval;
}

int rnd_sprintf(char *string, const char *fmt, ...)
{
	gds_t str;
	va_list args;

	memset(&str, 0, sizeof(str)); /* can't use gds_init, it'd allocate str.array */
	va_start(args, fmt);

	/* pretend the string is already allocated to something huge; this doesn't
	   make the code less safe but saves a copy */
	str.array = string;
	str.alloced = 1<<31;
	str.no_realloc = 1;

	rnd_safe_append_vprintf(&str, 0, fmt, args);

	va_end(args);
	return str.used;
}

int rnd_snprintf(char *string, size_t len, const char *fmt, ...)
{
	gds_t str;
	va_list args;

	memset(&str, 0, sizeof(str)); /* can't use gds_init, it'd allocate str.array */
	va_start(args, fmt);

	str.array = string;
	str.alloced = len;
	str.no_realloc = 1;

	rnd_safe_append_vprintf(&str, 0, fmt, args);

	va_end(args);
	return str.used;
}

int rnd_safe_snprintf(char *string, size_t len, rnd_safe_printf_t safe, const char *fmt, ...)
{
	gds_t str;
	va_list args;
	int res;

	memset(&str, 0, sizeof(str)); /* can't use gds_init, it'd allocate str.array */
	va_start(args, fmt);

	str.array = string;
	str.alloced = len;
	str.no_realloc = 1;
	if (len > 0)
		*string = '\0'; /* make sure the string is empty on error of the low level call didn't print anything */

	res = rnd_safe_append_vprintf(&str, safe, fmt, args);

	va_end(args);
	if (res < 0)
		return res;
	return str.used;
}


int rnd_vsnprintf(char *string, size_t len, const char *fmt, va_list args)
{
	gds_t str;

	memset(&str, 0, sizeof(str)); /* can't use gds_init, it'd allocate str.array */

	str.array = string;
	str.alloced = len;
	str.no_realloc = 1;

	rnd_safe_append_vprintf(&str, 0, fmt, args);

	return str.used;
}

int rnd_fprintf(FILE * fh, const char *fmt, ...)
{
	int rv;
	va_list args;

	va_start(args, fmt);
	rv = rnd_vfprintf(fh, fmt, args);
	va_end(args);

	return rv;
}

int rnd_vfprintf(FILE * fh, const char *fmt, va_list args)
{
	int rv;
	gds_t str;
	gds_init(&str);

	if (fh == NULL)
		rv = -1;
	else {
		rnd_safe_append_vprintf(&str, 0, fmt, args);
		rv = fprintf(fh, "%s", str.array);
	}

	gds_uninit(&str);
	return rv;
}

int rnd_printf(const char *fmt, ...)
{
	int rv;
	gds_t str;
	va_list args;

	gds_init(&str);
	va_start(args, fmt);

	rnd_safe_append_vprintf(&str, 0, fmt, args);
	rv = printf("%s", str.array);

	va_end(args);
	gds_uninit(&str);
	return rv;
}

char *rnd_strdup_printf(const char *fmt, ...)
{
	gds_t str;
	va_list args;

	gds_init(&str);

	va_start(args, fmt);
	rnd_safe_append_vprintf(&str, 0, fmt, args);
	va_end(args);

	return str.array; /* no other allocation has been made */
}

char *rnd_strdup_vprintf(const char *fmt, va_list args)
{
	gds_t str;
	gds_init(&str);

	rnd_safe_append_vprintf(&str, 0, fmt, args);

	return str.array; /* no other allocation has been made */
}


/* Wrapper for rnd_safe_append_vprintf that appends to a string using vararg API */
int rnd_append_printf(gds_t *str, const char *fmt, ...)
{
	int retval;

	va_list args;
	va_start(args, fmt);
	retval = rnd_safe_append_vprintf(str, 0, fmt, args);
	va_end(args);

	return retval;
}
