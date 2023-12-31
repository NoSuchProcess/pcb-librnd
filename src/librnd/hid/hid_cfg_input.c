/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <liblihata/lihata.h>
#include <liblihata/tree.h>
#include <genht/hash.h>
#include <genht/ht_utils.h>
#include <genvector/gds_char.h>

#include <librnd/rnd_config.h>
#include <librnd/hid/hid_cfg_input.h>
#include <librnd/hid/hid_menu.h>
#include <librnd/core/hid_cfg_action.h>
#include <librnd/core/rnd_conf.h>
#include <librnd/core/error.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/event.h>

/* split value into a list of '-' separated words; examine each word
   and set the bitmask of modifiers */
static rnd_hid_cfg_mod_t parse_mods(const char *value, const char **last, unsigned int vlen)
{
	rnd_hid_cfg_mod_t m = 0;
	int press = 0;
	const char *next;

	while(isspace(*value)) value++;

	if (*value != '<') {
		for(;;) {
			if ((vlen >= 5) && (rnd_strncasecmp(value, "shift", 5) == 0))        m |= RND_M_Shift;
			else if ((vlen >= 4) && (rnd_strncasecmp(value, "ctrl", 4) == 0))    m |= RND_M_Ctrl;
			else if ((vlen >= 3) && (rnd_strncasecmp(value, "alt", 3) == 0))     m |= RND_M_Alt;
			else if ((vlen >= 7) && (rnd_strncasecmp(value, "release", 7) == 0)) m |= RND_M_Release;
			else if ((vlen >= 5) && (rnd_strncasecmp(value, "press", 5) == 0))   press = 1;
			else
				rnd_message(RND_MSG_ERROR, "Unknown modifier: %s\n", value);
			/* skip to next word */
			next = strpbrk(value, "<- \t");
			if (next == NULL)
				break;
			if (*next == '<')
				break;
			vlen -= (next - value);
			value = next+1;
		}
	}

	if (last != NULL)
		*last = value;

	if (press && (m & RND_M_Release))
		rnd_message(RND_MSG_ERROR, "Bogus modifier: both press and release\n");

	return m;
}

static rnd_hid_cfg_mod_t button_name2mask(const char *name)
{
	/* All mouse-related resources must be named.  The name is the
	   mouse button number.  */
	if (!name)
		return 0;
	else if (rnd_strcasecmp(name, "left") == 0)   return RND_MB_LEFT;
	else if (rnd_strcasecmp(name, "middle") == 0) return RND_MB_MIDDLE;
	else if (rnd_strcasecmp(name, "right") == 0)  return RND_MB_RIGHT;

	else if (rnd_strcasecmp(name, "scroll-up") == 0)     return RND_MB_SCROLL_UP;
	else if (rnd_strcasecmp(name, "scroll-down") == 0)   return RND_MB_SCROLL_DOWN;
	else if (rnd_strcasecmp(name, "scroll-left") == 0)   return RND_MB_SCROLL_UP;
	else if (rnd_strcasecmp(name, "scroll-right") == 0)  return RND_MB_SCROLL_DOWN;
	else {
		rnd_message(RND_MSG_ERROR, "Error: unknown mouse button: %s\n", name);
		return 0;
	}
}

static unsigned int keyhash_int(htip_key_t a)      { return murmurhash32(a & 0xFFFF); }

static unsigned int keyb_hash(const void *key_)
{
	const rnd_hid_cfg_keyhash_t *key = key_;
	unsigned int i = 0;
	i += key->key_raw; i <<= 8;
	i += ((unsigned int)key->key_tr) << 4;
	i += key->mods;
	return murmurhash32(i);
}

static int keyb_eq(const void *keya_, const void *keyb_)
{
	const rnd_hid_cfg_keyhash_t *keya = keya_, *keyb = keyb_;
	return (keya->key_raw == keyb->key_raw) && (keya->key_tr == keyb->key_tr) && (keya->mods == keyb->mods);
}

/************************** MOUSE ***************************/

