#include "config.h"

#include "compat.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <liblihata/tree.h>

#include <librnd/core/rnd_printf.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/error.h>
#include <librnd/core/conf.h>
#include <librnd/core/hidlib.h>
#include <librnd/core/hid_cfg.h>
#include <librnd/core/hid_cfg_action.h>
#include <librnd/hid/tool.h>
#include <librnd/hid/hid_menu.h>

#include <librnd/plugins/lib_gtk_common/rnd_gtk.h>
#include "bu_menu.h"
#include <librnd/plugins/lib_gtk_common/in_keyboard.h>

#include <librnd/plugins/lib_hid_common/menu_helper.h>


/*** custom widget ***/

#define RND_GTK_MAIN_MENU_TYPE            (rnd_gtk_main_menu_get_type ())
#define RND_GTK_MAIN_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), RND_GTK_MAIN_MENU_TYPE, RndGtkMainMenu))
#define RND_GTK_MAIN_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), RND_GTK_MAIN_MENU_TYPE, RndGtkMainMenuClass))
#define IS_RND_GTK_MAIN_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RND_GTK_MAIN_MENU_TYPE))
#define IS_RND_GTK_MAIN_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RND_GTK_MAIN_MENU_TYPE))
typedef struct _RndGtkMainMenu RndGtkMainMenu;
typedef struct _RndGtkMainMenuClass RndGtkMainMenuClass;

static GType rnd_gtk_main_menu_get_type(void);


/*** menu implementation ***/


static int action_counter;

typedef struct {
	GtkWidget *widget;    /* for most uses */
	GtkWidget *destroy;   /* destroy this */
	GtkAction *action;    /* for removing from the central lists */
} menu_handle_t;

static menu_handle_t *handle_alloc(GtkWidget *widget, GtkWidget *destroy, GtkAction *action)
{
	menu_handle_t *m = malloc(sizeof(menu_handle_t));
	m->widget = widget;
	m->destroy = destroy;
	m->action = action;
	return m;
}

GtkWidget *rnd_gtk_menu_popup_pre(lht_node_t *node)
{
	menu_handle_t *m;

	if (node == NULL) return NULL;
	if (node->user_data == NULL) return NULL;

	m = node->user_data;

	if (!GTK_IS_MENU(m->widget))
		return NULL;

	return m->widget;
}

struct _RndGtkMainMenu {
	GtkMenuBar parent;
	GList *actions;
	GCallback action_cb;
};

struct _RndGtkMainMenuClass {
	GtkMenuBarClass parent_class;
};

static GtkWidget *rnd_gtk_menu_item_new(const char *label, const char *accel_label, int check)
{
	GtkWidget *w;
	GtkWidget *hbox = gtkc_hbox_new(FALSE, 0);
	GtkWidget *spring = gtkc_hbox_new(FALSE, 0);
	GtkWidget *l = gtk_label_new(label);
	GtkWidget *accel = gtk_label_new(accel_label);

	if (check)
		w = gtk_check_menu_item_new();
	else
		w = gtk_menu_item_new();
	gtk_box_pack_start(GTK_BOX(hbox), l, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), spring, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), accel, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(w), GTK_WIDGET(hbox));

	return w;
}

/* LHT HANDLER */

static void rnd_gtk_main_menu_real_add_node(rnd_gtk_menu_ctx_t *ctx, RndGtkMainMenu *menu, GtkMenuShell *shell, lht_node_t *ins_after, lht_node_t *base);

