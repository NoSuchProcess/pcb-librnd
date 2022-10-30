#include <librnd/core/conf_hid.h>
#include "topwin.h"

typedef struct {
	mbtk_widget_t *widget;    /* for most uses */
	mbtk_widget_t *destroy;   /* destroy this */
/*	GtkAction *action;        for removing from the central lists */
} menu_handle_t;

static menu_handle_t *handle_alloc(mbtk_widget_t *widget, mbtk_widget_t *destroy, void *action)
{
	menu_handle_t *m = malloc(sizeof(menu_handle_t));
	m->widget = widget;
	m->destroy = destroy;
TODO("what do we need this for?");
/*	m->action = action;*/
	return m;
}

/* Add a menu row in the menu system; if ins_after is not NULL, insert after that,
   else append at the end of parent */
static void menu_row_add(mbtk_box_t *row, mbtk_widget_t *parent, mbtk_widget_t *ins_after)
{
	if (ins_after != NULL)
		mbtk_menu_add_at(ins_after, &row->w, 0);
	else
		mbtk_menu_append((mbtk_menu_t *)parent, &row->w);
}

static mbtk_widget_t *ins_submenu(rnd_mbtk_t *mctx, const char *menu_label, mbtk_menu_t *submenu, mbtk_widget_t *parent, lht_node_t *ins_after)
{
	mbtk_widget_t *ins_after_w = (ins_after == NULL) ? NULL : ins_after->user_data;
	lht_dom_iterator_t it;
	mbtk_kw_t kw_menu_bar;

	mbtk_kw_cache(kw_menu_bar, "menu_bar");

	if (parent->type != kw_menu_bar) {
		mbtk_box_t *item;

		if (ins_after == rnd_hid_menu_ins_as_first)
			ins_after = (mbtk_widget_t *)parent->hvbox.ch_first; /* first child is always the tear-off */

		item = mbtk_menu_build_label_submenu_stfrow(menu_label);
		menu_row_add(item, parent, ins_after_w);
		mbtk_menu_item_attach_submenu(&item->w, submenu);
		return &item->w;
	}
	else
		return mbtk_menu_bar_add_at_label(ins_after_w, menu_label, submenu, 0);
}

static void rnd_mbtk_main_menu_real_add_node(rnd_mbtk_t *mctx, mbtk_widget_t *parent, lht_node_t *ins_after, lht_node_t *base);