int rnd_hid_cfg_mouse_init(rnd_hid_cfg_t *hr, rnd_hid_cfg_mouse_t *mouse)
{
	lht_node_t *btn, *m;

	mouse->mouse = rnd_hid_cfg_get_menu(hr, "/mouse");

	if (mouse->mouse == NULL) {
		rnd_message(RND_MSG_ERROR, "Warning: no /mouse section in the resource file - mouse is disabled\n");
		return -1;
	}

	if (mouse->mouse->type != LHT_LIST) {
		rnd_hid_cfg_error(mouse->mouse, "Warning: should be a list - mouse is disabled\n");
		return -1;
	}

	if (mouse->mouse_mask == NULL)
		mouse->mouse_mask = htip_alloc(keyhash_int, htip_keyeq);
	else
		htip_clear(mouse->mouse_mask);

	for(btn = mouse->mouse->data.list.first; btn != NULL; btn = btn->next) {
		rnd_hid_cfg_mod_t btn_mask = button_name2mask(btn->name);
		if (btn_mask == 0) {
			rnd_hid_cfg_error(btn, "unknown mouse button");
			continue;
		}
		if (btn->type != LHT_LIST) {
			rnd_hid_cfg_error(btn, "needs to be a list");
			continue;
		}
		for(m = btn->data.list.first; m != NULL; m = m->next) {
			rnd_hid_cfg_mod_t mod_mask = parse_mods(m->name, NULL, -1);
			htip_set(mouse->mouse_mask, btn_mask|mod_mask, m);
		}
	}
	return 0;
}

static lht_node_t *find_best_action(rnd_hid_cfg_mouse_t *mouse, rnd_hid_cfg_mod_t button_and_mask)
{
	lht_node_t *n;

	if (mouse->mouse_mask == NULL)
		return NULL;

	/* look for exact mod match */
	n = htip_get(mouse->mouse_mask, button_and_mask);
	if (n != NULL)
		return n;

	if (button_and_mask & RND_M_Release) {
		/* look for plain release for the given button */
		n = htip_get(mouse->mouse_mask, (button_and_mask & RND_M_ANY) | RND_M_Release);
		if (n != NULL)
			return n;
	}

	return NULL;
}

void rnd_hid_cfg_mouse_action(rnd_design_t *hl, rnd_hid_cfg_mouse_t *mouse, rnd_hid_cfg_mod_t button_and_mask, rnd_bool cmd_entry_active)
{
	rnd_conf.temp.click_cmd_entry_active = cmd_entry_active;
	rnd_hid_cfg_action(hl, find_best_action(mouse, button_and_mask));
	rnd_event(hl, RND_EVENT_USER_INPUT_POST, NULL);
	rnd_conf.temp.click_cmd_entry_active = 0;
}


/************************** KEYBOARD ***************************/
int rnd_hid_cfg_keys_init(rnd_hid_cfg_keys_t *km)
{
	htpp_init(&km->keys, keyb_hash, keyb_eq);
	return 0;
}

static void rnd_hid_cfg_keys_free(htpp_t *ht);

static void rnd_hid_cfg_key_free(rnd_hid_cfg_keyseq_t *ns)
{
	rnd_hid_cfg_keys_free(&ns->seq_next);
	htpp_uninit(&ns->seq_next);
	free(ns);
}

static void rnd_hid_cfg_keys_free(htpp_t *ht)
{
	htpp_entry_t *e;
	for(e = htpp_first(ht); e != NULL; e = htpp_next(ht, e))
		 rnd_hid_cfg_key_free(e->value);
}

int rnd_hid_cfg_keys_uninit(rnd_hid_cfg_keys_t *km)
{
	rnd_hid_cfg_keys_free(&km->keys);
	htpp_uninit(&km->keys);
	return 0;
}

rnd_hid_cfg_keyseq_t *rnd_hid_cfg_keys_add_under(rnd_hid_cfg_keys_t *km, rnd_hid_cfg_keyseq_t *parent, rnd_hid_cfg_mod_t mods, unsigned short int key_raw, unsigned short int key_tr, int terminal, const char **errmsg)
{
	rnd_hid_cfg_keyseq_t *ns;
	rnd_hid_cfg_keyhash_t addr;
	htpp_t *phash = (parent == NULL) ? &km->keys : &parent->seq_next;

	/* do not grow the tree under actions */
	if ((parent != NULL) && (parent->action_node != NULL)) {
		if (*errmsg != NULL)
			*errmsg = "unreachable multikey combo: prefix of the multikey stroke already has an action";
		return NULL;
	}

	addr.mods = mods;
	addr.key_raw = key_raw;
	addr.key_tr = key_tr;

	/* already in the tree */
	ns = htpp_get(phash, &addr);
	if (ns != NULL) {
		if (terminal) {
			if (*errmsg != NULL)
				*errmsg = "duplicate: already registered";
			return NULL; /* full-path-match is collision */
		}
		return ns;
	}

	/* new node on this level */
	ns = calloc(sizeof(rnd_hid_cfg_keyseq_t), 1);
	if (!terminal)
		htpp_init(&ns->seq_next, keyb_hash, keyb_eq);

	ns->addr.mods = mods;
	ns->addr.key_raw = key_raw;
	ns->addr.key_tr = key_tr;

	htpp_set(phash, &ns->addr, ns);
	return ns;
}

