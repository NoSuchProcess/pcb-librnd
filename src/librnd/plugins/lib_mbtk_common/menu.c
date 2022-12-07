#include <librnd/core/conf_hid.h>
#include <librnd/core/hid_cfg_action.h>
#include "topwin.h"

static const char *cookie_menu = "mbtk hid menu";

typedef struct {
	mbtk_box_t *row;    /* menu row */
	lht_node_t *node;
	const char *checked, *active; /* cached menu field str lookups */
	lht_node_t *action;           /* cached menu action node */
	gdl_elem_t link; /* in the list of topwin->menu_chk */
} menu_handle_t;


static mbtk_event_handled_t rnd_mbtk_menu_cb(mbtk_widget_t *w, mbtk_kw_t id, void *user_data)
{
	menu_handle_t *hand = user_data;

	if (hand->action != NULL) {
		rnd_design_t *hidlib = rnd_multi_get_current();
		rnd_hid_cfg_action(hidlib, hand->action);

		/* GUI updates to reflect the result of the above action */
		if (rnd_app.adjust_attached_objects != NULL)
			rnd_app.adjust_attached_objects(hidlib);
		else
			rnd_tool_adjust_attached(hidlib);

		rnd_gui->invalidate_all(rnd_gui);
	}

	return MBTK_EVENT_HANDLED;
}

static menu_handle_t *handle_alloc(mbtk_box_t *row, lht_node_t *node)
{
	menu_handle_t *m = calloc(sizeof(menu_handle_t), 1);
	m->row = row;
	m->node = node;
	m->checked = rnd_hid_cfg_menu_field_str(node, RND_MF_CHECKED);
	m->action = rnd_hid_cfg_menu_field(node, RND_MF_ACTION, NULL);

	mbtk_menu_stdrow_callback_invoked(row, rnd_mbtk_menu_cb, m);

	row->w.user_data = m;
	node->user_data = m;

	return m;
}

/* update the checkbox of a menu item */
static void menu_chkbox_update(rnd_mbtk_t *mctx, menu_handle_t *m)
{
	rnd_design_t *hidlib = mctx->hidlib;

	if (m->checked != NULL) {
		int v = rnd_hid_get_flag(hidlib, m->checked);
		if (v < 0) {
			mbtk_menu_stdrow_checkbox_set(m->row, 0);
TODO("set inactive");
/*			gtk_action_set_sensitive(act, 0);*/
		}
		else
			mbtk_menu_stdrow_checkbox_set(m->row, !!v);
	}
	if (m->active != NULL) {
		int v = rnd_hid_get_flag(hidlib, m->active);
TODO("set active");
/*		gtk_action_set_sensitive(act, !!v);*/
	}
}


static void rnd_mbtk_menu_update_checkboxes(rnd_mbtk_t *mctx, rnd_design_t *hidlib)
{
	menu_handle_t *m;
	for(m = gdl_first(&mctx->topwin->menu_chk); m != NULL; m = gdl_next(&mctx->topwin->menu_chk, m))
		menu_chkbox_update(mctx, m);
}


