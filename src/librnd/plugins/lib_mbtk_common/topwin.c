struct rnd_mbtk_topwin_s {
	mbtk_window_t *win;
	void *drawing_area;
};

static void rnd_mbtk_alloc_topwin(rnd_mbtk_t *mctx)
{
	mctx->topwin = calloc(sizeof(rnd_mbtk_topwin_t), 1);
}

static void rnd_mbtk_populate_topwin(rnd_mbtk_t *mctx)
{
	TODO("implement");
}