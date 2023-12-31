/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2011 Andrew Poelstra
 *  Copyright (C) 2016,2019 Tibor 'Igor2' Palinkas
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

/* This file defines a wrapper around sprintf, that
 *  defines new specifiers that take rnd_coord_t objects
 *  as input.
 *
 * There is a fair bit of nasty (repetitious) code in
 *  here, but I feel the gain in clarity for output
 *  code elsewhere in the project will make it worth
 *  it.
 *
 * The new coord/angle specifiers are:
 *   %mI    output a raw internal coordinate without any suffix
 *   %mm    output a measure in mm
 *   %mu    output a measure in um
 *   %mM    output a measure in scaled (mm/um) metric
 *   %ml    output a measure in mil
 *   %mL    output a measure in scaled (mil/in) imperial
 *   %ms    output a measure in most natural mm/mil units
 *   %mS    output a measure in most natural scaled units
 *   %mN    output a measure in requested natural units (enforces the choice of %m+ without further modifications)
 *   %mH    output a measure in most human readable scaled units
 *   %md    output a pair of measures in most natural mm/mil units
 *   %mD    output a pair of measures in most natural scaled units
 *   %m3    output 3 measures in most natural scaled units
 *     ...
 *   %m9    output 9 measures in most natural scaled units
 *   %m*    output a measure with unit given as an additional
 *          const char* parameter
 *   %m+    accepts an e_allow parameter that masks all subsequent
 *          "natural" (N/S/D/3/.../9) specifiers to only use certain
 *          units
 *   %ma    output an angle in degrees (expects degrees)
 *   %mf    output an a double (same as %f, expect it understands the .0n modifier)
 *   %mw    output an fgw_arg_t value
 *
 * Exotic formats, DO NOT USE:
 *   %mr    output a measure in a unit readable by pcb-rnd's io_pcb parse_l.l
 *          (outputs in centimil without units - compatibility with gEDA/pcb)
 *   %mk    output a measure in decimil (kicad legacy format)
 *   %mA    output an angle in decidegrees (degrees x 10) for kicad legacy
 *
 * These accept the usual printf modifiers for %f, as well as
 *     $    output a unit suffix after the measure (with space between measure and unit)
 *     $$   output a unit suffix after the measure (without space)
 *     .n   number of digits after the decimal point (the usual %f modifier)
 *     .0n  where n is a digit; same as %.n, but truncates trailing zeros
 *     [n]  use stored format n
 *     #    prevents all scaling for %mS/D/1/.../9 (this should
 *          ONLY be used for debug code since its output exposes
 *          librnd internal units).
 *
 * The usual printf(3) precision and length modifiers should work with
 * any format specifier that outputs coords, e.g. %.3mm will output in
 * mm up to 3 decimal digits after the decimal point.
 *
 * The new string specifiers are:
 *   %mq    output a quoted string (""); quote if contains quote. Use
 *          backslash to protect quotes within the quoted string. Modifiers:
 *           {chars} start quoting if any of these appear; "\}" in {chars} is a plain '}'
 *
 *
 * KNOWN ISSUES:
 *   No support for %zu size_t printf spec
 */

#ifndef RND_PRINTF_H
#define RND_PRINTF_H

#include <stdio.h>
#include <stdarg.h>
#include <genvector/gds_char.h>
#include <librnd/core/unit.h>

typedef enum {                    /* bitmask for printf hardening */
	RND_SAFEPRINT_arg_max = 1023,   /* for internal use */
	RND_SAFEPRINT_COORD_ONLY = 1024 /* print only coords (%m); anything else will result in error, returning -1  */
} rnd_safe_printf_t;

int rnd_fprintf(FILE * f, const char *fmt, ...);
int rnd_vfprintf(FILE * f, const char *fmt, va_list args);
int rnd_sprintf(char *string, const char *fmt, ...);
int rnd_snprintf(char *string, size_t len, const char *fmt, ...);
int rnd_safe_snprintf(char *string, size_t len, rnd_safe_printf_t safe, const char *fmt, ...);
int rnd_vsnprintf(char *string, size_t len, const char *fmt, va_list args);
int rnd_printf(const char *fmt, ...);
char *rnd_strdup_printf(const char *fmt, ...);
char *rnd_strdup_vprintf(const char *fmt, va_list args);

