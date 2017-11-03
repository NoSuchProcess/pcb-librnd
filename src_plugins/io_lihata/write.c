/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016, 2017 Tibor 'Igor2' Palinkas
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

/* Build an in-memory lihata document that represents the board then save it.
   A document is built for the merge-save. */

#include <stdio.h>
#include <stdarg.h>
#include <liblihata/tree.h>
#include <libminuid/libminuid.h>
#include "config.h"
#include "board.h"
#include "data.h"
#include "plugins.h"
#include "plug_io.h"
#include "flag_str.h"
#include "compat_misc.h"
#include "rats_patch.h"
#include "hid_actions.h"
#include "misc_util.h"
#include "macro.h"
#include "layer.h"
#include "common.h"
#include "write_style.h"
#include "io_lihata.h"
#include "paths.h"
#include "obj_subc_list.h"
#include "pcb_minuid.h"
#include "safe_fs.h"

/*#define CFMT "%[9]"*/
#define CFMT "%.08$$mH"
/*#define CFMT "%$$mn"*/

static int io_lihata_full_tree = 0;
static int wrver;

static lht_node_t *build_data(pcb_data_t *data);

#define lht_dom_list_append_safe(lst, item) \
do { \
	if (item != NULL) \
		lht_dom_list_append(lst, item); \
} while(0)

/* An invalid node will kill any existing node on an overwrite-save-merge */
static lht_node_t *dummy_node(const char *name)
{
	lht_node_t *n;
	n = lht_dom_node_alloc(LHT_TEXT, name);
	n->type = LHT_INVALID_TYPE;
	return n;
}

static lht_node_t *dummy_text_node(const char *name)
{
	if (io_lihata_full_tree) {
		lht_node_t *n = lht_dom_node_alloc(LHT_TEXT, name);
		n->data.text.value = pcb_strdup("");
		return n;
	}
	return dummy_node(name);
}

static lht_node_t *build_text(const char *key, const char *value)
{
	lht_node_t *field;

	if (value == NULL)
		return dummy_text_node(key);

	field = lht_dom_node_alloc(LHT_TEXT, key);
	if (value != NULL)
		field->data.text.value = pcb_strdup(value);
	return field;
}

static lht_node_t *build_textf(const char *key, const char *fmt, ...)
{
	lht_node_t *field;
	va_list ap;

	field = lht_dom_node_alloc(LHT_TEXT, key);
	va_start(ap, fmt);
	field->data.text.value = pcb_strdup_vprintf(fmt, ap);
	va_end(ap);
	return field;
}

static lht_node_t *build_minuid(const char *key, minuid_bin_t val)
{
	minuid_str_t tmp;
	minuid_bin2str(tmp, val);
	return build_text(key, tmp);
}

static lht_node_t *build_board_meta(pcb_board_t *pcb)
{
	lht_node_t *meta, *grp;

	meta = lht_dom_node_alloc(LHT_HASH, "meta");
	lht_dom_hash_put(meta, build_text("board_name", pcb->Name));

	grp = lht_dom_node_alloc(LHT_HASH, "grid");
	lht_dom_hash_put(meta, grp);
	lht_dom_hash_put(grp, build_textf("offs_x", CFMT, pcb->GridOffsetX));
	lht_dom_hash_put(grp, build_textf("offs_y", CFMT, pcb->GridOffsetY));
	lht_dom_hash_put(grp, build_textf("spacing", CFMT, pcb->Grid));

	grp = lht_dom_node_alloc(LHT_HASH, "size");
	lht_dom_hash_put(meta, grp);
	lht_dom_hash_put(grp, build_textf("x", CFMT, pcb->MaxWidth));
	lht_dom_hash_put(grp, build_textf("y", CFMT, pcb->MaxHeight));
	lht_dom_hash_put(grp, build_textf("isle_area_nm2", "%f", pcb->IsleArea));
	lht_dom_hash_put(grp, build_textf("thermal_scale", "%f", pcb->ThermScale));


	grp = lht_dom_node_alloc(LHT_HASH, "drc");
	lht_dom_hash_put(meta, grp);
	lht_dom_hash_put(grp, build_textf("bloat",     CFMT, pcb->Bloat));
	lht_dom_hash_put(grp, build_textf("shrink",    CFMT, pcb->Shrink));
	lht_dom_hash_put(grp, build_textf("min_width", CFMT, pcb->minWid));
	lht_dom_hash_put(grp, build_textf("min_silk",  CFMT, pcb->minSlk));
	lht_dom_hash_put(grp, build_textf("min_drill", CFMT, pcb->minDrill));
	lht_dom_hash_put(grp, build_textf("min_ring",  CFMT, pcb->minRing));

	grp = lht_dom_node_alloc(LHT_HASH, "cursor");
	lht_dom_hash_put(meta, grp);
	lht_dom_hash_put(grp, build_textf("x", CFMT, pcb->CursorX));
	lht_dom_hash_put(grp, build_textf("y", CFMT, pcb->CursorY));
	lht_dom_hash_put(grp, build_textf("zoom", "%.6f", pcb->Zoom));

	return meta;
}

static lht_node_t *build_attributes(pcb_attribute_list_t *lst)
{
	int n;
	lht_node_t *ln;

	if ((lst->Number == 0) && (!io_lihata_full_tree))
		return dummy_node("attributes");
	ln = lht_dom_node_alloc(LHT_HASH, "attributes");

	for (n = 0; n < lst->Number; n++) {
		if ((wrver < 3) && (strcmp(lst->List[n].name, "intconn") == 0))
			continue; /* do not write intconn as attribute for v1 and v2, we used a flag for those */
		lht_dom_hash_put(ln, build_text(lst->List[n].name, lst->List[n].value));
	}

	return ln;
}

