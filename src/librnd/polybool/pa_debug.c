/*
       Copyright (C) 2006 harry eaton

   based on:
       poly_Boolean: a polygon clip library
       Copyright (C) 1997  Alexey Nikitin, Michael Leonov
       (also the authors of the paper describing the actual algorithm)
       leonov@propro.iis.nsk.su

   in turn based on:
       nclip: a polygon clip library
       Copyright (C) 1993  Klamer Schutte
 
       This program is free software; you can redistribute it and/or
       modify it under the terms of the GNU General Public
       License as published by the Free Software Foundation; either
       version 2 of the License, or (at your option) any later version.
 
       This program is distributed in the hope that it will be useful,
       but WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
       General Public License for more details.
 
       You should have received a copy of the GNU General Public
       License along with this program; if not, write to the Free
       Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 
      polygon1.c
      (C) 1997 Alexey Nikitin, Michael Leonov
      (C) 1993 Klamer Schutte

      all cases where original (Klamer Schutte) code is present
      are marked
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
static char *theState(rnd_vnode_t * v);

static void pline_dump(rnd_vnode_t * v)
{
	rnd_vnode_t *s, *n;

	s = v;
	do {
		n = v->next;
		rnd_fprintf(stderr, "Line [%#mS %#mS %#mS %#mS 10 10 \"%s\"]\n",
								v->point[0], v->point[1], n->point[0], n->point[1], theState(v));
	}
	while ((v = v->next) != s);
}

static void poly_dump(rnd_polyarea_t * p)
{
	rnd_polyarea_t *f = p;
	rnd_pline_t *pl;

	do {
		pl = p->contours;
		do {
			pline_dump(pl->head);
			fprintf(stderr, "NEXT rnd_pline_t\n");
		}
		while ((pl = pl->next) != NULL);
		fprintf(stderr, "NEXT POLY\n");
	}
	while ((p = p->f) != f);
}
#endif
