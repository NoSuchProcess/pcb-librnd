/* author: Igor2 */
void rnd_polyarea_get_tree_seg(void *obj, rnd_coord_t *x1, rnd_coord_t *y1, rnd_coord_t *x2, rnd_coord_t *y2)
{
	pa_seg_t *s = obj;
	*x1 = s->v->point[0];
	*x2 = s->v->next->point[0];
	*y1 = s->v->point[1];
	*y2 = s->v->next->point[1];
}