/* create a menu row and return the box */
static mbtk_box_t *rnd_mbtk_menu_create_(rnd_mbtk_t *mctx, mbtk_widget_t *parent, lht_node_t *ins_after, lht_node_t *sub_res)
{
	const char *menu_label;
	mbtk_box_t *item;
	char *accel = NULL;
	mbtk_widget_t *ins_after_w = (ins_after == NULL) ? NULL : ins_after->user_data;
	lht_node_t *n_action = rnd_hid_cfg_menu_field(sub_res, RND_MF_ACTION, NULL);
	lht_node_t *n_keydesc = rnd_hid_cfg_menu_field(sub_res, RND_MF_ACCELERATOR, NULL);


	/* Resolve accelerator and save it */
	if (n_keydesc != NULL) {
		if (n_action != NULL) {
			rnd_hid_cfg_keys_add_by_desc(&rnd_mbtk_keymap, n_keydesc, n_action);
			accel = rnd_hid_cfg_keys_gen_accel(&rnd_mbtk_keymap, n_keydesc, 1, NULL);
		}
		else
			rnd_hid_cfg_error(sub_res, "No action specified for key accel\n");
	}

	/* Resolve menu name */
	menu_label = sub_res->name;

	if (rnd_hid_cfg_has_submenus(sub_res)) {
		/* SUBMENU */
		mbtk_menu_t *submenu = mbtk_menu_new(NULL);
		lht_node_t *n;

		item = ins_submenu(mctx, menu_label, submenu, parent, ins_after);
		sub_res->user_data = handle_alloc(item, item, NULL);


		TODO("make tear-off work");
		mbtk_menu_append_label((mbtk_menu_t *)parent, " > > > ");

		/* recurse on the newly-added submenu; rnd_hid_cfg_has_submenus() makes sure
		   the node format is correct; iterate over the list of submenus and create
		   them recursively. */
		n = rnd_hid_cfg_menu_field(sub_res, RND_MF_SUBMENU, NULL);
		for (n = n->data.list.first; n != NULL; n = n->next)
			rnd_mbtk_main_menu_real_add_node(mctx, submenu, NULL, n);
	}
	else {
		/* NON-SUBMENU: MENU ITEM */
		const char *checked = rnd_hid_cfg_menu_field_str(sub_res, RND_MF_CHECKED);
		const char *update_on = rnd_hid_cfg_menu_field_str(sub_res, RND_MF_UPDATE_ON);
		const char *label = rnd_hid_cfg_menu_field_str(sub_res, RND_MF_SENSITIVE);
		const char *tip = rnd_hid_cfg_menu_field_str(sub_res, RND_MF_TIP);

		if (checked) {
			/* TOGGLE ITEM */
			rnd_conf_native_t *nat = NULL;

			TODO("we are not drawing radio: strchr(checked, '=')");
			/* checked=foo       is a binary flag (checkbox)
			 * checked=foo=bar   is a flag compared to a value (radio) */

			item = mbtk_menu_build_label_full_stdrow(1, menu_label, accel);
			menu_row_add(item, parent, ins_after_w);
			sub_res->user_data = handle_alloc(item, item, NULL);

			TODO("tooltip: attach tip");

			if (update_on != NULL)
				nat = rnd_conf_get_field(update_on);
			else
				nat = rnd_conf_get_field(checked);

			if (nat != NULL) {
				static rnd_conf_hid_callbacks_t cbs;
				static int cbs_inited = 0;
				if (!cbs_inited) {
					memset(&cbs, 0, sizeof(rnd_conf_hid_callbacks_t));
					cbs.val_change_post = mctx->confchg_checkbox;
					cbs_inited = 1;
				}
				rnd_conf_hid_set_cb(nat, mctx->rnd_mbtk_menuconf_id, &cbs);
			}
			else {
				if ((update_on == NULL) || (*update_on != '\0'))
					rnd_message(RND_MSG_WARNING, "Checkbox menu item %s not updated on any conf change - try to use the update_on field\n", checked);
			}
		}
		else if (label && strcmp(label, "false") == 0) {
			/* INSENSITIVE ITEM */
			item = mbtk_menu_build_label_full_stdrow(1, menu_label, NULL);
			menu_row_add(item, parent, ins_after_w);
			TODO("tooltip: attach tip");

			TODO("set insensitive");
/*			gtk_widget_set_sensitive(item, FALSE);*/

			sub_res->user_data = handle_alloc(item, item, NULL);
		}
		else {
			/* NORMAL ITEM */
			item = mbtk_menu_build_label_full_stdrow(1, menu_label, accel);
			menu_row_add(item, parent, ins_after_w);
			TODO("tooltip: attach tip");
#if 0
			if ((tip != NULL) || (n_keydesc != NULL)) {
				char *acc = NULL, *s;
				if (n_keydesc != NULL)
					acc = rnd_hid_cfg_keys_gen_accel(&rnd_mbtk_keymap, n_keydesc, -1, "\nhotkey: ");
				s = rnd_concat((tip == NULL ? "" : tip), "\nhotkey: ", (acc == NULL ? "" : acc), NULL);
				gtk_widget_set_tooltip_text(item, s);
				free(s);
				free(acc);
			}
#endif

			sub_res->user_data = handle_alloc(item, item, NULL);
		}
	}

TODO("do this for CHECKED only");
#if 0
	/* By now this runs only for toggle items. */
	if (action) {
		mbtk_widget_t *item = rnd_gtk_menu_item_new(menu_label, accel, TRUE);

		g_signal_connect(G_OBJECT(action), "activate", menu->action_cb, (gpointer)n_action);
		g_object_set_data(G_OBJECT(action), "resource", (gpointer)sub_res);
		g_object_set(item, "use-action-appearance", FALSE, NULL);
		g_object_set(item, "related-action", action, NULL);
		ins_menu(item, shell, ins_after);
		menu->actions = g_list_append(menu->actions, action);
		sub_res->user_data = handle_alloc(item, item, action);
	}
#endif

	free(accel);
	return item;
}


