struct rnd_mbtk_topwin_s {
	mbtk_window_t *win;

	/*** widgets (local allocation) ***/

	/* top hbox with menu, toolbar and readouts */
	mbtk_box_t vbox_main, top_spring;
	mbtk_box_t top_bar_background, top_hbox, menu_hbox, menubar_toolbar_vbox;
	mbtk_menu_bar_t menu_bar;
	mbtk_box_t position_hbox;

	mbtk_pane_t hpaned_middle;
	mbtk_box_t left_toolbar, vbox_middle, info_hbox, drw_hbox1, drw_hbox2;
	mbtk_canvas_native_t drawing_area;
	mbtk_scrollbar_t vscroll, hscroll;

	/* docking */
	mbtk_box_t dockbox[RND_HID_DOCK_max];
	gdl_list_t dock[RND_HID_DOCK_max];
};

static void rnd_mbtk_topwin_alloc(rnd_mbtk_t *mctx)
{
	mctx->topwin = calloc(sizeof(rnd_mbtk_topwin_t), 1);
}

static const unsigned long rnd_dock_color[RND_HID_DOCK_max] = {0, 0, 0xffaa33ffUL, 0, 0, 0}; /* force change color when docked */

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

	/*** right side is our main content: drawing area with scroll bars ***/
	mbtk_vbox_new(&tw->vbox_middle);
	mbtk_box_set_span(&tw->vbox_middle, HVBOX_FILL);
	mbtk_pane_set_child(&tw->hpaned_middle, 1, &tw->vbox_middle.w);

	/* first the info box */
	mbtk_hbox_new(&tw->info_hbox);
	mbtk_box_add_widget(&tw->vbox_middle, &tw->info_hbox, 0);

	if (rnd_dock_color[RND_HID_DOCK_TOP_INFOBAR] != 0)
		mbtk_box_style_set_prop(&tw->info_hbox, mbtk_kw("bg.color"), mbtk_arg_long(rnd_dock_color[RND_HID_DOCK_TOP_INFOBAR]));

#ifdef TODO_REAL_CODE
	mbtk_hbox_new(&tw->dockbox[RND_HID_DOCK_TOP_INFOBAR]);
	mbtk_box_set_span(&tw->dockbox[RND_HID_DOCK_TOP_INFOBAR], HVBOX_FILL);
	mbtk_box_add_widget(&tw->info_hbox, &tw->dockbox[RND_HID_DOCK_TOP_INFOBAR], 0);
#else
	TODO("remove this");
	mbtk_box_add_widget(&tw->info_hbox, mbtk_label_new(NULL, "info-box!"),0);
#endif

	/* the drawing area is in a hbox with a vertical scrollbar */
	mbtk_hbox_new(&tw->drw_hbox1);
	mbtk_box_set_span(&tw->drw_hbox1, HVBOX_FILL);
	mbtk_box_add_widget(&tw->vbox_middle, &tw->drw_hbox1.w, 0);

	mbtk_canvas_native_new(&tw->drawing_area);
	mbtk_box_set_span(&tw->drawing_area, HVBOX_FILL);
	mbtk_box_add_widget(&tw->drw_hbox1, &tw->drawing_area.w, 0);

	mbtk_scrollbar_new(&tw->vscroll, 0);
	mbtk_box_add_widget(&tw->drw_hbox1, &tw->vscroll.w, 0);

	/* horizontal scrollbar and fullscreen button in another hbox */
	mbtk_hbox_new(&tw->drw_hbox2);
mbtk_box_set_span(&tw->drw_hbox1, HVBOX_FILL);
	mbtk_box_add_widget(&tw->vbox_middle, &tw->drw_hbox2.w, 0);

	mbtk_scrollbar_new(&tw->hscroll, 1);
	mbtk_scrollbar_set_span(&tw->hscroll, HVBOX_FILL);
	mbtk_box_add_widget(&tw->drw_hbox2, &tw->hscroll.w, 0);

	TODO("switch over to static allocation, use full screen XPM");
	{
		mbtk_button_t *btn = mbtk_button_new_with_label("FS");
		mbtk_box_add_widget(&tw->drw_hbox2, &btn->w, 0);
	}


}

static void rnd_mbtk_topwin_free(rnd_mbtk_t *mctx)
{
	free(mctx->topwin);
	mctx->topwin = NULL;
}
