/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1998,1999,2000,2001 harry eaton
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
 *
 *  this file, intersect.c, was written and is
 *  Copyright (c) 2001 C. Scott Ananian
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
 *
 *  Old contact info:
 *  harry eaton, 6697 Buttonhole Ct, Columbia, MD 21044 USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

#include "config.h"

#include <assert.h>
#include <stdlib.h>

#include "box_isc.h"
#include <librnd/core/box.h>

static int compareleft(const void *ptr1, const void *ptr2);
static int compareright(const void *ptr1, const void *ptr2);
static int comparepos(const void *ptr1, const void *ptr2);
static int nextpwrof2(int i);

typedef struct {
	rnd_coord_t left, right;
	int covered;
	rnd_coord_t area;
} seg_tree_node_t;

typedef struct {
	seg_tree_node_t *nodes;
	int size;
} seg_tree_t;

typedef struct {
	rnd_coord_t *p;
	int size;
} location_list_t;

/* Create a sorted list of unique y coords from an array of boxes. */
static location_list_t create_sorted_y_list(rnd_box_t *boxes, long len)
{
	location_list_t yCoords;
	rnd_coord_t last;
	int i, n;
	/* create sorted list of Y coordinates */
	yCoords.size = 2 * len;
	yCoords.p = (rnd_coord_t *) calloc(yCoords.size, sizeof(*yCoords.p));
	for (i = 0; i < len; i++) {
		yCoords.p[2 * i] = boxes[i].Y1;
		yCoords.p[2 * i + 1] = boxes[i].Y2;
	}
	qsort(yCoords.p, yCoords.size, sizeof(*yCoords.p), comparepos);
	/* count uniq y coords */
	last = 0;
	for (n = 0, i = 0; i < yCoords.size; i++)
		if (i == 0 || yCoords.p[i] != last)
			yCoords.p[n++] = last = yCoords.p[i];
	yCoords.size = n;
	return yCoords;
}

/* Create an empty segment tree from the given sorted list of uniq y coords. */
static seg_tree_t createseg_tree_t(rnd_coord_t * yCoords, int N)
{
	seg_tree_t st;
	int i;
	/* size is twice the nearest larger power of 2 */
	st.size = 2 * nextpwrof2(N);
	st.nodes = (seg_tree_node_t *) calloc(st.size, sizeof(*st.nodes));
	/* initialize the rightmost leaf node */
	st.nodes[st.size - 1].left = (N > 0) ? yCoords[--N] : 10;
	st.nodes[st.size - 1].right = st.nodes[st.size - 1].left + 1;
	/* initialize the rest of the leaf nodes */
	for (i = st.size - 2; i >= st.size / 2; i--) {
		st.nodes[i].right = st.nodes[i + 1].left;
		st.nodes[i].left = (N > 0) ? yCoords[--N] : st.nodes[i].right - 1;
	}
	/* initialize the internal nodes */
	for (; i > 0; i--) {					/* node 0 is not used */
		st.nodes[i].right = st.nodes[2 * i + 1].right;
		st.nodes[i].left = st.nodes[2 * i].left;
	}
	/* done! */
	return st;
}

void insertSegment(seg_tree_t * st, int n, rnd_coord_t Y1, rnd_coord_t Y2)
{
	rnd_coord_t discriminant;
	if (st->nodes[n].left >= Y1 && st->nodes[n].right <= Y2) {
		st->nodes[n].covered++;
	}
	else {
		assert(n < st->size / 2);
		discriminant = st->nodes[n * 2 + 1 /*right */ ].left;
		if (Y1 < discriminant)
			insertSegment(st, n * 2, Y1, Y2);
		if (discriminant < Y2)
			insertSegment(st, n * 2 + 1, Y1, Y2);
	}
	/* fixup area */
	st->nodes[n].area = (st->nodes[n].covered > 0) ?
		(st->nodes[n].right - st->nodes[n].left) : (n >= st->size / 2) ? 0 : st->nodes[n * 2].area + st->nodes[n * 2 + 1].area;
}

void deleteSegment(seg_tree_t * st, int n, rnd_coord_t Y1, rnd_coord_t Y2)
{
	rnd_coord_t discriminant;
	if (st->nodes[n].left >= Y1 && st->nodes[n].right <= Y2) {
		assert(st->nodes[n].covered);
		--st->nodes[n].covered;
	}
	else {
		assert(n < st->size / 2);
		discriminant = st->nodes[n * 2 + 1 /*right */ ].left;
		if (Y1 < discriminant)
			deleteSegment(st, n * 2, Y1, Y2);
		if (discriminant < Y2)
			deleteSegment(st, n * 2 + 1, Y1, Y2);
	}
	/* fixup area */
	st->nodes[n].area = (st->nodes[n].covered > 0) ?
		(st->nodes[n].right - st->nodes[n].left) : (n >= st->size / 2) ? 0 : st->nodes[n * 2].area + st->nodes[n * 2 + 1].area;
}

