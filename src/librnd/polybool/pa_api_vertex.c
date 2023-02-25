void rnd_poly_vertex_exclude(rnd_pline_t *parent, rnd_vnode_t * node)
{
	assert(node != NULL);
	if (parent != NULL) {
		if (parent->head == node) /* if node is excluded from a pline, it can not remain the head */
			parent->head = node->next;
	}
	if (node->cvc_next) {
		free(node->cvc_next);
		free(node->cvc_prev);
	}
	node->prev->next = node->next;
	node->next->prev = node->prev;

	if (parent != NULL) {
		if (parent->head == node) /* special case: removing head which was the last node in pline  */
			parent->head = NULL;
	}
}

RND_INLINE void rnd_poly_vertex_include_force_(rnd_vnode_t *after, rnd_vnode_t *node)
{
	assert(after != NULL);
	assert(node != NULL);

	node->prev = after;
	node->next = after->next;
	after->next = after->next->prev = node;
}

void rnd_poly_vertex_include_force(rnd_vnode_t *after, rnd_vnode_t *node)
{
	rnd_poly_vertex_include_force_(after, node);
}

void rnd_poly_vertex_include(rnd_vnode_t *after, rnd_vnode_t *node)
{
	double a, b;

	rnd_poly_vertex_include_force_(after, node);

	/* remove points on same line */
	if (node->prev->prev == node)
		return;											/* we don't have 3 points in the poly yet */
	a = (node->point[1] - node->prev->prev->point[1]);
	a *= (node->prev->point[0] - node->prev->prev->point[0]);
	b = (node->point[0] - node->prev->prev->point[0]);
	b *= (node->prev->point[1] - node->prev->prev->point[1]);
	if (fabs(a - b) < EPSILON) {
		rnd_vnode_t *t = node->prev;
		t->prev->next = node;
		node->prev = t->prev;
		free(t);
	}
}
