struct rnd_mbtk_topwin_s {
	mbtk_window_t *win;

	/*** widgets (local allocation) ***/

	/* top hbox with menu, toolbar and readouts */
	mbtk_box_t vbox_main, top_spring;
	mbtk_box_t top_bar_background, top_hbox, menu_hbox, menubar_toolbar_vbox;
	mbtk_menu_bar_t menu_bar;
	mbtk_box_t position_hbox;

	mbtk_pane_t hpaned_middle;
	mbtk_box_t left_toolbar, vbox_middle;

	void *drawing_area;

	/* docking */
	mbtk_box_t dockbox[RND_HID_DOCK_max];
	gdl_list_t dock[RND_HID_DOCK_max];
};

static void rnd_mbtk_topwin_alloc(rnd_mbtk_t *mctx)
{
	mctx->topwin = calloc(sizeof(rnd_mbtk_topwin_t), 1);
}

static void rnd_mbtk_populate_topwin(rnd_mbtk_t *mctx)
{
	rnd_mbtk_topwin_t *tw = mctx->topwin;
	

	mbtk_vbox_new(&tw->vbox_main);
	mbtk_window_set_root_widget(tw->win, &tw->vbox_main.w);

	/*** top control bar ***/
	mbtk_hbox_new(&tw->top_bar_background);
	mbtk_box_add_widget(&tw->vbox_main, &tw->top_bar_background.w, 0);

	mbtk_hbox_new(&tw->top_hbox);
	mbtk_box_set_span(&tw->top_hbox, HVBOX_FILL);
	mbtk_box_add_widget(&tw->top_bar_background, &tw->top_hbox.w, 0);

	/* menu_hbox will be disabled when the GUI waits for a get_coords */
	mbtk_hbox_new(&tw->menu_hbox);
	mbtk_box_add_widget(&tw->top_hbox, &tw->menu_hbox.w, 0);

	mbtk_vbox_new(&tw->menubar_toolbar_vbox);
	mbtk_box_add_widget(&tw->menu_hbox, &tw->menubar_toolbar_vbox.w, 0);

	/* top left: main menu and toolbar */
	mbtk_menu_bar_new(&tw->menu_bar);
	mbtk_box_add_widget(&tw->menubar_toolbar_vbox, &tw->menu_bar.w, 0);

	mbtk_hbox_new(&tw->dockbox[RND_HID_DOCK_TOP_LEFT]);
	mbtk_box_add_widget(&tw->menubar_toolbar_vbox, &tw->dockbox[RND_HID_DOCK_TOP_LEFT].w, 0);

	/* pushes the top right position box to the right */
	mbtk_hbox_new(&tw->top_spring);
	mbtk_box_set_span(&tw->top_spring, HVBOX_FILL);
	mbtk_box_add_widget(&tw->top_hbox, &tw->top_spring.w, 0);

	/* top right */
	mbtk_hbox_new(&tw->position_hbox);
	mbtk_box_add_widget(&tw->top_hbox, &tw->position_hbox.w, 0);

	mbtk_vbox_new(&tw->dockbox[RND_HID_DOCK_TOP_RIGHT]);
	mbtk_box_add_widget(&tw->position_hbox, &tw->dockbox[RND_HID_DOCK_TOP_RIGHT].w, 0);

	/*** middle section ***/
	mbtk_pane_new(&tw->hpaned_middle, 1);
	mbtk_pane_set_span(&tw->hpaned_middle, HVBOX_FILL);
	mbtk_box_add_widget(&tw->vbox_main, &tw->hpaned_middle.w, 0);

	/* the left toolbar box will be disabled in get_coords */
	mbtk_vbox_new(&tw->left_toolbar);
	mbtk_pane_set_child(&tw->hpaned_middle, 0, &tw->left_toolbar.w);

	mbtk_vbox_new(&tw->dockbox[RND_HID_DOCK_LEFT]);
	mbtk_box_set_span(&tw->dockbox[RND_HID_DOCK_LEFT], HVBOX_FILL);
	mbtk_box_add_widget(&tw->left_toolbar, &tw->dockbox[RND_HID_DOCK_LEFT], 0);

	/* right side is our main content: drawing area with scroll bars */
	mbtk_vbox_new(&tw->vbox_middle);
	mbtk_pane_set_child(&tw->hpaned_middle, 1, &tw->vbox_middle.w);

}

static void rnd_mbtk_topwin_free(rnd_mbtk_t *mctx)
{
	free(mctx->topwin);
	mctx->topwin = NULL;
}
