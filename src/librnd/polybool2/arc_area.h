/* Return an estimation of the area under the arc (circle sector area) described
   with start and endpoint (sx;sy and ex;ey) and center point (cx;cy) and
   adir (0=ccw 1=cw in svg coord system). */
double pa_sect_area(rnd_coord_t sx, rnd_coord_t sy, rnd_coord_t ex, rnd_coord_t ey, rnd_coord_t cx, rnd_coord_t cy, int adir);