static lht_node_t *build_flags(pcb_flag_t *f, int object_type, int intconn)
{
	int n, layer, added = 0, thrm = 0;
	lht_node_t *hsh, *txt, *lst;
	io_lihata_flag_holder fh;


	fh.Flags = *f;

	hsh = lht_dom_node_alloc(LHT_HASH, "flags");

	/* create normal flag nodes */
	for (n = 0; n < pcb_object_flagbits_len; n++) {
		if ((pcb_object_flagbits[n].object_types & object_type) && (PCB_FLAG_TEST(pcb_object_flagbits[n].mask, &fh))) {
			lht_dom_hash_put(hsh, build_text(pcb_object_flagbits[n].name, "1"));
			PCB_FLAG_CLEAR(pcb_object_flagbits[n].mask, &fh);
			added++;
		}
	}

	/* thermal flags per layer */
	lst = lht_dom_node_alloc(LHT_HASH, "thermal");

	for(layer = 0; layer < pcb_max_layer; layer++) {
		if (PCB_FLAG_THERM_TEST_ANY(&fh)) {
			int t = PCB_FLAG_THERM_GET(layer, &fh);
			if (t != 0) {
				const char *name;
				txt = lht_dom_node_alloc(LHT_TEXT, PCB->Data->Layer[layer].name);
				name = io_lihata_thermal_style(t);
				if (name != NULL)
					txt->data.text.value = pcb_strdup(name);
				else
					txt->data.text.value = pcb_strdup_printf("%d", t);
				lht_dom_hash_put(lst, txt);
				added++;
				thrm++;
			}
		}
	}

	if (thrm > 0)
		lht_dom_hash_put(hsh, lst);
	else
		lht_dom_node_free(lst);

	if (f->q > 0) {
		lht_dom_hash_put(hsh, build_textf("shape", "%d", f->q));
		added++;
	}

	if ((intconn > 0) && (wrver < 3)) {
		lht_dom_hash_put(hsh, build_textf("intconn", "%d", intconn));
		added++;
	}

	if ((added == 0) && (!io_lihata_full_tree)) {
		lht_dom_node_free(hsh);
		return dummy_node("flags");
	}

	return hsh;
}

static lht_node_t *build_line(pcb_line_t *line, int local_id, pcb_coord_t dx, pcb_coord_t dy, int simple)
{
	char buff[128];
	lht_node_t *obj;

	sprintf(buff, "line.%ld", (local_id >= 0 ? local_id : line->ID));
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	if (!simple) {
		lht_dom_hash_put(obj, build_attributes(&line->Attributes));
		lht_dom_hash_put(obj, build_flags(&line->Flags, PCB_TYPE_LINE, line->intconn));
		lht_dom_hash_put(obj, build_textf("clearance", CFMT, line->Clearance));
	}

	lht_dom_hash_put(obj, build_textf("thickness", CFMT, line->Thickness));
	lht_dom_hash_put(obj, build_textf("x1", CFMT, line->Point1.X+dx));
	lht_dom_hash_put(obj, build_textf("y1", CFMT, line->Point1.Y+dy));
	lht_dom_hash_put(obj, build_textf("x2", CFMT, line->Point2.X+dx));
	lht_dom_hash_put(obj, build_textf("y2", CFMT, line->Point2.Y+dy));

	return obj;
}

static lht_node_t *build_simplearc(pcb_arc_t *arc, int local_id)
{
	char buff[128];
	lht_node_t *obj;

	sprintf(buff, "simplearc.%ld", (local_id >= 0 ? local_id : arc->ID));
	obj = lht_dom_node_alloc(LHT_HASH, buff);
	lht_dom_hash_put(obj, build_textf("x", CFMT, arc->X));
	lht_dom_hash_put(obj, build_textf("y", CFMT, arc->Y));
	lht_dom_hash_put(obj, build_textf("r", CFMT, arc->Height));
	lht_dom_hash_put(obj, build_textf("astart", "%f", arc->StartAngle));
	lht_dom_hash_put(obj, build_textf("adelta", "%f", arc->Delta));
	lht_dom_hash_put(obj, build_textf("thickness", CFMT, arc->Thickness));

	return obj;
}


static lht_node_t *build_simplepoly(pcb_poly_t *poly, int local_id)
{
	char buff[128];
	lht_node_t *obj;
	pcb_point_t *pnt;
	int n;

	sprintf(buff, "simplepoly.%ld", (local_id >= 0 ? local_id : poly->ID));
	obj = lht_dom_node_alloc(LHT_LIST, buff);

	for(n = 0, pnt = poly->Points; n < poly->PointN; n++,pnt++) {
		lht_dom_list_append(obj, build_textf(NULL, CFMT, pnt->X));
		lht_dom_list_append(obj, build_textf(NULL, CFMT, pnt->Y));
	}
	return obj;
}


static lht_node_t *build_rat(pcb_rat_t *rat)
{
	char buff[128];
	lht_node_t *obj;
	sprintf(buff, "rat.%ld", rat->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&rat->Attributes));
	lht_dom_hash_put(obj, build_flags(&rat->Flags, PCB_TYPE_LINE, rat->intconn));
	lht_dom_hash_put(obj, build_textf("x1", CFMT, rat->Point1.X));
	lht_dom_hash_put(obj, build_textf("y1", CFMT, rat->Point1.Y));
	lht_dom_hash_put(obj, build_textf("x2", CFMT, rat->Point2.X));
	lht_dom_hash_put(obj, build_textf("y2", CFMT, rat->Point2.Y));
	lht_dom_hash_put(obj, build_textf("lgrp1", "%d", rat->group1));
	lht_dom_hash_put(obj, build_textf("lgrp2", "%d", rat->group2));

	return obj;
}

static lht_node_t *build_arc(pcb_arc_t *arc, pcb_coord_t dx, pcb_coord_t dy)
{
	char buff[128];
	lht_node_t *obj;

	sprintf(buff, "arc.%ld", arc->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&arc->Attributes));
	lht_dom_hash_put(obj, build_flags(&arc->Flags, PCB_TYPE_ARC, arc->intconn));
	lht_dom_hash_put(obj, build_textf("thickness", CFMT, arc->Thickness));
	lht_dom_hash_put(obj, build_textf("clearance", CFMT, arc->Clearance));
	lht_dom_hash_put(obj, build_textf("x", CFMT, arc->X+dx));
	lht_dom_hash_put(obj, build_textf("y", CFMT, arc->Y+dy));
	lht_dom_hash_put(obj, build_textf("width", CFMT, arc->Width));
	lht_dom_hash_put(obj, build_textf("height", CFMT, arc->Height));
	lht_dom_hash_put(obj, build_textf("astart", "%ma", arc->StartAngle));
	lht_dom_hash_put(obj, build_textf("adelta", "%ma", arc->Delta));

	return obj;
}

static lht_node_t *build_pin(pcb_pin_t *pin, int is_via, pcb_coord_t dx, pcb_coord_t dy)
{
	char buff[128];
	lht_node_t *obj;

	sprintf(buff, "%s.%ld", is_via ? "via" : "pin", pin->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&pin->Attributes));
	lht_dom_hash_put(obj, build_flags(&pin->Flags, PCB_TYPE_VIA, pin->intconn));
	lht_dom_hash_put(obj, build_textf("thickness", CFMT, pin->Thickness));
	lht_dom_hash_put(obj, build_textf("clearance", CFMT, pin->Clearance));
	lht_dom_hash_put(obj, build_textf("mask", CFMT, pin->Mask));
	lht_dom_hash_put(obj, build_textf("hole", CFMT, pin->DrillingHole));
	lht_dom_hash_put(obj, build_textf("x", CFMT, pin->X+dx));
	lht_dom_hash_put(obj, build_textf("y", CFMT, pin->Y+dy));
	lht_dom_hash_put(obj, build_text("name", pin->Name));
	lht_dom_hash_put(obj, build_text("number", pin->Number));
	return obj;
}

