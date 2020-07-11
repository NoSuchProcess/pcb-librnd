/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2020 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* menu file loading, menu merging: all files are loaded into the memory
   where the final lihata document that represents the menu is merged;
   then the new version is compared to the old version and differneces
   applied to the menu system */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <genvector/vtp0.h>
#include <liblihata/lihata.h>
#include <liblihata/tree.h>

#include "hidlib.h"
#include "actions.h"
#include "file_loaded.h"
#include "hidlib_conf.h"
#include "paths.h"
#include "funchash_core.h"

#include "hid_menu.h"

/*** load & merge ***/

typedef struct {
	char *cookie;
	rnd_hid_cfg_t cfg;
	int prio;
} rnd_menu_patch_t;

typedef struct {
	vtp0_t patches; /* list of (rnd_menu_patch_t *), ordered by priority, ascending */
	rnd_hid_cfg_t *merged;
	long changes, last_merged; /* if changes > last_merged, we need to merge */
	int inhibit;
	unsigned gui_ready:1; /* ready for the first merge */
	unsigned gui_nomod:1; /* do the merge but do not send any modification request - useful for the initial menu setup */
	unsigned alloced:1;   /* whether ->merged is alloced (it is not, for the special case of patches->used <= 1 at the time of merging) */
} rnd_menu_sys_t;

static rnd_menu_sys_t menu_sys;

void rnd_menu_sys_init(rnd_menu_sys_t *msys)
{
	memset(msys, 0, sizeof(rnd_menu_sys_t));
}

static void rnd_menu_sys_insert(rnd_menu_sys_t *msys, rnd_menu_patch_t *menu)
{
	int n;
	rnd_menu_patch_t *m;
	/* assume only a dozen of patch files -> linear search is good enough for now */
	for(n = 0; n < msys->patches.used; n++) {
		m = msys->patches.array[n];
		if (m->prio > menu->prio) {
			vtp0_insert_len(&msys->patches, n, (void **)&menu, 1);
			msys->changes++;
			return;
		}
	}
	vtp0_append(&msys->patches, menu);
	msys->changes++;
}

static void rnd_menu_sys_remove_cookie(rnd_menu_sys_t *msys, const char *cookie)
{
	int n;

	/* assume only a dozen of patch files -> linear search is good enough for now */
	for(n = 0; n < msys->patches.used; n++) {
		rnd_menu_patch_t *m = msys->patches.array[n];
		if (strcmp(m->cookie, cookie) == 0) {
			vtp0_remove(&msys->patches, n, 1);
			msys->changes++;
			n--;
		}
	}
}

	/* recursive merge */
static void menu_merge_node(lht_node_t *dst, lht_node_t *src)
{
}

static lht_doc_t *dup_base(rnd_menu_patch_t *base)
{
	lht_node_t *tmp;
	lht_doc_t *new = lht_dom_init();

	new->root = lht_dom_node_alloc(LHT_HASH, "rnd-menu-v1");
	new->root->doc = new;
	tmp = lht_dom_duptree(base->cfg.doc->root);
	lht_tree_merge(new->root, tmp);
	return new;
}

static void menu_merge(rnd_hid_t *hid)
{
	rnd_menu_patch_t *base = NULL;
	int just_created = 0;

	if (!menu_sys.gui_ready || (menu_sys.inhibit > 0))
		return;

	if (menu_sys.patches.used > 0)
		base = menu_sys.patches.array[0];

	if (base != NULL) {
		if ((base->cfg.doc->root->type != LHT_HASH) || (strcmp(base->cfg.doc->root->name, "rnd-menu-v1") != 0)) {
			rnd_message(RND_MSG_ERROR, "Base menu file %s has invalid root (should be: ha:rnd-menu-v1)\n");
			base = NULL;
		}
	}
	else {
		rnd_message(RND_MSG_ERROR, "Menu merging error: no menu file\n");
		return;
	}

	if (hid->menu == NULL) {
		hid->menu = calloc(sizeof(rnd_hid_cfg_t), 1); /* make sure the cache is cleared */
		hid->menu->doc = dup_base(base);
		just_created = 1;
	}

	if ((just_created == 0) || (menu_sys.patches.used > 1)) {
		lht_doc_t *new = lht_dom_init();
		new->root = lht_dom_duptree(base->cfg.doc->root);

		menu_merge_node(hid->menu->doc->root, new->root);
		lht_dom_uninit(new);
	}

	menu_sys.last_merged = menu_sys.changes;
}

