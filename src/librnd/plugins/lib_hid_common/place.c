/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <librnd/rnd_config.h>

#include <genht/htsi.h>
#include <liblihata/tree.h>

#include <librnd/core/event.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/error.h>
#include <librnd/core/conf.h>
#include <librnd/core/conf_hid.h>
#include <librnd/core/safe_fs.h>

static const char *place_cookie = "dialogs/place";

#include "dialogs_conf.h"
extern conf_dialogs_t dialogs_conf;


typedef struct {
	int x, y, w, h;
	htsi_t panes;
	unsigned panes_inited:1;
} wingeo_t;

/* We store pane positions in memory as integers but as floating point decimals
   in the file; this is the conversion factor */
#define PANE_INT2DBL 10000

wingeo_t wingeo_invalid = {0, 0, 0, 0, {0}, 0};

typedef const char *htsw_key_t;
typedef wingeo_t htsw_value_t;
#define HT(x) htsw_ ## x
#define HT_INVALID_VALUE wingeo_invalid;
#include <genht/ht.h>
#include <genht/ht.c>
#undef HT
#include <genht/hash.h>

static htsw_t wingeo;

static void rnd_dialog_store(const char *id, int x, int y, int w, int h)
{
	htsw_entry_t *e;
	wingeo_t wg;

/*	rnd_trace("dialog place set: '%s' %d;%d  %d*%d\n", id, x, y, w, h);*/

	e = htsw_getentry(&wingeo, (char *)id);
	if (e != NULL) {
		e->value.x = x;
		e->value.y = y;
		e->value.w = w;
		e->value.h = h;
		return;
	}

	memset(&wg, 0, sizeof(wg));
	wg.x = x;
	wg.y = y;
	wg.w = w;
	wg.h = h;
	htsw_set(&wingeo, rnd_strdup(id), wg);
}


static void rnd_pane_store(const char *dlg_id, const char *pane_id, double val)
{
	htsw_entry_t *e;
	htsi_entry_t *i;
	int new_val = val * PANE_INT2DBL;

	e = htsw_getentry(&wingeo, (char *)dlg_id);
	if (e == NULL) {
		wingeo_t wg = {0};
		htsw_set(&wingeo, rnd_strdup(dlg_id), wg);
		e = htsw_getentry(&wingeo, (char *)dlg_id);
	}

	if (!e->value.panes_inited) {
		htsi_init(&e->value.panes, strhash, strkeyeq);
		e->value.panes_inited = 1;
	}

	i = htsi_getentry(&e->value.panes, (char *)pane_id);
	if (i != NULL)
		i->value = new_val;
	else
		htsi_set(&e->value.panes, rnd_strdup(pane_id), new_val);
}

void rnd_dialog_place(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	const char *id;
	int *geo;
	htsw_entry_t *e;

	if ((argc < 3) || (argv[1].type != RND_EVARG_PTR) || (argv[2].type != RND_EVARG_STR))
		return;

	id = argv[2].d.s;
	geo = argv[3].d.p;

	e = htsw_getentry(&wingeo, (char *)id);
	if (e != NULL) {
		geo[0] = e->value.x;
		geo[1] = e->value.y;
		geo[2] = e->value.w;
		geo[3] = e->value.h;
	}
/*	rnd_trace("dialog place: %p '%s'\n", hid_ctx, id);*/
}

void rnd_dialog_resize(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if ((argc < 7) || (argv[1].type != RND_EVARG_PTR) || (argv[2].type != RND_EVARG_STR))
		return;

/*	hid_ctx = argv[1].d.p;*/
	rnd_dialog_store(argv[2].d.s, argv[3].d.i, argv[4].d.i, argv[5].d.i, argv[6].d.i);
}

static vtp0_t cleanup_later;
static char *str_cleanup_later(const char *path)
{
	char *s = rnd_strdup(path);
	vtp0_append(&cleanup_later, s);
	return s;
}

static void place_conf_set(rnd_conf_role_t role, const char *path, int val)
{
	static int dummy;

	if (rnd_conf_get_field(path) == NULL)
		rnd_conf_reg_field_(&dummy, 1, RND_CFN_INTEGER, str_cleanup_later(path), "", 0);
	rnd_conf_setf(role, path, -1, "%d", val);
}