static lht_node_t *build_pad(pcb_pad_t *pad, pcb_coord_t dx, pcb_coord_t dy)
{
	char buff[128];
	lht_node_t *obj;

	sprintf(buff, "pad.%ld", pad->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&pad->Attributes));
	lht_dom_hash_put(obj, build_flags(&pad->Flags, PCB_TYPE_PAD, pad->intconn));
	lht_dom_hash_put(obj, build_textf("thickness", CFMT, pad->Thickness));
	lht_dom_hash_put(obj, build_textf("clearance", CFMT, pad->Clearance));
	lht_dom_hash_put(obj, build_textf("mask", CFMT, pad->Mask));
	lht_dom_hash_put(obj, build_textf("x1", CFMT, pad->Point1.X+dx));
	lht_dom_hash_put(obj, build_textf("y1", CFMT, pad->Point1.Y+dy));
	lht_dom_hash_put(obj, build_textf("x2", CFMT, pad->Point2.X+dx));
	lht_dom_hash_put(obj, build_textf("y2", CFMT, pad->Point2.Y+dy));
	lht_dom_hash_put(obj, build_text("name", pad->Name));
	lht_dom_hash_put(obj, build_text("number", pad->Number));
	return obj;
}

static lht_node_t *build_polygon(pcb_poly_t *poly)
{
	char buff[128];
	lht_node_t *obj, *tbl, *geo;
	pcb_cardinal_t n, hole = 0;

	sprintf(buff, "polygon.%ld", poly->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&poly->Attributes));
	lht_dom_hash_put(obj, build_flags(&poly->Flags, PCB_TYPE_POLYGON, poly->intconn));

	if ((wrver >= 3) && (poly->Clearance > 0))
		lht_dom_hash_put(obj, build_textf("clearance", CFMT, poly->Clearance));

	geo = lht_dom_node_alloc(LHT_LIST, "geometry");
	lht_dom_hash_put(obj, geo);

	tbl = lht_dom_node_alloc(LHT_TABLE, "contour");
	tbl->data.table.cols = 2;
	lht_dom_list_append(geo, tbl);

	for(n = 0; n < poly->PointN; n++) {
		int row;
		if ((hole < poly->HoleIndexN) && (n == poly->HoleIndex[hole])) {
			tbl = lht_dom_node_alloc(LHT_TABLE, "hole");
			tbl->data.table.cols = 2;
			lht_dom_list_append(geo, tbl);
			hole++;
		}

		row = tbl->data.table.rows;
		lht_tree_table_ins_row(tbl, row);
		tbl->data.table.r[row][0] = build_textf(NULL, CFMT, poly->Points[n].X);
		tbl->data.table.r[row][1] = build_textf(NULL, CFMT, poly->Points[n].Y);
	}

	return obj;
}

static lht_node_t *build_pcb_text(const char *role, pcb_text_t *text)
{
	char buff[128];
	lht_node_t *obj;

	sprintf(buff, "text.%ld", text->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&text->Attributes));
	lht_dom_hash_put(obj, build_flags(&text->Flags, PCB_TYPE_TEXT, text->intconn));
	lht_dom_hash_put(obj, build_text("string", text->TextString));
	lht_dom_hash_put(obj, build_textf("fid", "%ld", text->fid));
	lht_dom_hash_put(obj, build_textf("scale", "%d", text->Scale));
	lht_dom_hash_put(obj, build_textf("direction", "%d", text->Direction));
	lht_dom_hash_put(obj, build_textf("x", CFMT, text->X));
	lht_dom_hash_put(obj, build_textf("y", CFMT, text->Y));

	if (role != NULL)
		lht_dom_hash_put(obj, build_text("role", role));

	return obj;
}

static lht_node_t *build_element(pcb_element_t *elem)
{
	char buff[128];
	pcb_line_t *li;
	pcb_arc_t *ar;
	pcb_pin_t *pi;
	pcb_pad_t *pa;
	lht_node_t *obj, *lst;

	sprintf(buff, "element.%ld", elem->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&elem->Attributes));
	lht_dom_hash_put(obj, build_flags(&elem->Flags, PCB_TYPE_ELEMENT, 0));


	/* build drawing primitives */
	lst = lht_dom_node_alloc(LHT_LIST, "objects");
	lht_dom_hash_put(obj, lst);

	lht_dom_list_append(lst, build_pcb_text("desc", &elem->Name[PCB_ELEMNAME_IDX_DESCRIPTION]));
	lht_dom_list_append(lst, build_pcb_text("name", &elem->Name[PCB_ELEMNAME_IDX_REFDES]));
	lht_dom_list_append(lst, build_pcb_text("value", &elem->Name[PCB_ELEMNAME_IDX_VALUE]));
	lht_dom_hash_put(obj, build_textf("x", CFMT, elem->MarkX));
	lht_dom_hash_put(obj, build_textf("y", CFMT, elem->MarkY));


	for(li = linelist_first(&elem->Line); li != NULL; li = linelist_next(li))
		lht_dom_list_append(lst, build_line(li, -1, -elem->MarkX, -elem->MarkY, 0));

	for(ar = arclist_first(&elem->Arc); ar != NULL; ar = arclist_next(ar))
		lht_dom_list_append(lst, build_arc(ar, -elem->MarkX, -elem->MarkY));

	for(pi = pinlist_first(&elem->Pin); pi != NULL; pi = pinlist_next(pi))
		lht_dom_list_append(lst, build_pin(pi, 0, -elem->MarkX, -elem->MarkY));

	for(pa = padlist_first(&elem->Pad); pa != NULL; pa = padlist_next(pa))
		lht_dom_list_append(lst, build_pad(pa, -elem->MarkX, -elem->MarkY));

	return obj;
}

static lht_node_t *build_subc_element(pcb_subc_t *sc)
{
#warning subc TODO: implement conversion for backward compatibility
	pcb_message(PCB_MSG_ERROR, "Can't save subcircuits in lihata versions lower than v3\n");
	return NULL;
}

static lht_node_t *build_subc(pcb_subc_t *sc)
{
	char buff[128];
	lht_node_t *obj;

	if (wrver < 3)
		return build_subc_element(sc);

	sprintf(buff, "subc.%ld", sc->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&sc->Attributes));
	lht_dom_hash_put(obj, build_flags(&sc->Flags, PCB_TYPE_SUBC, 0));
	lht_dom_hash_put(obj, build_data(sc->data));
	lht_dom_hash_put(obj, build_minuid("uid", sc->uid));

	return obj;
}

