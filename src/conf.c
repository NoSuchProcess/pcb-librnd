#include <assert.h>
#include <genht/htsp.h>
#include <genht/hash.h>
#include "conf.h"
#include "hid_cfg.h"
#include "misc.h"
#include "error.h"

#warning TODO: this should do settings_postproc too


typedef enum {
	POL_PREPEND,
	POL_APPEND,
	POL_OVERWRITE,
	POL_DISABLE
} policy_t;

static lht_doc_t *conf_root[CFR_max];
static htsp_t *conf_fields = NULL;

/*static lht_doc_t *conf_plugin;*/


int conf_load_as(conf_role_t role, const char *fn)
{
	lht_doc_t *d;
	if (conf_root[role] != NULL)
		lht_dom_uninit(conf_root[role]);
	d = hid_cfg_load_lht(fn);
	if (d == NULL) {
		Message("error: failed to load lh config: %s\n", fn);
		conf_root[role] = NULL;
		return -1;
	}
	if (d->root->type != LHT_LIST) {
		hid_cfg_error(d->root, "Config root must be a list\n");
		conf_root[role] = NULL;
		return -1;
	}
	conf_root[role] = d;
	return 0;
}

static const char *get_hash_text(lht_node_t *parent, const char *name, lht_node_t **nout)
{
	lht_node_t *n;

	if ((parent == NULL) || (parent->type != LHT_HASH)) {
		if (nout != NULL)
			*nout = NULL;
		return NULL;
	}

	n = lht_dom_hash_get(parent, name);
	if (nout != NULL)
		*nout = n;

	if ((n == NULL) || (n->type != LHT_TEXT))
		return NULL;

	return n->data.text.value;
}

static const int get_hash_int(long *out, lht_node_t *parent, const char *name)
{
	lht_node_t *n;
	const char *s;
	char *end;
	long l;

	s = get_hash_text(parent, name, &n);
	if (s == NULL)
		return -1;

	l = strtol(s, &end, 10);
	if (*end != '\0') {
		hid_cfg_error(parent, "%s should be an integer\n", s);
		return -1;
	}
	if (out != NULL)
		*out = l;
	return 0;
}

static const int get_hash_policy(policy_t *out, lht_node_t *parent, const char *name)
{
	lht_node_t *n;
	const char *s;
	char *end;
	policy_t p;

	s = get_hash_text(parent, name, &n);
	if (s == NULL)
		return -1;

	if (strcasecmp(s, "overwrite") == 0)
		p = POL_OVERWRITE;
	else if (strcasecmp(s, "prepend") == 0)
		p = POL_PREPEND;
	else if (strcasecmp(s, "append") == 0)
		p = POL_APPEND;
	else if (strcasecmp(s, "disable") == 0)
		p = POL_DISABLE;
	else {
		hid_cfg_error(parent, "invalid policy %s\n", s);
		return -1;
	}

	if (out != NULL)
		*out = p;
	return 0;
}

int conf_parse_text(confitem_t *dst, int idx, conf_native_type_t type, const char *text, lht_node_t *err_node)
{
	char *strue[]  = {"true",  "yes",  "on",   "1", NULL};
	char *sfalse[] = {"false", "no",   "off",  "0", NULL};
	char **s, *end;
	long l;
	int base = 10;
	double d;
	const Unit *u;

	switch(type) {
		case CFN_STRING:
			dst->string[idx] = text;
			break;
		case CFN_BOOLEAN:
			for(s = strue; *s != NULL; s++)
				if (strcasecmp(*s, text) == 0) {
					dst->boolean[idx] = 1;
					return 0;
				}
			for(s = sfalse; *s != NULL; s++)
				if (strcasecmp(*s, text) == 0) {
					dst->boolean[idx] = 0;
					return 0;
				}
			hid_cfg_error(err_node, "Invalid boolean value: %s\n", text);
			return -1;
		case CFN_INTEGER:
			if ((text[0] == '0') && (text[1] == 'x')) {
				text += 2;
				base = 16;
			}
			l = strtol(text, &end, base);
			if (*end == '\0') {
				dst->integer[idx] = l;
				return 0;
			}
			hid_cfg_error(err_node, "Invalid integer value: %s\n", text);
			return -1;
		case CFN_REAL:
			d = strtod(text, &end);
			if (*end == '\0') {
				dst->real[idx] = d;
				return 0;
			}
			hid_cfg_error(err_node, "Invalid numeric value: %s\n", text);
			return -1;
		case CFN_COORD:
#warning TODO: write a new version of GetValue where absolute is optional and error is properly returned
			dst->coord[idx] = GetValue(text, NULL, NULL);
			break;
		case CFN_UNIT:
			u = get_unit_struct(text);
			if (u == NULL)
				hid_cfg_error(err_node, "Invalid unit: %s\n", text);
			else
				dst->unit[idx] = u;
			break;
		case CFN_COLOR:
#warning TODO: perhaps make some tests about validity?
			dst->color[idx] = text;
			break;
		default:
			/* unknown field type registered in the fields hash: internal error */
			return -1;
	}
	return 0;
}