/* Compute the area of the intersection of the given rectangles; that is,
 * the area covered by more than one rectangle (counted twice if the area is
 * covered by three rectangles, three times if covered by four rectangles,
 * etc.).
 * Runs in O(N ln N) time.
 */
double rnd_intersect_box_box(rnd_box_t *boxes, long len)
{
	rnd_cardinal_t i;
	double area = 0.0;
	/* first get the aggregate area. */
	for (i = 0; i < len; i++)
		area += (double)(boxes[i].X2 - boxes[i].X1) * (double)(boxes[i].Y2 - boxes[i].Y1);
	/* intersection area is aggregate - union. */
	return area * 0.0001 - rnd_union_box_box(boxes, len);
}

/* Compute the area of the union of the given rectangles. O(N ln N) time. */
double rnd_union_box_box(rnd_box_t *boxes, long len)
{
	rnd_box_t **rectLeft, **rectRight;
	rnd_cardinal_t i, j;
	location_list_t yCoords;
	seg_tree_t segtree;
	rnd_coord_t lastX;
	double area = 0.0;

	if (len == 0)
		return 0.0;
	/* create sorted list of Y coordinates */
	yCoords = create_sorted_y_list(boxes, len);
	/* now create empty segment tree */
	segtree = createseg_tree_t(yCoords.p, yCoords.size);
	free(yCoords.p);
	/* create sorted list of left and right X coordinates of rectangles */
	rectLeft = (rnd_box_t **) calloc(len, sizeof(*rectLeft));
	rectRight = (rnd_box_t **) calloc(len, sizeof(*rectRight));
	for (i = 0; i < len; i++) {
		assert(boxes[i].X1 <= boxes[i].X2);
		assert(boxes[i].Y1 <= boxes[i].Y2);
		rectLeft[i] = rectRight[i] = &boxes[i];
	}
	qsort(rectLeft, len, sizeof(*rectLeft), compareleft);
	qsort(rectRight, len, sizeof(*rectRight), compareright);
	/* sweep through x segments from left to right */
	i = j = 0;
	lastX = rectLeft[0]->X1;
	while (j < len) {
		assert(i <= len);
		/* i will step through rectLeft, j will through rectRight */
		if (i == len || rectRight[j]->X2 < rectLeft[i]->X1) {
			/* right edge of rectangle */
			rnd_box_t *b = rectRight[j++];
			/* check lastX */
			if (b->X2 != lastX) {
				assert(lastX < b->X2);
				area += (double) (b->X2 - lastX) * segtree.nodes[1].area;
				lastX = b->X2;
			}
			/* remove a segment from the segment tree. */
			deleteSegment(&segtree, 1, b->Y1, b->Y2);
		}
		else {
			/* left edge of rectangle */
			rnd_box_t *b = rectLeft[i++];
			/* check lastX */
			if (b->X1 != lastX) {
				assert(lastX < b->X1);
				area += (double) (b->X1 - lastX) * segtree.nodes[1].area;
				lastX = b->X1;
			}
			/* add a segment from the segment tree. */
			insertSegment(&segtree, 1, b->Y1, b->Y2);
		}
	}
	free(rectLeft);
	free(rectRight);
	free(segtree.nodes);
	return area * 0.0001;
}

static int compareleft(const void *ptr1, const void *ptr2)
{
	rnd_box_t **b1 = (rnd_box_t **) ptr1, **b2 = (rnd_box_t **) ptr2;
	return (*b1)->X1 - (*b2)->X1;
}

static int compareright(const void *ptr1, const void *ptr2)
{
	rnd_box_t **b1 = (rnd_box_t **) ptr1, **b2 = (rnd_box_t **) ptr2;
	return (*b1)->X2 - (*b2)->X2;
}

static int comparepos(const void *ptr1, const void *ptr2)
{
	return *((rnd_coord_t *) ptr1) - *((rnd_coord_t *) ptr2);
}

static int nextpwrof2(int n)
{
	int r = 1;
	while (n != 0) {
		n /= 2;
		r *= 2;
	}
	return r;
}