static void build_layer_stack_flag(void *ctx, pcb_layer_type_t bit, const char *name, int class, const char *class_name)
{
	lht_node_t *dst = ctx;
	lht_dom_hash_put(dst, build_text(name, "1"));
}

static void build_data_layer_comb(void *ctx, pcb_layer_combining_t bit, const char *name)
{
	lht_node_t *comb = ctx;
	lht_dom_hash_put(comb, build_text(name, "1"));
}


static lht_node_t *build_padstack_protos(pcb_vtpadstack_proto_t *pp)
{
	lht_node_t *lst, *nproto, *nmask, *nshape, *nshapelst, *ncomb, *nshapeo;
	pcb_cardinal_t n, sn, pn;
	pcb_padstack_tshape_t *ts;

	lst = lht_dom_node_alloc(LHT_LIST, "padstack_prototypes");
	for(n = 0; n < pcb_vtpadstack_proto_len(pp); n++) {
		pcb_padstack_proto_t *proto = pp->array+n;

		if (!proto->in_use) {
			lht_dom_list_append(lst, build_text("unused", "1"));
			continue;
		}

		lht_dom_list_append(lst, nproto = lht_dom_node_alloc(LHT_HASH, "ps_proto_v4"));

		lht_dom_hash_put(nproto, build_textf("hdia", CFMT, proto->hdia));
		lht_dom_hash_put(nproto, build_textf("htop", "%d", proto->htop));
		lht_dom_hash_put(nproto, build_textf("hbottom", "%d", proto->hbottom));
		lht_dom_hash_put(nproto, build_textf("hplated", "%d", proto->hplated));

		/* save each shape */
		lht_dom_hash_put(nproto, nshapelst = lht_dom_node_alloc(LHT_LIST, "shape"));
		ts = &proto->tr.array[0]; /* save the canonical shape only, the transformation cache is generated runtime */
		for(sn = 0; sn < ts->len; sn++) {
			pcb_padstack_shape_t *shape = ts->shape + sn;

			lht_dom_list_append(nshapelst, nshape = lht_dom_node_alloc(LHT_HASH, "ps_shape_v4"));

			lht_dom_hash_put(nshape, nmask = lht_dom_node_alloc(LHT_HASH, "layer_mask"));
			pcb_layer_type_map(shape->layer_mask, nmask, build_layer_stack_flag);

			lht_dom_hash_put(nshape, ncomb = lht_dom_node_alloc(LHT_HASH, "combining"));
			pcb_layer_comb_map(shape->comb, ncomb, build_data_layer_comb);

			lht_dom_hash_put(nshape, build_textf("clearance", CFMT, shape->clearance));

			switch(shape->shape) {
				case PCB_PSSH_POLY:
					nshapeo = lht_dom_node_alloc(LHT_LIST, "ps_poly");
					for(pn = 0; pn < shape->data.poly.len; pn++) {
						lht_dom_list_append(nshapeo, build_textf(NULL, CFMT, shape->data.poly.x[pn]));
						lht_dom_list_append(nshapeo, build_textf(NULL, CFMT, shape->data.poly.y[pn]));
					}
					break;
				case PCB_PSSH_LINE:
					nshapeo = lht_dom_node_alloc(LHT_HASH, "ps_line");
					lht_dom_hash_put(nshapeo, build_textf("x1", CFMT, shape->data.line.x1));
					lht_dom_hash_put(nshapeo, build_textf("y1", CFMT, shape->data.line.y1));
					lht_dom_hash_put(nshapeo, build_textf("x2", CFMT, shape->data.line.x2));
					lht_dom_hash_put(nshapeo, build_textf("y2", CFMT, shape->data.line.y2));
					lht_dom_hash_put(nshapeo, build_textf("thickness", CFMT, shape->data.line.thickness));
					lht_dom_hash_put(nshapeo, build_textf("square", "%d", shape->data.line.square));
					break;
				case PCB_PSSH_CIRC:
					nshapeo = lht_dom_node_alloc(LHT_HASH, "ps_circ");
					lht_dom_hash_put(nproto, build_textf("x", CFMT, shape->data.circ.x));
					lht_dom_hash_put(nproto, build_textf("y", CFMT, shape->data.circ.y));
					lht_dom_hash_put(nproto, build_textf("dia", CFMT, shape->data.circ.dia));
					break;
				default:
					pcb_message(PCB_MSG_ERROR, "Internal error: unimplemented pad stack shape %d\n", shape->shape);
					abort();
			}
			lht_dom_hash_put(nshape, nshapeo);
		}
	}

	return lst;
}

static lht_node_t *build_padstack(pcb_padstack_t *ps)
{
	char buff[128];
	lht_node_t *obj, *thr;
	unsigned long n;

	sprintf(buff, "padstack_ref.%ld", ps->ID);
	obj = lht_dom_node_alloc(LHT_HASH, buff);

	lht_dom_hash_put(obj, build_attributes(&ps->Attributes));
	lht_dom_hash_put(obj, build_flags(&ps->Flags, PCB_TYPE_PADSTACK, ps->intconn));

	lht_dom_hash_put(obj, build_textf("proto", "%ld", (long int)ps->proto));
	lht_dom_hash_put(obj, build_textf("x", CFMT, ps->x));
	lht_dom_hash_put(obj, build_textf("y", CFMT, ps->y));
	lht_dom_hash_put(obj, build_textf("clearance", CFMT, ps->Clearance));
	lht_dom_hash_put(obj, thr = lht_dom_node_alloc(LHT_LIST, "thermal"));
	for(n = 0; n < ps->thermals.used; n++)
		lht_dom_list_append(thr, build_textf(NULL, "%d", ps->thermals.shape[n]));

	return obj;
}

static lht_node_t *build_layer_stack(pcb_board_t *pcb)
{
	lht_node_t *lstk, *grps, *grp, *layers, *flags;
	int n, i;

	lstk = lht_dom_node_alloc(LHT_HASH, "layer_stack");
	lht_dom_hash_put(lstk, grps = lht_dom_node_alloc(LHT_LIST, "groups"));

	for(n = 0; n < pcb->LayerGroups.len; n++) {
		pcb_layergrp_t *g = &pcb->LayerGroups.grp[n];
		char tmp[32];
		sprintf(tmp, "%d", n);
		lht_dom_list_append(grps, grp = lht_dom_node_alloc(LHT_HASH, tmp));

		lht_dom_hash_put(grp, build_text("name", g->name));
		lht_dom_hash_put(grp, layers = lht_dom_node_alloc(LHT_LIST, "layers"));
		for(i = 0; i < g->len; i++)
			lht_dom_list_append(layers, build_textf("", "%ld", g->lid[i]));

		lht_dom_hash_put(grp, flags = lht_dom_node_alloc(LHT_HASH, "type"));
		pcb_layer_type_map(g->type, flags, build_layer_stack_flag);
	}

	return lstk;
}