const rnd_hid_cfg_keytrans_t rnd_hid_cfg_key_default_trans[] = {
	{ "semicolon", ';'  },
	{ NULL,        0    },
};

static unsigned short int translate_key(rnd_hid_cfg_keys_t *km, const char *desc, int len)
{
	char tmp[256];

	if ((km->auto_chr) && (len == 1))
			return *desc;

	if (len > sizeof(tmp)-1) {
		rnd_message(RND_MSG_ERROR, "key sym name too long\n");
		return 0;
	}
	strncpy(tmp, desc, len);
	tmp[len] = '\0';

	if (km->auto_tr != NULL) {
		const rnd_hid_cfg_keytrans_t *t;
		for(t = km->auto_tr; t->name != NULL; t++) {
			if (rnd_strcasecmp(tmp, t->name) == 0) {
				tmp[0] = t->sym;
				tmp[1] = '\0';
				len = 1;
				break;
			}
		}
	}

	return km->translate_key(tmp, len);
}

static int parse_keydesc(rnd_hid_cfg_keys_t *km, const char *keydesc, rnd_hid_cfg_mod_t *mods, unsigned short int *key_raws, unsigned short int *key_trs, int arr_len, const lht_node_t *loc)
{
	const char *curr, *next, *last, *k;
	int slen, len;

	slen = 0;
	curr = keydesc;
	do {
		if (slen >= arr_len)
			return -1;
		while(isspace(*curr)) curr++;
		if (*curr == '\0')
			break;
		next = strchr(curr, ';');
		if (next != NULL) {
			len = next - curr;
			while(*next == ';') next++;
		}
		else
			len = strlen(curr);

		mods[slen] = parse_mods(curr, &last, len);

		k = strchr(last, '<');
		if (k == NULL) {
			rnd_message(RND_MSG_ERROR, "Missing <key> in the key description: '%s' at %s:%ld\n", keydesc, loc->file_name, loc->line+1);
			return -1;
		}
		len -= k-last;
		k++; len--;
		if (rnd_strncasecmp(k, "key>", 4) == 0) {
			k+=4; len-=4;
			key_raws[slen] = translate_key(km, k, len);
			key_trs[slen] = 0;
		}
		else if (rnd_strncasecmp(k, "char>", 5) == 0) {
			k+=5; len-=5;
			key_raws[slen] = 0;
			if (!isalnum(*k))
				key_trs[slen] = *k;
			else
				key_trs[slen] = 0;
			k++; len--;
		}
		else {
			rnd_message(RND_MSG_ERROR, "Missing <key> or <char> in the key description starting at %s at %s:%ld\n", k-1, loc->file_name, loc->line+1);
			return -1;
		}

		if ((key_raws[slen] == 0) && (key_trs[slen] == 0)) {
			char *s;
			s = malloc(len+1);
			memcpy(s, k, len);
			s[len] = '\0';
			rnd_message(RND_MSG_ERROR, "Unrecognised key symbol in key description: %s at %s:%ld\n", s, loc->file_name, loc->line+1);
			free(s);
			return -1;
		}

		slen++;
		curr = next;
	} while(curr != NULL);
	return slen;
}

