typedef struct {
	lht_node_t *parent;       /* the menu node that this popover/dialog is open for */
	GtkWidget *popwin;        /* host popover or window/dialog - this one needs to be popped down to close this instance */
	GtkWidget *lbox;
	vtp0_t mnd;               /* lht_node_t * for each menu item as indexed in the dialog */
	unsigned int floating:1;  /* tear-off menu; 0=popover, 1=non-modal dialog */
	gdl_elem_t link;          /* in list of all open menus */
} open_menu_t;

static gdl_list_t open_menu;

static open_menu_t *gtkc_open_menu_new(lht_node_t *parent, GtkWidget *popwin, GtkWidget *lbox, int floating)
{
	open_menu_t *om = calloc(sizeof(open_menu_t), 1);

	om->parent = parent;
	om->floating = floating;
	om->popwin = popwin;
	om->lbox = lbox;

	gdl_append(&open_menu, om, link);
	return om;
}

static void gtkc_open_menu_del(open_menu_t *om)
{
	vtp0_uninit(&om->mnd);
	gdl_remove(&open_menu, om, link);
	free(om);
}


