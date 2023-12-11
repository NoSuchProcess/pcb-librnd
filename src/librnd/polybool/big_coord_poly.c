
/* Incremental calculation of area with big coords */
void pa_big_area_incremental(pa_big2_coord_t big_area, rnd_vnode_t *big_curr, rnd_vnode_t *big_prev)
{
	pa_big2_coord_t a;
	pa_big_vector_t vb_curr, vb_prev;
	pa_big_coord_t dx, dy;

	rnd_pa_big_load_cvc(&vb_prev, big_prev);
	rnd_pa_big_load_cvc(&vb_curr, big_curr);

	big_subn(dx, vb_prev.x, vb_curr.x, W, 0);
	big_addn(dy, vb_prev.y, vb_curr.y, W, 0);
	big_signed_mul(a, W2, dx, dy, W);
	big_addn(big_area, big_area, a, W2, 0);
}

int pa_big_area2ori(pa_big2_coord_t big_area)
{
	return (big_sgn(big_area, W2) < 0) ? RND_PLF_INV : RND_PLF_DIR;
}