int rnd_hid_cfg_keys_add_by_strdesc_(rnd_hid_cfg_keys_t *km, const char *keydesc, const lht_node_t *action_node, rnd_hid_cfg_keyseq_t **out_seq, int out_seq_len)
{
	rnd_hid_cfg_mod_t mods[RND_HIDCFG_MAX_KEYSEQ_LEN];
	unsigned short int key_raws[RND_HIDCFG_MAX_KEYSEQ_LEN];
	unsigned short int key_trs[RND_HIDCFG_MAX_KEYSEQ_LEN];
	rnd_hid_cfg_keyseq_t *lasts;
	int slen, n;
	const char *errmsg;

	slen = parse_keydesc(km, keydesc, mods, key_raws, key_trs, RND_HIDCFG_MAX_KEYSEQ_LEN, action_node);
	if (slen <= 0)
		return slen;

	if ((out_seq != NULL) && (slen >= out_seq_len))
		return -1;

/*	printf("KEY insert\n");*/

	lasts = NULL;
	for(n = 0; n < slen; n++) {
		rnd_hid_cfg_keyseq_t *s;
		int terminal = (n == slen-1);

/*		printf(" mods=%x sym=%x\n", mods[n], key_chars[n]);*/

		s = rnd_hid_cfg_keys_add_under(km, lasts, mods[n], key_raws[n], key_trs[n], terminal, &errmsg);
		if (s == NULL) {
			rnd_message(RND_MSG_ERROR, "Failed to add hotkey binding: %s: %s\n", keydesc, errmsg);
			return -1; /* no need to free anything, uninit will recursively free the leftover at exit */
		}
		if (terminal)
			s->action_node = action_node;

		if (out_seq != NULL)
			out_seq[n] = s;
		lasts = s;
	}

	return slen;
}

int rnd_hid_cfg_keys_add_by_strdesc(rnd_hid_cfg_keys_t *km, const char *keydesc, const lht_node_t *action_node)
{
	return rnd_hid_cfg_keys_add_by_strdesc_(km, keydesc, action_node, NULL, 0);
}

int rnd_hid_cfg_keys_add_by_desc_(rnd_hid_cfg_keys_t *km, const lht_node_t *keydescn, const lht_node_t *action_node, rnd_hid_cfg_keyseq_t **out_seq, int out_seq_len)
{
	switch(keydescn->type) {
		case LHT_TEXT: return rnd_hid_cfg_keys_add_by_strdesc_(km, keydescn->data.text.value, action_node, out_seq, out_seq_len);
		case LHT_LIST:
		{
			int ret = -1, cnt;
			lht_node_t *n;
			for(n = keydescn->data.list.first, cnt = 0; n != NULL; n = n->next, cnt++) {
				if (n->type != LHT_TEXT)
					break;
				if (cnt == 0)
					ret = rnd_hid_cfg_keys_add_by_strdesc_(km, n->data.text.value, action_node, out_seq, out_seq_len);
				else
					rnd_hid_cfg_keys_add_by_strdesc_(km, n->data.text.value, action_node, NULL, 0);
			}
			return ret;
		}
		default:;
	}
	return -1;
}

int rnd_hid_cfg_keys_add_by_desc(rnd_hid_cfg_keys_t *km, const lht_node_t *keydescn, const lht_node_t *action_node)
{
	return rnd_hid_cfg_keys_add_by_desc_(km, keydescn, action_node, NULL, 0);
}

int rnd_hid_cfg_keys_del_by_strdesc(rnd_hid_cfg_keys_t *km, const char *keydesc)
{
	rnd_hid_cfg_mod_t mods[RND_HIDCFG_MAX_KEYSEQ_LEN];
	unsigned short int key_raws[RND_HIDCFG_MAX_KEYSEQ_LEN];
	unsigned short int key_trs[RND_HIDCFG_MAX_KEYSEQ_LEN];
	int slen, n;
	htpp_t *phash = &km->keys;
	rnd_hid_cfg_keyseq_t *ns;
	rnd_hid_cfg_keyhash_t addr;
	htpp_entry_t *e;

	slen = parse_keydesc(km, keydesc, mods, key_raws, key_trs, RND_HIDCFG_MAX_KEYSEQ_LEN, NULL);
	if (slen <= 0)
		return slen;

	for(n = 0; n < slen; n++) {
		int terminal = (n == slen-1);

		addr.mods = mods[n];
		addr.key_raw = key_raws[n];
		addr.key_tr = key_trs[n];

		ns = htpp_get(phash, &addr);
		if (ns == NULL)
			return -1;

		if (!terminal)
			phash = &ns->seq_next;
	}

	e = htpp_popentry(phash, &addr);
	if (e != NULL)
		rnd_hid_cfg_key_free(e->value);

	return 0;
}

