/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016, 2017, 2024 Tibor 'Igor2' Palinkas
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

#ifndef LIBRND_CORE_PATHS_H
#define LIBRND_CORE_PATHS_H

/* Resolve paths, build paths using template */

#include <genvector/gds_char.h>
#include <librnd/core/global_typedefs.h>

/* [4.3.1] Normally called from hid's rnd_exec_prefix(); if hid is not used
   or rnd_exec_prefix() is not called, call this early on so that
   paths are figured before used e.g. in loading configs. When configured
   with --floating-fhs, this functio figures the exec root dir and
   sets rnd_w32_* variables */
void rnd_path_init(void);


/* Allocate *out and copy the path from in to out, replacing ~ with conf_core.rc.path.home
   If extra_room is non-zero, allocate this many bytes extra for each slot;
   this leaves some room to append a file name. If quiet is non-zero, suppress
   error messages. */
void rnd_path_resolve(rnd_design_t *design, const char *in, char **out, unsigned int extra_room, int quiet);

/* Same as resolve_path, but it returns the pointer to the new path and calls
   free() on in */
char *rnd_path_resolve_inplace(rnd_design_t *design, char *in, unsigned int extra_room, int quiet);


/* Resolve all paths from a in[] into out[](should be large enough) */
void rnd_paths_resolve(rnd_design_t *design, const char **in, char **out, int numpaths, unsigned int extra_room, int quiet);

/* Resolve all paths from a char *in[] into a freshly allocated char **out */
#define rnd_paths_resolve_all(design, in, out, extra_room, quiet) \
do { \
	int __numpath__; \
	for(__numpath__ = 0; in[__numpath__] != NULL; __numpath__++) ; \
	__numpath__++; \
	if (__numpath__ > 0) { \
		out = malloc(sizeof(char *) * __numpath__); \
		rnd_paths_resolve(design, in, out, __numpath__, extra_room, quiet); \
	} \
} while(0)


/* generic file name template substitution callbacks for rnd_strdup_subst:
    %P    pid
    %F    load-time file name of the current design
    %B    basename (load-time file name of the current design without path)
    %D    dirname (load-time file path of the current design, without file name, with trailing slash, might be ./)
    %N    name of the current design
    %T    wall time (Epoch)
   ctx must be the current (rnd_design_t *)
*/
int rnd_build_fn_cb(void *ctx, gds_t *s, const char **input);

char *rnd_build_fn(rnd_design_t *design, const char *template);


/* Same as above, but also replaces lower case formatting to the members of
   the array if they are not NULL; use with rnd_build_argfn() */
typedef struct {
	const char *params['z' - 'a' + 1]; /* [0] for 'a' */
	rnd_design_t *design; /* if NULL, some of the substitutions (e.g. %B, %D, %N) won't be performed */
} rnd_build_argfn_t;

char *rnd_build_argfn(const char *template, rnd_build_argfn_t *arg);

int rnd_build_argfn_cb(void *ctx, gds_t *s, const char **input);

/* Used when ./configured with --floating-fhs: $PREFIX is not a fixed dir */
#ifdef RND_WANT_FLOATING_FHS
extern char *rnd_w32_root;     /* installation prefix; what would be $PREFIX on FHS, e.g. /usr/local */
extern char *rnd_w32_libdir;   /* on FHS this would be $PREFIX/lib*/
extern char *rnd_w32_bindir;   /* on FHS this would be $PREFIX/bin - on win32 this also hosts the dlls */
extern char *rnd_w32_sharedir; /* on FHS this would be $PREFIX/share */
extern char *rnd_w32_cachedir; /* where to store cache files, e.g. gdk pixbuf loader cache; persistent, but not part of the distribution */
#endif

#endif
