/*
      poly_Boolean: a polygon clip library
      Copyright (C) 1997  Alexey Nikitin, Michael Leonov
      leonov@propro.iis.nsk.su

      This library is free software; you can redistribute it and/or
      modify it under the terms of the GNU Library General Public
      License as published by the Free Software Foundation; either
      version 2 of the License, or (at your option) any later version.

      This library is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
      Library General Public License for more details.

      You should have received a copy of the GNU Library General Public
      License along with this library; if not, write to the Free
      Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

      polyarea.h
      (C) 1997 Alexey Nikitin, Michael Leonov
      (C) 1997 Klamer Schutte (minor patches)
*/

#ifndef RND_POLYAREA_H
#define RND_POLYAREA_H

#define RND_PLF_DIR 1
#define RND_PLF_INV 0
#define RND_PLF_MARK 1

/* x;y point coordinates; it's an array instead of a struct so it's
   guaranteed to be "packed", no padding, so that memcpy/memcmp work */
typedef rnd_coord_t rnd_vertex_t[2];
typedef rnd_vertex_t rnd_vector_t;

extern const rnd_vector_t rnd_vect_zero;

typedef struct pa_conn_desc_s pa_conn_desc_t; /* A "descriptor" that makes up a "connectivity list" in the paper */
typedef struct rnd_vnode_s rnd_vnode_t;
struct rnd_vnode_s {
	rnd_vnode_t *next, *prev, *shared;
	struct {
		unsigned int plabel:3; /* one of pa_plinept_label_t; for the edge {v to v->next} (the edge forward of v) */
		unsigned int mark:1;
		unsigned int in_hub:1;
	} flg;
	pa_conn_desc_t *cvclst_prev, *cvclst_next; /* "cross vertex connection list" */
	rnd_vector_t point;
};

typedef struct rnd_pline_s rnd_pline_t;
struct rnd_pline_s {
	rnd_coord_t xmin, ymin, xmax, ymax;
	rnd_pline_t *next;
	rnd_vnode_t *head;
	unsigned int Count;
	double area;
	rnd_rtree_t *tree;
	rnd_bool is_round;  /* unused */
	rnd_coord_t cx, cy; /* unused */
	rnd_coord_t radius; /* unused */
	struct {
		unsigned int llabel:3; /* one of pa_pline_label_t */
		unsigned int orient:1;
	} flg;
};

rnd_pline_t *pa_pline_new(const rnd_vector_t v);

void pa_pline_init(rnd_pline_t *c);
void rnd_poly_contour_clear(rnd_pline_t *c); /* clears list of vertices */
void pa_pline_free(rnd_pline_t **c);

/* Allocate a new polyline and copy src into it (losing labels). The dup()
   variant simply returns the new polyline or returns NULL on allocation
   error. The alloc_copy() variant places the result in dst and returns true
   on success. */
rnd_pline_t *pa_pline_dup(const rnd_pline_t *src);
rnd_bool pa_pline_alloc_copy(rnd_pline_t **dst, const rnd_pline_t *src);

/* Recalculate bounding box, area, number-of-nodes and rebuuld the rtree. If
   optimize is true also remove excess colinear points. */
void pa_pline_update(rnd_pline_t *c, rnd_bool optimize);

void pa_pline_invert(rnd_pline_t *c); /* invert contour */

rnd_vnode_t *rnd_poly_node_create(rnd_vector_t v);

void rnd_poly_vertex_include(rnd_vnode_t *after, rnd_vnode_t *node);
void rnd_poly_vertex_include_force(rnd_vnode_t *after, rnd_vnode_t *node); /* do not remove nodes even if on the same line */
void rnd_poly_vertex_exclude(rnd_pline_t *parent, rnd_vnode_t *node);

/* this should be an internal function but it's also used in self-isc code */
rnd_vnode_t *rnd_poly_node_add_single(rnd_vnode_t *dest, rnd_vector_t po);

/* pline->tree boxes are opaq seg structs; this call converts them to vnode */
rnd_vnode_t *rnd_pline_seg2vnode(void *box);

/**********************************************************************/

struct rnd_polyarea_s {
	rnd_polyarea_t *f, *b;
	rnd_pline_t *contours;
	rnd_rtree_t *contour_tree;
};

