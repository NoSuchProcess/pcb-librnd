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
#include <liblihata/lihata.h>
#include <liblihata/tree.h>

#include "hidlib.h"
#include "file_loaded.h"
#include "hidlib_conf.h"
#include "paths.h"

#include "hid_menu.h"

rnd_hid_cfg_t *rnd_hid_cfg_load(rnd_hidlib_t *hidlib, const char *fn, int exact_fn, const char *embedded_fallback)
{
	lht_doc_t *doc;
	rnd_hid_cfg_t *hr;

	if (embedded_fallback == NULL)
		embedded_fallback = rnd_hidlib_default_embedded_menu;

	/* override HID defaults with the configured path */
	if ((rnd_conf.rc.menu_file != NULL) && (*rnd_conf.rc.menu_file != '\0')) {
		fn = rnd_conf.rc.menu_file;
		exact_fn = (strchr(rnd_conf.rc.menu_file, '/') != NULL);
	}

	if (!exact_fn) {
		/* try different paths to find the menu file inventing its exact name */
		char **paths = NULL, **p;
		int fn_len = strlen(fn);

		doc = NULL;
		rnd_paths_resolve_all(hidlib, rnd_menu_file_paths, paths, fn_len+32, rnd_false);
		for(p = paths; *p != NULL; p++) {
			if (doc == NULL) {
				char *end = *p + strlen(*p);
				sprintf(end, rnd_menu_name_fmt, fn);
				doc = rnd_hid_cfg_load_lht(hidlib, *p);
				if (doc != NULL)
					rnd_file_loaded_set_at("menu", "HID main", *p, "main menu system");
			}
			free(*p);
		}
		free(paths);
	}
	else
		doc = rnd_hid_cfg_load_lht(hidlib, fn);

	if (doc == NULL) {
		doc = rnd_hid_cfg_load_str(embedded_fallback);
		rnd_file_loaded_set_at("menu", "HID main", "<internal>", "main menu system");
	}
	if (doc == NULL)
		return NULL;

	hr = calloc(sizeof(rnd_hid_cfg_t), 1); /* make sure the cache is cleared */
	hr->doc = doc;

	return hr;
}

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