static lht_node_t *build_data_layer(pcb_data_t *data, pcb_layer_t *layer, pcb_layergrp_id_t layer_group, pcb_layer_id_t lid)
{
	lht_node_t *obj, *grp, *comb;
	pcb_line_t *li;
	pcb_arc_t *ar;
	pcb_poly_t *po;
	pcb_text_t *tx;
	int added = 0;

	obj = lht_dom_node_alloc(LHT_HASH, layer->name);

	if (!layer->is_bound) {
		lht_dom_hash_put(obj, build_text("visible", layer->meta.real.vis ? "1" : "0"));
		lht_dom_hash_put(obj, build_textf("group", "%ld", layer_group));
	}
	else {
		if (wrver >= 3) {
			lht_node_t *type;

			if (layer->meta.bound.stack_offs != 0)
				lht_dom_hash_put(obj, build_textf("stack_offs", "%d", layer->meta.bound.stack_offs));

			lht_dom_hash_put(obj, type = lht_dom_node_alloc(LHT_HASH, "type"));
			pcb_layer_type_map(layer->meta.bound.type, type, build_layer_stack_flag);
		}
		else
			pcb_message(PCB_MSG_WARNING, "io_lihata: attempting to save bound layers in lihata version lower than 3; feature not supported by the format.\n");
	}

	if (!layer->is_bound)
		lht_dom_hash_put(obj, build_attributes(&layer->meta.real.Attributes));

	if (wrver >= 2) {
		lht_dom_hash_put(obj, build_textf("lid", "%ld", lid));
		lht_dom_hash_put(obj, comb = lht_dom_node_alloc(LHT_HASH, "combining"));
		pcb_layer_comb_map(layer->comb, comb, build_data_layer_comb);
	}

	grp = lht_dom_node_alloc(LHT_LIST, "objects");

	for(li = linelist_first(&layer->Line); li != NULL; li = linelist_next(li)) {
		lht_dom_list_append(grp, build_line(li, -1, 0, 0, 0));
		added++;
	}

	for(ar = arclist_first(&layer->Arc); ar != NULL; ar = arclist_next(ar)) {
		lht_dom_list_append(grp, build_arc(ar, 0, 0));
		added++;
	}

	for(po = polylist_first(&layer->Polygon); po != NULL; po = polylist_next(po)) {
		lht_dom_list_append(grp, build_polygon(po));
		added++;
	}

	for(tx = textlist_first(&layer->Text); tx != NULL; tx = textlist_next(tx)) {
		lht_dom_list_append(grp, build_pcb_text(NULL, tx));
		added++;
	}

	lht_dom_hash_put(obj, grp);

	return obj;
}

#define LAYER_GID_FIX_V1() \
	do { \
		if (wrver == 1) { \
			if (gid >= 0) { \
				if (gid < sizeof(grp) / sizeof(grp[0])) \
					gid = grp[gid]; \
				else \
					gid = -1; \
			} \
		} \
	} while(0)


static lht_node_t *build_data_layers(pcb_data_t *data)
{
	long int n;
	lht_node_t *layers;
	pcb_layergrp_id_t gm, grp[PCB_MAX_LAYERGRP], gtop = -1, gbottom = -1;
	pcb_layergrp_t *g;

	layers = lht_dom_node_alloc(LHT_LIST, "layers");

	{ /* produce an old layer group assignment from top to bottom (needed for v1, good for other versions too) */
		gm = 0;
		for(n = 0; n < pcb_max_group(PCB); n++) {
			unsigned int gflg = pcb_layergrp_flags(PCB, n);
			if (gflg & PCB_LYT_COPPER) {
				if (gflg & PCB_LYT_TOP)
					gtop = gm;
				if (gflg & PCB_LYT_BOTTOM)
					gbottom = gm;
				grp[n] = gm;
/*				pcb_trace("build data layers: %d -> %ld {%ld %ld}\n", n, gm, gtop, gbottom); */
				gm++;
			}
			else
				grp[n] = -1;
		}

		g = pcb_get_grp(&PCB->LayerGroups, PCB_LYT_BOTTOM, PCB_LYT_SILK);
		grp[g - PCB->LayerGroups.grp] = gbottom;

		g = pcb_get_grp(&PCB->LayerGroups, PCB_LYT_TOP, PCB_LYT_SILK);
		grp[g - PCB->LayerGroups.grp] = gtop;
	}

	if (wrver == 1) {
		/* v1 needs to have silk at the end of the list */
		for(n = 0; n < pcb_max_layer; n++) {
			if ((pcb_layer_flags(PCB, n) & PCB_LYT_SILK) == 0) {
				pcb_layergrp_id_t gid = pcb_layer_get_group(PCB, n);
				LAYER_GID_FIX_V1();
				lht_dom_list_append(layers, build_data_layer(data, data->Layer+n, gid, n));
			}
		}
		for(n = 0; n < pcb_max_layer; n++) {
			if (pcb_layer_flags(PCB, n) & PCB_LYT_SILK) {
				pcb_layer_id_t gid = pcb_layer_get_group(PCB, n);
				LAYER_GID_FIX_V1();
				lht_dom_list_append(layers, build_data_layer(data, data->Layer+n, gid, n));
			}
		}
	}
	else {
		/* keep the original order from v2, to minimize diffs */
		for(n = 0; n < data->LayerN; n++) {
			pcb_layergrp_id_t gid = pcb_layer_get_group(PCB, n);
			lht_dom_list_append(layers, build_data_layer(data, data->Layer+n, gid, n));
		}
	}

	return layers;
}


static lht_node_t *build_data(pcb_data_t *data)
{
	lht_node_t *grp, *ndt;
	pcb_padstack_t *ps;
	pcb_pin_t *pi;
	pcb_element_t *el;
	pcb_subc_t *sc;
	gdl_iterator_t it;
	pcb_rat_t *line;

	ndt = lht_dom_node_alloc(LHT_HASH, "data");

	/* build layers */
	lht_dom_hash_put(ndt, build_data_layers(data));

	/* build a list of all global objects */
	grp = lht_dom_node_alloc(LHT_LIST, "objects");
	lht_dom_hash_put(ndt, grp);

	if (wrver >= 4) {
		lht_dom_hash_put(ndt, build_padstack_protos(&data->ps_protos));
		for(ps = padstacklist_first(&data->padstack); ps != NULL; ps = padstacklist_next(ps))
			lht_dom_list_append(grp, build_padstack(ps));
	}

	for(pi = pinlist_first(&data->Via); pi != NULL; pi = pinlist_next(pi))
		lht_dom_list_append(grp, build_pin(pi, 1, 0, 0));

	for(el = elementlist_first(&data->Element); el != NULL; el = elementlist_next(el))
		lht_dom_list_append(grp, build_element(el));

	for(sc = pcb_subclist_first(&data->subc); sc != NULL; sc = pcb_subclist_next(sc))
		lht_dom_list_append_safe(grp, build_subc(sc));

	ratlist_foreach(&data->Rat, &it, line)
		lht_dom_list_append(grp, build_rat(line));

	return ndt;
}