int rnd_hid_cfg_keys_del_by_desc(rnd_hid_cfg_keys_t *km, const lht_node_t *keydescn)
{
	switch(keydescn->type) {
		case LHT_TEXT: return rnd_hid_cfg_keys_del_by_strdesc(km, keydescn->data.text.value);
		case LHT_LIST:
		{
			int ret = 0;
			lht_node_t *n;
			for(n = keydescn->data.list.first; n != NULL; n = n->next) {
				if (n->type != LHT_TEXT)
					break;
				ret |= rnd_hid_cfg_keys_del_by_strdesc(km, n->data.text.value);
			}
			return ret;
		}
		default:;
	}
	return -1;
}


static void gen_accel(gds_t *s, rnd_hid_cfg_keys_t *km, const char *keydesc, int *cnt, const char *sep, const lht_node_t *loc)
{
	rnd_hid_cfg_mod_t mods[RND_HIDCFG_MAX_KEYSEQ_LEN];
	unsigned short int key_raws[RND_HIDCFG_MAX_KEYSEQ_LEN];
	unsigned short int key_trs[RND_HIDCFG_MAX_KEYSEQ_LEN];
	int slen, n;

	slen = parse_keydesc(km, keydesc, mods, key_raws, key_trs, RND_HIDCFG_MAX_KEYSEQ_LEN, loc);
	if (slen <= 0)
		return;

	if (*cnt > 0)
		gds_append_str(s, sep);

	for(n = 0; n < slen; n++) {
		char buff[64];

		if (n > 0)
			gds_append(s, ' ');

		
		if (key_raws[n]) {
			if (km->key_name(key_raws[n], buff, sizeof(buff)) != 0)
				strcpy(buff, "<unknown>");
		}
		else if (key_trs[n] != 0) {
			buff[0] = key_trs[n];
			buff[1] ='\0';
		}
		else
			strcpy(buff, "<empty?>");

		if (mods[n] & RND_M_Alt)   gds_append_str(s, "Alt-");
		if (mods[n] & RND_M_Ctrl)  gds_append_str(s, "Ctrl-");
		if (mods[n] & RND_M_Shift) gds_append_str(s, "Shift-");
		gds_append_str(s, buff);
	}
}

char *rnd_hid_cfg_keys_gen_accel(rnd_hid_cfg_keys_t *km, const lht_node_t *keydescn, unsigned long mask, const char *sep)
{
	gds_t s;
	int cnt = 0;

	memset(&s, 0, sizeof(s));

	switch(keydescn->type) {
		case LHT_TEXT:
			if (mask & 1)
				gen_accel(&s, km, keydescn->data.text.value, &cnt, sep, keydescn);
			break;
		case LHT_LIST:
		{
			int cnt;
			lht_node_t *n;
			for(n = keydescn->data.list.first, cnt = 0; n != NULL; n = n->next, cnt++, mask >>= 1) {
				if (n->type != LHT_TEXT)
					break;
				if (!(mask & 1))
					continue;
				gen_accel(&s, km, n->data.text.value, &cnt, sep, keydescn);
			}
		}
		default:;
	}
	return s.array;
}

char *rnd_hid_cfg_keys_gen_desc(rnd_hid_cfg_mod_t mods, unsigned short int key_raw, unsigned short int key_tr)
{
	gds_t s = {0};

	if (key_tr != 0) {
		if ((key_tr <= 32) || (key_tr >= 127))
			return NULL;
		gds_append_str(&s, "<char>");
		gds_append(&s, key_tr);
		return s.array;
	}


	TODO("We should call km->key_name, but where this call is coming from, km is not available");
	if ((key_raw <= 32) || (key_raw >= 127))
		return NULL;

	if (mods & RND_M_Alt)   gds_append_str(&s, "Alt-");
	if (mods & RND_M_Ctrl)  gds_append_str(&s, "Ctrl-");
	if (mods & RND_M_Shift) gds_append_str(&s, "Shift-");
	if (s.used > 0) s.used--; /* remove the trailing '-' */
	gds_append_str(&s, "<key>");
	gds_append(&s, key_raw);
	return s.array;
}


