/* Note: a raw line is specified by its endpoints p1;p2; a raw arc also
   includes its center and adir */

/* return raw arc bbox in dst */
void pb2_raw_arc_bbox(rnd_rtree_box_t *dst, rnd_vector_t p1, rnd_vector_t p2, rnd_vector_t center, int adir);

/* return 1 if pt is on raw arc */
int pb2_raw_pt_on_arc(rnd_vector_t pt,  rnd_vector_t p1, rnd_vector_t p2, rnd_vector_t center, int adir);

