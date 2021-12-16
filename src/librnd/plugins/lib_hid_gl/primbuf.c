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

/* Dynamic growing opengl primitive buffer */

#include "config.h"

#define MARKER_STACK_SIZE       16
#define RESERVE_PRIMITIVE_EXTRA 256

enum {
	PRIM_MASK_CREATE = 1000,
	PRIM_MASK_DESTROY,
	PRIM_MASK_USE
};

/* Primitive Buffer Data */

typedef struct {
	int type; /* The type of the primitive, eg. GL_LINES, GL_TRIANGLES, GL_TRIANGLE_FAN */
	GLint first; /* The index of the first vertex in the vertex buffer. */
	GLsizei count; /* The number of vertices */
	GLuint texture_id; /* The id of a texture to use, or 0 for no texture */
} primitive_t;

typedef struct {
	primitive_t *data; /* A dynamic array of primitives */
	int capacity; /* The number of primitives that can fit into the primitive_data */
	int size; /* The actual number of primitives in the primitive buffer */
	int marker; /* An index that allows primitives after the marker to be removed. */
	int dirty_index; /* The index of the first primitive that hasn't been drawn yet. */
} primbuf_t;

static primbuf_t primbuf = { 0 };

RND_INLINE void primbuf_clear(void)
{
	primbuf.size = 0;
	primbuf.dirty_index = 0;
	primbuf.marker = 0;
}

RND_INLINE void primbuf_destroy(void)
{
	primbuf_clear();
	if (primbuf.data) {
		free(primbuf.data);
		primbuf.data = NULL;
	}
}

/* Ensure that the total capacity of the primitive buffer is at least 'size'
   primitives.  When reallocating the buffer, extra primitives will be added
   to avoid many small reallocations. */
static int primbuf_reserve(int size)
{
	int result = 0;

	if (size > primbuf.capacity) {
		primitive_t *p_data = realloc(primbuf.data, (size + RESERVE_PRIMITIVE_EXTRA) * sizeof(primitive_t));
		if (p_data == NULL)
			result = -1;
		else {
			primbuf.data = p_data;
			primbuf.capacity = size + RESERVE_PRIMITIVE_EXTRA;
		}
	}

	return result;
}

/* Ensure that the capacity of the primitive buffer can accomodate an
   allocation of at least 'size' primitives. */
RND_INLINE int primbuf_reserve_extra(int size)
{
	return primbuf_reserve(primbuf.size + size);
}

/* Set the marker to the end of the active primitive data. This allows
   primitives added after the marker to be discarded. This is required
   when temporary primitives are required to draw something that will
   not be required for the final render pass. */
RND_INLINE int primbuf_set_marker(void)
{
	primbuf.marker = primbuf.size;
	return primbuf.marker;
}

/* Discard primitives added after the marker was set. The end of the buffer
   will then be the position of the marker. */
RND_INLINE void primbuf_rewind(void)
{
	primbuf.size = primbuf.marker;
}

RND_INLINE primitive_t *primbuf_back(void)
{
	return (primbuf.size > 0) && (primbuf.data) ? &primbuf.data[primbuf.size - 1] : NULL;
}

RND_INLINE int primitive_dirty_count(void)
{
	return primbuf.size - primbuf.dirty_index;
}

static void primbuf_add(int type, GLint first, GLsizei count, GLuint texture_id)
{
	primitive_t *last = primbuf_back();

	/* If the last primitive is the same type AND that type can be extended
	   AND the last primitive is dirty AND 'first' follows the last vertex of
	   the previous primitive THEN we can simply append the new primitive to
	   the last one. */
	if (last && (primitive_dirty_count() > 0) && (last->type == type) && ((type == GL_LINES) || (type == GL_TRIANGLES) || (type == GL_POINTS)) && ((last->first + last->count) == first))
		last->count += count;
	else if (primbuf_reserve_extra(1) == 0) {
		primitive_t *p_prim = &primbuf.data[primbuf.size++];
		p_prim->type = type;
		p_prim->first = first;
		p_prim->count = count;
		p_prim->texture_id = texture_id;
	}
}

RND_INLINE int primbuf_last_type(void)
{
	return primbuf.size > 0 ? primbuf.data[primbuf.size - 1].type : GL_ZERO;
}