static lht_node_t *build_symbol(pcb_symbol_t *sym, const char *name)
{
	lht_node_t *lst, *ndt;
	pcb_line_t *li;
	pcb_poly_t *poly;
	pcb_arc_t *arc;
	int n;

	ndt = lht_dom_node_alloc(LHT_HASH, name);
	lht_dom_hash_put(ndt, build_textf("width", CFMT, sym->Width));
	lht_dom_hash_put(ndt, build_textf("height", CFMT, sym->Height));
	lht_dom_hash_put(ndt, build_textf("delta", CFMT, sym->Delta));

	lst = lht_dom_node_alloc(LHT_LIST, "objects");
	lht_dom_hash_put(ndt, lst);
	for(n = 0, li = sym->Line; n < sym->LineN; n++, li++)
		lht_dom_list_append(lst, build_line(li, n, 0, 0, 1));

	for(arc = arclist_first(&sym->arcs); arc != NULL; arc = arclist_next(arc), n++)
		lht_dom_list_append(lst, build_simplearc(arc, n));

	for(poly = polylist_first(&sym->polys); poly != NULL; poly = polylist_next(poly), n++)
		lht_dom_list_append(lst, build_simplepoly(poly, n));

	return ndt;
}

static lht_node_t *build_font(pcb_font_t *font)
{
	lht_node_t *syms, *ndt;
	char *sid, sidbuff[64];
	int n;

	if (font->id > 0) {
		sprintf(sidbuff, "%ld", font->id);
		sid = sidbuff;
	}
	else
		sid = "geda_pcb"; /* special case for comaptibility: font 0 is the geda_pcb font */

	ndt = lht_dom_node_alloc(LHT_HASH, sid);

	lht_dom_hash_put(ndt, build_textf("cell_height", CFMT, font->MaxHeight));
	lht_dom_hash_put(ndt, build_textf("cell_width", CFMT, font->MaxWidth));
	lht_dom_hash_put(ndt, build_textf("id", "%ld", font->id));
	if (font->name != NULL)
		lht_dom_hash_put(ndt, build_text("name", font->name));
/*	lht_dom_hash_put(ndt, build_symbol(&font->DefaultSymbol)); */


	syms = lht_dom_node_alloc(LHT_HASH, "symbols");
	lht_dom_hash_put(ndt, syms);
	for(n = 0; n < PCB_MAX_FONTPOSITION + 1; n++) {
		char sname[32];
		if (!font->Symbol[n].Valid)
			continue;
		if ((n <= 32) || (n > 126) || (n == '&') || (n == '#') || (n == '{') || (n == '}') || (n == '/') || (n == ':') || (n == ';') || (n == '=') || (n == '\\') || (n == ':')) {
			sprintf(sname, "&%02x", n);
		}
		else {
			sname[0] = n;
			sname[1] = '\0';
		}
		lht_dom_hash_put(syms, build_symbol(&font->Symbol[n], sname));
	}
	return ndt;
}

static lht_node_t *build_fontkit(pcb_fontkit_t *fk)
{
	lht_node_t *ndt, *frt;

	frt = lht_dom_node_alloc(LHT_HASH, "font");

	/* write the default font first - it's always there */
	ndt = build_font(&fk->dflt);
	lht_dom_hash_put(frt, ndt);

	/* write extra fonts, if there are */
	if (fk->hash_inited) {
		htip_entry_t *e;
		for(e = htip_first(&fk->fonts); e; e = htip_next(&fk->fonts, e)) {
			ndt = build_font(e->value);
			lht_dom_hash_put(frt, ndt);
		}
	}


	return frt;
}


static lht_node_t *build_styles(vtroutestyle_t *styles)
{
	lht_node_t *stl, *sn;
	int n;

	stl = lht_dom_node_alloc(LHT_LIST, "styles");
	for(n = 0; n < vtroutestyle_len(styles); n++) {
		pcb_route_style_t *s = styles->array + n;
		sn = lht_dom_node_alloc(LHT_HASH, s->name);
		lht_dom_list_append(stl, sn);
		lht_dom_hash_put(sn, build_textf("thickness", CFMT, s->Thick));
		lht_dom_hash_put(sn, build_textf("diameter", CFMT, s->Diameter));
		lht_dom_hash_put(sn, build_textf("hole", CFMT, s->Hole));
		lht_dom_hash_put(sn, build_textf("clearance", CFMT, s->Clearance));
		lht_dom_hash_put(sn, build_attributes(&s->attr));
	}
	return stl;
}

/* Build a plain old netlist */
static lht_node_t *build_netlist(pcb_lib_t *netlist, const char *name, int *nonempty)
{
	lht_node_t *nl, *pl, *pn, *nnet;
	pcb_cardinal_t n, p;

	if (netlist->MenuN < 0)
		return dummy_node(name);

	(*nonempty)++;

	nl = lht_dom_node_alloc(LHT_LIST, name);
	for (n = 0; n < netlist->MenuN; n++) {
		pcb_lib_menu_t *menu = &netlist->Menu[n];
		const char *netname = &menu->Name[2];
		const char *style = menu->Style;

		/* create the net hash */
		nnet = lht_dom_node_alloc(LHT_HASH, netname);
		pl = lht_dom_node_alloc(LHT_LIST, "conn");
		lht_dom_hash_put(nnet, pl);
		if ((style != NULL) && (*style == '\0')) style = NULL;
		lht_dom_hash_put(nnet, build_text("style", style));

		/* grow the connection list */
		for (p = 0; p < menu->EntryN; p++) {
			pcb_lib_entry_t *entry = &menu->Entry[p];
			const char *pin = entry->ListEntry;
			pn = lht_dom_node_alloc(LHT_TEXT, "");
			pn->data.text.value = pcb_strdup(pin);
			lht_dom_list_append(pl, pn);
		}
		lht_dom_list_append(nl, nnet);
	}
	return nl;
}

typedef struct {
	lht_node_t *patch, *info;
} build_net_patch_t;

