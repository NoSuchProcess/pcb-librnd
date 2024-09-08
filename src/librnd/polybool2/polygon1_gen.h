#ifndef RND_POLYGON1_GEN_H
#define RND_POLYGON1_GEN_H

#include <librnd/core/global_typedefs.h>
#include <librnd/polybool2/polyarea.h>

rnd_polyarea_t *rnd_poly_from_contour(rnd_pline_t *pl);
rnd_polyarea_t *rnd_poly_from_contour_nochk(rnd_pline_t *pl); /* do not update or optimize pl, do not assert(valid(pa)) */
rnd_polyarea_t *rnd_poly_from_contour_autoinv(rnd_pline_t *pl);

/* create a circle approximation from lines */
rnd_polyarea_t *rnd_poly_from_circle(rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius);

/* make a sharp corner rectangle */
rnd_polyarea_t *rnd_poly_from_rect(rnd_coord_t x1, rnd_coord_t x2, rnd_coord_t y1, rnd_coord_t y2);

/* make a rounded-corner rectangle bloated with radius r beyond x1,x2,y1,y2 */
rnd_polyarea_t *rnd_poly_from_round_rect(rnd_coord_t x1, rnd_coord_t x2, rnd_coord_t y1, rnd_coord_t y2, rnd_coord_t r);


/* generate a polygon of a round or square cap line of a given thickness */
rnd_polyarea_t *rnd_poly_from_line(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t thick, rnd_bool square);

/* generate a polygon of a round cap arc of a given thickness */
rnd_polyarea_t *rnd_poly_from_arc(rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t astart, rnd_angle_t adelta, rnd_coord_t thick);

/* Slice up a polyarea-with-holes into a set of polygon islands with no
   holes, within the clip area. If the clip area is all-zero, do not clip.
   Free's pa but doesn't free (rnd_pline_t *) passed to emit(), emit() needs
   to free it. */
void rnd_polyarea_no_holes_dicer(rnd_polyarea_t *pa, rnd_coord_t clipX1, rnd_coord_t clipY1, rnd_coord_t clipX2, rnd_coord_t clipY2, void (*emit)(rnd_pline_t *, void *), void *user_data);

/* Add vertices in a fractional-circle starting from v centered at cx, cy and
   going counter-clockwise. Does not include the first and last point. Range:
    - 1 for a full circle
    - 2 for a half circle
    - 4 for a quarter circle */
void rnd_poly_frac_circle(rnd_pline_t *c, rnd_coord_t cx, rnd_coord_t cy, rnd_vector_t v, int range);

/* same but adds the last vertex as well, if range==4 */
void rnd_poly_frac_circle_end(rnd_pline_t *c, rnd_coord_t cx, rnd_coord_t cy, rnd_vector_t v, int range);

/* Draw a CCW fractional-circle from start to end (assuming start and end
   are added by the caller). Start and end must be on the same radius from cx;cy */
void rnd_poly_frac_circle_to(rnd_pline_t *c, rnd_vnode_t *insert_at, rnd_coord_t cx, rnd_coord_t cy, const rnd_vector_t start, const rnd_vector_t end);


#endif
