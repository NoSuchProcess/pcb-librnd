/* returns 1 if sa and sb are in full overlap */
RND_INLINE int seg_seg_olap(pb2_seg_t *sa, pb2_seg_t *sb)
{
	TODO("arc: this is only for line at the moment");
	if (Vequ2(sa->start, sb->start) && Vequ2(sa->end, sb->end))
		return 1;
	if (Vequ2(sa->start, sb->end) && Vequ2(sa->end, sb->start))
		return 1;
	return 0;
}

/* swap start and end of a segment (reverse orientation); rtree doesn't change */
RND_INLINE void seg_reverse(pb2_seg_t *s)
{
	rnd_swap(rnd_coord_t, s->start[0], s->end[0]);
	rnd_swap(rnd_coord_t, s->start[1], s->end[1]);
	TODO("arc: also need to swap angles");
}

RND_INLINE void seg_angle_from(pa_angle_t *dst, pb2_seg_t *seg, rnd_vector_t from)
{
	TODO("bignum: this is not correct: for 64 bits we need higher precision angles than doubles");
	if (Vequ2(seg->start, from))
		pa_calc_angle_nn(dst, from, seg->end);
	else
		pa_calc_angle_nn(dst, from, seg->start);
}

/* vector slope used to compare direction vector to segment in step 3 point-in-poly */
typedef struct {
	rnd_coord_t dx, dy;
	double s;
} pb2_slope_t;

RND_INLINE pb2_slope_t pb2_slope(const rnd_vector_t origin, const rnd_vector_t other)
{
	pb2_slope_t slp;

	slp.dx = (other[0] - origin[0]);
	slp.dy = (other[1] - origin[1]);
	TODO("bignum: double is not enough for 64 bit coords; this large number should be inf really");
	if (slp.dx != 0)
		slp.s = (double)slp.dy / (double)slp.dx;
	else
		slp.s = 0;
	return slp;
}

RND_INLINE int PB2_SLOPE_LT(pb2_slope_t a, pb2_slope_t b)
{
	/* slope value is invalid for vertical vectors */
	assert(a.dx != 0);
	assert(b.dx != 0);
	return a.s < b.s;
}