static void build_net_patch_cb(void *ctx_, pcb_rats_patch_export_ev_t ev, const char *netn, const char *key, const char *val)
{
	build_net_patch_t *ctx = ctx_;
	lht_node_t *n;
	switch(ev) {
		case PCB_RPE_INFO_BEGIN:
			ctx->info = lht_dom_node_alloc(LHT_LIST, "net_info");
			lht_dom_list_append(ctx->info, build_text("net", netn));
			break;
		case PCB_RPE_INFO_TERMINAL:
			lht_dom_list_append(ctx->info, build_text("term", val));
			break;
		case PCB_RPE_INFO_END:
			lht_dom_list_append(ctx->patch, ctx->info);
			ctx->info = NULL;
			break;
		case PCB_RPE_CONN_ADD:
			n = lht_dom_node_alloc(LHT_HASH, "add_conn");
			goto append_term;

		case PCB_RPE_CONN_DEL:
			n = lht_dom_node_alloc(LHT_HASH, "del_conn");
			append_term:;
			lht_dom_hash_put(n, build_text("net", netn));
			lht_dom_hash_put(n, build_text("term", val));
			lht_dom_list_append(ctx->patch, n);
			break;

		case PCB_RPE_ATTR_CHG:
			n = lht_dom_node_alloc(LHT_HASH, "change_attrib");
			lht_dom_hash_put(n, build_text("net", netn));
			lht_dom_hash_put(n, build_text("key", key));
			lht_dom_hash_put(n, build_text("val", val));
			lht_dom_list_append(ctx->patch, n);
			break;
	}
}

/* Build a netlist patch so that we don't need to export a complete new set of "as built" netlist */
static lht_node_t *build_net_patch(pcb_board_t *pcb, pcb_ratspatch_line_t *pat, int *nonempty)
{
	lht_node_t *pn;
	build_net_patch_t ctx;

	pn = lht_dom_node_alloc(LHT_LIST, "netlist_patch");

	ctx.patch = pn;
	rats_patch_export(pcb, pat, pcb_false, build_net_patch_cb, &ctx);

	if (pn->data.list.first == NULL) {
		lht_dom_node_free(pn);
		return dummy_node("netlist_patch");
	}

	(*nonempty)++;
	return pn;
}


static lht_node_t *build_netlists(pcb_board_t *pcb, pcb_lib_t *netlists, pcb_ratspatch_line_t *pat, int num_netlists)
{
	lht_node_t *nls;
	int n, nonempty = 0;

	if (num_netlists > PCB_NUM_NETLISTS)
		return dummy_node("netlists");

	nls = lht_dom_node_alloc(LHT_HASH, "netlists");

	for(n = 0; n < num_netlists; n++) {
		lht_node_t *nl;
		if (n == PCB_NETLIST_EDITED)
			nl = build_net_patch(pcb, pat, &nonempty);
		else
			nl = build_netlist(netlists+n, pcb_netlist_names[n], &nonempty);
		lht_dom_hash_put(nls, nl);
	}

	if (!nonempty) {
		lht_dom_node_free(nls);
		return dummy_node("netlists");
	}

	return nls;
}

extern lht_doc_t *conf_root[CFR_max_alloc];
static lht_node_t *build_conf()
{
	const char **s, *del_paths[] = { "editor/mode", NULL };
	lht_node_t *root, *n;
	if ((conf_root[CFR_DESIGN] == NULL) || (conf_root[CFR_DESIGN]->root == NULL) || (conf_root[CFR_DESIGN]->root->type != LHT_LIST))
		return lht_dom_node_alloc(LHT_LIST, "pcb-rnd-conf-v1");

	root = conf_root[CFR_DESIGN]->root;
	for(n = root->data.list.first; n != NULL; n = n->next) {
		for(s = del_paths; *s != NULL; s++) {
			lht_node_t *sub = lht_tree_path_(n->doc, n, *s, 0, 0, NULL);
			if (sub != NULL) {
				lht_tree_del(sub);
			}
		}
	}

	return lht_dom_duptree(root);
}

static lht_doc_t *build_board(pcb_board_t *pcb)
{
	char vers[32];
	lht_doc_t *brd = lht_dom_init();

	sprintf(vers, "pcb-rnd-board-v%d", wrver);
	brd->root = lht_dom_node_alloc(LHT_HASH, vers);
	lht_dom_hash_put(brd->root, build_board_meta(pcb));
	if (wrver >= 2)
		lht_dom_hash_put(brd->root, build_layer_stack(pcb));
	lht_dom_hash_put(brd->root, build_data(pcb->Data));
	lht_dom_hash_put(brd->root, build_attributes(&pcb->Attributes));
	lht_dom_hash_put(brd->root, build_fontkit(&pcb->fontkit));
	lht_dom_hash_put(brd->root, build_styles(&pcb->RouteStyle));
	lht_dom_hash_put(brd->root, build_netlists(pcb, pcb->NetlistLib, pcb->NetlistPatches, PCB_NUM_NETLISTS));
	lht_dom_hash_put(brd->root, build_conf());
	return brd;
}

static lhtpers_ev_res_t check_text(void *ev_ctx, lht_perstyle_t *style, lht_node_t *inmem_node, const char *ondisk_value)
{
	/* for coords, preserve formatting as long as values match */
	if (lhtpers_rule_find(io_lihata_out_coords, inmem_node) != NULL) {
		pcb_coord_t v1, v2;
		pcb_bool success1, success2;

/*		fprintf(stderr, "SMART d='%s' m='%s'\n", ondisk_value, inmem_node->data.text.value);*/

		v1 = pcb_get_value_ex(ondisk_value, NULL, NULL, NULL, NULL, &success1);
		v2 = pcb_get_value_ex(inmem_node->data.text.value, NULL, NULL, NULL, NULL, &success2);
/*		pcb_fprintf(stderr, " %d %d | %mm %mm\n", success1, success2, v1, v2);*/
		if (success1 && success2) {
			/* smart: if values are the same, keep the on-disk version */
			if (v1 == v2)
				return LHTPERS_DISK;
			else
				return LHTPERS_MEM;
		}
		/* else fall back to the string compare below */
	}

	if (inmem_node->data.text.value == NULL)
		return LHTPERS_INHIBIT;

	/* classic method: string mismatch => overwrite from mem */
	if (strcmp(inmem_node->data.text.value, ondisk_value) != 0)
		return LHTPERS_MEM;

	return LHTPERS_DISK;
}

static void clean_invalid(lht_node_t *node)
{
	lht_node_t *n;
	lht_dom_iterator_t it;
	for(n = lht_dom_first(&it, node); n != NULL; n = lht_dom_next(&it)) {
		if (n->type == LHT_INVALID_TYPE)
			lht_tree_del(n);
		else
			clean_invalid(n);
	}
}

