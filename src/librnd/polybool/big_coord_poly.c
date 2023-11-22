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

void pa_pline_optimize(rnd_pline_t *pl);


/* This variant is called from pa_coll_gather() when collecting high
   resolution polylines into small resolution output polylines. Each
   input vertex abuses it's ->cvc_next to remember the input vertex
   it was rounded from, so the original (high resolution) area can
   be calculated and the original oritentation can be returned. This
   is then used on caller's side to evade a nasty corner case of triangle
   flipping direction due to ronded corner jumping over an edge. */
int pa_pline_update_big2small(rnd_pline_t *pl)
{
	double area = 0;
	pa_big2_coord_t big_area = {0};
	rnd_vnode_t *p, *c; /* previous and current node in the iteration */
	int res_ori;

	assert(pl != NULL);

	pa_pline_optimize(pl);


	/* Update count and bbox and calculate area */
	pl->Count = 0;
	pl->xmin = pl->xmax = pl->head->point[0];
	pl->ymin = pl->ymax = pl->head->point[1];
	p = (c = pl->head)->prev;
	if (c != p) {
		rnd_vnode_t *big_curr, *big_prev = (rnd_vnode_t *)c->prev->LINK_BACK;
		do {
			pa_big2_coord_t a;
			pa_big_vector_t vb_curr, vb_prev;
			pa_big_coord_t dx, dy;

			/* calculate area for orientation (small) */
			area += (double)(p->point[0] - c->point[0]) * (double)(p->point[1] + c->point[1]);
			pa_pline_box_bump(pl, c->point);
			pl->Count++;

			/* calculate area for orientation (big) */
			big_curr = (rnd_vnode_t *)c->LINK_BACK; /* ->cvclst_next is abused to store a link back to the input, high resolution node */
			c->LINK_BACK = NULL;

			rnd_pa_big_load_cvc(&vb_prev, big_prev);
			rnd_pa_big_load_cvc(&vb_curr, big_curr);

			big_subn(dx, vb_prev.x, vb_curr.x, W, 0);
			big_addn(dy, vb_prev.y, vb_curr.y, W, 0);
			big_signed_mul(a, W2, dx, dy, W);
			big_addn(big_area, big_area, a, W2, 0);

			big_prev = big_curr;
		}
		while ((c = (p = c)->next) != pl->head);
	}
	else
		c->LINK_BACK = NULL;
	pl->area = RND_ABS(area);

	/* inverse orientation results in negative area */
	if (pl->Count > 2)
		pl->flg.orient = ((area < 0) ? RND_PLF_INV : RND_PLF_DIR);

	res_ori = big_sgn(big_area, W2);

	pl->tree = rnd_poly_make_edge_tree(pl);
	return res_ori;
}
