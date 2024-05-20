int rnd_vertices_are_coaxial(rnd_vnode_t *node);

RND_INLINE rnd_vnode_t *pl_append_xy(rnd_pline_t *dst, rnd_coord_t x, rnd_coord_t y)
{
	rnd_vnode_t *vn = calloc(sizeof(rnd_vnode_t), 1);

	vn->point[0] = x;
	vn->point[1] = y;

	if (dst->head != NULL) {
		vn->next = dst->head;
		vn->prev = dst->head->prev;
		dst->head->prev->next = vn;
		dst->head->prev = vn;
	}
	else {
		vn->next = vn->prev = vn;
		dst->head = vn;
	}

	return vn;
}

rnd_pline_t *rnd_pline_dup_with_offset_round(const rnd_pline_t *src, rnd_coord_t offs)
{
	const rnd_vnode_t *curr, *next;
	rnd_vnode_t *vnew;
	rnd_pline_t *dst = calloc(sizeof(rnd_pline_t), 1);
	rnd_vector_t isc1, isc2;

	if (dst == NULL)
		return NULL;

	/* offset each edge also keeping the original corners but marked */
	curr = src->head;
	do {
		double vx, vy, nx, ny, len;

		if ((curr->point[0] == curr->next->point[0]) && (curr->point[1] == curr->next->point[1]))
			continue;

		if (rnd_vertices_are_coaxial(curr->next))
			continue;

		vnew = pl_append_xy(dst, curr->point[0], curr->point[1]);
		vnew->flg.mark = 1;

		vx = curr->next->point[0] - curr->point[0];
		vy = curr->next->point[1] - curr->point[1];
		len = sqrt(vx*vx + vy*vy);
		vx /= len;
		vy /= len;
		nx = (-vy) * -(double)offs;
		ny = (vx) * -(double)offs;
		pl_append_xy(dst, curr->point[0] + nx, curr->point[1] + ny);
		pl_append_xy(dst, curr->next->point[0] + nx, curr->next->point[1] + ny);
	} while((curr = curr->next) != src->head);


	/* the next section doesn't work correctly when starting on a marked node
	   (because of all the node removals) */
	if (dst->head->flg.mark)
		dst->head = dst->head->next;

	/* visit original corners (marked) and check if they are now the center of
	   a round outer corner (convex) or an external point on an inverted corner
	   (concave) */
	curr = dst->head;
	do {
		next = curr->next;

		if (curr->flg.mark) {
			if (rnd_vect_inters2(curr->prev->prev->point, curr->prev->point, curr->next->point, curr->next->next->point, isc1, isc2) == 1) {
				/* prev and next seg intersect -> concave */
				rnd_poly_vertex_exclude(dst, (rnd_vnode_t *)curr);
				rnd_poly_vertex_exclude(dst, (rnd_vnode_t *)curr->prev);
				curr->next->point[0] = isc1[0];
				curr->next->point[1] = isc1[1];
			}
			else {
				/* convex: add a rounded corner */
				rnd_poly_vertex_exclude(dst, (rnd_vnode_t *)curr);
			}
		}

	} while((curr = next) != dst->head);


	pa_pline_update(dst, 1);
	return dst;
}
