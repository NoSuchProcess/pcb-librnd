/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2023 Tibor 'Igor2' Palinkas
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

RND_INLINE void PA_DEBUGP_DUMMY(const char *fmt, ...) { }
#ifndef NDEBUG
#include <stdarg.h>
RND_INLINE void DEBUGP(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	rnd_vfprintf(stderr, fmt, ap);
	va_end(ap);
}
#else
RND_INLINE void DEBUGP(const char *fmt, ...) { }
#endif


#if DEBUG_ANGLE
#	undef DEBUG_ANGLE
#	define DEBUG_ANGLE DEBUGP
#	define DEBUG_ANGLE_EN 1
#else
#	undef DEBUG_ANGLE
#	define DEBUG_ANGLE PA_DEBUGP_DUMMY
#	define DEBUG_ANGLE_EN 0
#endif

#ifndef DEBUG
#	undef DEBUG_ALL_LABELS
#	define DEBUG_ALL_LABELS 0
#endif


#if DEBUG_DUMP || DEBUG_PAISC_DUMP || DEBUG_PA_DUMP_PA
RND_INLINE void pa_debug_print_vnode_coord(rnd_vnode_t *n)
{
	DEBUGP(" %$mD ", n->point[0], n->point[1]);
}
#endif

#if DEBUG_DUMP || DEBUG_PAISC_DUMP || DEBUG_ANGLE_EN || DEBUG_PA_DUMP_PA
void pa_debug_print_angle(pa_angle_t a)
{
DEBUGP("%.09f", a);
}
#endif


#if DEBUG_ISC
RND_INLINE void pa_debug_print_isc(int num_isc, const char *name, rnd_vector_t isc1, rnd_vector_t isc2, rnd_vnode_t *a1, rnd_vnode_t *a2, rnd_vnode_t *b1, rnd_vnode_t *b2)
{
	DEBUGP("ISC %s: %$mD..%$mD and %$mD..%$mD\n", name, a1->point[0], a1->point[1], a2->point[0], a2->point[1], b1->point[0], b1->point[1], b2->point[0], b2->point[1]);
	if (num_isc > 0) DEBUGP(" %$mD\n", isc1[0], isc1[1]);
	if (num_isc > 1) DEBUGP(" %$mD\n", isc2[0], isc2[1]);
}
RND_INLINE void pa_debug_print_isc2(int num_isc, const char *name, rnd_vector_t *crd, rnd_vnode_t *nd)
{
	DEBUGP("  new node? isc=#%d on %s ", num_isc, name);
	DEBUGP("%$mD", (*crd)[0], (*crd)[1]);
	DEBUGP(" -> %p\n", nd);
}

#else
RND_INLINE void pa_debug_print_isc(int num_isc, const char *name, rnd_vector_t isc1, rnd_vector_t isc2, rnd_vnode_t *a1, rnd_vnode_t *a2, rnd_vnode_t *b1, rnd_vnode_t *b2) {}
RND_INLINE void pa_debug_print_isc2(int num_isc, const char *name, rnd_vector_t *crd, rnd_vnode_t *nd) {}
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
#endif

#if DEBUG_DUMP || DEBUG_PAISC_DUMP
static void pa_debug_dump(FILE *f, const char *title, rnd_polyarea_t *pa, pa_debug_dump_extra_t extra)
{
	pa_debug_dump_(f, title, pa, extra);
}
#else
static void pa_debug_dump(FILE *f, const char *title, rnd_polyarea_t *pa, pa_debug_dump_extra_t extra) {}
#endif

#if DEBUG_GATHER || DEBUG_JUMP
	static void DEBUG_COORDS(const char *prefix, rnd_vnode_t *n, const char *remark)
	{
		DEBUGP("%s at %ld %ld %s\n", prefix, (long)n->point[0], (long)n->point[1], remark);
	}
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