void rnd_hid_menu_gui_ready_to_create(rnd_hid_t *hid)
{
	menu_sys.gui_ready = 1;
	menu_sys.gui_nomod = 1;
	menu_merge(hid);
}

void rnd_hid_menu_gui_ready_to_modify(rnd_hid_t *hid)
{
	menu_sys.gui_nomod = 0;
}

int rnd_hid_menu_load(rnd_hid_t *hid, rnd_hidlib_t *hidlib, const char *cookie, int prio, const char *fn, int exact_fn, const char *embedded_fallback, const char *desc)
{
	lht_doc_t *doc;
	rnd_menu_patch_t *menu;

	if (fn != NULL) {
		if (!exact_fn) {
			/* try different paths to find the menu file inventing its exact name */
			char **paths = NULL, **p;
			int fn_len = strlen(fn);

			doc = NULL;
			rnd_paths_resolve_all(hidlib, rnd_menu_file_paths, paths, fn_len+4, rnd_false);
			for(p = paths; *p != NULL; p++) {
				if (doc == NULL) {
					strcpy((*p)+strlen(*p), fn);
					doc = rnd_hid_cfg_load_lht(hidlib, *p);
					if (doc != NULL)
						rnd_file_loaded_set_at("menu", cookie, *p, desc);
				}
				free(*p);
			}
			free(paths);
		}
		else {
			doc = rnd_hid_cfg_load_lht(hidlib, fn);
			if (doc != NULL)
				rnd_file_loaded_set_at("menu", cookie, fn, desc);
		}
	}

	if ((doc == NULL) && (embedded_fallback != NULL)) {
		doc = rnd_hid_cfg_load_str(embedded_fallback);
		if (doc != NULL)
			rnd_file_loaded_set_at("menu", cookie, "<internal>", desc);
	}
	if (doc == NULL)
		return -1;

	menu = calloc(sizeof(rnd_menu_patch_t), 1); /* make sure the cache is cleared */
	menu->cfg.doc = doc;

	rnd_menu_sys_insert(&menu_sys, menu);

	menu_merge(hid);
	return 0;
}

/*** utility ***/

lht_node_t *rnd_hid_cfg_get_menu_at(rnd_hid_cfg_t *hr, lht_node_t *at, const char *menu_path, lht_node_t *(*cb)(void *ctx, lht_node_t *node, const char *path, int rel_level), void *ctx)
{
	lht_err_t err;
	lht_node_t *curr;
	int level = 0, len = strlen(menu_path), iafter = 0;
	char *next_seg, *path;

	if (hr == NULL)
		return NULL;

 path = malloc(len+4); /* need a few bytes after the end for the ':' */
 strcpy(path, menu_path);

	next_seg = path;
	curr = (at == NULL) ? hr->doc->root : at;

	/* Have to descend manually because of the submenu nodes */
	for(;;) {
		char *next, *end;
		lht_dom_iterator_t it;

		while(*next_seg == '/') next_seg++;

		if (curr != hr->doc->root) {
			if (level > 1) {
				curr = lht_tree_path_(hr->doc, curr, "submenu", 1, 0, &err);
				if (curr == NULL)
					break;
			}
		}
		next = end = strchr(next_seg, '/');
		if (end == NULL)
			end = next_seg + strlen(next_seg);
		
		*end = '\0';

		/* find next_seg in the current level */
		for(curr = lht_dom_first(&it, curr); curr != NULL; curr = lht_dom_next(&it)) {
			if (*next_seg == '@') {
				/* looking for an anon text node with the value matching the anchor name */
				if ((curr->type == LHT_TEXT) && (strcmp(curr->data.text.value, next_seg) == 0)) {
					iafter = 1;
					break;
				}
			}
			else {
				/* looking for a hash node */
				if (strcmp(curr->name, next_seg) == 0)
					break;
			}
		}

		if (cb != NULL)
			curr = cb(ctx, curr, path, level);

		if (next != NULL) /* restore previous / so that path is a full path */
			*next = '/';
		next_seg = next;
		if ((curr == NULL) || (next_seg == NULL))
			break;
		next_seg++;
		level++;
		if (iafter) {
			/* special case: insert after an @anchor and exit */
			if (cb != NULL)
				curr = cb(ctx, NULL, path, level);
			break;
		}
	}

	free(path);
	return curr;
}

