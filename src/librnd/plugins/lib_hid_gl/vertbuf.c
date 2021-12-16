/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2009-2017 PCB Contributers (See ChangeLog for details)
 *  Copyright (C) 2017 Adrian Purser
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
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
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Dynamic growing vertex buffer */

#include "config.h"

#define MARKER_STACK_SIZE       16
#define RESERVE_VERTEX_EXTRA    1024

typedef struct {
	vertex_t *data; /* A dynamic array of vertices. */
	int capacity; /* The capacity of the buffer (by number of vertices) */
	int size; /* The number of vertices in the buffer. */
	int marker; /* An index that allows vertices after the marker to be removed. */
} vertbuf_t;

static vertbuf_t vertbuf = { 0 };

RND_INLINE void vertbuf_clear(void)
{
	vertbuf.size = 0;
	vertbuf.marker = 0;
}

RND_INLINE void vertbuf_destroy(void)
{
	vertbuf_clear();
	if (vertbuf.data) {
		free(vertbuf.data);
		vertbuf.data = NULL;
	}
}

/* Ensure that the total capacity of the vertex buffer is at least 'size' vertices.
   When the buffer is reallocated, extra vertices will be added to avoid many small 
   allocations being made. */
static int vertbuf_reserve(int size)
{
	int result = 0;
	if (size > vertbuf.capacity) {
		vertex_t *p_data = realloc(vertbuf.data, (size + RESERVE_VERTEX_EXTRA) * sizeof(vertex_t));
		if (p_data == NULL)
			result = -1;
		else {
			vertbuf.data = p_data;
			vertbuf.capacity = size + RESERVE_VERTEX_EXTRA;
		}
	}
	return result;
}

/* Ensure that the capacity of the vertex buffer can accomodate an allocation
   of at least 'size' vertices. */
RND_INLINE int vertbuf_reserve_extra(int size)
{
	return vertbuf_reserve(vertbuf.size + size);
}

/* Set the marker to the end of the active vertex data. This allows vertices
   added after the marker has been set to be discarded. This is required when
   temporary vertices are required to draw something that will not be required
   for the final render pass. */
RND_INLINE int vertbuf_set_marker(void)
{
	vertbuf.marker = vertbuf.size;
	return vertbuf.marker;
}

/* Discard vertices added after the marker was set. The end of the buffer
   will then be the position of the marker. */
RND_INLINE void vertbuf_rewind(void)
{
	vertbuf.size = vertbuf.marker;
}

RND_INLINE vertex_t *vertbuf_allocate(int size)
{
	vertex_t *p_vertex = NULL;
	if (vertbuf_reserve_extra(size) == 0) {
		p_vertex = &vertbuf.data[vertbuf.size];
		vertbuf.size += size;
	}

	return p_vertex;
}

#if 0
/* not yet clear if this is universal */
RND_INLINE void vertbuf_add(GLfloat x, GLfloat y)
{
	vertex_t *p_vert = vertbuf_allocate(1);
	if (p_vert) {
		p_vert->x = x;
		p_vert->y = y;
		p_vert->r = red;
		p_vert->g = green;
		p_vert->b = blue;
		p_vert->a = alpha;
	}
}

RND_INLINE void vertbuf_add_xyuv(GLfloat x, GLfloat y, GLfloat u, GLfloat v)
{
	vertex_t *p_vert = vertbuf_allocate(1);
	if (p_vert) {
		p_vert->x = x;
		p_vert->y = y;
		p_vert->u = u;
		p_vert->v = v;
		p_vert->r = 1.0;
		p_vert->g = 1.0;
		p_vert->b = 1.0;
		p_vert->a = 1.0;
	}
}
#endif

