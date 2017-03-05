/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  netlist build helper
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <genht/hash.h>

#include "netlist_helper.h"
#include "compat_misc.h"
#include "pcb-printf.h"
#include "error.h"
#include "hid_actions.h"


nethlp_ctx_t *nethlp_new(nethlp_ctx_t *prealloc)
{
	if (prealloc == NULL) {
		prealloc = malloc(sizeof(nethlp_ctx_t));
		prealloc->alloced = 1;
	}
	else
		prealloc->alloced = 0;

	prealloc->part_rules = NULL;
	htsp_init(&prealloc->id2refdes, strhash, strkeyeq);
	return prealloc;
}

void nethlp_destroy(nethlp_ctx_t *nhctx)
{
	htsp_entry_t *e;
	nethlp_rule_t *r, *next;

	for(r = nhctx->part_rules; r != NULL; r = next) {
		next = r->next;
		re_se_free(r->key);
		re_se_free(r->val);
		free(r->new_key);
		free(r->new_val);
		free(r);
	}

	for (e = htsp_first(&nhctx->id2refdes); e; e = htsp_next(&nhctx->id2refdes, e)) {
		free(e->key);
		free(e->value);
	}
	htsp_uninit(&nhctx->id2refdes);
	if (nhctx->alloced)
		free(nhctx);
}

#define ltrim(s) while(isspace(*s)) s++;

static int part_map_split(char *s, char *argv[], int maxargs)
{
	int argc;
	int n;

	ltrim(s);
	if ((*s == '#') || (*s == '\0'))
		return 0;

	for(argc = n = 0; n < maxargs; n++) {
		argv[argc] = s;
		if (s != NULL) {
			argc++;
			s = strchr(s, '|');
			if (s != NULL) {
				*s = '\0';
				s++;
			}
		}
	}
	return argc;
}

static int part_map_parse(nethlp_ctx_t *nhctx, int argc, char *argv[], const char *fn, int lineno)
{
	char *end;
	nethlp_rule_t *r;
	re_se_t *kr, *vr;
	int prio;

	if (argc != 5) {
		pcb_message(PCB_MSG_ERROR, "Loading part map: wrong number of fields %d in %s:%d - expected 5 - ignoring this rule\n", argc, fn, lineno);
		return -1;
	}
	if (*argv[0] != '*') {
		prio = strtol(argv[0], &end, 10);
		if (*end != '\0') {
			pcb_message(PCB_MSG_ERROR, "Loading part map: invaid priority '%s' in %s:%d - ignoring this rule\n", argv[0], fn, lineno);
			return -1;
		}
	}
	else
		prio = nethlp_prio_always;
	kr = re_se_comp(argv[1]);
	if (kr == NULL) {
		pcb_message(PCB_MSG_ERROR, "Loading part map: can't compile attribute name regex in %s:%d - ignoring this rule\n", fn, lineno);
		return -1;
	}
	vr = re_se_comp(argv[2]);
	if (vr == NULL) {
		re_se_free(kr);
		pcb_message(PCB_MSG_ERROR, "Loading part map: can't compile attribute value regex in %s:%d - ignoring this rule\n", fn, lineno);
		return -1;
	}

	r = malloc(sizeof(nethlp_rule_t));
	r->prio = prio;
	r->key = kr;
	r->val = vr;
	r->new_key = pcb_strdup(argv[3]);
	r->new_val = pcb_strdup(argv[4]);
	r->next = nhctx->part_rules;
	nhctx->part_rules = r;

	return 0;
}

int nethlp_load_part_map(nethlp_ctx_t *nhctx, const char *fn)
{
	FILE *f;
	int cnt, argc, lineno;
	char line[1024], *argv[8];

	f = fopen(fn, "r");
	if (f == NULL)
		return -1;

	lineno = 0;
	while(fgets(line, sizeof(line), f) != NULL) {
		lineno++;
		argc = part_map_split(line, argv, 6);
		if ((argc > 0) && (part_map_parse(nhctx, argc, argv, fn, lineno) == 0)) {
/*			printf("MAP %d '%s' '%s' '%s' '%s' '%s'\n", argc, argv[0], argv[1], argv[2], argv[3], argv[4]);*/
			cnt++;
		}
	}

	fclose(f);
	return cnt;
}


nethlp_elem_ctx_t *nethlp_elem_new(nethlp_ctx_t *nhctx, nethlp_elem_ctx_t *prealloc, const char *id)
{
	if (prealloc == NULL) {
		prealloc = malloc(sizeof(nethlp_elem_ctx_t));
		prealloc->alloced = 1;
	}
	else
		prealloc->alloced = 0;
	prealloc->nhctx = nhctx;
	prealloc->id = pcb_strdup(id);
	htsp_init(&prealloc->attr, strhash, strkeyeq);
	return prealloc;
}

void nethlp_elem_refdes(nethlp_elem_ctx_t *ectx, const char *refdes)
{
	htsp_set(&ectx->nhctx->id2refdes, pcb_strdup(ectx->id), pcb_strdup(refdes));
}

void nethlp_elem_attr(nethlp_elem_ctx_t *ectx, const char *key, const char *val)
{
	htsp_set(&ectx->attr, pcb_strdup(key), pcb_strdup(val));
}

void nethlp_elem_done(nethlp_elem_ctx_t *ectx)
{
	htsp_entry_t *e;

	printf("Elem '%s' -> '%s'\n", ectx->id, (char *)htsp_get(&ectx->nhctx->id2refdes, ectx->id));

	for (e = htsp_first(&ectx->attr); e; e = htsp_next(&ectx->attr, e)) {
		free(e->key);
		free(e->value);
	}
	htsp_uninit(&ectx->attr);
	free(ectx->id);
	if (ectx->alloced)
		free(ectx);
}


nethlp_net_ctx_t *nethlp_net_new(nethlp_ctx_t *nhctx, nethlp_net_ctx_t *prealloc, const char *netname)
{
	if (prealloc == NULL) {
		prealloc = malloc(sizeof(nethlp_net_ctx_t));
		prealloc->alloced = 1;
	}
	else
		prealloc->alloced = 0;
	prealloc->nhctx = nhctx;

	prealloc->netname = pcb_strdup(netname);
	return prealloc;
}

void nethlp_net_add_term(nethlp_net_ctx_t *nctx, const char *part, const char *pin)
{
	char *refdes = htsp_get(&nctx->nhctx->id2refdes, part);
	char term[256];
	if (refdes == NULL) {
		pcb_message(PCB_MSG_ERROR, "nethelper: can't resolve refdes of part %s\n", part);
	}
	pcb_snprintf(term, sizeof(term), "%s:%s", refdes, pin);
	pcb_hid_actionl("Netlist", "Add",  nctx->netname, term, NULL);
}

void nethlp_net_destroy(nethlp_net_ctx_t *nctx)
{
	free(nctx->netname);
	if (nctx->alloced)
		free(nctx);
}