TODO("librnd4: mbtk_glboal should come user_data, set it at registration");
static rnd_mbtk_t mbtk_global;
static void rnd_mbtk_confchg_checkbox(rnd_conf_native_t *cfg, int arr_idx, void *user_data)
{
	if ((mbtk_global.hid_active) && (mbtk_global.hidlib != NULL))
		rnd_mbtk_menu_update_checkboxes(&mbtk_global, mbtk_global.hidlib);
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

static mbtk_widget_t *ins_submenu(rnd_mbtk_t *mctx, const char *menu_label, mbtk_menu_t *submenu, mbtk_widget_t *parent, lht_node_t *ins_after, lht_node_t *node)
{
	mbtk_widget_t *ins_after_w = (ins_after == NULL) ? NULL : ins_after->user_data;
	lht_dom_iterator_t it;
	mbtk_kw_t kw_menu_bar;

	mbtk_kw_cache(kw_menu_bar, "menu_bar");

	if (parent->type != kw_menu_bar) {
		mbtk_box_t *item;

		if (ins_after == rnd_hid_menu_ins_as_first)
			ins_after_w = (mbtk_widget_t *)parent->hvbox.ch_first; /* first child is always the tear-off */

		item = mbtk_menu_build_label_submenu_stdrow(menu_label);
		menu_row_add(item, parent, ins_after_w);
		handle_alloc(item, node);
		mbtk_menu_item_attach_submenu(&item->w, submenu);
		return &item->w;
	}
	else {
		if (ins_after_w != NULL)
			return mbtk_menu_bar_add_at_label(ins_after_w, menu_label, submenu, 0);
		return mbtk_menu_bar_append_label(&mctx->topwin->menu_bar, menu_label, submenu);
	}
}

static void rnd_mbtk_main_menu_real_add_node(rnd_mbtk_t *mctx, mbtk_widget_t *parent, lht_node_t *ins_after, lht_node_t *base);

/* create a menu row and return the box */
static mbtk_box_t *rnd_mbtk_menu_create_(rnd_mbtk_t *mctx, mbtk_widget_t *parent, lht_node_t *ins_after, lht_node_t *sub_res)
{
	const char *menu_label;
	mbtk_box_t *row;
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

		ins_submenu(mctx, menu_label, submenu, parent, ins_after, sub_res);


		TODO("make tear-off work");
		mbtk_menu_append_label(submenu, " > > > ");

		/* recurse on the newly-added submenu; rnd_hid_cfg_has_submenus() makes sure
		   the node format is correct; iterate over the list of submenus and create
		   them recursively. */
		n = rnd_hid_cfg_menu_field(sub_res, RND_MF_SUBMENU, NULL);
		for (n = n->data.list.first; n != NULL; n = n->next)
			rnd_mbtk_main_menu_real_add_node(mctx, &submenu->w, NULL, n);
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
			menu_handle_t *hand;

			TODO("we are not drawing radio: strchr(checked, '=')");
			/* checked=foo       is a binary flag (checkbox)
			 * checked=foo=bar   is a flag compared to a value (radio) */

			row = mbtk_menu_build_label_full_stdrow(1, menu_label, accel);
			menu_row_add(row, parent, ins_after_w);
			hand = handle_alloc(row, sub_res);

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
					cbs.val_change_post = rnd_mbtk_confchg_checkbox;
					cbs_inited = 1;
				}
				rnd_conf_hid_set_cb(nat, mctx->topwin->menuconf_id, &cbs);
				gdl_append(&mctx->topwin->menu_chk, hand, link);
			}
			else {
				if ((update_on == NULL) || (*update_on != '\0'))
					rnd_message(RND_MSG_WARNING, "Checkbox menu row %s not updated on any conf change - try to use the update_on field\n", checked);
			}
		}
		else if (label && strcmp(label, "false") == 0) {
			/* INSENSITIVE ITEM */
			row = mbtk_menu_build_label_full_stdrow(1, menu_label, NULL);
			menu_row_add(row, parent, ins_after_w);
			TODO("tooltip: attach tip");

			TODO("set insensitive");
/*			gtk_widget_set_sensitive(row, FALSE);*/

			handle_alloc(row, sub_res);
		}
		else {
			/* NORMAL ITEM */
			row = mbtk_menu_build_label_full_stdrow(0, menu_label, accel);
			menu_row_add(row, parent, ins_after_w);
			TODO("tooltip: attach tip");
#if 0
			if ((tip != NULL) || (n_keydesc != NULL)) {
				char *acc = NULL, *s;
				if (n_keydesc != NULL)
					acc = rnd_hid_cfg_keys_gen_accel(&rnd_mbtk_keymap, n_keydesc, -1, "\nhotkey: ");
				s = rnd_concat((tip == NULL ? "" : tip), "\nhotkey: ", (acc == NULL ? "" : acc), NULL);
				gtk_widget_set_tooltip_text(row, s);
				free(s);
				free(acc);
			}
#endif

			handle_alloc(row, sub_res);
		}
	}

	free(accel);
	return row;
}


/* Translate a lihata tree into an mbtk menu structure */
static void rnd_mbtk_main_menu_real_add_node(rnd_mbtk_t *mctx, mbtk_widget_t *parent, lht_node_t *ins_after, lht_node_t *base)
{
	mbtk_widget_t *ins_after_w = (ins_after == NULL) ? NULL : ins_after->user_data;

	switch (base->type) {
		case LHT_HASH:                /* leaf submenu */
			rnd_mbtk_menu_create_(mctx, parent, ins_after, base);
			break;

		case LHT_TEXT:                /* separator or anchor */
		{
			if ((strcmp(base->data.text.value, "sep") == 0) || (strcmp(base->data.text.value, "-") == 0)) {
				mbtk_box_t *row = mbtk_hbox_new(NULL);
				mbtk_separator_t *sep = mbtk_hsep_new(NULL);

				mbtk_separator_set_span(sep, HVBOX_FILL);
				mbtk_box_add_widget(row, &sep->w, 0);
				menu_row_add(row, parent, ins_after_w);
				
				/* Not allocating a menu_handle for this */
			}
			else if (base->data.text.value[0] == '@') {
				/* anchor; ignore */
			}
			else
				rnd_hid_cfg_error(base, "Unexpected text node; the only text accepted here is sep, -, or @\n");
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
		rnd_mbtk_main_menu_real_add_node(mctx, &mctx->topwin->menu_bar.w, NULL, n);
}

static void rnd_mbtk_main_menu_add_popup_node(rnd_mbtk_t *mctx, lht_node_t *base)
{
TODO("Figure how to do popups with mbtk");
#if 0
	lht_node_t *submenu, *i;
	mbtk_box_t *new_menu;

	submenu = rnd_hid_cfg_menu_field_path(base, "submenu");
	if (submenu == NULL) {
		rnd_hid_cfg_error(base, "can not create popup without submenu list\n");
		return;
	}

	new_menu = mbtk_submenu_new();
	g_object_ref_sink(new_menu);
	handle_alloc(new_menu, base);

	for (i = submenu->data.list.first; i != NULL; i = i->next)
		rnd_mbtk_main_menu_real_add_node(ctx, new_menu, NULL, i);
#endif
}


int rnd_mbtk_load_menus(rnd_mbtk_t *mctx)
{
	const lht_node_t *mr;

	rnd_hid_menu_gui_ready_to_create(rnd_gui);

	mctx->topwin->menuconf_id = rnd_conf_hid_reg(cookie_menu, NULL);

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