rnd_bool rnd_polyarea_alloc_copy(rnd_polyarea_t **dst, const rnd_polyarea_t *srcfst);
void rnd_polyarea_m_include(rnd_polyarea_t **list, rnd_polyarea_t *a);

/* Allocate a new polyarea and copy src into it; returns NULL on error */
rnd_polyarea_t *rnd_polyarea_dup(const rnd_polyarea_t *src);

/* Allocate a new polyarea in dst and copy src into it; returns false on error */
rnd_bool pa_polyarea_alloc_copy(rnd_polyarea_t **dst, const rnd_polyarea_t *src);

/* Copy plines from src to pre-allocated dst, overwriting (but no freeing) dst's
   pline list. Caller needs to make sure dst is empty */
rnd_bool pa_polyarea_copy_plines(rnd_polyarea_t *dst, const rnd_polyarea_t *src);

/* Insert pl (outer contour or hole) in pa. Returns true on success. pl's
   direction matters: positive means outer, negative means hole. Fails
   when inserting an outer in a pa that already has outer, or inserting a hole
   in a pa that doesn't have an outer. */
rnd_bool pa_polyarea_insert_pline(rnd_polyarea_t *pa, rnd_pline_t *pl);

rnd_bool rnd_polyarea_contour_exclude(rnd_polyarea_t *p, rnd_pline_t *c);


rnd_bool rnd_polyarea_contour_check(rnd_pline_t *a);

rnd_bool rnd_polyarea_contour_inside(rnd_polyarea_t *c, rnd_vector_t v0);
rnd_bool rnd_polyarea_touching(rnd_polyarea_t *p1, rnd_polyarea_t *p2);

/*** tools for clipping ***/

/* checks whether point lies within contour independently of its orientation */
int pa_pline_is_point_inside(const rnd_pline_t *c, rnd_vector_t v);

/* Return whether inner is within outer. Caller must ensure they are not
   intersecting. Returns the correct result if pl and inner share common
   points on contour. (Identical contours are treated as being inside each
   other). */
rnd_bool pa_pline_inside_pline(rnd_pline_t *outer, rnd_pline_t *inner);

/* Allocate and initialize an empty polyarea (single island initially) */
rnd_polyarea_t *pa_polyarea_alloc(void);

void pa_polyarea_free_all(rnd_polyarea_t **p);
void pa_polyarea_init(rnd_polyarea_t *p); /* reset/initialize p's fields (call after malloc) */
void rnd_poly_plines_free(rnd_pline_t **pl);
rnd_bool rnd_poly_valid(rnd_polyarea_t *p);

enum rnd_poly_bool_op_e {
	RND_PBO_UNITE,
	RND_PBO_ISECT,
	RND_PBO_SUB,
	RND_PBO_XOR
} rnd_poly_bool_op_t;

double rnd_vect_dist2(rnd_vector_t v1, rnd_vector_t v2);
double rnd_vect_det2(rnd_vector_t v1, rnd_vector_t v2);
double rnd_vect_len2(rnd_vector_t v1);

/* Calculate the intersection(s) of two lines A..B and C..D; return number
   of intersections (0, 1 or 2) and load R1 and R2 with the intersection coords
   accordingly */
int rnd_vect_inters2(rnd_vector_t A, rnd_vector_t B, rnd_vector_t C, rnd_vector_t D, rnd_vector_t R1, rnd_vector_t R2);

/* Generic polyarea boolean operation between a and b with resulting polyarea
   put in res; op is one of rnd_poly_bool_op_t. The _free() variant frees a
   and b and is faster. The non-_free variant makes a copy of a and b to preserve
   them. Return 0 on success. */
int rnd_polyarea_boolean(const rnd_polyarea_t *a, const rnd_polyarea_t *b, rnd_polyarea_t **res, int op);
int rnd_polyarea_boolean_free(rnd_polyarea_t *a, rnd_polyarea_t *b, rnd_polyarea_t **res, int op);


/* compute the intersection and subtraction (divides "a" into two pieces)
   and frees the input polys. This assumes that bi is a single simple polygon.
   Runs faster than calling rnd_polyarea_boolean_free() twice because
   some mapping needs to be done only once. */
int rnd_polyarea_and_subtract_free(rnd_polyarea_t *a, rnd_polyarea_t *b, rnd_polyarea_t **aandb, rnd_polyarea_t **aminusb);

int rnd_polyarea_save(rnd_polyarea_t *PA, char *fname);