static void place_conf_load(rnd_conf_role_t role, const char *path, int *val)
{
	rnd_conf_native_t *nat = rnd_conf_get_field(path);
	rnd_conf_role_t crole;
	static int dummy;

	if (rnd_conf_get_field(path) == NULL) {
		rnd_conf_reg_field_(&dummy, 1, RND_CFN_INTEGER, str_cleanup_later(path), "", 0);
		rnd_conf_update(path, -1);
	}

	nat = rnd_conf_get_field(path);
	if ((nat == NULL) || (nat->prop->src == NULL) || (nat->prop->src->type != LHT_TEXT)) {
		rnd_message(RND_MSG_ERROR, "Can not load window geometry from invalid node for %s\n", path);
		return;
	}

	/* there are priorities which is hanled by conf merging. To make sure
	   only the final value is loaded, check if the final native's source
	   role matches the role that's being loaded. Else the currently loading
	   role is lower prio and didn't contribute to the final values and should
	   be ignored. */
	crole = rnd_conf_lookup_role(nat->prop->src);
	if (crole != role)
		return;

	/* need to atoi() directly from the lihata node because the native value
	   is dummy and shared among all nodes - cheaper to atoi than to do
	   a proper allocation for the native value. */
	*val = atoi(nat->prop->src->data.text.value);
}


/* Since this conf subtree is dynamic, we need to create the hlist for each
   dialog; this, after the merge, will trigger callbacks from the conf system
   to wplc_new_hlist_item() where the actual load happens */
static void place_conf_trigger_load_panes(rnd_conf_role_t role, const char *path)
{
	if (rnd_conf_get_field(path) == NULL) {
		static lht_node_t dummy;
		rnd_conf_reg_field_(&dummy, 1, RND_CFN_HLIST, str_cleanup_later(path), "", 0);
		rnd_conf_update(path, -1);
	}
}

#define BASEPATH "plugins/dialogs/window_geometry/"
void rnd_wplc_load(rnd_conf_role_t role)
{
	char *end, *end2, path[128 + sizeof(BASEPATH)];
	lht_node_t *nd, *root;
	lht_dom_iterator_t it;
	int x, y, w, h;

	strcpy(path, BASEPATH);
	end = path + strlen(BASEPATH);

	root = rnd_conf_lht_get_at(role, path, 0);
	if (root == NULL)
		return;

	for(nd = lht_dom_first(&it, root); nd != NULL; nd = lht_dom_next(&it)) {
		int len;
		if (nd->type != LHT_HASH)
			continue;
		len = strlen(nd->name);
		if (len > 64)
			continue;
		memcpy(end, nd->name, len);
		end[len] = '/';
		end2 = end + len+1;

		x = y = -1;
		w = h = 0;
		strcpy(end2, "x"); place_conf_load(role, path, &x);
		strcpy(end2, "y"); place_conf_load(role, path, &y);
		strcpy(end2, "width"); place_conf_load(role, path, &w);
		strcpy(end2, "height"); place_conf_load(role, path, &h);
		rnd_dialog_store(nd->name, x, y, w, h);
		strcpy(end2, "panes");
		place_conf_trigger_load_panes(role, path);
	}
}

lht_node_t *rnd_pref_ensure_conf_root(rnd_design_t *hidlib, rnd_conf_role_t role);

