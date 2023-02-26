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

#undef DEBUG_LABEL
#undef DEBUG_ALL_LABELS
#undef DEBUG_JUMP
#undef DEBUG_GATHER
#undef DEBUG_ANGLE
#undef DEBUG

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

#ifdef DEBUG
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

#ifdef DEBUG_ALL_LABELS
static void pa_print_pline_labels(rnd_pline_t *a)
{
	rnd_vnode_t *c = a->head;
	do {
		DEBUGP("%#mD->%#mD labeled %s\n", c->point[0], c->point[1], c->next->point[0], c->next->point[1], node_label_to_str(c));
	} while((c = c->next) != a->head);
}
#endif

#endif
