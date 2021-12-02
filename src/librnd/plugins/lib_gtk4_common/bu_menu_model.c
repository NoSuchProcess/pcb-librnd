typedef enum { /* bitfield */
	OM_FLAG_CHKBOX = 1
} open_menu_flag_t;

typedef struct {
	lht_node_t *parent;       /* the menu node that this popover/dialog is open for */
	GtkWidget *popwin;        /* host popover or window/dialog - this one needs to be popped down to close this instance */
	GtkWidget *lbox;
	vtp0_t mnd;               /* lht_node_t * for each menu item as indexed in the dialog; [0] is for the tear-off and contains the menu ctx */
	vti0_t flag;              /* open_menu_flag_t for each menu item */
	unsigned int floating:1;  /* tear-off menu; 0=popover, 1=non-modal dialog */
	unsigned int ctx_popup:1; /* context popup menu; 0=normal, 1=context popup; context popups can not be teared off as they should be modal */
	gdl_elem_t link;          /* in list of all open menus */
} open_menu_t;

static gdl_list_t open_menu;

static open_menu_t *gtkc_open_menu_new(lht_node_t *parent, GtkWidget *popwin, GtkWidget *lbox, int floating, int ctx_popup)
{
	open_menu_t *om = calloc(sizeof(open_menu_t), 1);

	om->parent = parent;
	om->floating = floating;
	om->ctx_popup = ctx_popup;
	om->popwin = popwin;
	om->lbox = lbox;

	gdl_append(&open_menu, om, link);
	return om;
}

static void gtkc_open_menu_del(open_menu_t *om)
{
	vtp0_uninit(&om->mnd);
	vti0_uninit(&om->flag);
	gdl_remove(&open_menu, om, link);
	free(om);
}