static void ins_menu(GtkWidget *item, GtkMenuShell *shell, lht_node_t *ins_after)
{
	int pos;
	lht_dom_iterator_t it;
	lht_node_t *n;

	/* append at the end of the shell */
	if (ins_after == NULL) {
		gtk_menu_shell_append(shell, item);
		return;
	}

	if (ins_after == rnd_hid_menu_ins_as_first) {
		gtk_menu_shell_insert(shell, item, 1); /* 0th is the tear-off */
		return;
	}

	/* insert after ins_after or append after a specific item */
	for(n = lht_dom_first(&it, ins_after->parent), pos = 1; n != NULL; n = lht_dom_next(&it)) {
		if (n == ins_after) {
			if (n->user_data != NULL)
				pos++;
			break;
		}
		if (n->user_data == NULL)
			continue;
		pos++;
	}

	gtk_menu_shell_insert(shell, item, pos);
}

static GtkAction *rnd_gtk_add_menu(rnd_gtk_menu_ctx_t *ctx, RndGtkMainMenu *menu, GtkMenuShell *shell, lht_node_t *ins_after, lht_node_t *sub_res)
{
	const char *tmp_val;
	GtkAction *action = NULL;
	char *accel = NULL;
	char *menu_label;
	lht_node_t *n_action = rnd_hid_cfg_menu_field(sub_res, RND_MF_ACTION, NULL);
	lht_node_t *n_keydesc = rnd_hid_cfg_menu_field(sub_res, RND_MF_ACCELERATOR, NULL);

	/* Resolve accelerator and save it */
	if (n_keydesc != NULL) {
		if (n_action != NULL) {
			rnd_hid_cfg_keys_add_by_desc(&rnd_gtk_keymap, n_keydesc, n_action);
			accel = rnd_hid_cfg_keys_gen_accel(&rnd_gtk_keymap, n_keydesc, 1, NULL);
		}
		else
			rnd_hid_cfg_error(sub_res, "No action specified for key accel\n");
	}

	/* Resolve menu name */
	tmp_val = sub_res->name;


	menu_label = g_strdup(tmp_val);

	if (rnd_hid_cfg_has_submenus(sub_res)) {
		/* SUBMENU */
		GtkWidget *submenu = gtk_menu_new();
		GtkWidget *item = gtk_menu_item_new_with_mnemonic(menu_label);
		GtkWidget *tearoff = gtk_tearoff_menu_item_new();
		lht_node_t *n;

		sub_res->user_data = handle_alloc(submenu, item, NULL);
		ins_menu(item, shell, ins_after);

		gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

		/* add tearoff to menu */
		gtk_menu_shell_append(GTK_MENU_SHELL(submenu), tearoff);

		/* recurse on the newly-added submenu; rnd_hid_cfg_has_submenus() makes sure
		   the node format is correct; iterate over the list of submenus and create
		   them recursively. */
		n = rnd_hid_cfg_menu_field(sub_res, RND_MF_SUBMENU, NULL);
		for (n = n->data.list.first; n != NULL; n = n->next)
			rnd_gtk_main_menu_real_add_node(ctx, menu, GTK_MENU_SHELL(submenu), NULL, n);
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
			gchar *name = g_strdup_printf("MainMenuAction%d", action_counter++);
			action = GTK_ACTION(gtk_toggle_action_new(name, menu_label, tip, NULL));
			/* checked=foo       is a binary flag (checkbox)
			 * checked=foo=bar   is a flag compared to a value (radio) */
			gtk_toggle_action_set_draw_as_radio(GTK_TOGGLE_ACTION(action), ! !strchr(checked, '='));

			if (update_on != NULL)
				nat = rnd_conf_get_field(update_on);
			else
				nat = rnd_conf_get_field(checked);

			if (nat != NULL) {
				static rnd_conf_hid_callbacks_t cbs;
				static int cbs_inited = 0;
				if (!cbs_inited) {
					memset(&cbs, 0, sizeof(rnd_conf_hid_callbacks_t));
					cbs.val_change_post = ctx->confchg_checkbox;
					cbs_inited = 1;
				}
				rnd_conf_hid_set_cb(nat, ctx->rnd_gtk_menuconf_id, &cbs);
			}
			else {
				if ((update_on == NULL) || (*update_on != '\0'))
					rnd_message(RND_MSG_WARNING, "Checkbox menu item %s not updated on any conf change - try to use the update_on field\n", checked);
			}
		}
		else if (label && strcmp(label, "false") == 0) {
			/* INSENSITIVE ITEM */
			GtkWidget *item = gtk_menu_item_new_with_label(menu_label);
			gtk_widget_set_sensitive(item, FALSE);
			gtk_menu_shell_append(shell, item);
			sub_res->user_data = handle_alloc(item, item, NULL);
		}
		else {
			/* NORMAL ITEM */
			GtkWidget *item = rnd_gtk_menu_item_new(menu_label, accel, FALSE);
			ins_menu(item, shell, ins_after);
			sub_res->user_data = handle_alloc(item, item, NULL);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(menu->action_cb), (gpointer) n_action);
			if ((tip != NULL) || (n_keydesc != NULL)) {
				char *acc = NULL, *s;
				if (n_keydesc != NULL)
					acc = rnd_hid_cfg_keys_gen_accel(&rnd_gtk_keymap, n_keydesc, -1, "\nhotkey: ");
				s = rnd_concat((tip == NULL ? "" : tip), "\nhotkey: ", (acc == NULL ? "" : acc), NULL);
				gtk_widget_set_tooltip_text(item, s);
				free(s);
				free(acc);
			}
		}

	}

	/* By now this runs only for toggle items. */
	if (action) {
		GtkWidget *item = rnd_gtk_menu_item_new(menu_label, accel, TRUE);

		g_signal_connect(G_OBJECT(action), "activate", menu->action_cb, (gpointer)n_action);
		g_object_set_data(G_OBJECT(action), "resource", (gpointer)sub_res);
		g_object_set(item, "use-action-appearance", FALSE, NULL);
		g_object_set(item, "related-action", action, NULL);
		ins_menu(item, shell, ins_after);
		menu->actions = g_list_append(menu->actions, action);
		sub_res->user_data = handle_alloc(item, item, action);
	}

	free(accel);

	return action;
}