/* Translate a lihata tree into an mbtk menu structure */
static void rnd_mbtk_main_menu_real_add_node(rnd_mbtk_t *mctx, mbtk_widget_t *parent, lht_node_t *ins_after, lht_node_t *base)
{
	switch (base->type) {
		case LHT_HASH:                /* leaf submenu */
			rnd_mbtk_menu_create_(mctx, parent, ins_after, base);
			break;

		case LHT_TEXT:                /* separator or anchor */
		{
TODO("implement sep");
#if 0
			if ((strcmp(base->data.text.value, "sep") == 0) || (strcmp(base->data.text.value, "-") == 0)) {
				mbtk_widget_t *item = gtk_separator_menu_item_new();
				ins_menu(item, shell, ins_after);
				base->user_data = handle_alloc(item, item, NULL);
			}
			else if (base->data.text.value[0] == '@') {
				/* anchor; ignore */
			}
			else
				rnd_hid_cfg_error(base, "Unexpected text node; the only text accepted here is sep, -, or @\n");
#endif
		}
		break;
		default:
			rnd_hid_cfg_error(base, "Unexpected node type; should be hash (submenu) or text (separator or @special)\n");
	}
}

static void rnd_mbtk_main_menu_add_node(rnd_mbtk_t *mctx, const lht_node_t *base)
{
	lht_node_t *n;
	if (base->type != LHT_LIST) {
		rnd_hid_cfg_error(base, "Menu description shall be a list (li)\n");
		abort();
	}
	for(n = base->data.list.first; n != NULL; n = n->next)
		rnd_mbtk_main_menu_real_add_node(mctx, &mctx->topwin->menu_bar, NULL, n);
}

static void rnd_mbtk_main_menu_add_popup_node(rnd_mbtk_t *mctx, lht_node_t *base)
{
TODO("Figure how to do popups with mbtk");
#if 0
	lht_node_t *submenu, *i;
	mbtk_widget_t *new_menu;

	submenu = rnd_hid_cfg_menu_field_path(base, "submenu");
	if (submenu == NULL) {
		rnd_hid_cfg_error(base, "can not create popup without submenu list\n");
		return;
	}

	new_menu = mbtk_submenu_new();
	g_object_ref_sink(new_menu);
	base->user_data = handle_alloc(new_menu, new_menu, NULL);

	for (i = submenu->data.list.first; i != NULL; i = i->next)
		rnd_mbtk_main_menu_real_add_node(ctx, new_menu, NULL, i);
#endif
}


int rnd_mbtk_load_menus(rnd_mbtk_t *mctx)
{
	const lht_node_t *mr;

	rnd_hid_menu_gui_ready_to_create(rnd_gui);

	mr = rnd_hid_cfg_get_menu(rnd_gui->menu, "/main_menu");
	if (mr != NULL) {
		rnd_mbtk_main_menu_add_node(mctx, mr);
		mr->doc->root->user_data = mctx;
	}

	mr = rnd_hid_cfg_get_menu(rnd_gui->menu, "/popups");
	if (mr != NULL) {
		if (mr->type == LHT_LIST) {
			lht_node_t *n;
			for (n = mr->data.list.first; n != NULL; n = n->next)
				rnd_mbtk_main_menu_add_popup_node(mctx, n);
		}
		else
			rnd_hid_cfg_error(mr, "/popups should be a list\n");
		mr->doc->root->user_data = mctx;
	}

	mr = rnd_hid_cfg_get_menu(rnd_gui->menu, "/mouse");
	if (rnd_hid_cfg_mouse_init(rnd_gui->menu, &rnd_mbtk_mouse) != 0)
		rnd_message(RND_MSG_ERROR, "Error: failed to load mouse actions from the hid config lihata - mouse input will not work.");

	rnd_hid_menu_gui_ready_to_modify(rnd_gui);

	return 0;
}