int conf_merge_patch_text(conf_native_t *dest, lht_node_t *src, int prio, policy_t pol)
{
	if ((pol == POL_DISABLE) || (dest->prop[0].prio > prio))
		return 0;

	if (conf_parse_text(&dest->val, 0, dest->type, src->data.text.value, src) == 0) {
		dest->prop[0].prio = prio;
		dest->prop[0].src  = src;
		dest->used         = 1;
	}
	return 0;
}

int conf_merge_patch_array(conf_native_t *dest, lht_node_t *src_lst, int prio, policy_t pol)
{
	lht_node_t *s;
	int res;

	if (pol == POL_DISABLE)
		return 0;

#warning TODO: respect policy
	for(s = src_lst->data.list.first; s != NULL; s = s->next) {
		if (s->type == LHT_TEXT) {

			if ((dest->prop[dest->used].prio > prio))
				continue;

			if (dest->used >= dest->array_size) {
				hid_cfg_error(s, "Array is already full\n");
				res = -1;
				break;
			}

			if (conf_parse_text(&dest->val, dest->used, dest->type, s->data.text.value, s) == 0) {
				dest->prop[dest->used].prio = prio;
				dest->prop[dest->used].src  = s;
				dest->used++;
			}
		}
		else {
			hid_cfg_error(s, "List item must be text\n");
			res = -1;
		}
	}

	return res;
}

int conf_merge_patch_list(conf_native_t *dest, lht_node_t *src_lst, int prio, policy_t pol)
{
	lht_node_t *s;
	int res = 0;

	if (pol == POL_DISABLE)
		return 0;

#warning TODO: respect policy
	for(s = src_lst->data.list.first; s != NULL; s = s->next) {
		if (s->type == LHT_TEXT) {
			conf_listitem_t *i;
			i = calloc(sizeof(conf_listitem_t), 1);
			i->val.string = &i->payload;
			i->prop.prio = prio;
			i->prop.src  = s;
			if (conf_parse_text(&i->val, 0, CFN_STRING, s->data.text.value, s) != 0) {
				free(i);
				continue;
			}
			conflist_append(dest->val.list, i);
		}
		else {
			hid_cfg_error(s, "List item must be text\n");
			res = -1;
		}
	}

	return res;
}

/* merge main subtree of a patch */
int conf_merge_patch_recurse(lht_node_t *sect, int default_prio, policy_t default_policy, const char *path_prefix)
{
	lht_dom_iterator_t it;
	lht_node_t *n, *h;
	char path[256], *pathe;
	char name[256], *namee;
	int nl, ppl = strlen(path_prefix), res = 0;
	conf_native_t *target;

#warning TODO: use gds with static length

	memcpy(path, path_prefix, ppl);
	path[ppl] = '/';
	pathe = path + ppl + 1;

	for(n = lht_dom_first(&it, sect); n != NULL; n = lht_dom_next(&it)) {
		nl = strlen(n->name);
		memcpy(pathe, n->name, nl);
		namee = pathe+nl;
		*namee = '\0';
		target = conf_get_field(path);

		switch(n->type) {
			case LHT_TEXT:
				if (target == NULL) {
					hid_cfg_error(n, "ignoring unrecognized field: %s\n", path);
					break;
				}
				conf_merge_patch_text(target, n, default_prio, default_policy);
				break;
			case LHT_HASH:
				res |= conf_merge_patch_recurse(n, default_prio, default_policy, path);
				break;
			case LHT_LIST:
				if (target->type == CFN_LIST)
					res |= conf_merge_patch_list(target, n, default_prio, default_policy);
				else if (target->array_size > 1)
					res |= conf_merge_patch_array(target, n, default_prio, default_policy);
				else
					hid_cfg_error(n, "Attempt to initialize a scalar with a list - this node should be a text node\n");
				break;
		}
	}
	return res;
}

