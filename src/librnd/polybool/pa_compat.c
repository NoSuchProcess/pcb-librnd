rnd_pline_t *rnd_poly_contour_new(const rnd_vector_t v) { return pa_pline_new(v); }
void rnd_poly_contour_init(rnd_pline_t *c) { pa_pline_init(c); }
void rnd_poly_contour_del(rnd_pline_t **c) { pa_pline_free(c); }
void rnd_poly_contour_pre(rnd_pline_t *c, rnd_bool optimize) { pa_pline_update(c, optimize); }
void rnd_poly_contours_free(rnd_pline_t **pl) { rnd_poly_plines_free(pl); }
rnd_bool rnd_poly_contour_copy(rnd_pline_t **dst, const rnd_pline_t *src) { return pa_pline_alloc_copy(dst, src); }
void rnd_poly_contour_inv(rnd_pline_t *c) { pa_pline_invert(c); }
int rnd_poly_contour_inside(const rnd_pline_t *c, rnd_vector_t v) { return pa_pline_is_point_inside(c, v); }