int rnd_hid_cfg_keys_input_(rnd_design_t *hl, rnd_hid_cfg_keys_t *km, rnd_hid_cfg_mod_t mods, unsigned short int key_raw, unsigned short int key_tr, rnd_hid_cfg_keyseq_t **seq, int *seq_len)
{
	rnd_hid_cfg_keyseq_t *ns;
	rnd_hid_cfg_keyhash_t addr;
	htpp_t *phash = (*seq_len == 0) ? &km->keys : &((seq[(*seq_len)-1])->seq_next);

	if (key_raw == 0) {
		static int warned = 0;
		if (!warned) {
			rnd_message(RND_MSG_ERROR, "Keyboard translation error: probably the US layout is not enabled. Some hotkeys may not work properly\n");
			warned = 1;
		}
		/* happens if only the translated version is available. Try to reverse
		   engineer it as good as we can. */
		if (isalpha(key_tr)) {
			if (isupper(key_tr)) {
				key_raw = tolower(key_tr);
				mods |= RND_M_Shift;
			}
			else
				key_raw = key_tr;
		}
		else if (isdigit(key_tr))
			key_raw = key_tr;
		/* the rest: punctuations should be handled by key_tr; special keys: well... */
	}

	/* first check for base key + mods */
	addr.mods = mods;
	addr.key_raw = key_raw;
	addr.key_tr = 0;
	
	ns = htpp_get(phash, &addr);

	/* if not found, try with translated key + limited mods */
	if (ns == NULL) {
		addr.mods = mods & RND_M_Ctrl;
		addr.key_raw = 0;
		addr.key_tr = key_tr;
		ns = htpp_get(phash, &addr);
	}

	/* already in the tree */
	if (ns == NULL) {
		(*seq_len) = 0;
		return -1;
	}

	seq[*seq_len] = ns;
	(*seq_len)++;

	/* found a terminal node with an action */
	if (ns->action_node != NULL) {
		km->seq_len_action = *seq_len;
		(*seq_len) = 0;
		rnd_event(hl, RND_EVENT_USER_INPUT_KEY, NULL);
		return km->seq_len_action;
	}

	rnd_event(hl, RND_EVENT_USER_INPUT_KEY, NULL);
	return 0;
}


/*** key translation hash ***/
static int xlate_conf_rev = -1; /* last conf rev the translation table got checked on */
static int xlate_avail = 0;
static htpp_t xlate_hash;

static void xlate_uninit(void)
{
	genht_uninit_deep(htpp, &xlate_hash, {
		free(htent->key);
		free(htent->value);
	});
}

static void xlate_reload(rnd_hid_cfg_keys_t *km, rnd_conf_native_t *nat)
{
	gdl_iterator_t it;
	rnd_conf_listitem_t *kt;

	if (xlate_avail) {
		xlate_uninit();
		xlate_avail = 0;
	}

	if (rnd_conflist_length(nat->val.list) < 1)
		return;

	htpp_init(&xlate_hash, keyb_hash, keyb_eq);
	rnd_conflist_foreach(nat->val.list, &it, kt) {
		rnd_hid_cfg_keyhash_t k, v, *hk, *hv;
		rnd_hid_cfg_mod_t m;

		/* parse key and value to temporary k & v */
		if (parse_keydesc(km, kt->name, &m, &k.key_raw, &k.key_tr, 1, kt->prop.src) != 1) {
			rnd_message(RND_MSG_ERROR, "Invalid key translation left side: '%s'\n (config problem, check your editor/translate_key)\n", kt->name);
			continue;
		}
		k.mods = m;
		if (parse_keydesc(km, kt->payload, &m, &v.key_raw, &v.key_tr, 1, kt->prop.src) != 1) {
			rnd_message(RND_MSG_ERROR, "Invalid key translation left side: '%s'\n (config problem, check your editor/translate_key)\n", kt->payload);
			continue;
		}
		v.mods = m;
		hv = htpp_get(&xlate_hash, &k);
		if (hv != NULL) {
			rnd_message(RND_MSG_ERROR, "Ignoring redundant key translation: '%s'\n (config problem, check your editor/translate_key)\n", kt->payload);
			continue;
		}
		else {
			/* new key-val pair, add */
			hk = malloc(sizeof(rnd_hid_cfg_keyhash_t)); memcpy(hk, &k, sizeof(k));
			hv = malloc(sizeof(rnd_hid_cfg_keyhash_t)); memcpy(hv, &v, sizeof(v));
			htpp_insert(&xlate_hash, hk, hv);
		}
	}
	xlate_avail = 1;
}