static void place_maybe_save(rnd_design_t *hidlib, rnd_conf_role_t role, int force)
{
	htsw_entry_t *e;
	char path[128 + sizeof(BASEPATH)];
	char *end, *end2;
	lht_node_t *nroot;

	switch(role) {
		case RND_CFR_USER:    if (!force && !dialogs_conf.plugins.dialogs.auto_save_window_geometry.to_user) return; break;
		case RND_CFR_DESIGN:  if (!force && !dialogs_conf.plugins.dialogs.auto_save_window_geometry.to_design) return; break;
		case RND_CFR_PROJECT: if (!force && !dialogs_conf.plugins.dialogs.auto_save_window_geometry.to_project) return; break;
		default: return;
	}

	nroot = rnd_pref_ensure_conf_root(hidlib, role);
	if (nroot == NULL) {
		rnd_message(RND_MSG_ERROR, "Internal error: failed to create conf root lht\n");
		return;
	}

	strcpy(path, BASEPATH);
	end = path + strlen(BASEPATH);
	for(e = htsw_first(&wingeo); e != NULL; e = htsw_next(&wingeo, e)) {
		int len = strlen(e->key);
		if (len > 64)
			continue;
		memcpy(end, e->key, len);
		end[len] = '/';
		end2 = end + len+1;

		strcpy(end2, "x"); place_conf_set(role, path, e->value.x);
		strcpy(end2, "y"); place_conf_set(role, path, e->value.y);
		strcpy(end2, "width"); place_conf_set(role, path, e->value.w);
		strcpy(end2, "height"); place_conf_set(role, path, e->value.h);

		if (e->value.panes_inited) {
			htsi_entry_t *i;
			lht_node_t *wnd, *pnd;
			lht_err_t err;


			end2[-1] = '\0';
			wnd = lht_tree_path_(nroot->doc, nroot, path, 1, 1, &err);
			if (wnd == NULL) {
				TODO("if *path is not in the project file already, it the lihata node will not exist");
				rnd_message(RND_MSG_ERROR, "Failed to write conf subtree '%s'\n", path);
				continue;
			}
			end2[-1] = '/';

			strcpy(end2, "panes");

			pnd = rnd_conf_lht_get_at(role, path, 0);
			if (pnd == NULL) {
				pnd = lht_dom_node_alloc(LHT_LIST, "panes");
				lht_dom_hash_put(wnd, pnd);
			}

			/* save each know pane pos */
			for(i = htsi_first(&e->value.panes); i != NULL; i = htsi_next(&e->value.panes, i)) {
				lht_node_t *nd, *pos;
				lht_dom_iterator_t it;
				int found = 0;

				for(nd = lht_dom_first(&it, pnd); nd != NULL; nd = lht_dom_next(&it)) {
					if ((nd->name != NULL) && (strcmp(nd->name, i->key) == 0)) {
						found = 1;
						break;
					}
				}

				if (!found) {
					nd = lht_dom_node_alloc(LHT_HASH, i->key);
					lht_dom_list_append(pnd, nd);
				}

				pos = lht_dom_hash_get(nd, "pos");
				if (pos == NULL) {
					pos = lht_dom_node_alloc(LHT_TEXT, "pos");
					lht_dom_hash_put(nd, pos);
				}

				pos->data.text.value = rnd_strdup_printf("%.05f", (double)i->value / PANE_INT2DBL);
			}
		}
	}


	if (role != RND_CFR_DESIGN) {
		int r = rnd_conf_save_file(hidlib, NULL, (hidlib == NULL ? NULL : hidlib->loadname), role, NULL);
		if (r != 0)
			rnd_message(RND_MSG_ERROR, "Failed to save window geometry in %s\n", rnd_conf_role_name(role));
	}
}

/* event handlers that run before the current design is saved to save win geo
   in the design conf and after loading a new board to fetch window placement
   info. */
static void place_save_pre(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	place_maybe_save(hidlib, RND_CFR_PROJECT, 0);
	place_maybe_save(hidlib, RND_CFR_DESIGN, 0);
}

static void place_load_post(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	rnd_wplc_load(RND_CFR_PROJECT);
	rnd_wplc_load(RND_CFR_DESIGN);
}

static void place_pane_changed(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if ((argv[1].type != RND_EVARG_STR) || (argv[2].type != RND_EVARG_STR) || (argv[3].type != RND_EVARG_DOUBLE))
		return;

	rnd_pane_store(argv[1].d.s, argv[2].d.s, argv[3].d.d);
}

static void place_new_pane(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	htsw_entry_t *e;
	htsi_entry_t *i;
	double *pos;

	if ((argv[1].type != RND_EVARG_STR) || (argv[2].type != RND_EVARG_STR) || (argv[3].type != RND_EVARG_PTR))
		return;

	e = htsw_getentry(&wingeo, (char *)argv[1].d.s);
	if ((e == NULL) || !e->value.panes_inited)
		return;

	i = htsi_getentry(&e->value.panes, (char *)argv[2].d.s);
	if (i == NULL)
		return;

	pos = (double *)argv[3].d.p;
	*pos = (double)i->value / (double)PANE_INT2DBL;
}

void rnd_wplc_save_to_role(rnd_design_t *hidlib, rnd_conf_role_t role)
{
	place_maybe_save(hidlib, role, 1);
}

