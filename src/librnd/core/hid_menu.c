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
#include "compat_misc.h"

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
			lht_dom_uninit(m->cfg.doc);
			free(m->cookie);
			free(m);
			msys->changes++;
			n--;
		}
	}
}

static rnd_menu_patch_t *rnd_menu_sys_find_cookie(rnd_menu_sys_t *msys, const char *cookie)
{
	int n;

	for(n = 0; n < msys->patches.used; n++) {
		rnd_menu_patch_t *m = msys->patches.array[n];
		if (strcmp(m->cookie, cookie) == 0)
			return m;
	}

	return NULL;
}

static lht_node_t *search_list(lht_node_t *list, const char *name)
{
	lht_node_t *n;
	lht_dom_iterator_t it;

	for(n = lht_dom_first(&it, list); n != NULL; n = lht_dom_next(&it))
		if (strcmp(n->name, name) == 0)
			return n;
	return NULL;
}

#define submenu(node) ((node->type == LHT_HASH) ? (lht_dom_hash_get((node), "submenu")) : NULL)

void lht_tree_merge_list_submenu(lht_node_t *dst, lht_node_t *src, lht_err_t recurse(lht_node_t *, lht_node_t *))
{
	lht_dom_iterator_t it;
	lht_node_t *s, *a;

	for(s = lht_dom_first(&it, src); s != NULL; s = lht_dom_next(&it)) {
		a = search_list(dst, s->name);
		if (a == NULL) { /* append new items at the end */
			lht_tree_detach(s); /* removing an item from the list from the iterator is a special case that works since r584 */
			lht_dom_list_append(dst, s);
		}
		else {
			lht_tree_detach(s); /* removing an item from the list from the iterator is a special case that works since r584 */
			recurse(a, s);
		}
	}
}

static lht_err_t lht_tree_merge_menu(lht_node_t *dst, lht_node_t *src)
{
	lht_err_t e;

	switch(dst->type) {
		case LHT_INVALID_TYPE: return LHTE_BROKEN_DOC;
		case LHT_TEXT: lht_tree_merge_text(dst, src); break;
		case LHT_TABLE: e = lht_tree_merge_table(dst, src); if (e != LHTE_SUCCESS) return e; break;
		case LHT_HASH:  e = lht_tree_merge_hash(dst, src, lht_tree_merge_menu); if (e != LHTE_SUCCESS) return e; break;
		case LHT_LIST:
			if ((strcmp(dst->name, "submenu") == 0) || (strcmp(dst->name, "main_menu") == 0) || (strcmp(dst->name, "popups") == 0))
				lht_tree_merge_list_submenu(dst, src, lht_tree_merge_menu);
			else
				lht_tree_merge_list(dst, src);
			break;
		case LHT_SYMLINK:
			/* symlink text match */
			break;
	}
	lht_dom_node_free(src);
	return LHTE_SUCCESS;
}

#define GET_PATH_TEXT(instname, n, path) \
do { \
	path = lht_dom_hash_get(inst, "path"); \
	if (path == NULL) { \
		rnd_message(RND_MSG_ERROR, "Menu merging error: " instname " patch instruction without a menu path\n"); \
		return; \
	} \
	if (path->type != LHT_TEXT) { \
		rnd_message(RND_MSG_ERROR, "Menu merging error: " instname " patch instruction menu path must be text\n"); \
		return; \
	} \
	n = rnd_hid_cfg_get_menu_at_node(dst, path->data.text.value, NULL, NULL); \
	if (n == NULL) \
		return; \
} while(0)

static void menu_patch_apply_remove_menu(lht_node_t *dst, lht_node_t *inst)
{
	lht_node_t *n, *path;

	GET_PATH_TEXT("remove-menu", n, path);

	if ((strcmp(n->name, "main_menu") == 0) || (strcmp(n->name, "popups") == 0) || (strcmp(n->name, "anchored") == 0)) {
		if (submenu(n->parent) == NULL) {
			rnd_message(RND_MSG_ERROR, "Menu merging error: remove-menu patch attempted to remove a menu root\n");
			return;
		}
	}

	lht_tree_del(n);
}