int rnd_hid_cfg_keys_input(rnd_design_t *hl, rnd_hid_cfg_keys_t *km, rnd_hid_cfg_mod_t mods, unsigned short int key_raw, unsigned short int key_tr)
{
	rnd_hid_cfg_keyhash_t ck, *cv;

	if (rnd_conf_rev > xlate_conf_rev) {
		rnd_conf_native_t *nat = rnd_conf_get_field("editor/translate_key");
		if ((nat != NULL) && (nat->rnd_conf_rev > xlate_conf_rev))
			xlate_reload(km, nat);
		xlate_conf_rev = rnd_conf_rev;
	}

	if (xlate_avail) {
		/* apply the key translation table from our config */
		ck.mods = mods;
		ck.key_raw = key_raw;
		ck.key_tr = 0;

		/* check by raw... */
		cv = htpp_get(&xlate_hash, &ck);
		if (cv == NULL) {
			/* if not found also check by translated */
			ck.key_raw = 0;
			ck.key_tr = key_tr;
			cv = htpp_get(&xlate_hash, &ck);
		}

		/* hit: replace input key from the xlate table */
		if (cv != NULL) {
			mods = cv->mods;
			if (cv->key_raw != 0)
				key_tr = key_raw = cv->key_raw;
			if (cv->key_tr != 0)
				key_tr = key_raw = cv->key_tr;
		}
	}

	return rnd_hid_cfg_keys_input_(hl, km, mods, key_raw, key_tr, km->seq, &km->seq_len);
}


/*** key translation hash ends **/

int rnd_hid_cfg_keys_action_(rnd_design_t *hl, rnd_hid_cfg_keyseq_t **seq, int seq_len)
{
	int res;

	if (seq_len < 1)
		return -1;

	res = rnd_hid_cfg_action(hl, seq[seq_len-1]->action_node);
	rnd_event(hl, RND_EVENT_USER_INPUT_POST, NULL);
	return res;
}

int rnd_hid_cfg_keys_action(rnd_design_t *hl, rnd_hid_cfg_keys_t *km)
{
	int ret = rnd_hid_cfg_keys_action_(hl, km->seq, km->seq_len_action);
	km->seq_len_action = 0;
	return ret;
}

int rnd_hid_cfg_keys_seq_(rnd_hid_cfg_keys_t *km, rnd_hid_cfg_keyseq_t **seq, int seq_len, char *dst, int dst_len)
{
	int n, sum = 0;
	char *end = dst;

	dst_len -= 25; /* make room for a full key with modifiers, the \0 and the potential ellipsis */

	for(n = 0; n < seq_len; n++) {
		int k = seq[n]->addr.key_raw, mods = seq[n]->addr.mods, ll = 0, l;

		if (n != 0) {
			*end = ' ';
			end++;
			ll = 1;
		}

		if (mods & RND_M_Alt)   { strncpy(end, "Alt-", dst_len); end += 4; ll += 4; }
		if (mods & RND_M_Ctrl)  { strncpy(end, "Ctrl-", dst_len); end += 5; ll += 5; }
		if (mods & RND_M_Shift) { strncpy(end, "Shift-", dst_len); end += 6; ll += 6; }

		if (k == 0)
			k = seq[n]->addr.key_tr;

		if (km->key_name(k, end, dst_len) == 0) {
			l = strlen(end);
		}
		else {
			strncpy(end, "<unknown>", dst_len);
			l = 9;
		}

		ll += l;

		sum += ll;
		dst_len -= ll;
		end += l;

		if (dst_len <= 1) {
			strcpy(dst, " ...");
			sum += 4;
			dst_len -= 4;
			end += 4;
			break;
		}
	}
	*end = '\0';
	return sum;
}

int rnd_hid_cfg_keys_seq(rnd_hid_cfg_keys_t *km, char *dst, int dst_len)
{
	if (km->seq_len_action > 0)
		return rnd_hid_cfg_keys_seq_(km, km->seq, km->seq_len_action, dst, dst_len);
	else
		return rnd_hid_cfg_keys_seq_(km, km->seq, km->seq_len, dst, dst_len);
}

void rnd_hid_cfg_keys_uninit_module(void)
{
	if (xlate_avail)
		xlate_uninit();
}

