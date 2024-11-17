/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2024)
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

#include <assert.h>
#include "polygon1_gen.h"
#include "pa_config.h"
#include "pa_math.c"

TODO("arc: upgrade this code to deal with arcs in input");

int rnd_vertices_are_coaxial(rnd_vnode_t *node);
void pa_dump_pl(rnd_pline_t *pl, const char *fn);

long rnd_polybool_dump_offset_round = DEBUG_DUMP_OFFSET_ROUND;

#define Vequ2(a,b)       (memcmp((a),   (b),   sizeof(rnd_vector_t)) == 0)

#define Vsub2(r, a, b) \
	do { \
		(r)[0] = (a)[0] - (b)[0]; \
		(r)[1] = (a)[1] - (b)[1]; \
	} while(0)

/* Corner case handler: when p1..p2 and q1..q2 are parallel */
RND_INLINE int rnd_iline_inters2_par(rnd_vector_t p1, rnd_vector_t p2, rnd_vector_t q1, rnd_vector_t q2)
{
	rnd_vector_t q1p1, q1q2;

	Vsub2(q1p1, q1, p1);
	Vsub2(q1q2, q1, q2);

	/* check if p1..p2 and q1..q2 are not on same line (no intersections then) */
	if (rnd_vect_det2(q1p1, q1q2) != 0)
		return 0;

	return -1; /* coaxial lines */
}

/* same as pa_vect_inters2 but for infinitely extended lines, but return
   value differs:
    -1: coaxial lines (infinite number of intersections; Sout is not filled in)
     0: no intersection (prallel lines; Sout is not filled in)
     1: single intersection (Sout filled in) */
RND_INLINE int pa_iline_inters2(rnd_vector_t p1, rnd_vector_t p2, rnd_vector_t q1, rnd_vector_t q2, rnd_vector_t Sout)
{
	double t, divider;
	double pdx, pdy, qdx, qdy;

	/* calculate and cache delta_x/delta_y for p and q */
	pdx = p2[0] - p1[0]; pdy = p2[1] - p1[1];
	qdx = q2[0] - q1[0]; qdy = q2[1] - q1[1];

	/* for the actual algorithm see comments in pa_vect_inters2() */
	divider = pdy * qdx - pdx * qdy;
	if (divider == 0) /* divider is zero if the lines are parallel */
		return rnd_iline_inters2_par(p1, p2, q1, q2);

	/* degenerate cases: endpoint on the other line */
	if (Vequ2(q1, p1) || Vequ2(q1, p2)) {
		Sout[0] = q1[0]; Sout[1] = q1[1];
		return 1;
	}
	if (Vequ2(q2, p1) || Vequ2(q2, p2)) {
		Sout[0] = q2[0]; Sout[1] = q2[1];
		return 1;
	}

	t = (pdy * (p1[0] - q1[0]) + pdx * (q1[1] - p1[1])) / divider;
	Sout[0] = q1[0] + PA_ROUND(t * qdx);
	Sout[1] = q1[1] + PA_ROUND(t * qdy);
	return 1;
}



RND_INLINE rnd_vnode_t *pl_append_xy(rnd_pline_t *dst, rnd_coord_t x, rnd_coord_t y)
{
	rnd_vnode_t *vn = calloc(sizeof(rnd_vnode_t), 1);

	vn->point[0] = x;
	vn->point[1] = y;

	if (dst->head != NULL) {
		vn->next = dst->head;
		vn->prev = dst->head->prev;
		dst->head->prev->next = vn;
		dst->head->prev = vn;
	}
	else {
		vn->next = vn->prev = vn;
		dst->head = vn;
	}

	return vn;
}

/* how to handle conerns where the two offseted edges don't intersect */
typedef enum {
	RND_PLINE_CORNER_ROUND = 1,
	RND_PLINE_CORNER_SHARP,     /* sharp corner with only the intersection point of the two edges */
	RND_PLINE_CORNER_SHARP2,    /* semi-sharp corner with the offseted original endpoints and the intersection point of the two edges; this keeps edge slopes intact but introduces 2 extra points */
	RND_PLINE_CORNER_FLAT       /* keep the offseted original endpoints and connect them with a straight line */
} rnd_pline_corner_type;