static void menu_patch_apply_append_menu(lht_node_t *dst, lht_node_t *inst)
{
	lht_node_t *dn, *path, *isub, *tmp, *dsub;

	GET_PATH_TEXT("append-menu", dn, path);

	isub = lht_dom_hash_get(inst, "submenu"); \
	if ((isub == NULL) || (isub->type != LHT_LIST)) {
		rnd_message(RND_MSG_ERROR, "Menu merging error: append-menu patch instruction submenu must a list\n");
		return;
	}
	dsub = submenu(dn);
	if (dsub == NULL) {
		rnd_message(RND_MSG_ERROR, "Menu merging error: append-menu patch instruction attempted to append to a non-submenu %s\n", dn->name);
		return;
	}
	
	tmp = lht_dom_duptree(isub);
	lht_tree_merge_using(dsub, tmp, lht_tree_merge_menu);
}

static void menu_patch_apply(lht_node_t *dst, lht_node_t *src)
{
	lht_node_t *tmp;

	/* merging a complete menu file is easy: use lihata's default merge algorithm,
	   except for submenu lists where by-name merging is required */
	if ((src->type == LHT_HASH) && (strcmp(src->name, "rnd-menu-v1") == 0)) {
		tmp = lht_dom_duptree(src);
		lht_tree_merge_using(dst, tmp, lht_tree_merge_menu);
		return;
	}

	/* execute patching instructions */
	if ((src->type == LHT_HASH) && (strcmp(src->name, "rnd-menu-patch-v1") == 0)) {
		lht_node_t *p, *patch = lht_dom_hash_get(src, "patch");
		if ((patch == NULL) || (patch->type != LHT_LIST)) {
			rnd_message(RND_MSG_ERROR, "Menu merging error: patch instructions must be in a li:patch\n");
			return;
		}
		for(p = patch->data.list.first; p != NULL; p = p->next) {
			if (p->type != LHT_HASH) {
				rnd_message(RND_MSG_ERROR, "Menu merging error: invalid patch instruction %s (not a hash)\n", p->name);
				continue;
			}
			if (strcmp(p->name, "remove-menu") == 0)
				menu_patch_apply_remove_menu(dst, p);
			else if (strcmp(p->name, "append-menu") == 0)
				menu_patch_apply_append_menu(dst, p);
			else {
				rnd_message(RND_MSG_ERROR, "Menu merging error: unknown patch instruction %s\n", p->name);
				continue;
			}
		}
		return;
	}

	rnd_message(RND_MSG_ERROR, "Menu merging error: invalid menu file root: %s\n", src->name);
}

static void create_menu_by_node(lht_node_t *dst, lht_node_t *ins_after, int is_popup)
{
	lht_node_t *parent = dst->parent;
	int is_main = (strcmp(parent->name, "submenu") != 0);

	if (!is_main)
		parent = parent->parent;

	rnd_gui->create_menu_by_node(rnd_gui, is_popup, dst->name, is_main, parent, ins_after, dst);
}

/* execute submenu creation: call the hid to add each item in dst recursively */
static void menu_merge_submenu_exec_add(lht_node_t *dst, lht_node_t *ins_after, int is_popup)
{
	create_menu_by_node(dst, ins_after, is_popup);
}

static void menu_merge_remove_recursive(lht_node_t *node)
{
	lht_node_t *n, *sub = submenu(node);
TODO("The old approach is that remove_menu_node() recursively removes everything from the lihata tree; reenable this when that part is removed");
#if 0
	if (sub != NULL) {
		lht_dom_iterator_t it;
		for(n = lht_dom_first(&it, node); n != NULL; n = lht_dom_next(&it))
			menu_merge_remove_recursive(n);
		
	}
#endif
	rnd_gui->remove_menu_node(rnd_gui, node);
#if 0
	lht_tree_del(node);
#endif
}

