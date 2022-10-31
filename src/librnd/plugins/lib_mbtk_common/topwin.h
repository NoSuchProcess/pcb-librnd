#ifndef LIBRND_LIB_MBTK_COMMON_TOPWIN
#define LIBRND_LIB_MBTK_COMMON_TOPWIN

#include <libmbtk/widget.h>

struct rnd_mbtk_topwin_s {
	mbtk_window_t *win;

	unsigned active:1;

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

	mbtk_box_t bottom_hbox;
	mbtk_label_t cmd_prompt, cmd;

	/* docking */
	mbtk_box_t dockbox[RND_HID_DOCK_max];
	gdl_list_t dock[RND_HID_DOCK_max];

	/* menu system */
	gdl_list_t menu_chk;  /* list of menu_handle_t that need to be updated for checkbox changes */
};

#endif