/* Translate a resource tree into a menu structure; shell is the base menu
   shell (a menu bar or popup menu) */
static void rnd_gtk_main_menu_real_add_node(rnd_gtk_menu_ctx_t *ctx, RndGtkMainMenu *menu, GtkMenuShell *shell, lht_node_t *ins_after, lht_node_t *base)
{
	switch (base->type) {
	case LHT_HASH:                /* leaf submenu */
		{
			GtkAction *action = NULL;
			action = rnd_gtk_add_menu(ctx, menu, shell, ins_after, base);
			if (action) {
				const char *val;

				val = rnd_hid_cfg_menu_field_str(base, RND_MF_CHECKED);
				if (val != NULL)
					g_object_set_data(G_OBJECT(action), "checked-flag", (gpointer *) val);
			}
		}
		break;
	case LHT_TEXT:                /* separator */
		{
			GList *children;

			children = gtk_container_get_children(GTK_CONTAINER(shell));
			g_list_free(children);

			if ((strcmp(base->data.text.value, "sep") == 0) || (strcmp(base->data.text.value, "-") == 0)) {
				GtkWidget *item = gtk_separator_menu_item_new();
				ins_menu(item, shell, ins_after);
				base->user_data = handle_alloc(item, item, NULL);
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

/* CONSTRUCTOR */
static void rnd_gtk_main_menu_init(RndGtkMainMenu *mm)
{
	/* Hookup signal handlers */
}

static void rnd_gtk_main_menu_class_init(RndGtkMainMenuClass *klass)
{
}

static GType rnd_gtk_main_menu_get_type(void)
{
	static GType mm_type = 0;

	if (!mm_type) {
		const GTypeInfo mm_info = {
			sizeof(RndGtkMainMenuClass),
			NULL,                       /* base_init */
			NULL,                       /* base_finalize */
			(GClassInitFunc)rnd_gtk_main_menu_class_init,
			NULL,                       /* class_finalize */
			NULL,                       /* class_data */
			sizeof(RndGtkMainMenu),
			0,                          /* n_preallocs */
			(GInstanceInitFunc) rnd_gtk_main_menu_init,
		};

		mm_type = g_type_register_static(GTK_TYPE_MENU_BAR, "RndGtkMainMenu", &mm_info, 0);
	}

	return mm_type;
}

static GtkWidget *rnd_gtk_main_menu_new(GCallback action_cb)
{
	RndGtkMainMenu *mm = g_object_new(RND_GTK_MAIN_MENU_TYPE, NULL);

	mm->action_cb = action_cb;
	mm->actions = NULL;

	return GTK_WIDGET(mm);
}

static void rnd_gtk_main_menu_add_node(rnd_gtk_menu_ctx_t *ctx, RndGtkMainMenu *menu, const lht_node_t *base)
{
	lht_node_t *n;
	if (base->type != LHT_LIST) {
		rnd_hid_cfg_error(base, "Menu description shall be a list (li)\n");
		abort();
	}
	for (n = base->data.list.first; n != NULL; n = n->next) {
		rnd_gtk_main_menu_real_add_node(ctx, menu, GTK_MENU_SHELL(menu), NULL, n);
	}
}

static void rnd_gtk_main_menu_add_popup_node(rnd_gtk_menu_ctx_t *ctx, RndGtkMainMenu *menu, lht_node_t *base)
{
	lht_node_t *submenu, *i;
	GtkWidget *new_menu;

	submenu = rnd_hid_cfg_menu_field_path(base, "submenu");
	if (submenu == NULL) {
		rnd_hid_cfg_error(base, "can not create popup without submenu list\n");
		return;
	}

	new_menu = gtk_menu_new();
	g_object_ref_sink(new_menu);
	base->user_data = handle_alloc(new_menu, new_menu, NULL);

	for (i = submenu->data.list.first; i != NULL; i = i->next)
		rnd_gtk_main_menu_real_add_node(ctx, menu, GTK_MENU_SHELL(new_menu), NULL, i);

	gtk_widget_show_all(new_menu);
}

/* callback for rnd_gtk_main_menu_update_toggle_state() */
static void menu_toggle_update_cb(rnd_design_t *hidlib, GtkAction *act, const char *tflag, const char *aflag)
{
	if (tflag != NULL) {
		int v = rnd_hid_get_flag(hidlib, tflag);
		if (v < 0) {
			gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act), 0);
			gtk_action_set_sensitive(act, 0);
		}
		else
			gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act), !!v);
	}
	if (aflag != NULL) {
		int v = rnd_hid_get_flag(hidlib, aflag);
		gtk_action_set_sensitive(act, !!v);
	}
}

