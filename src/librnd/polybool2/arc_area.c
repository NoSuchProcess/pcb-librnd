/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
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

#include <librnd/rnd_config.h>
#include <assert.h>
#include <math.h>


#include "arc_area.h"
#include "arc_area_factor.h"

#define LEN (sizeof(arc_area_factor) / sizeof(double))

/* binary search in arc_area_factor[] for key, in the first column; returns
   index of the first column of range bottom, so that
   key >= arc_area_factor[return] */
RND_INLINE int lookup(double key)
{
	int idx, step;

	idx = LEN/2;
	if (idx % 2)
		idx++;
	step = LEN/8;
	for(;;) {
/*		printf("chk: %d %f %f step=%d\n", idx, arc_area_factor[idx], arc_area_factor[idx+2], step);*/
		if ((arc_area_factor[idx] <= key) && (arc_area_factor[idx+2] >= key))
			return idx;
		if (key < arc_area_factor[idx])
			idx -= step*2;
		else
			idx += step*2;
		if ((idx < 0) || (idx > LEN))
			return -1;
		if (step > 1)
			step = (step+1)/2;
	}
}

double pa_sect_area(rnd_coord_t sx, rnd_coord_t sy, rnd_coord_t ex, rnd_coord_t ey, rnd_coord_t cx, rnd_coord_t cy, int adir)
{
	double r2, rs2, re2, dx, dy, len2, key, area_factor;
	double x1, y1, x2, y2, cp;
	int idx, large;

	if ((sx == ex) && (sy == ey)) {
		/* zero length because single object full circuit is invalid */
		return 0;
	}

	/* compute key: chord_len^2 / r^2 */
	dx = sx - cx; dy = sy - cy;
	rs2 = dx*dx + dy*dy;
	dx = ex - cx; dy = ey - cy;
	re2 = dx*dx + dy*dy;
	r2 = (rs2+re2)/2.0;
	dx = ex - sx; dy = ey - sy;
	len2 = dx*dx + dy*dy;
	key = len2 / r2;

/*	printf("key=%f r: %f %f -> %f\n", key, rs2, re2, r2);*/

	if (key >= 4.0)
		return r2 * M_PI / 2;

	idx = lookup(key);


	/* linear interpolation within the section found in the table */
	x1 = arc_area_factor[idx+0]; y1 = arc_area_factor[idx+1];
	x2 = arc_area_factor[idx+2]; y2 = arc_area_factor[idx+3];
	area_factor = y1 + (key - x1) * (y2 - y1) / (x2 - x1);

	/* figure if center is left of or right of the chord */
	cp = (double)(ex - sx)*(double)(cy - sy) - (double)(ey - sy)*(double)(cx - sx);

	/* cp==0 means 180 deg; must have resulted key==4.0 above */
	assert(cp != 0);

	/* decide if sweep is larger than 180 deg */
	large = ((cp > 0) != adir);

/*	printf("%d %f %f : %f %f -> %f large=%d cp=%f\n", idx, arc_area_factor[idx], arc_area_factor[idx+2], arc_area_factor[idx+1], arc_area_factor[idx+3], area_factor, large, cp);*/

	/* if larger than 180 degree the sector area computed is really the part
	   that is missing from a full circle area so the result is
	   r^2 * M_PI - area_factor * r2 */
	if (large)
		return (M_PI - area_factor) * r2;

	return area_factor * r2;
}

