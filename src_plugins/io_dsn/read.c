/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  dsn IO plugin - file format read, parser
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <stdio.h>
#include <gensexpr/gsxl.h>
#include <ctype.h>

#include "board.h"
#include "data.h"
#include "plug_io.h"
#include "error.h"
#include "pcb_bool.h"
#include "safe_fs.h"
#include "compat_misc.h"
#include "layer_grp.h"

#include "read.h"

typedef struct {
	gsxl_dom_t dom;
	pcb_board_t *pcb;
	const pcb_unit_t *unit;
	unsigned has_pcb_boundary:1;
} dsn_read_t;

static char *STR(gsxl_node_t *node)
{
	if (node == NULL)
		return NULL;
	return node->str;
}

static char *STRE(gsxl_node_t *node)
{
	if (node == NULL)
		return "";
	if (node->str == NULL)
		return "";
	return node->str;
}

/* check if node is named name and if so, save the node in nname for
   later reference; assumes node->str is not NULL */
#define if_save_uniq(node, name) \
	if (pcb_strcasecmp(node->str, #name) == 0) { \
		if (n ## name != NULL) { \
			pcb_message(PCB_MSG_ERROR, "Multiple " #name " nodes where only one is expected (at %ld:%ld)\n", (long)node->line, (long)node->col); \
			return -1; \
		} \
		n ## name = node; \
	}

static pcb_coord_t COORD(dsn_read_t *ctx, gsxl_node_t *n)
{
	char *end, *s = STRE(n);
	double v = strtod(s, &end);

	if (*end != '\0') {
		pcb_message(PCB_MSG_ERROR, "Invalid coord: '%s' (at %ld:%ld)\n", s, (long)n->line, (long)n->col);
		return 0;
	}
	v /= ctx->unit->scale_factor;
	if (ctx->unit->family == PCB_UNIT_METRIC)
		return PCB_MM_TO_COORD(v);
	return PCB_MIL_TO_COORD(v);
}

static const pcb_unit_t *push_unit(dsn_read_t *ctx, gsxl_node_t *nu)
{
	const pcb_unit_t *old = ctx->unit;

	if ((nu == NULL) || (nu->children == NULL))
		return ctx->unit;
	
	ctx->unit = get_unit_struct(STRE(gsxl_children(nu)));
	if (ctx->unit == NULL) {
		pcb_message(PCB_MSG_ERROR, "Invalid unit: '%s' (at %ld:%ld)\n", STRE(gsxl_children(nu)), (long)nu->line, (long)nu->col);
		return NULL;
	}

	return old;
}

static void pop_unit(dsn_read_t *ctx, const pcb_unit_t *saved)
{
	ctx->unit = saved;
}

/*** tree parse ***/

static int dsn_parse_rule(dsn_read_t *ctx, gsxl_node_t *bnd)
{
#warning TODO
	return 0;
}

static void boundary_line(pcb_layer_t *oly, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	pcb_line_new(oly, x1, y1, x2, 2, PCB_MM_TO_COORD(0.1), 0, pcb_no_flags());
}


static int dsn_parse_boundary(dsn_read_t *ctx, gsxl_node_t *bnd)
{
	gsxl_node_t *n;
	pcb_layer_id_t olid;
	pcb_layer_t *oly;

	ctx->has_pcb_boundary = 0;
	if (bnd == NULL)
		goto none;

	if (pcb_layer_list(ctx->pcb, PCB_LYT_BOUNDARY, &olid, 1) < 1) {
		pcb_message(PCB_MSG_ERROR, "Intenal error: no boundary layer found\n");
		return -1;
	}
	oly = pcb_get_layer(ctx->pcb->Data, olid);

	for(bnd = bnd->children; bnd != NULL; bnd = bnd->next) {
		if (bnd->str == NULL)
			continue;
		if (pcb_strcasecmp(bnd->str, "path") == 0) {
			pcb_coord_t x, y, lx, ly, fx, fy;
			int len;

			n = gsxl_children(bnd);
			if (pcb_strcasecmp(STRE(n), "pcb") == 0) {
				pcb_message(PCB_MSG_ERROR, "PCB boundary shall be a rect, not a path;\naccepting the path, but other software may choke on this file\n");
				ctx->has_pcb_boundary = 1;
			}
			for(len = 0, n = n->next; n != NULL; len++) {
				x = COORD(ctx, n);
				if (n->next == NULL) {
					pcb_message(PCB_MSG_ERROR, "Not enough coordinate values (missing y)\n");
					break;
				}
				n = n->next;
				y = COORD(ctx, n);
				n = n->next;
				if (len == 0) {
					fx = x;
					fy = y;
				}
				else
					boundary_line(oly, lx, ly, x, y);
				lx = x;
				ly = y;
			}
			if ((x != fx) && (y != fy)) /* close the boundary */
				boundary_line(oly, lx, ly, x, y);
		}
		if (pcb_strcasecmp(bnd->str, "rect") == 0) {
			n = gsxl_children(bnd);
			if (pcb_strcasecmp(STRE(n), "pcb") == 0)
				ctx->has_pcb_boundary = 1;
		}
		if (pcb_strcasecmp(bnd->str, "rule") == 0) {
			if (dsn_parse_rule(ctx, bnd) != 0)
				return -1;
		}
	}

	none:;
#warning TODO: make up the boundary later on from bbox
	if (!ctx->has_pcb_boundary)
		pcb_message(PCB_MSG_ERROR, "Missing pcb boundary; every dsn design must have a pcb boundary.\ntrying to make up one using the bounding box.\n");
	return 0;
}

#define CHECK_TOO_MANY_LAYERS(node, num) \
do { \
	if (num >= PCB_MAX_LAYERGRP) { \
		pcb_message(PCB_MSG_ERROR, "Too many layer groups in the layer stack (at %ld:%ld)\n", (long)node->line, (long)node->col); \
		return -1; \
	} \
} while(0)

static int dsn_parse_struct(dsn_read_t *ctx, gsxl_node_t *str)
{
	const pcb_dflgmap_t *m;
	gsxl_node_t *n, *nboundary = NULL;
	pcb_layergrp_t *topcop = NULL, *botcop = NULL;

	if (str == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can not parse board without a structure subtree\n");
		return -1;
	}

	pcb_layergrp_inhibit_inc();
	for(m = pcb_dflgmap; m <= pcb_dflgmap_last_top_noncopper; m++) {
		CHECK_TOO_MANY_LAYERS(str, ctx->pcb->LayerGroups.len);
		pcb_layergrp_set_dflgly(ctx->pcb, &ctx->pcb->LayerGroups.grp[ctx->pcb->LayerGroups.len++], m, NULL, NULL);
	}

	for(n = str->children; n != NULL; n = n->next) {
		if (n->str == NULL)
			continue;
		else if (pcb_strcasecmp(n->str, "layer") == 0) {
			if (botcop != NULL) {
				CHECK_TOO_MANY_LAYERS(n, ctx->pcb->LayerGroups.len);
				pcb_layergrp_set_dflgly(ctx->pcb, &ctx->pcb->LayerGroups.grp[ctx->pcb->LayerGroups.len++], &pcb_dflg_substrate, NULL, NULL);
			}
			CHECK_TOO_MANY_LAYERS(n, ctx->pcb->LayerGroups.len);
			botcop = &ctx->pcb->LayerGroups.grp[ctx->pcb->LayerGroups.len++];
			if (topcop == NULL)
				topcop = botcop;
			pcb_layergrp_set_dflgly(ctx->pcb, botcop, &pcb_dflg_int_copper, STR(gsxl_children(n)), STR(gsxl_children(n)));
		}
		else if_save_uniq(n, boundary)
	}

	if (topcop == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can not parse board without a copper layers\n");
		return -1;
	}

	topcop->ltype = PCB_LYT_TOP | PCB_LYT_COPPER;
	botcop->ltype = PCB_LYT_BOTTOM | PCB_LYT_COPPER;

	CHECK_TOO_MANY_LAYERS(str, ctx->pcb->LayerGroups.len);
	pcb_layergrp_set_dflgly(ctx->pcb, &ctx->pcb->LayerGroups.grp[ctx->pcb->LayerGroups.len++], &pcb_dflg_outline, NULL, NULL);

	for(m = pcb_dflgmap_first_bottom_noncopper; m->name != NULL; m++) {
		CHECK_TOO_MANY_LAYERS(str, ctx->pcb->LayerGroups.len);
		pcb_layergrp_set_dflgly(ctx->pcb, &ctx->pcb->LayerGroups.grp[ctx->pcb->LayerGroups.len++], m, NULL, NULL);
	}

	pcb_layergrp_inhibit_dec();

	if (dsn_parse_boundary(ctx, nboundary) != 0)
		return -1;

	return 0;
}

static int dsn_parse_pcb(dsn_read_t *ctx, gsxl_node_t *root)
{
	gsxl_node_t *n, *nunit = NULL, *nstructure = NULL, *nplacement = NULL, *nlibrary = NULL, *nnetwork = NULL, *nwiring = NULL, *ncolors = NULL, *nresolution = NULL;

	/* default unit in case the file does not specify one */
	ctx->unit = get_unit_struct("inch");

	for(n = root->children->next; n != NULL; n = n->next) {
		if (n->str == NULL)
			continue;
		else if_save_uniq(n, unit)
		else if_save_uniq(n, resolution)
		else if_save_uniq(n, structure)
		else if_save_uniq(n, placement)
		else if_save_uniq(n, library)
		else if_save_uniq(n, network)
		else if_save_uniq(n, wiring)
		else if_save_uniq(n, colors)
	}

	if ((nresolution != NULL) && (nresolution->children != NULL)) {
		ctx->unit = get_unit_struct(STRE(gsxl_children(nresolution)));
		if (ctx->unit == NULL) {
			pcb_message(PCB_MSG_ERROR, "Invalid resolution unit: '%s'\n", STRE(gsxl_children(nresolution)));
			return -1;
		}
	}

	if (push_unit(ctx, nunit) == NULL)
		return -1;

	if (dsn_parse_struct(ctx, nstructure) != 0)
		return -1;

	return 0;
}

/*** glue and low level ***/


int io_dsn_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
{
	char line[1024], *s;
	int phc = 0, in_pcb = 0, lineno = 0;

	if (typ != PCB_IOT_PCB)
		return 0;

	while(!(feof(f)) && (lineno < 512)) {
		if (fgets(line, sizeof(line), f) != NULL) {
			lineno++;
			for(s = line; *s != '\0'; s++)
				if (*s == '(')
					phc++;
			s = line;
			if ((phc > 0) && (strstr(s, "pcb") != 0))
				in_pcb = 1;
			if ((phc > 2) && in_pcb && (strstr(s, "space_in_quoted_tokens") != 0))
				return 1;
			if ((phc > 2) && in_pcb && (strstr(s, "host_cad") != 0))
				return 1;
			if ((phc > 2) && in_pcb && (strstr(s, "host_version") != 0))
				return 1;
		}
	}

	/* hit eof before seeing a valid root -> bad */
	return 0;
}

static int dsn_parse_file(dsn_read_t *rdctx, const char *fn)
{
	int c, blen = -1;
	gsx_parse_res_t res;
	FILE *f;
	char buff[12];
	long q_offs = -1, offs;


	f = pcb_fopen(fn, "r");
	if (f == NULL)
		return -1;

	/* find the offset of the quote char so it can be ignored during the s-expression parse */
	offs = 0;
	while(!(feof(f))) {
		c = fgetc(f);
		if (c == 's')
			blen = 0;
		if (blen >= 0) {
			buff[blen] = c;
			if (blen == 12) {
				if (memcmp(buff, "string_quote", 12) == 0) {
					for(c = fgetc(f),offs++; isspace(c); c = fgetc(f),offs++) ;
					q_offs = offs;
					printf("quote is %c at %ld\n", c, q_offs);
					break;
				}
				blen = -1;
			}
			blen++;
		}
		offs++;
	}
	rewind(f);


	gsxl_init(&rdctx->dom, gsxl_node_t);
	rdctx->dom.parse.line_comment_char = '#';
	offs = 0;
	do {
		c = fgetc(f);
		if (offs == q_offs) /* need to ignore the quote char else it's an unbalanced quote */
			c = '.';
		offs++;
	} while((res = gsxl_parse_char(&rdctx->dom, c)) == GSX_RES_NEXT);

	fclose(f);
	if (res != GSX_RES_EOE)
		return -1;

	return 0;
}

int io_dsn_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *Filename, conf_role_t settings_dest)
{
	dsn_read_t rdctx;
	gsxl_node_t *rn;
	int ret;

	memset(&rdctx, 0, sizeof(rdctx));
	if (dsn_parse_file(&rdctx, Filename) != 0) {
		pcb_message(PCB_MSG_ERROR, "s-expression parse error\n");
		goto error;
	}

	gsxl_compact_tree(&rdctx.dom);
	rn = rdctx.dom.root;
	if ((rn == NULL) || (rn->str == NULL) || (pcb_strcasecmp(rn->str, "pcb") != 0)) {
		pcb_message(PCB_MSG_ERROR, "Root node should be pcb, got %s instead\n", rn->str);
		goto error;
	}
	if (gsxl_next(rn) != NULL) {
		pcb_message(PCB_MSG_ERROR, "Multiple root nodes?!\n");
		goto error;
	}

	rdctx.pcb = pcb;
	ret = dsn_parse_pcb(&rdctx, rn);
	gsxl_uninit(&rdctx.dom);
	return ret;

	error:;
	gsxl_uninit(&rdctx.dom);
	return -1;
}