void rnd_gtk_main_menu_update_toggle_state(rnd_design_t *hidlib, GtkWidget *menubar)
{
	GList *list;
	RndGtkMainMenu *menu = RND_GTK_MAIN_MENU(menubar);

	for (list = menu->actions; list; list = list->next) {
		lht_node_t *res = g_object_get_data(G_OBJECT(list->data), "resource");
		lht_node_t *act = rnd_hid_cfg_menu_field(res, RND_MF_ACTION, NULL);
		const char *tf = g_object_get_data(G_OBJECT(list->data), "checked-flag");
		const char *af = g_object_get_data(G_OBJECT(list->data), "active-flag");
		g_signal_handlers_block_by_func(G_OBJECT(list->data), menu->action_cb, act);
		menu_toggle_update_cb(hidlib, GTK_ACTION(list->data), tf, af);
		g_signal_handlers_unblock_by_func(G_OBJECT(list->data), menu->action_cb, act);
	}
}

/* Create a new popup window */
static GtkWidget *new_popup(lht_node_t *menu_item)
{
	GtkWidget *new_menu = gtk_menu_new();

	g_object_ref_sink(new_menu);
	menu_item->user_data = handle_alloc(new_menu, new_menu, NULL);

	return new_menu;
}

int rnd_gtk_create_menu_widget(void *ctx_, int is_popup, const char *name, int is_main, lht_node_t *parent, lht_node_t *ins_after, lht_node_t *menu_item)
{
	rnd_gtk_menu_ctx_t *ctx = ctx_;
	menu_handle_t *ph = parent->user_data;
	GtkWidget *w = (is_main) ? (is_popup ? new_popup(menu_item) : ctx->menu_bar) : ph->widget;

	rnd_gtk_main_menu_real_add_node(ctx, RND_GTK_MAIN_MENU(ctx->menu_bar), GTK_MENU_SHELL(w), ins_after, menu_item);

/* make sure new menu items appear on screen */
	gtk_widget_show_all(w);
	return 0;
}

