rnd_pline_t *rnd_poly_contour_new(const rnd_vector_t v) { return pa_pline_new(v); }
void rnd_poly_contour_init(rnd_pline_t *c) { pa_pline_init(c); }
void rnd_poly_contour_del(rnd_pline_t **c) { pa_pline_free(c); }
void rnd_poly_contour_pre(rnd_pline_t *c, rnd_bool optimize) { pa_pline_update(c, optimize); }
void rnd_poly_contours_free(rnd_pline_t **pl) { rnd_poly_plines_free(pl); }
rnd_bool rnd_poly_contour_copy(rnd_pline_t **dst, rnd_pline_t *src) { return pa_pline_alloc_copy(dst, src); }
void rnd_poly_contour_inv(rnd_pline_t *c) { pa_pline_invert(c); }
int rnd_poly_contour_inside(const rnd_pline_t *c, rnd_vector_t v) { return pa_pline_is_point_inside(c, v); }
int rnd_poly_contour_in_contour(rnd_pline_t *outer, rnd_pline_t *inner) { return pa_pline_inside_pline(outer, inner); }


rnd_bool rnd_polyarea_copy1(rnd_polyarea_t *dst, const rnd_polyarea_t *src) { return pa_polyarea_copy_plines(dst, src); }
rnd_bool rnd_polyarea_copy0(rnd_polyarea_t **dst, const rnd_polyarea_t *src) { return pa_polyarea_alloc_copy(dst, src); }
rnd_bool rnd_polyarea_m_copy0(rnd_polyarea_t **dst, const rnd_polyarea_t *srcfst) { return rnd_polyarea_alloc_copy_all(dst, srcfst); }
rnd_bool rnd_polyarea_contour_include(rnd_polyarea_t *p, rnd_pline_t *c) { return pa_polyarea_insert_pline(p, c); }
void rnd_polyarea_init(rnd_polyarea_t *p) { pa_polyarea_init(p); }
rnd_polyarea_t *rnd_polyarea_create(void) { return pa_polyarea_alloc(); }
void rnd_polyarea_free(rnd_polyarea_t **p) { pa_polyarea_free_all(p); }

