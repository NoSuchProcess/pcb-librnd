/* Note: a raw line is specified by its endpoints p1;p2; a raw arc also
   includes its center and adir */

/* return raw arc bbox in dst */
void pb2_raw_arc_bbox(rnd_rtree_box_t *dst, rnd_vector_t p1, rnd_vector_t p2, rnd_vector_t center, int adir);

/* return 1 if pt is on raw arc */
int pb2_raw_pt_on_arc(rnd_vector_t pt,  rnd_vector_t p1, rnd_vector_t p2, rnd_vector_t center, int adir);


/* return number of intersections; load rounded intersection points to isc_out* */
int pb2_raw_isc_arc_line(rnd_vector_t ap1, rnd_vector_t ap2, rnd_vector_t center, int adir, rnd_vector_t lp1, rnd_vector_t lp2, rnd_vector_t isc_out1, rnd_vector_t isc_out2);
int pb2_raw_isc_arc_arc(rnd_vector_t a1p1, rnd_vector_t a1p2, rnd_vector_t a1center, int a1adir, rnd_vector_t a2p1, rnd_vector_t a2p2, rnd_vector_t a2center, int a2adir, rnd_vector_t isc_out1, rnd_vector_t isc_out2);

/* load tangential vector to dst, from from_endpt pointing into the arc */
void pb2_raw_tangent_from_arc(rnd_vector_t dst, rnd_vector_t ap1, rnd_vector_t ap2, rnd_vector_t center, int adir, rnd_vector_t from_endpt);