int conf_merge_patch(lht_node_t *root)
{
	long gprio = 0;
	policy_t gpolicy = POL_OVERWRITE;
	const char *ps;
	lht_node_t *n;
	lht_dom_iterator_t it;

	if (root->type != LHT_HASH) {
		hid_cfg_error(root, "patch root should be a hash\n");
		return -1;
	}

	/* get global settings */
	get_hash_int(&gprio, root, "priority");
	get_hash_policy(&gpolicy, root, "policy");

	/* iterate over all hashes and insert them recursively */
	for(n = lht_dom_first(&it, root); n != NULL; n = lht_dom_next(&it))
		if (n->type == LHT_HASH)
			conf_merge_patch_recurse(n, gprio, gpolicy, n->name);

	return 0;
}

int conf_merge_all()
{
	int n, ret = 0;
	for(n = 0; n < CFR_max; n++) {
		lht_node_t *r;
		if (conf_root[n] == NULL)
			continue;
		for(r = conf_root[n]->root->data.list.first; r != NULL; r = r->next)
			if (conf_merge_patch(r) != 0)
				ret = -1;
	}
	return ret;
}

void conf_update(void)
{
	conf_load_as(CFR_SYSTEM, "./pcb-conf.lht");
	conf_merge_all();
}

static int keyeq(char *a, char *b)
{
	return !strcmp(a, b);
}

void conf_reg_field_(void *value, int array_size, conf_native_type_t type, const char *path)
{
	conf_native_t *node;

	if (conf_fields == NULL) {
		conf_fields = htsp_alloc(strhash, keyeq);
		assert(conf_fields != NULL);
	}
	assert(array_size >= 1);

	assert(htsp_get(conf_fields, (char *)path) == NULL);

	node = malloc(sizeof(conf_native_t));
	node->description = "n/a";
	node->array_size  = array_size;
	node->type        = type;
	node->val.any     = value;
	node->prop        = calloc(sizeof(confprop_t), array_size);
	node->used        = 0;
	htsp_set(conf_fields, (char *)path, node);
}

conf_native_t *conf_get_field(const char *path)
{
	return htsp_get(conf_fields, (char *)path);
}

#warning TODO: move this to a debug plugin
static void conf_dump_(FILE *f, const char *prefix, int verbose, confitem_t *val, conf_native_type_t type, confprop_t *prop, int idx)
{
	switch(type) {
		case CFN_STRING:  fprintf(f, "%s", val->string[idx] == NULL ? "<NULL>" : val->string[idx]); break;
		case CFN_BOOLEAN: fprintf(f, "%d", val->boolean[idx]); break;
		case CFN_INTEGER: fprintf(f, "%ld", val->integer[idx]); break;
		case CFN_REAL:    fprintf(f, "%f", val->real[idx]); break;
		case CFN_COORD:   pcb_fprintf(f, "%$mS", val->coord[idx]); break;
		case CFN_UNIT:    pcb_fprintf(f, "%s", val->unit[idx]->suffix); break;
		case CFN_COLOR:   fprintf(f, "%s", val->color[idx]); break;
		case CFN_LIST:
			{
				conf_listitem_t *n;
				if (conflist_length(val->list) > 0) {
					fprintf(f, "{");
					for(n = conflist_first(val->list); n != NULL; n = conflist_next(n)) {
						fprintf(f, "{");
						conf_dump_(f, prefix, verbose, &n->val, n->type, &n->prop, 0);
						fprintf(f, "};");
					}
					fprintf(f, "}");
				}
				else
					fprintf(f, "<empty list>");
			}
			break;
	}
	if (verbose) {
		fprintf(f, " <<prio=%d", prop[idx].prio);
		if (prop[idx].src != NULL) {
			fprintf(f, " from=%s:%d", prop[idx].src->file_name, prop[idx].src->line);
		}
		fprintf(f, ">>");
	}
}

void conf_dump(FILE *f, const char *prefix, int verbose)
{
	htsp_entry_t *e;
	for (e = htsp_first(conf_fields); e; e = htsp_next(conf_fields, e)) {
		conf_native_t *node = (conf_native_t *)e->value;
		if (node->array_size > 1) {
			int n;
			for(n = 0; n < node->used; n++) {
				fprintf(f, "%s I %s[%d] = ", prefix, e->key, n);
				conf_dump_(f, prefix, verbose, &node->val, node->type, node->prop, n);
				fprintf(f, "\n");
			}
			if (node->used == 0)
				fprintf(f, "%s I %s[] = <empty>\n", prefix, e->key);
		}
		else {
			fprintf(f, "%s I %s = ", prefix, e->key);
			conf_dump_(f, prefix, verbose, &node->val, node->type, node->prop, 0);
			fprintf(f, "\n");
		}
	}
}