lht_node_t *rnd_hid_cfg_get_menu(rnd_hid_cfg_t *hr, const char *menu_path)
{
	return rnd_hid_cfg_get_menu_at(hr, NULL, menu_path, NULL, NULL);
}


/*** minimal menu support for feature plugins - needs to stay in core ***/

static void map_anchor_menus(vtp0_t *dst, lht_node_t *node, const char *name)
{
	lht_dom_iterator_t it;

	switch(node->type) {
		case LHT_HASH:
		case LHT_LIST:
			for(node = lht_dom_first(&it, node); node != NULL; node = lht_dom_next(&it))
				map_anchor_menus(dst, node, name);
			break;
		case LHT_TEXT:
			if ((node->parent != NULL) && (node->parent->type == LHT_LIST) && (strcmp(node->data.text.value, name) == 0))
				vtp0_append(dst, node);
			break;
		default:
			break;
	}
}

void rnd_hid_cfg_map_anchor_menus(const char *name, void (*cb)(void *ctx, rnd_hid_cfg_t *cfg, lht_node_t *n, char *path), void *ctx)
{
	vtp0_t anchors;

	if ((rnd_gui == NULL) || (rnd_gui->menu == NULL) || (rnd_gui->menu->doc == NULL) || (rnd_gui->menu->doc->root == NULL))
		return;

	/* extract anchors; don't do the callbacks from there because the tree
	   is going to be changed from the callbacks. It is however guaranteed
	   that anchors are not removed. */
	vtp0_init(&anchors);
	map_anchor_menus(&anchors, rnd_gui->menu->doc->root, name);

	/* iterate over all anchors extracted and call the callback */
	{
		char *path = NULL;
		int used = 0, alloced = 0, l0 = strlen(name) + 128;
		size_t an;

		for(an = 0; an < vtp0_len(&anchors); an++) {
			lht_node_t *n, *node = anchors.array[an];

			for(n = node->parent; n != NULL; n = n->parent) {
				int len;
				if (strcmp(n->name, "submenu") == 0)
					continue;
				len = strlen(n->name);
				if (used+len+2+l0 >= alloced) {
					alloced = used+len+2+l0 + 128;
					path = realloc(path, alloced);
				}
				memmove(path+len+1, path, used);
				memcpy(path, n->name, len);
				path[len] = '/';
				used += len+1;
			}
			memcpy(path+used, name, l0+1);
/*			rnd_trace("path='%s' used=%d\n", path, used);*/
			cb(ctx, rnd_gui->menu, node, path);
			used = 0;
		}
		free(path);
	}

	vtp0_uninit(&anchors);
}

int rnd_hid_cfg_del_anchor_menus(lht_node_t *node, const char *cookie)
{
	lht_node_t *nxt;

	if ((node->type != LHT_TEXT) || (node->data.text.value == NULL) || (node->data.text.value[0] != '@'))
		return -1;

	for(node = node->next; node != NULL; node = nxt) {
		lht_node_t *ncookie;

		if (node->type != LHT_HASH)
			break;
		ncookie = lht_dom_hash_get(node, "cookie");

/*		rnd_trace("  '%s' cookie='%s'\n", node->name, ncookie == NULL ? "NULL":ncookie->data.text.value );*/
		if ((ncookie == NULL) || (ncookie->type != LHT_TEXT) || (strcmp(ncookie->data.text.value, cookie) != 0))
			break;

		nxt = node->next;
		rnd_gui->remove_menu_node(rnd_gui, node);
	}
	return 0;
}

/*** actions ***/