int rnd_append_printf(gds_t *str, const char *fmt, ...);

/* Low level call that does the job */
int rnd_safe_append_vprintf(gds_t *string, rnd_safe_printf_t safe, const char *fmt, va_list args);

/* Predefined slots (macros): %[n] will use the nth string from this list.
   The first 8 are user-definable. */
typedef enum {
	RND_PRINTF_SLOT_USER0,
	RND_PRINTF_SLOT_USER1,
	RND_PRINTF_SLOT_USER2,
	RND_PRINTF_SLOT_USER3,
	RND_PRINTF_SLOT_USER4,
	RND_PRINTF_SLOT_USER5,
	RND_PRINTF_SLOT_USER6,
	RND_PRINTF_SLOT_USER7,
	RND_PRINTF_SLOT_FF_ORIG_COORD, /* %[8] pcb-rnd: original .pcb file format coord prints */
	RND_PRINTF_SLOT_FF_SAFE_COORD, /* %[9] pcb-rnd: safe .pcb file format coord print that doesn't lose precision */
	RND_PRINTF_SLOT_max
} rnd_printf_slot_idx_t;
extern const char *rnd_printf_slot[RND_PRINTF_SLOT_max];

/* strdup and return a string built from a template, controlled by flags:

   RND_SUBST_HOME: replace leading ~ with the user's home directory

   RND_SUBST_PERCENT: attempt to replace printf-like formatting
   directives (e.g. %P) using an user provided callback function. The callback
   function needs to recognize the directive at **input (pointing to the byte
   after the %) and append the substitution to s and increase *input to point
   beyond the format directive. The callback returns 0 on success or -1
   on unknown directive (which will be copied verbatim). %% will always
   be translated into a single %, without calling cb.

   RND_SUBST_CONF: replace $(conf) automatically (no callback)

   Implemented in paths.c because it depends on conf_core.h and error.h .
*/
typedef enum {
	RND_SUBST_HOME = 1,
	RND_SUBST_PERCENT = 2,
	RND_SUBST_CONF = 4,
	RND_SUBST_BACKSLASH = 8, /* substitute \ sequences as printf(3) does */

	RND_SUBST_ALL = 0x7f, /* substitute all, but do not enable quiet */

	RND_SUBST_QUIET = 0x80
} rnd_strdup_subst_t;

/* Substitute template using cb, leaving extra room at the end and append the
   result to s. Returns 0 on success. */
int rnd_subst_append(gds_t *s, const char *template, int (*cb)(void *ctx, gds_t *s, const char **input), void *ctx, rnd_strdup_subst_t flags, size_t extra_room);

/* Same as rnd_subst_append(), but returns a dynamic allocated string
   (the caller needs to free() after use) or NULL on error. */
char *rnd_strdup_subst(const char *template, int (*cb)(void *ctx, gds_t *s, const char **input), void *ctx, rnd_strdup_subst_t flags);

/* Optional: ringdove apps may provide this format print callback to render
   %r formats. Output should be appended to string. Standard specifier
   sequence (the part between %r and the format letter) is in spec. The
   format letter is in fmt; fmt should be incremented by the callee depending
   on how many format letters are consumed; at the end fmt should point to the
   last format char processed. Mask and suffix are information
   extracted from earlier format portions about how units should be printed.
   The function is called twice per custom format:
    - first with arg==NULL, it should return what type ofargument shall be
      retrieved from varargs (shouldn't print anything in string)
    - second with arg!=NULL, vararg copied there (should print to string)
   The function may use cookie to store data between the invocations; cookie
   is allocated by the caller and is reset to all-0 before the first call of
   the pair and discarded after the second call. */
typedef enum { RND_AFT_error, RND_AFT_INT, RND_AFT_LONG, RND_AFT_DOUBLE, RND_AFT_PTR } rnd_aft_ret_t;
typedef union { int i; long l; double d; void *p; } rnd_aft_arg_t;
extern rnd_aft_ret_t (*rnd_printf_app_format)(gds_t *string, gds_t *spec, const char **fmt, rnd_unit_allow_t mask, rnd_unit_suffix_t suffix, rnd_aft_arg_t *arg, rnd_aft_arg_t *cookie);


#endif
