/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2023,2024 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2023,2024)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.*
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

RND_INLINE void PA_DEBUGP_DUMMY(const char *first, ...) { }

#ifndef NDEBUG
#	include <stdarg.h>
#	include "pa_prints.h"
#	define DEBUGP pa_trace
#else
	RND_INLINE void DEBUGP(const char *first, ...) { }
#endif

typedef enum { /* bitfield of extra info the dump should contain */
	PA_DBG_DUMP_dummy = 1
} pa_debug_dump_extra_t;

#if DEBUG_DUMP || DEBUG_PAISC_DUMP || DEBUG_PA_DUMP_PA

RND_INLINE void pa_debug_dump_vnode_coord(FILE *f, rnd_vnode_t *n, pa_debug_dump_extra_t extra)
{
	fprintf(f, "   %ld %ld\n", (long)n->point[0], (long)n->point[1]);
}

RND_INLINE void pa_debug_dump_pline_from(FILE *f, rnd_vnode_t *v, pa_debug_dump_extra_t extra)
{
	rnd_vnode_t *start = v;
	do {
		if (v == NULL) {
			fprintf(f, "   <NULL>\n");
			break;
		}
		pa_debug_dump_vnode_coord(f, v, extra);
	} while((v = v->next) != start);
}

RND_INLINE void pa_debug_dump_pline(FILE *f, rnd_pline_t *pl, pa_debug_dump_extra_t extra)
{
	pa_debug_dump_pline_from(f, pl->head, extra);
}

static void pa_debug_dump_(FILE *f, const char *title, rnd_polyarea_t *pa, pa_debug_dump_extra_t extra)
{
	rnd_pline_t *pl;
	rnd_polyarea_t *pn = pa;

	if (title != NULL)
		fprintf(f, "DUMP: %s\n", title);

	if (pa == NULL) {
		fprintf(f, " Polyarea\nEnd\n\n");
		return;
	}

	fprintf(f, " Polyarea\n");
	do {
	/* check if we have a contour for the given island */
		pl = pn->contours;
		if (pl != NULL) {

			fprintf(f, "  Contour\n");
			pa_debug_dump_pline(f, pl, extra);

			/* iterate over all holes within this island */
			for(pl = pa->contours->next; pl != NULL; pl = pl->next) {
				fprintf(f, "  Hole\n");
				pa_debug_dump_pline(f, pl, extra);
			}
		}
	} while ((pn = pn->f) != pa);
	fprintf(f, " End\n\n");
}

#undef fopen
void pa_dump_pa(rnd_polyarea_t *pa, const char *fn)
{
	FILE *f = fopen(fn, "w");
	pa_debug_dump_(f, NULL, pa, 0);
	fclose(f);
}

void pa_dump_pl(rnd_pline_t *pl, const char *fn)
{
	FILE *f = fopen(fn, "w");

	if (pl == NULL) {
		fprintf(f, " Polyarea\nEnd\n\n");
		return;
	}

	fprintf(f, " Polyarea\n");
	fprintf(f, "  Contour\n");
	pa_debug_dump_pline(f, pl, 0);
	fprintf(f, " End\n\n");

	fclose(f);
}
#endif

#if DEBUG_DUMP || DEBUG_PAISC_DUMP
static void pa_debug_dump(FILE *f, const char *title, rnd_polyarea_t *pa, pa_debug_dump_extra_t extra)
{
	pa_debug_dump_(f, title, pa, extra);
}
#else
static void pa_debug_dump(FILE *f, const char *title, rnd_polyarea_t *pa, pa_debug_dump_extra_t extra) {}
#endif

#if DEBUG_CLIP
#	undef DEBUG_CLIP
#	define DEBUG_CLIP DEBUGP
#else
#	undef DEBUG_CLIP
#	define DEBUG_CLIP PA_DEBUGP_DUMMY
#endif

#if DEBUG_SLICE
#	undef DEBUG_SLICE
#	define DEBUG_SLICE DEBUGP
#	define WANT_DEBUG_SLICE
#else
#	undef DEBUG_SLICE
#	define DEBUG_SLICE PA_DEBUGP_DUMMY
#	undef WANT_DEBUG_SLICE
#endif