int rnd_wplc_save_to_file(rnd_design_t *hidlib, const char *fn)
{
	htsw_entry_t *e;
	FILE *f;

	f = rnd_fopen(hidlib, fn, "w");
	if (f == NULL)
		return -1;

	fprintf(f, "li:pcb-rnd-conf-v1 {\n");
	fprintf(f, " ha:overwrite {\n");
	fprintf(f, "  ha:plugins {\n");
	fprintf(f, "   ha:dialogs {\n");
	fprintf(f, "    ha:window_geometry {\n");


	for(e = htsw_first(&wingeo); e != NULL; e = htsw_next(&wingeo, e)) {
		fprintf(f, "     ha:%s {\n", e->key);
		fprintf(f, "      x=%d\n", e->value.x);
		fprintf(f, "      y=%d\n", e->value.x);
		fprintf(f, "      width=%d\n", e->value.w);
		fprintf(f, "      height=%d\n", e->value.h);
		if (e->value.panes_inited && (e->value.panes.used > 0)) {
			htsi_entry_t *i;
			fprintf(f, "      li:%s {\n", e->key);
			for(i = htsi_first(&e->value.panes); i != NULL; i = htsi_next(&e->value.panes, i))
				rnd_fprintf(f, "       ha:%s={pos=%.05f}\n", i->key, ((double)i->value / PANE_INT2DBL));
			fprintf(f, "      }\n");
		}
		fprintf(f, "     }\n");
	}

	fprintf(f, "    }\n");
	fprintf(f, "   }\n");
	fprintf(f, "  }\n");
	fprintf(f, " }\n");
	fprintf(f, "}\n");
	fclose(f);
	return 0;
}


#define WIN_GEO_HPATH "plugins/dialogs/window_geometry/"
static void wplc_new_hlist_item(rnd_conf_native_t *cfg, rnd_conf_listitem_t *i, void *user_data)
{
	lht_node_t *n = i->prop.src, *vn;
	double val;
	char *end;

	if (strncmp(cfg->hash_path, WIN_GEO_HPATH, strlen(WIN_GEO_HPATH)) != 0)
		return;

	if (n->type != LHT_HASH)
		return;

	vn = lht_dom_hash_get(n, "pos");
	if ((vn == NULL) || (vn->type != LHT_TEXT))
		return;
	val = strtod(vn->data.text.value, &end);
	if (*end != '\0') {
		rnd_message(RND_MSG_ERROR, "window placement: invalid pane position '%s'\n(not a decimal number; in %s)\n", vn->data.text.value, cfg->hash_path);
		return;
	}

/*	rnd_trace("wplc hlist!! '%s' in '%s' '%f'\n", n->name, n->parent->parent->name, val);*/
	rnd_pane_store(n->parent->parent->name, n->name, val);
}


static rnd_conf_hid_callbacks_t cbs;

void rnd_dialog_place_uninit(void)
{
	htsw_entry_t *e;
	int n;

	rnd_conf_unreg_fields(BASEPATH);

	place_maybe_save(NULL, RND_CFR_USER, 0);

	for(e = htsw_first(&wingeo); e != NULL; e = htsw_next(&wingeo, e)) {
		htsi_entry_t *i;
		if (e->value.panes_inited) {
			for(i = htsi_first(&e->value.panes); i != NULL; i = htsi_next(&e->value.panes, i))
				free((char *)i->key);
			htsi_uninit(&e->value.panes);
		}
		free((char *)e->key);
	}
	htsw_uninit(&wingeo);
	rnd_event_unbind_allcookie(place_cookie);

	for(n = 0; n < cleanup_later.used; n++)
		free(cleanup_later.array[n]);
	vtp0_uninit(&cleanup_later);

	rnd_conf_hid_unreg(place_cookie);
}

void rnd_dialog_place_init(void)
{
	htsw_init(&wingeo, strhash, strkeyeq);
	rnd_event_bind(RND_EVENT_SAVE_PRE, place_save_pre, NULL, place_cookie);
	rnd_event_bind(RND_EVENT_LOAD_POST, place_load_post, NULL, place_cookie);
	rnd_event_bind(RND_EVENT_DAD_NEW_PANE, place_new_pane, NULL, place_cookie);
	rnd_event_bind(RND_EVENT_DAD_PANE_GEO_CHG, place_pane_changed, NULL, place_cookie);

	cbs.new_hlist_item_post = wplc_new_hlist_item;
	/*cfgid =*/ rnd_conf_hid_reg(place_cookie, &cbs);


	rnd_wplc_load(RND_CFR_INTERNAL);
	rnd_wplc_load(RND_CFR_ENV);
	rnd_wplc_load(RND_CFR_SYSTEM);
	rnd_wplc_load(RND_CFR_USER);
	rnd_wplc_load(RND_CFR_CLI);
}