static const char pcb_acts_CreateMenu[] = "CreateMenu(path)\nCreateMenu(path, action, tooltip, cookie)";
static const char pcb_acth_CreateMenu[] = "Creates a new menu, popup (only path specified) or submenu (at least path and action are specified)";
static fgw_error_t pcb_act_CreateMenu(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	if (rnd_gui == NULL) {
		rnd_message(RND_MSG_ERROR, "Error: can't create menu, there's no GUI hid loaded\n");
		RND_ACT_IRES(-1);
		return 0;
	}

	RND_ACT_CONVARG(1, FGW_STR, CreateMenu, ;);
	RND_ACT_MAY_CONVARG(2, FGW_STR, CreateMenu, ;);
	RND_ACT_MAY_CONVARG(3, FGW_STR, CreateMenu, ;);
	RND_ACT_MAY_CONVARG(4, FGW_STR, CreateMenu, ;);

	if (argc > 1) {
		rnd_menu_prop_t props;

		memset(&props, 0, sizeof(props));
		props.action = (argc > 2) ? argv[2].val.str : NULL;
		props.tip = (argc > 3) ? argv[3].val.str : NULL;
		props.cookie = (argc > 4) ? argv[4].val.str : NULL;

		rnd_gui->create_menu(rnd_gui, argv[1].val.str, &props);

		RND_ACT_IRES(0);
		return 0;
	}

	RND_ACT_FAIL(CreateMenu);
}

static const char pcb_acts_RemoveMenu[] = "RemoveMenu(path|cookie)";
static const char pcb_acth_RemoveMenu[] = "Recursively removes a new menu, popup (only path specified) or submenu. ";
static fgw_error_t pcb_act_RemoveMenu(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	if (rnd_gui == NULL) {
		rnd_message(RND_MSG_ERROR, "can't remove menu, there's no GUI hid loaded\n");
		RND_ACT_IRES(-1);
		return 0;
	}

	if (rnd_gui->remove_menu == NULL) {
		rnd_message(RND_MSG_ERROR, "can't remove menu, the GUI doesn't support it\n");
		RND_ACT_IRES(-1);
		return 0;
	}

	RND_ACT_CONVARG(1, FGW_STR, RemoveMenu, ;);
	if (rnd_gui->remove_menu(rnd_gui, argv[1].val.str) != 0) {
		rnd_message(RND_MSG_ERROR, "failed to remove some of the menu items\n");
		RND_ACT_IRES(-1);
	}
	else
		RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_MenuPatch[] = 
	"MenuPatch(load, cookie, path, desc)\n"
	"MenuPatch(unload, cookie)\n"
	"MenuPatch(list)";
static const char pcb_acth_MenuPatch[] = "Manage menu patches";
fgw_error_t pcb_act_MenuPatch(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;
	const char *cookie = NULL, *path = NULL, *desc = "";
	rnd_menu_patch_t *menu;

	RND_ACT_CONVARG(1, FGW_KEYWORD, MenuPatch, op = fgw_keyword(&argv[1]));
	RND_ACT_MAY_CONVARG(2, FGW_STR, MenuPatch, cookie = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, MenuPatch, path = argv[3].val.str);
	RND_ACT_MAY_CONVARG(4, FGW_STR, MenuPatch, desc = argv[4].val.str);

	switch(op) {
		case F_Load:
			if ((cookie == NULL) || (path == NULL))
				RND_ACT_FAIL(MenuPatch);
			rnd_hid_menu_load(rnd_gui, NULL, cookie, 500, path, 1, NULL, desc);
			RND_ACT_IRES(0);
			return;
		case F_Unload:
			if (cookie == NULL)
				RND_ACT_FAIL(MenuPatch);
			rnd_menu_sys_remove_cookie(&menu_sys, cookie);
			RND_ACT_IRES(0);
			return;
		case F_List:
			break;
	}

	rnd_message(RND_MSG_ERROR, "not yet implemented\n");
	RND_ACT_IRES(-1);
	return 0;
}


static rnd_action_t rnd_menu_action_list[] = {
	{"CreateMenu", pcb_act_CreateMenu, pcb_acth_CreateMenu, pcb_acts_CreateMenu},
	{"RemoveMenu", pcb_act_RemoveMenu, pcb_acth_RemoveMenu, pcb_acts_RemoveMenu},
	{"MenuPatch", pcb_act_MenuPatch, pcb_acth_MenuPatch, pcb_acts_MenuPatch},
};

void rnd_menu_init1(void)
{
	rnd_menu_sys_init(&menu_sys);
}

void rnd_menu_act_init2(void)
{
	RND_REGISTER_ACTIONS(rnd_menu_action_list, NULL);
}
