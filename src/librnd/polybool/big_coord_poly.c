int pa_big_array_area(pa_big2_coord_t dst, rnd_vnode_t **nds, long len)
{
	long n;

	memset(dst, 0, sizeof(pa_big2_coord_t));

	for(n = 0; n < len; n++) {
		pa_big_vector_t curr, prev;
		pa_big_coord_t dx, dy;
		pa_big2_coord_t a;

		rnd_pa_big_load_cvc(&prev, nds[n]);
		rnd_pa_big_load_cvc(&curr, nds[(n+1) % len]);

		big_subn(dx, prev.x, curr.x, W, 0);
		big_addn(dy, prev.y, curr.y, W, 0);
		big_signed_mul(a, W2, dx, dy, W);
		big_addn(dst, dst, a, W2, 0);
	}

	return big_sgn(dst, W2);
}