int rnd_gtk_remove_menu_widget(void *ctx, lht_node_t * nd)
{
	menu_handle_t *h = nd->user_data;
	if (h != NULL) {
		RndGtkMainMenu *menu = (RndGtkMainMenu *)ctx;
		lht_node_t *n_keydesc = rnd_hid_cfg_menu_field(nd, RND_MF_ACCELERATOR, NULL);
		menu->actions = g_list_remove(menu->actions, h->action);
		if (n_keydesc != NULL)
			rnd_hid_cfg_keys_del_by_desc(&rnd_gtk_keymap, n_keydesc);
		gtk_widget_destroy(h->destroy);
		free(h);
		nd->user_data = NULL;
	}
	return 0;
}


/* Menu action callback function
   This is the main menu callback function.  The callback receives
   the original lihata action node pointer HID actions to be
   executed. */
static void rnd_gtk_menu_cb(GtkAction *action, const lht_node_t *node)
{
	if (action == NULL || node == NULL)
		return;

	rnd_hid_cfg_action(ghidgui->hidlib, node);

	/* GUI updates to reflect the result of the above action */
	if (rnd_app.adjust_attached_objects != NULL)
		rnd_app.adjust_attached_objects(ghidgui->hidlib);
	else
		rnd_tool_adjust_attached(ghidgui->hidlib);

	rnd_gui->invalidate_all(rnd_gui);
}


GtkWidget *rnd_gtk_load_menus(rnd_gtk_menu_ctx_t *menu, rnd_design_t *hidlib)
{
	const lht_node_t *mr;
	GtkWidget *menu_bar = NULL;

	menu->hidlib = hidlib;

	rnd_hid_menu_gui_ready_to_create(rnd_gui);

	mr = rnd_hid_cfg_get_menu(rnd_gui->menu, "/main_menu");
	if (mr != NULL) {
		menu_bar = rnd_gtk_main_menu_new(G_CALLBACK(rnd_gtk_menu_cb));
		rnd_gtk_main_menu_add_node(menu, RND_GTK_MAIN_MENU(menu_bar), mr);
		mr->doc->root->user_data = menu;
	}

	mr = rnd_hid_cfg_get_menu(rnd_gui->menu, "/popups");
	if (mr != NULL) {
		if (mr->type == LHT_LIST) {
			lht_node_t *n;
			for (n = mr->data.list.first; n != NULL; n = n->next)
				rnd_gtk_main_menu_add_popup_node(menu, RND_GTK_MAIN_MENU(menu_bar), n);
		}
		else
			rnd_hid_cfg_error(mr, "/popups should be a list\n");
		mr->doc->root->user_data = menu;
	}

	mr = rnd_hid_cfg_get_menu(rnd_gui->menu, "/mouse");
	if (rnd_hid_cfg_mouse_init(rnd_gui->menu, &rnd_gtk_mouse) != 0)
		rnd_message(RND_MSG_ERROR, "Error: failed to load mouse actions from the hid config lihata - mouse input will not work.");

	rnd_hid_menu_gui_ready_to_modify(rnd_gui);

	return menu_bar;
}
