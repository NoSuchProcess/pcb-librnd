/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2023 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2023)
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
 *  This is a full rewrite of pcb-rnd's (and PCB's) polygon lib originally
 *  written by Harry Eaton in 2006, in turn building on "poly_Boolean: a
 *  polygon clip library" by Alexey Nikitin, Michael Leonov from 1997 and
 *  "nclip: a polygon clip library" Klamer Schutte from 1993.
 *
 *  English translation of the original paper the lib is largely based on:
 *  https://web.archive.org/web/20160418014630/http://www.complex-a5.ru/polyboolean/downloads/polybool_eng.pdf
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

#if defined(DEBUG) || DEBUG_CVC || DEBUG_DUMP || DEBUG_PAISC_DUMP

static const char *node_label_to_str(rnd_vnode_t *v)
{
	switch (v->flg.plabel) {
		case PA_PTL_INSIDE:   return "INSIDE";
		case PA_PTL_OUTSIDE:  return "OUTSIDE";
		case PA_PTL_SHARED:   return "SHARED";
		case PA_PTL_SHARED2:  return "SHARED2";
		case PA_PTL_UNKNWN:   break;
	}
	return "UNKNOWN";
}

#endif

#ifdef DEBUG

static void pa_pline_dump(rnd_vnode_t *v)
{
	rnd_vnode_t *start = v;

	do {
		rnd_fprintf(stderr,
			"Line [%#mS %#mS %#mS %#mS 10 10 \"%s\"]\n",
			v->point[0], v->point[1],
			v->next->point[0], v->next->point[1],
			node_label_to_str(v));
	} while((v = v->next) != start);
}

static void pa_poly_dump(rnd_polyarea_t *p)
{
	rnd_polyarea_t *start = p;

	do {
		rnd_pline_t *pl = p->contours;
		do {
			pa_pline_dump(pl->head);
			fprintf(stderr, "NEXT rnd_pline_t\n");
		}
		while((pl = pl->next) != NULL);
		fprintf(stderr, "NEXT POLY\n");
	}
	while((p = p->f) != start);
}
#else
#	undef DEBUG_ALL_LABELS
#	define DEBUG_ALL_LABELS 0
#endif


#if DEBUG_ALL_LABELS
RND_INLINE void pa_debug_print_pline_labels(rnd_pline_t *a)
{
	rnd_vnode_t *c = a->head;
	do {
		DEBUGP("%$mD->%$mD labeled %s\n", c->point[0], c->point[1], c->next->point[0], c->next->point[1], node_label_to_str(c));
	} while((c = c->next) != a->head);
	DEBUGP("\n\n");
}
#else
RND_INLINE void pa_debug_print_pline_labels(rnd_pline_t *a) {}
#endif

#if DEBUG_CVC || DEBUG_DUMP || DEBUG_PAISC_DUMP
RND_INLINE void pa_debug_print_vnode_coord(rnd_vnode_t *n)
{
#ifdef PA_BIGCOORD_ISC
	if (n->cvclst_next != NULL) {
		DEBUGP(" (%.012f, %.012f) ", pa_big_double(n->cvclst_next->isc.x), pa_big_double(n->cvclst_next->isc.y));
		return;
	}
#endif
	DEBUGP(" %$mD ", n->point[0], n->point[1]);
}
#endif

#if DEBUG_CVC || DEBUG_DUMP || DEBUG_PAISC_DUMP || DEBUG_ANGLE_EN
void pa_debug_print_angle(pa_big_angle_t a)
{
#ifdef PA_BIGCOORD_ISC
DEBUGP("%lu`%lu`%lu`%lu", a[3], a[2], a[1], a[0]);
#else
DEBUGP("%.09f", a);
#endif
}
#endif

#if DEBUG_CVC || DEBUG_DUMP || DEBUG_PAISC_DUMP
RND_INLINE void pa_debug_print_cvc_(pa_conn_desc_t *head, const char *ind)
{
	pa_conn_desc_t *n = head;

	DEBUGP("%sCVC:\n", ind);
	if (n == NULL) {
		DEBUGP("%s (empty cvc)\n", ind);
		return;
	}

	do {
		DEBUGP("%s %c %c ", ind, n->poly, n->side);
		pa_debug_print_angle(n->angle);
		pa_debug_print_vnode_coord(n->parent);
		if (n->side == 'N') {
			pa_debug_print_vnode_coord(n->parent->next);
			DEBUGP("%s\n", node_label_to_str(n->parent));
		}
		else {
			pa_debug_print_vnode_coord(n->parent->prev);
			DEBUGP("%s\n", node_label_to_str(n->parent->prev));
		}
	} while((n = n->next) != head);
}

RND_INLINE void pa_debug_print_cvc(pa_conn_desc_t *head)
{
	pa_debug_print_cvc_(head, "");
}

#else
RND_INLINE void pa_debug_print_cvc(pa_conn_desc_t *conn_list) {}
#endif