/* calculate the bounding box of a rnd_polyarea_t and save result in b */
void rnd_polyarea_bbox(rnd_polyarea_t *p, rnd_box_t *b);

/* Move each point of pa1 by dx and dy */
void rnd_polyarea_move(rnd_polyarea_t *pa1, rnd_coord_t dx, rnd_coord_t dy);

/*** Tools for building polygons for common object shapes ***/


#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif
double rnd_round(double x); /* from math_helper.h */

/* Calculate an endpoint of an arc and return the result in *x;*y */
RND_INLINE void rnd_arc_get_endpt(rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t astart, rnd_angle_t adelta, int which, rnd_coord_t *x, rnd_coord_t *y)
{
	if (which == 0) {
		*x = rnd_round((double)cx - (double)width * cos(astart * (M_PI/180.0)));
		*y = rnd_round((double)cy + (double)height * sin(astart * (M_PI/180.0)));
	}
	else {
		*x = rnd_round((double)cx - (double)width * cos((astart + adelta) * (M_PI/180.0)));
		*y = rnd_round((double)cy + (double)height * sin((astart + adelta) * (M_PI/180.0)));
	}
}

/*** constants for the poly shape generator ***/

/* polygon diverges from modelled arc no more than MAX_ARC_DEVIATION * thick */
#define RND_POLY_ARC_MAX_DEVIATION 0.02

#define RND_POLY_CIRC_SEGS 40
#define RND_POLY_CIRC_SEGS_F ((float)RND_POLY_CIRC_SEGS)

/* adjustment to make the segments outline the circle rather than connect
   points on the circle: 1 - cos (\alpha / 2) < (\alpha / 2) ^ 2 / 2 */
#define RND_POLY_CIRC_RADIUS_ADJ \
	(1.0 + M_PI / RND_POLY_CIRC_SEGS_F * M_PI / RND_POLY_CIRC_SEGS_F / 2.0)

/*** special purpose, internal tools ***/
/* Convert a struct seg *obj extracted from a pline->tree into coords */
void rnd_polyarea_get_tree_seg(void *obj, rnd_coord_t *x1, rnd_coord_t *y1, rnd_coord_t *x2, rnd_coord_t *y2);

/* create a (rnd_rtree_t *) of each segment derived from a contour object of src */
void *rnd_poly_make_edge_tree(rnd_pline_t *src);


/* EPSILON^2 for endpoint matching; the bool algebra code is not
   perfect and causes tiny self intersections at the end of sharp
   spikes. Accept at most 10 nanometer of such intersection */
#define RND_POLY_ENDP_EPSILON 100

/*** generic geo ***/
int rnd_point_in_triangle(rnd_vector_t A, rnd_vector_t B, rnd_vector_t C, rnd_vector_t P);


/*** COMPATIBILITY ***/
/* These declarations provide compatibility with librnd's poly lib; see pa_compat.c */

rnd_pline_t *rnd_poly_contour_new(const rnd_vector_t v);
void rnd_poly_contour_init(rnd_pline_t *c);
void rnd_poly_contour_del(rnd_pline_t **c);
void rnd_poly_contour_pre(rnd_pline_t *c, rnd_bool optimize);
void rnd_poly_contours_free(rnd_pline_t **pl);
rnd_bool rnd_poly_contour_copy(rnd_pline_t **dst, const rnd_pline_t *src);
void rnd_poly_contour_inv(rnd_pline_t *c);
int rnd_poly_contour_inside(const rnd_pline_t *c, rnd_vector_t v);
int rnd_poly_contour_in_contour(rnd_pline_t *outer, rnd_pline_t *inner);

rnd_bool rnd_polyarea_copy1(rnd_polyarea_t *dst, const rnd_polyarea_t *src);
rnd_bool rnd_polyarea_copy0(rnd_polyarea_t **dst, const rnd_polyarea_t *src);
rnd_bool rnd_polyarea_m_copy0(rnd_polyarea_t **dst, const rnd_polyarea_t *srcfst);
rnd_bool rnd_polyarea_contour_include(rnd_polyarea_t *p, rnd_pline_t *c);
void rnd_polyarea_init(rnd_polyarea_t *p);
rnd_polyarea_t *rnd_polyarea_create(void);
void rnd_polyarea_free(rnd_polyarea_t **p);

#endif /* RND_POLYAREA_H */