static void menu_merge_submenu(lht_node_t *dst, lht_node_t *src, int is_popup)
{
	lht_node_t *dn, *sn, *ssub, *dsub, *tmp;
	lht_dom_iterator_t it;

	/* find nodes that are present in dst but not in src -> remove */
	for(dn = lht_dom_first(&it, dst); dn != NULL; dn = lht_dom_next(&it)) {
		sn = search_list(src, dn->name);
		if (sn == NULL)
			menu_merge_remove_recursive(dn);
	}

	/* find nodes that are present in both -> either recurse or modify */
	for(dn = lht_dom_first(&it, dst); dn != NULL; dn = lht_dom_next(&it)) {
		sn = search_list(src, dn->name);
		if (sn != NULL) {
			ssub = submenu(sn);
			dsub = submenu(dn);
			if ((dsub != NULL) && (ssub != NULL))
				menu_merge_submenu(dsub, ssub, is_popup); /* same submenu -> recurse */
			else if ((dsub != NULL) && (ssub == NULL)) {
				TODO("modify: a submenu is replaced by a plain node");
			}
			else if ((dsub == NULL) && (ssub != NULL)) {
				TODO("modify: a plain node is replaced by a submenu");
			}
			else if ((dsub == NULL) && (ssub == NULL)) {
				TODO("modify: plain node is replaced by a plain node");
			}
		}
	}

	/* find nodes that are present in src but not in dst -> add */
	for(sn = lht_dom_first(&it, src); sn != NULL; sn = lht_dom_next(&it)) {
		dn = search_list(dst, sn->name);
		if (dn == NULL) {
			tmp = lht_dom_duptree(sn);
			lht_dom_list_append(dst, tmp);

			dn = search_list(dst, sn->name);
			if (dn != NULL)
				create_menu_by_node(dn, NULL, is_popup);
		}
	}
}

typedef struct {
	char *name;   /* points into path*/
	char path[1]; /* extends longer */
} anchor_t;

static void map_anchors_submenu(vtp0_t *anch, gds_t *path, lht_node_t *root)
{
	lht_node_t *n, *sm;
	assert(root->type == LHT_LIST);
	for(n = root->data.list.first; n != NULL; n = n->next) {
		if ((n->type == LHT_TEXT) && (n->data.text.value != NULL) && (n->data.text.value[0] == '@')) {
			anchor_t *a;
			int save = path->used;
			gds_append(path, '/');
			gds_append_str(path, n->data.text.value);
			a = malloc(sizeof(anchor_t) + path->used+1);
			memcpy(a->path, path->array, path->used+1);
			a->name = a->path + save + 1;
/*			printf(">anchor: '%s': %s\n", a->name, a->path);*/
			gds_truncate(path, save);
			vtp0_append(anch, a);
		}
		sm = submenu(n);
		if (sm != NULL) {
			int save = path->used;
			gds_append(path, '/');
			gds_append_str(path, n->name);
			map_anchors_submenu(anch, path, sm);
			gds_truncate(path, save);
		}
	}
}

static void menu_merge_anchored(vtp0_t *anch, lht_node_t *dst, lht_node_t *src)
{

	if (src->type != LHT_LIST) {
		rnd_message(RND_MSG_ERROR, "Menu merging error: /anchored must be a list\n");
		return;
	}

	for(src = src->data.list.first; src != NULL; src = src->next) {
		long n, found = 0;

		if ((src->name == NULL) || (src->name[0] != '@')) {
			rnd_message(RND_MSG_ERROR, "Menu merging error: /anchored subtree names must started with a '@' (ignoring offending node: %s)\n", src->name);
			continue;
		}

		for(n = 0; n < anch->used; n++) {
			anchor_t *a = anch->array[n];
			if (strcmp(src->name, a->name) == 0) {
				lht_node_t *anode = rnd_hid_cfg_get_menu_at_node(dst, a->path, NULL, NULL);
				if (anode != NULL) {
					printf(" anchored! '%s' at %s: %p\n", src->name, a->path, anode);
					found++;
				}
			}
		}
		if (found == 0) {
			rnd_message(RND_MSG_ERROR, "Menu merging error: anchor %s not found\n", src->name);
		}
	}
}

	/* recursive merge of the final trees starting from the root */