static rnd_pline_t *pline_dup_with_offset_corner(const rnd_pline_t *src, rnd_coord_t offs, rnd_pline_corner_type ct)
{
	const rnd_vnode_t *curr, *next;
	rnd_vnode_t *vnew, *discard;
	rnd_pline_t *dst = calloc(sizeof(rnd_pline_t), 1);
	rnd_vector_t isc1, isc2;
	char dumpfn[128];
	double offs2 = -1;

	if (dst == NULL)
		return NULL;

	if (rnd_polybool_dump_offset_round) {
		rnd_polybool_dump_offset_round++;
		sprintf(dumpfn, "off_%06ld.I.poly", rnd_polybool_dump_offset_round);
		pa_dump_pl(src, dumpfn);
	}

	/* offset each edge also keeping the original corners but marked */
	curr = src->head;
	do {
		double vx, vy, nx, ny, len;

		if ((curr->point[0] == curr->next->point[0]) && (curr->point[1] == curr->next->point[1]))
			continue;

		if (rnd_vertices_are_coaxial(curr->next))
			continue;

		vnew = pl_append_xy(dst, curr->point[0], curr->point[1]);
		vnew->flg.mark = 1;

		vx = curr->next->point[0] - curr->point[0];
		vy = curr->next->point[1] - curr->point[1];
		len = sqrt(vx*vx + vy*vy);
		vx /= len;
		vy /= len;
		nx = (-vy) * -(double)offs;
		ny = (vx) * -(double)offs;
		pl_append_xy(dst, curr->point[0] + nx, curr->point[1] + ny);
		pl_append_xy(dst, curr->next->point[0] + nx, curr->next->point[1] + ny);
	} while((curr = curr->next) != src->head);


	/* the next section doesn't work correctly when starting on a marked node
	   (because of all the node removals) */
	if (dst->head->flg.mark)
		dst->head = dst->head->next;

	/* visit original corners (marked) and check if they are now the center of
	   a round outer corner (convex) or an external point on an inverted corner
	   (concave) */
	curr = dst->head;
	do {
		next = curr->next;

		if (curr->flg.mark) {
			if (rnd_vect_inters2(curr->prev->prev->point, curr->prev->point, curr->next->point, curr->next->next->point, isc1, isc2) == 1) {
				/* prev and next seg intersect -> concave */
				discard = curr->prev;
				rnd_poly_vertex_exclude(dst, (rnd_vnode_t *)curr);
				rnd_poly_vertex_exclude(dst, (rnd_vnode_t *)discard);
				curr->next->point[0] = isc1[0];
				curr->next->point[1] = isc1[1];
				free((rnd_vnode_t *)curr); /* curr is from dst now so it's safe to free it */
				free(discard);
			}
			else {
				rnd_coord_t cx = curr->point[0], cy = curr->point[1];
				rnd_vector_t isc;
				int ir, skip;

				/* convex: add a corner */
				switch(ct) {
					case RND_PLINE_CORNER_ROUND:
						rnd_poly_vertex_exclude(dst, (rnd_vnode_t *)curr);
						rnd_poly_frac_circle_to(dst, next->prev, cx, cy, next->prev->point, next->point, RND_POLY_FCT_SKIP_TINY | ((offs >= 0) ? 0 : RND_POLY_FCT_REVERSE));
						free((rnd_vnode_t *)curr); /* curr is from dst now so it's safe to free it */
						break;
					case RND_PLINE_CORNER_FLAT:
						rnd_poly_vertex_exclude(dst, (rnd_vnode_t *)curr);
						break;
					case RND_PLINE_CORNER_SHARP:
					case RND_PLINE_CORNER_SHARP2:
						ir = pa_iline_inters2(curr->prev->prev->point, curr->prev->point, curr->next->point, curr->next->next->point, isc);

						/* special case: see offset03: almost parallel edges with offset so
						   large that the intersection point gets too far away. The isc
						   point can not be further from any offseted endpoint than offs */
						if (offs2 < 0)
							offs2 = (double)offs * (double)offs;
						skip = 1;
						if (rnd_vect_dist2(isc, curr->prev->point) < offs2) skip = 0;
						if (skip && (rnd_vect_dist2(isc, curr->next->point) < offs2)) skip = 0;

						if (!skip) {
							if (ct == RND_PLINE_CORNER_SHARP) {
								/* remove original offseted ends, close to the isc, to preserve number of corners */
								rnd_poly_vertex_exclude(dst, (rnd_vnode_t *)curr->prev);
								rnd_poly_vertex_exclude(dst, (rnd_vnode_t *)curr->next);
							}

							rnd_poly_vertex_include_force(curr->prev, rnd_poly_node_create(isc));
						}

						rnd_poly_vertex_exclude(dst, (rnd_vnode_t *)curr);
/*						rndo_trace(" ISC iline: ", Pint(ir), " ", Pvect(isc), "\n", 0);*/
						break;
					default:
						assert(!"pline_dup_with_offset_corner(): invalid corner type");
						abort();
				}
			}
		}

	} while((curr = next) != dst->head);


	pa_pline_update(dst, 1);
	if (rnd_polybool_dump_offset_round) {
		sprintf(dumpfn, "off_%06ld.O.poly", rnd_polybool_dump_offset_round);
		pa_dump_pl(dst, dumpfn);
	}


	return dst;
}

rnd_pline_t *rnd_pline_dup_with_offset_round(const rnd_pline_t *src, rnd_coord_t offs)
{
	return pline_dup_with_offset_corner(src, offs, RND_PLINE_CORNER_ROUND);
}

