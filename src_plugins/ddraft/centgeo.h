#ifndef PCB_CENTGEO_H
#define PCB_CENTGEO_H

#include "obj_line.h"

/*** Calculate centerpoint intersections of objects (thickness ignored) ***/

/* Calculate the intersection point(s) of two lines and store them in ip
   and/or offs (on Line1) if they are not NULL. Returns:
   0 = no intersection
   1 = one intersection (X1;Y1 of ip is loaded)
   2 = overlapping segments (overlap endpoitns are stored in X1;Y1 and X2;Y2 of ip) */
int pcb_intersect_cline_cline(pcb_line_t *Line1, pcb_line_t *Line2, pcb_box_t *ip, double offs[2]);

/* Calculate the intersection point(s) of a lines and an arc and store them
   in ip and/or offs (on Line) if they are not NULL. Returns:
   0 = no intersection
   1 = one intersection (X1;Y1 of ip is loaded)
   2 = two intersections (stored in X1;Y1 and X2;Y2 of ip) */
int pcb_intersect_cline_carc(pcb_line_t *Line, pcb_arc_t *Arc, pcb_box_t *ip, double offs[2]);


/* Calculate the point on a line corresponding to a [0..1] offset and store
   the result in dstx;dsty. */
void pcb_cline_offs(pcb_line_t *line, double offs, pcb_coord_t *dstx, pcb_coord_t *dsty);

/* Project a point (px;py) onto a line and return the offset from point1 */
double pcb_cline_pt_offs(pcb_line_t *line, pcb_coord_t px, pcb_coord_t py);

#endif