static void menu_merge_root(lht_node_t *dst, lht_node_t *src)
{
	lht_node_t *dn, *sn, *sanch;
	vtp0_t anch = {0};
	gds_t path = {0};
	long n;

	assert(dst->type == LHT_HASH);
	assert(src->type == LHT_HASH);

	dn = lht_dom_hash_get(dst, "main_menu");
	sn = lht_dom_hash_get(src, "main_menu");
	menu_merge_submenu(dn, sn, 0);
	path.used = 0;
	gds_append_str(&path, "main_menu");
	map_anchors_submenu(&anch, &path, dn);


	dn = lht_dom_hash_get(dst, "popups");
	sn = lht_dom_hash_get(src, "popups");
	menu_merge_submenu(dn, sn, 1);
	path.used = 0;
	gds_append_str(&path, "popups");
	map_anchors_submenu(&anch, &path, dn);

	sn = lht_dom_hash_get(src, "anchored");
	if (sn != NULL)
		menu_merge_anchored(&anch, dst, sn);

	for(n = 0; n < anch.used; n++)
		free(anch.array[n]);
	vtp0_uninit(&anch);
	TODO("mouse, toolbar_static, scripts");
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

static lht_doc_t *new_menu_file()
{
	lht_doc_t *new = lht_dom_init();

	new->root = lht_dom_node_alloc(LHT_HASH, "rnd-menu-v1");
	new->root->doc = new;
	lht_dom_hash_put(new->root, lht_dom_node_alloc(LHT_LIST, "main_menu"));
	lht_dom_hash_put(new->root, lht_dom_node_alloc(LHT_LIST, "popups"));
	lht_dom_hash_put(new->root, lht_dom_node_alloc(LHT_LIST, "anchored"));
	return new;
}

static void lht_set_doc(lht_node_t *node, lht_doc_t *doc)
{
	lht_node_t *n;
	lht_dom_iterator_t it;

	node->doc = doc;
	for(n = lht_dom_first(&it, node); n != NULL; n = lht_dom_next(&it))
		lht_set_doc(n, doc);
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
		int n;
		lht_doc_t *new = lht_dom_init();
		new->root = lht_dom_duptree(base->cfg.doc->root);
		lht_set_doc(new->root, new);

		/* apply all patches */
		for(n = 1; n < menu_sys.patches.used; n++) {
			rnd_menu_patch_t *m = menu_sys.patches.array[n];
			menu_patch_apply(new->root, m->cfg.doc->root);
		}

		/* perform the final tree merge */
		menu_merge_root(hid->menu->doc->root, new->root);

TODO("remove debug:");
#if 1
		{
#undef fopen
			FILE *f = fopen("A_merged.lht", "w");
			lht_dom_export(hid->menu->doc->root, f, "");
			fclose(f);
		}
#endif

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

static int determine_prio(lht_node_t *node, int default_prio)
{
	long l;
	char *end;

	if ((node->type != LHT_HASH) || (strcmp(node->name, "rnd-menu-patch-v1") != 0))
		return default_prio;
	node = lht_dom_hash_get(node, "prio");
	if (node == NULL)
		return default_prio;
	if (node->type != LHT_TEXT) {
		rnd_message(RND_MSG_ERROR, "Menu merging error: ignoring prio (must be a text node)\n");
		return default_prio;
	}

	l = strtol(node->data.text.value, &end, 10);
	if ((*end != '\0') || (l < 1) || (l > 32767)) {
		rnd_message(RND_MSG_ERROR, "Menu merging error: ignoring prio (must be an integer between 1 and 32k)\n");
		return default_prio;
	}

	return l;
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
	menu->prio = determine_prio(doc->root, prio);
	menu->cookie = rnd_strdup(cookie);

	rnd_menu_sys_insert(&menu_sys, menu);

	menu_merge(hid);
	return 0;
}

/*** utility ***/

lht_node_t *rnd_hid_cfg_get_menu_at_node(lht_node_t *at, const char *menu_path, lht_node_t *(*cb)(void *ctx, lht_node_t *node, const char *path, int rel_level), void *ctx)
{
	lht_err_t err;
	lht_node_t *curr;
	int level = 0, len = strlen(menu_path), iafter = 0;
	char *next_seg, *path;

 path = malloc(len+4); /* need a few bytes after the end for the ':' */
 strcpy(path, menu_path);

	next_seg = path;
	curr = at;

	/* Have to descend manually because of the submenu nodes */
	for(;;) {
		char *next, *end;
		lht_dom_iterator_t it;

		while(*next_seg == '/') next_seg++;

		if (curr != at->doc->root) {
			if (level > 1) {
				curr = lht_tree_path_(at->doc, curr, "submenu", 1, 0, &err);
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

lht_node_t *rnd_hid_cfg_get_menu_at(rnd_hid_cfg_t *hr, lht_node_t *at, const char *menu_path, lht_node_t *(*cb)(void *ctx, lht_node_t *node, const char *path, int rel_level), void *ctx)
{
	if (hr == NULL)
		return NULL;

	return rnd_hid_cfg_get_menu_at_node(((at == NULL) ? hr->doc->root : at), menu_path, cb, ctx);
}

lht_node_t *rnd_hid_cfg_get_menu(rnd_hid_cfg_t *hr, const char *menu_path)
{
	return rnd_hid_cfg_get_menu_at(hr, NULL, menu_path, NULL, NULL);
}


TODO("make this public and remove from lib_hid_common/menu_helper.[ch]");
/* Fields are retrieved using this enum so that HIDs don't need to hardwire
   lihata node names */
typedef enum {
	PCB_MF_ACCELERATOR,
	PCB_MF_SUBMENU,
	PCB_MF_CHECKED,
	PCB_MF_UPDATE_ON,
	PCB_MF_SENSITIVE,
	PCB_MF_TIP,
	PCB_MF_ACTIVE,
	PCB_MF_ACTION,
	PCB_MF_FOREGROUND,
	PCB_MF_BACKGROUND,
	PCB_MF_FONT
} pcb_hid_cfg_menufield_t;

TODO("make this public and remove from lib_hid_common/menu_helper.[ch]");
static lht_node_t *pcb_hid_cfg_menu_field(const lht_node_t *submenu, pcb_hid_cfg_menufield_t field, const char **field_name)
{
	lht_err_t err;
	const char *fieldstr = NULL;

	switch(field) {
		case PCB_MF_ACCELERATOR:  fieldstr = "a"; break;
		case PCB_MF_SUBMENU:      fieldstr = "submenu"; break;
		case PCB_MF_CHECKED:      fieldstr = "checked"; break;
		case PCB_MF_UPDATE_ON:    fieldstr = "update_on"; break;
		case PCB_MF_SENSITIVE:    fieldstr = "sensitive"; break;
		case PCB_MF_TIP:          fieldstr = "tip"; break;
		case PCB_MF_ACTIVE:       fieldstr = "active"; break;
		case PCB_MF_ACTION:       fieldstr = "action"; break;
		case PCB_MF_FOREGROUND:   fieldstr = "foreground"; break;
		case PCB_MF_BACKGROUND:   fieldstr = "background"; break;
		case PCB_MF_FONT:         fieldstr = "font"; break;
	}
	if (field_name != NULL)
		*field_name = fieldstr;

	if (fieldstr == NULL)
		return NULL;

	return lht_tree_path_(submenu->doc, submenu, fieldstr, 1, 0, &err);
}

typedef struct {
	rnd_hid_cfg_t *hr;
	lht_node_t *parent;
	rnd_menu_prop_t props;
	int target_level;
	int err;
	lht_node_t *after;
} create_menu_ctx_t;

static lht_node_t *create_menu_cb(void *ctx, lht_node_t *node, const char *path, int rel_level)
{
	create_menu_ctx_t *cmc = ctx;
	lht_node_t *psub;

	if (node == NULL) { /* level does not exist, create it */
		const char *name;
		name = strrchr(path, '/');
		if (name != NULL)
			name++;
		else
			name = path;

		if (rel_level <= 1) {
			/* creating a main menu */
			char *end, *name = rnd_strdup(path);
			for(end = name; *end == '/'; end++) ;
			end = strchr(end, '/');
			*end = '\0';
			psub = cmc->parent = rnd_hid_cfg_get_menu(cmc->hr, name);
			free(name);
		}
		else
			psub = pcb_hid_cfg_menu_field(cmc->parent, PCB_MF_SUBMENU, NULL);

		if (rel_level == cmc->target_level) {
			node = rnd_hid_cfg_create_hash_node(psub, cmc->after, name, "dyn", "1", "cookie", cmc->props.cookie, "a", cmc->props.accel, "tip", cmc->props.tip, "action", cmc->props.action, "checked", cmc->props.checked, "update_on", cmc->props.update_on, "foreground", cmc->props.foreground, "background", cmc->props.background, NULL);
			if (node != NULL)
				cmc->err = 0;
		}
		else
			node = rnd_hid_cfg_create_hash_node(psub, cmc->after, name, "dyn", "1", "cookie", cmc->props.cookie,  NULL);

		if (node == NULL)
			return NULL;

		if ((rel_level != cmc->target_level) || (cmc->props.action == NULL))
			lht_dom_hash_put(node, lht_dom_node_alloc(LHT_LIST, "submenu"));

		if (node->parent == NULL) {
			lht_dom_list_append(psub, node);
		}
		else {
			assert(node->parent == psub);
		}
	}
	else {
		/* existing level */
		if ((node->type == LHT_TEXT) && (node->data.text.value[0] == '@')) {
			cmc->after = node;
			goto skip_parent;
		}
	}
	cmc->parent = node;

	skip_parent:;
	return node;
}

static int create_menu_manual_prop(rnd_menu_sys_t *msys, const char *path, const rnd_menu_prop_t *props)
{
	const char *name;
	rnd_menu_patch_t *mp;
	create_menu_ctx_t cmc;

	if (props->cookie == NULL)
		return -1;

	mp = rnd_menu_sys_find_cookie(msys, props->cookie);
	if (mp == NULL) {
		mp = calloc(sizeof(rnd_menu_patch_t), 1); /* make sure the cache is cleared */
		mp->cfg.doc = new_menu_file();
		mp->prio = 500;
		mp->cookie = rnd_strdup(props->cookie);
		rnd_menu_sys_insert(msys, mp);
	}

	memset(&cmc, 0, sizeof(cmc));
	cmc.hr = &mp->cfg;
	cmc.err = -1;
	cmc.props = *props;

	/* Allow creating new nodes only under certain main paths that correspond to menus */
	if (strncmp(path, "rnd-menu-v1/", 12) == 0)
		path += 11; /* but keep the / */

	name = path;
	while(*name == '/') name++;

	if ((strncmp(name, "main_menu/", 10) == 0) || (strncmp(name, "popups/", 7) == 0) || (strncmp(name, "anchored/", 9) == 0)) {
		/* calculate target level */
		for(cmc.target_level = 0; *name != '\0'; name++) {
			if (*name == '/') {
				cmc.target_level++;
				while(*name == '/') name++;
				name--;
			}
		}

		/* descend and visit each level, create missing levels */
		rnd_hid_cfg_get_menu_at(cmc.hr, NULL, path, create_menu_cb, &cmc);
	}

	if (cmc.err == 0) {
		msys->changes++;
		menu_merge(rnd_gui);
	}

	return cmc.err;
}

static int create_menu_manual(rnd_menu_sys_t *msys, const char *path, const char *action, const char *tip, const char *cookie)
{
	rnd_menu_prop_t props = {0};

	props.action = action;
	props.tip = tip;
	props.cookie = cookie;
	return create_menu_manual_prop(msys, path, &props);
}

int rnd_hid_menu_create(const char *path, const rnd_menu_prop_t *props)
{
	return create_menu_manual_prop(&menu_sys, path, props);
}

static int remove_menu_manual(rnd_menu_sys_t *msys, const char *path, const char *cookie)
{
	rnd_menu_patch_t *mp = rnd_menu_sys_find_cookie(msys, cookie);
	lht_node_t *nd;

	if (mp == NULL)
		return -1;

	nd = rnd_hid_cfg_get_menu_at(&mp->cfg, NULL, path, NULL, NULL);
	if (nd == NULL)
		return -1;
	lht_tree_del(nd);
	msys->changes++;
	menu_merge(rnd_gui);
	return 0;
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
		int r = create_menu_manual(&menu_sys, argv[1].val.str, (argc > 2) ? argv[2].val.str : NULL, (argc > 3) ? argv[3].val.str : NULL, (argc > 4) ? argv[4].val.str : NULL);
		if (r != 0)
			rnd_message(RND_MSG_ERROR, "Error: failed to create the menu\n");
		RND_ACT_IRES(r);
		return 0;
	}

	RND_ACT_FAIL(CreateMenu);
}

static const char pcb_acts_RemoveMenu[] = "RemoveMenu(path, cookie)";
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
	RND_ACT_CONVARG(2, FGW_STR, RemoveMenu, ;);
	if (remove_menu_manual(&menu_sys, argv[1].val.str, argv[2].val.str) != 0) {
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
			if (rnd_hid_menu_load(rnd_gui, NULL, cookie, 500, path, 1, NULL, desc) != 0)
				rnd_message(RND_MSG_ERROR, "Failed to load menu patch %s\n", path);
			RND_ACT_IRES(0);
			return;
		case F_Unload:
			if (cookie == NULL)
				RND_ACT_FAIL(MenuPatch);
			rnd_menu_sys_remove_cookie(&menu_sys, cookie);
			menu_merge(rnd_gui);
			RND_ACT_IRES(0);
			return;
		case F_List:
			{
				int n;
				rnd_message(RND_MSG_INFO, "Menu system:\n");
				for(n = 0; n < menu_sys.patches.used; n++) {
					rnd_menu_patch_t *m = menu_sys.patches.array[n];
					rnd_message(RND_MSG_INFO, " [%d] %s prio=%d %s: %s\n", n, (n == 0 ? "base " : "addon"), m->prio, m->cookie, m->cfg.doc->root->file_name);
				}
			}
			RND_ACT_IRES(0);
			return;
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