#if DEBUG_ISC
RND_INLINE void pa_debug_print_isc(int num_isc, const char *name, pa_big_vector_t isc1, pa_big_vector_t isc2, rnd_vnode_t *a1, rnd_vnode_t *a2, rnd_vnode_t *b1, rnd_vnode_t *b2)
{
#ifdef PA_BIGCOORD_ISC
	int i;
	DEBUGP("ISC %s: %.03f;%.03f..%.03f;%.03f and %.03f;%.03f..%.03f;%.03f\n", name, pa_big_vnxd(a1), pa_big_vnyd(a1), pa_big_vnxd(a2), pa_big_vnyd(a2), pa_big_vnxd(b1), pa_big_vnyd(b1), pa_big_vnxd(b2), pa_big_vnyd(b2));
	if (num_isc > 0) {
		DEBUGP(" %.03f;%.03f (x=", pa_big_double(isc1.x), pa_big_double(isc1.y));
		for(i = 0; i < PA_BIGCRD_WIDTH; i++) DEBUGP("%ld ", (long)isc1.x[i]);
		DEBUGP("y=");
		for(i = 0; i < PA_BIGCRD_WIDTH; i++) DEBUGP("%ld ", (long)isc1.y[i]);
		DEBUGP(")\n");
	}
	if (num_isc > 1) {
		DEBUGP(" %.03f;%.03f (x=", pa_big_double(isc2.x), pa_big_double(isc2.y));
		for(i = 0; i < PA_BIGCRD_WIDTH; i++) DEBUGP("%ld ", (long)isc2.x[i]);
		DEBUGP("y=");
		for(i = 0; i < PA_BIGCRD_WIDTH; i++) DEBUGP("%ld ", (long)isc2.y[i]);
		DEBUGP(")\n");
	}
#else
	DEBUGP("ISC %s: %$mD..%$mD and %$mD..%$mD\n", name, a1->point[0], a1->point[1], a2->point[0], a2->point[1], b1->point[0], b1->point[1], b2->point[0], b2->point[1]);
	if (num_isc > 0) DEBUGP(" %$mD\n", isc1[0], isc1[1]);
	if (num_isc > 1) DEBUGP(" %$mD\n", isc2[0], isc2[1]);
#endif
}
RND_INLINE void pa_debug_print_isc2(int num_isc, const char *name, pa_big_vector_t *crd, rnd_vnode_t *nd)
{
	DEBUGP("  new node? isc=#%d on %s ", num_isc, name);
#ifdef PA_BIGCOORD_ISC
	DEBUGP("%.03f;%.03f", pa_big_double(crd->x), pa_big_double(crd->y));
#else
	DEBUGP("%$mD", (*crd)[0], (*crd)[1]);
#endif
	DEBUGP(" -> %p\n", nd);
}

#else
RND_INLINE void pa_debug_print_isc(int num_isc, const char *name, pa_big_vector_t isc1, pa_big_vector_t isc2, rnd_vnode_t *a1, rnd_vnode_t *a2, rnd_vnode_t *b1, rnd_vnode_t *b2) {}
RND_INLINE void pa_debug_print_isc2(int num_isc, const char *name, pa_big_vector_t *crd, rnd_vnode_t *nd) {}
#endif

typedef enum { /* bitfield of extra info the dump should contain */
	PA_DBG_DUMP_CVC = 1
} pa_debug_dump_extra_t;

#if DEBUG_DUMP || DEBUG_PAISC_DUMP

RND_INLINE void pa_debug_dump_vnode_coord(FILE *f, rnd_vnode_t *n, pa_debug_dump_extra_t extra)
{
#ifdef PA_BIGCOORD_ISC
	if (n->cvclst_next != NULL) {
		int i;

		fprintf(f, "   %.012f %.012f (x=", pa_big_double(n->cvclst_next->isc.x), pa_big_double(n->cvclst_next->isc.y));
			for(i = 0; i < PA_BIGCRD_WIDTH; i++) fprintf(f, "%ld ", (long)n->cvclst_next->isc.x[i]);
			fprintf(f, "y=");
			for(i = 0; i < PA_BIGCRD_WIDTH; i++) fprintf(f, "%ld ", (long)n->cvclst_next->isc.y[i]);
		fprintf(f, ")\n");
		if (extra & PA_DBG_DUMP_CVC) {
			pa_debug_print_cvc_(n->cvclst_next, "    ");
		}
	}
#endif
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

static void pa_debug_dump(FILE *f, const char *title, rnd_polyarea_t *pa, pa_debug_dump_extra_t extra)
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

#else
static void pa_debug_dump(FILE *f, const char *title, rnd_polyarea_t *pa, pa_debug_dump_extra_t extra) {}

#endif

#if DEBUG_GATHER || DEBUG_JUMP
	static void DEBUG_COORDS(const char *prefix, rnd_vnode_t *n, const char *remark)
	{
#	ifdef PA_BIGCOORD_ISC
		if (n->cvclst_next != NULL) {
			DEBUGP("%s at %.2f %.2f (%ld %ld) %s\n", prefix, pa_big_double(n->cvclst_next->isc.x), pa_big_double(n->cvclst_next->isc.y), (long)n->point[0], (long)n->point[1], remark);
			return;
		}
#	endif
		DEBUGP("%s at %ld %ld %s\n", prefix, (long)n->point[0], (long)n->point[1], remark);
	}
#endif

#if DEBUG_GATHER
#define DEBUG_GATHER_COORDS(prefix, nd, remark) DEBUG_COORDS(prefix, nd, remark)
#else
#define DEBUG_GATHER_COORDS(prefix, nd, remark)
#endif

#if DEBUG_JUMP
#define DEBUG_JUMP_COORDS(prefix, nd, remark) DEBUG_COORDS(prefix, nd, remark)
#else
#define DEBUG_JUMP_COORDS(prefix, nd, remark)
#endif

#if DEBUG_JUMP
#	undef DEBUG_JUMP
#	define DEBUG_JUMP DEBUGP
#else
#	undef DEBUG_JUMP
#	define DEBUG_JUMP PA_DEBUGP_DUMMY
#endif

#if DEBUG_GATHER
#	undef DEBUG_GATHER
#	define DEBUG_GATHER DEBUGP
#else
#	undef DEBUG_GATHER
#	define DEBUG_GATHER PA_DEBUGP_DUMMY
#endif