static int io_lihata_write_pcb(pcb_plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency, int ver)
{
	int res;
	lht_doc_t *brd;

	wrver = ver;
	brd = build_board(PCB);

#if IO_LIHATA_SAVE_BACKUP_IN_PCB
	{
		const char *fnpat = conf_io_lihata.plugins.io_lihata.aux_pcb_pattern;

		/* Backup in .pcb before saving in .lht so if the save code crashes,
		   the board is preserved. WARNING: since .pcb can't represent a lot
		   of details, it may degrade the board _before_ we save it in lht.
		   Thus this feature should be disabled */
		if ((fnpat != NULL) && (*fnpat != '\0')) {
			char *orig_fn, *end;
			char *pcb_fn = pcb_strdup_subst(fnpat, pcb_build_fn_cb, NULL, PCB_SUBST_ALL);
			
			orig_fn = PCB->Filename;
			PCB->Filename = NULL;

			/* avoid .lht.lht.pcb */
			end = pcb_fn + strlen(pcb_fn) - 1;
			if ((end-11 > pcb_fn) && (strcmp(end-11, ".lht.lht.pcb") == 0))
				strcpy(end-11, ".lht.pcb");

			fprintf(stderr, "NOTE: io_lihata_write_pcb will save to '%s' but first saves in '%s': res=%d (expected: 0)\n", new_filename, pcb_fn, pcb_save_pcb(pcb_fn, "pcb"));
			free(pcb_fn);
			
			/* restore these because SaveTo() has changed them */
			free(PCB->Filename);
			PCB->Filename = orig_fn;
			PCB->Data->loader = ctx;
		}
	}
#endif

	if ((emergency) || ((old_filename == NULL) && (new_filename == NULL))) {
		/* emergency or pipe save: use the canonical form */
		clean_invalid(brd->root); /* remove invalid nodes placed for persistency */
		res = lht_dom_export(brd->root, FP, "");
	}
	else {
		FILE *inf = NULL;
		char *errmsg;
		lhtpers_ev_t events;

		if (old_filename != NULL)
			inf = pcb_fopen(old_filename, "r");

		memset(&events, 0, sizeof(events));
		events.text = check_text;
/*		events.list_empty = check_list_empty;
		events.list_elem_removed = check_list_elem_removed;*/
		events.output_rules = io_lihata_out_rules;

		res = lhtpers_fsave_as(&events, brd, inf, FP, old_filename, &errmsg);
		if ((res == LHTPERS_ERR_ROOT_MISMATCH) || (res == LHTPERS_ERR_ROOT_MISSING)) {
			/* target is a different lht file or not even an lht file; the user requested the save, including overwrite, so do that */
			rewind(FP);
			res = lhtpers_fsave_as(&events, brd, NULL, FP, old_filename, &errmsg);
		}
		if (res != 0) {
			FILE *fe;
			char *fe_name = pcb_concat(old_filename, ".mem.lht", NULL);
			fe = pcb_fopen(fe_name, "w");
			if (fe != NULL) {
				res = lht_dom_export(brd->root, fe, "");
				fclose(fe);
			}
			pcb_message(PCB_MSG_ERROR, "lhtpers_fsave_as() failed. Please include files %s and %s and %s in your bugreport\n", inf, old_filename, fe_name);
			pcb_message(PCB_MSG_ERROR, "in case this broke your file %s, please use the emergency save %s instead.\n", old_filename, fe_name);
		}
		fflush(FP);
		if (inf != NULL)
			fclose(inf);
	}

	lht_dom_uninit(brd);
	return res;
}

int io_lihata_write_pcb_v1(pcb_plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	return io_lihata_write_pcb(ctx, FP, old_filename, new_filename, emergency, 1);
}

int io_lihata_write_pcb_v2(pcb_plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	return io_lihata_write_pcb(ctx, FP, old_filename, new_filename, emergency, 2);
}

int io_lihata_write_pcb_v3(pcb_plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	return io_lihata_write_pcb(ctx, FP, old_filename, new_filename, emergency, 3);
}

int io_lihata_write_pcb_v4(pcb_plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	return io_lihata_write_pcb(ctx, FP, old_filename, new_filename, emergency, 4);
}

int io_lihata_write_font(pcb_plug_io_t *ctx, pcb_font_t *font, const char *Filename)
{
	FILE *f;
	int res;
	lht_doc_t *doc;


	f = pcb_fopen(Filename, "w");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Failed to open font file %s for write\n", Filename);
		return -1;
	}

	/* create the doc */
	io_lihata_full_tree = 1;
	doc = lht_dom_init();
	doc->root = lht_dom_node_alloc(LHT_LIST, "pcb-rnd-font-v1");
	lht_dom_list_append(doc->root, build_font(font));

	res = lht_dom_export(doc->root, f, "");

	fclose(f);
	lht_dom_uninit(doc);
	io_lihata_full_tree = 0;
	return res;
}

static int io_lihata_dump_1st_subc(pcb_plug_io_t *ctx, FILE *f, pcb_data_t *data, int enforce1)
{
	int res;
	lht_doc_t *doc;

	if ((enforce1) && (pcb_subclist_length(&data->subc)) > 1) {
		pcb_message(PCB_MSG_ERROR, "Can't save more than one subcircuit from a buffer\n");
		return -1;
	}
	if (pcb_subclist_length(&data->subc) < 1) {
		pcb_message(PCB_MSG_ERROR, "there's no subcircuit in the buffer\n");
		return -1;
	}

	/* create the doc */
	io_lihata_full_tree = 1;
	doc = lht_dom_init();
	wrver = 3;
	doc->root = lht_dom_node_alloc(LHT_LIST, "pcb-rnd-subcircuit-v3");
	lht_dom_list_append(doc->root, build_subc(pcb_subclist_first(&data->subc)));

	res = lht_dom_export(doc->root, f, "");

	lht_dom_uninit(doc);
	io_lihata_full_tree = 0;
	return res;
}

int io_lihata_write_buffer(pcb_plug_io_t *ctx, FILE *f, pcb_buffer_t *buff, pcb_bool elem_only)
{
	if (!elem_only) {
		pcb_message(PCB_MSG_ERROR, "Can't save full buffer (yet), only a single subcircuits from a buffer\n");
		return -1;
	}

	return io_lihata_dump_1st_subc(ctx, f, buff->Data, 1);
}

int io_lihata_write_element(pcb_plug_io_t *ctx, FILE *f, pcb_data_t *dt)
{
	return io_lihata_dump_1st_subc(ctx, f, dt, 1);
}
