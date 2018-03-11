/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#include "mesh.h"
#include "layer.h"
#include "layer_ui.h"
#include "board.h"
#include "data.h"
#include "hid_dad.h"
#include "event.h"

static pcb_mesh_t mesh;
static const char *mesh_ui_cookie = "mesh ui layer cookie";

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	int dens_obj, dens_gap, min_space, smooth, hor, ver, noimpl;
	int bnd[6], subslines, air_top, air_bot, dens_air, smoothz, max_air;
} mesh_dlg_t;
static mesh_dlg_t ia;


#if 1
	static void mesh_trace(const char *fmt, ...) { }
#else
#	define mesh_trace pcb_trace
#endif


static void mesh_add_edge(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, pcb_coord_t crd)
{
	vtc0_append(&mesh->line[dir].edge, crd);
}

static void mesh_add_range(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, pcb_coord_t c1, pcb_coord_t c2, pcb_coord_t dens)
{
	pcb_range_t *r = vtr0_alloc_append(&mesh->line[dir].dens, 1);
	r->begin = c1;
	r->end = c2;
	r->data[0].c = dens;
}

static void mesh_add_obj(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, pcb_coord_t c1, pcb_coord_t c2, int aligned)
{
	if (aligned) {
		mesh_add_edge(mesh, dir, c1 - mesh->dens_obj * 2 / 3);
		mesh_add_edge(mesh, dir, c1 + mesh->dens_obj * 1 / 3);
		mesh_add_edge(mesh, dir, c2 - mesh->dens_obj * 1 / 3);
		mesh_add_edge(mesh, dir, c2 + mesh->dens_obj * 2 / 3);
	}
	mesh_add_range(mesh, dir, c1, c2, mesh->dens_obj);
}

/* generate edges and ranges looking at objects on the given layer */
static int mesh_gen_obj(pcb_mesh_t *mesh, pcb_mesh_dir_t dir)
{
	pcb_line_t *line;
	pcb_line_t *arc;
	pcb_poly_t *poly;
	gdl_iterator_t it;
	

	linelist_foreach(&mesh->layer->Line, &it, line) {
		pcb_coord_t x1 = line->Point1.X, y1 = line->Point1.Y, x2 = line->Point2.X, y2 = line->Point2.Y;
		int aligned = (x1 == x2) || (y1 == y2);

		switch(dir) {
			case PCB_MESH_HORIZONTAL:
				if (y1 < y2)
					mesh_add_obj(mesh, dir, y1 - line->Thickness/2, y2 + line->Thickness/2, aligned);
				else
					mesh_add_obj(mesh, dir, y2 - line->Thickness/2, y1 + line->Thickness/2, aligned);
				break;
			case PCB_MESH_VERTICAL:
				if (x1 < x2)
					mesh_add_obj(mesh, dir, x1 - line->Thickness/2, x2 + line->Thickness/2, aligned);
				else
					mesh_add_obj(mesh, dir, x2 - line->Thickness/2, x1 + line->Thickness/2, aligned);
				break;
			default: break;
		}
	}

	arclist_foreach(&mesh->layer->Arc, &it, arc) {
		/* no point in encorcinf 1/3 2/3 rule, just set the range */
		switch(dir) {
			case PCB_MESH_HORIZONTAL: mesh_add_range(mesh, dir, arc->BoundingBox.Y1 + arc->Clearance/2, arc->BoundingBox.Y2 - arc->Clearance/2, mesh->dens_obj); break;
			case PCB_MESH_VERTICAL:   mesh_add_range(mesh, dir, arc->BoundingBox.X1 + arc->Clearance/2, arc->BoundingBox.X2 - arc->Clearance/2, mesh->dens_obj); break;
			default: break;
		}
	}

#warning TODO: text and padstacks

	polylist_foreach(&mesh->layer->Polygon, &it, poly) {
		pcb_poly_it_t it;
		pcb_polyarea_t *pa;

		for(pa = pcb_poly_island_first(poly, &it); pa != NULL; pa = pcb_poly_island_next(&it)) {
			pcb_coord_t x, y;
			pcb_pline_t *pl;
			int go;

			pl = pcb_poly_contour(&it);
			if (pl != NULL) {
				pcb_coord_t lx, ly, minx, miny, maxx, maxy;
				
				pcb_poly_vect_first(&it, &minx, &miny);
				maxx = minx;
				maxy = miny;
				pcb_poly_vect_peek_prev(&it, &lx, &ly);
				/* find axis aligned contour edges for the 2/3 1/3 rule */
				for(go = pcb_poly_vect_first(&it, &x, &y); go; go = pcb_poly_vect_next(&it, &x, &y)) {
					switch(dir) {
						case PCB_MESH_HORIZONTAL:
							if (y == ly) {
								int sign = (x < lx) ? +1 : -1;
								mesh_add_edge(mesh, dir, y - sign * mesh->dens_obj * 2 / 3);
								mesh_add_edge(mesh, dir, y + sign * mesh->dens_obj * 1 / 3);
							}
							break;
						case PCB_MESH_VERTICAL:
							if (x == lx) {
								int sign = (y < ly) ? +1 : -1;
								mesh_add_edge(mesh, dir, x - sign * mesh->dens_obj * 2 / 3);
								mesh_add_edge(mesh, dir, x + sign * mesh->dens_obj * 1 / 3);
							}
							break;
						default: break;
					}
					lx = x;
					ly = y;
					if (x < minx) minx = x;
					if (y < miny) miny = y;
					if (x > maxx) maxx = x;
					if (y > maxy) maxy = y;
				}
				switch(dir) {
					case PCB_MESH_HORIZONTAL: mesh_add_range(mesh, dir, miny, maxy, mesh->dens_obj); break;
					case PCB_MESH_VERTICAL:   mesh_add_range(mesh, dir, minx, maxx, mesh->dens_obj); break;
					default: break;
				}
				/* Note: holes can be ignored: holes are sorrunded by polygons, the grid is dense over them already */
			}
		}
	}
	return 0;
}

static int cmp_coord(const void *v1, const void *v2)
{
	const pcb_coord_t *c1 = v1, *c2 = v2;
	return *c1 < *c2 ? -1 : +1;
}

static int cmp_range(const void *v1, const void *v2)
{
	const pcb_range_t *c1 = v1, *c2 = v2;
	return c1->begin < c2->begin ? -1 : +1;
}


typedef struct {
	pcb_coord_t min;
	pcb_coord_t max;
	int found;
} mesh_maybe_t;

static int cmp_maybe_add(const void *k, const void *v)
{
	const mesh_maybe_t *ctx = k;
	const pcb_coord_t *c = v;

	if ((*c >= ctx->min) && (*c <= ctx->max))
		return 0;
	if (*c < ctx->min)
		return +1;
	return -1;
}

static void mesh_maybe_add_edge(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, pcb_coord_t at, pcb_coord_t dist)
{
	mesh_maybe_t ctx;
	pcb_coord_t *c;
	ctx.min = at - dist;
	ctx.max = at + dist;
	ctx.found = 0;

	c = bsearch(&ctx, mesh->line[dir].edge.array, vtc0_len(&mesh->line[dir].edge), sizeof(pcb_coord_t), cmp_maybe_add);
	if (c == NULL) {
#warning TODO: optimization: run a second bsearch and insert instead of this; testing: 45 deg line (won't have axis aligned edge for the 2/3 1/3 rule)
		vtc0_append(&mesh->line[dir].edge, at);
		qsort(mesh->line[dir].edge.array, vtc0_len(&mesh->line[dir].edge), sizeof(pcb_coord_t), cmp_coord);
	}
}

static int mesh_sort(pcb_mesh_t *mesh, pcb_mesh_dir_t dir)
{
	size_t n;
	pcb_range_t *r;

	if (vtr0_len(&mesh->line[dir].dens) < 1) {
		pcb_message(PCB_MSG_ERROR, "There are not enough objects to do the meshing\n");
		return -1;
	}

	qsort(mesh->line[dir].edge.array, vtc0_len(&mesh->line[dir].edge), sizeof(pcb_coord_t), cmp_coord);
	qsort(mesh->line[dir].dens.array, vtr0_len(&mesh->line[dir].dens), sizeof(pcb_range_t), cmp_range);

	/* merge edges too close */
	for(n = 0; n < vtc0_len(&mesh->line[dir].edge)-1; n++) {
		pcb_coord_t c1 = mesh->line[dir].edge.array[n], c2 = mesh->line[dir].edge.array[n+1];
		if (c2 - c1 < mesh->min_space) {
			mesh->line[dir].edge.array[n] = (c1 + c2) / 2;
			vtc0_remove(&mesh->line[dir].edge, n+1, 1);
		}
	}


	/* merge overlapping ranges of the same density */
	for(n = 0; n < vtr0_len(&mesh->line[dir].dens)-1; n++) {
		pcb_range_t *r1 = &mesh->line[dir].dens.array[n], *r2 = &mesh->line[dir].dens.array[n+1];
		if (r1->data[0].c != r2->data[0].c) continue;
		if (r2->begin < r1->end) {
			if (r2->end > r1->end)
				r1->end = r2->end;
			vtr0_remove(&mesh->line[dir].dens, n+1, 1);
			n--; /* make sure to check the next range against the current one, might be overlapping as well */
		}
	}

	/* continous ranges: fill in the gaps */
	for(n = 0; n < vtr0_len(&mesh->line[dir].dens)-1; n++) {
		pcb_range_t *r1 = &mesh->line[dir].dens.array[n], *r2 = &mesh->line[dir].dens.array[n+1];
		if (r1->end < r2->begin) {
			pcb_coord_t my_end = r2->begin; /* the insert will change r2 pointer */
			pcb_range_t *r = vtr0_alloc_insert(&mesh->line[dir].dens, n+1, 1);
			r->begin = r1->end;
			r->end = my_end;
			r->data[0].c = mesh->dens_gap;
			n++; /* no need to check the new block */
		}
	}

	/* make sure there's a forced mesh line at region transitions */
	for(n = 0; n < vtr0_len(&mesh->line[dir].dens); n++) {
		pcb_range_t *r = &mesh->line[dir].dens.array[n];
		if (n == 0)
			mesh_maybe_add_edge(mesh, dir, r->begin, mesh->dens_gap);
		mesh_maybe_add_edge(mesh, dir, r->end, mesh->dens_gap);
	}

	/* continous ranges: start and end */
	r = vtr0_alloc_insert(&mesh->line[dir].dens, 0, 1);
	r->begin = 0;
	r->end = mesh->line[dir].dens.array[1].begin;
	r->data[0].c = mesh->dens_gap;

	r = vtr0_alloc_append(&mesh->line[dir].dens, 1);
	r->begin = mesh->line[dir].dens.array[vtr0_len(&mesh->line[dir].dens)-2].end;
	r->end = (dir == PCB_MESH_HORIZONTAL) ? PCB->MaxHeight : PCB->MaxWidth;
	r->data[0].c = mesh->dens_gap;


	return 0;
}

static int cmp_range_at(const void *key_, const void *v_)
{
	const pcb_coord_t *key = key_;
	const pcb_range_t *v = v_;

	if ((*key >= v->begin) && (*key <= v->end))
		return 0;
	if (*key < v->begin) return -1;
	return +1;
}

static pcb_range_t *mesh_find_range(const vtr0_t *v, pcb_coord_t at, pcb_coord_t *dens, pcb_coord_t *dens_left, pcb_coord_t *dens_right)
{
	pcb_range_t *r;
	r = bsearch(&at, v->array, vtr0_len((vtr0_t *)v), sizeof(pcb_range_t), cmp_range_at);
	if (dens != NULL)
		*dens = r->data[0].c;
	if (dens_left != NULL) {
		if (r == v->array)
			*dens_left = r->data[0].c;
		else
			*dens_left = r[-1].data[0].c;
	}
	if (dens_right != NULL) {
		if (r == v->array+v->used-1)
			*dens_right = r->data[0].c;
		else
			*dens_right = r[+1].data[0].c;
	}
	return r;
}

static void mesh_draw_line(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, pcb_coord_t at, pcb_coord_t aux1, pcb_coord_t aux2, pcb_coord_t thick)
{
	if (dir == PCB_MESH_HORIZONTAL)
		pcb_line_new(mesh->ui_layer, aux1, at, aux2, at, thick, 0, pcb_no_flags());
	else
		pcb_line_new(mesh->ui_layer, at, aux1, at, aux2, thick, 0, pcb_no_flags());
}

static void mesh_draw_range(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, pcb_coord_t at1, pcb_coord_t at2, pcb_coord_t aux, pcb_coord_t thick)
{
	if (dir == PCB_MESH_HORIZONTAL)
		pcb_line_new(mesh->ui_layer, aux, at1, aux, at2, thick, 0, pcb_no_flags());
	else
		pcb_line_new(mesh->ui_layer, at1, aux, at2, aux, thick, 0, pcb_no_flags());
}

static void mesh_draw_label(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, pcb_coord_t aux, const char *label)
{
	aux -= PCB_MM_TO_COORD(0.6);
	if (dir == PCB_MESH_HORIZONTAL)
		pcb_text_new(mesh->ui_layer, pcb_font(PCB, 0, 0), aux, 0, 1, 75, label, pcb_no_flags());
	else
		pcb_text_new(mesh->ui_layer, pcb_font(PCB, 0, 0), 0, aux, 0, 75, label, pcb_no_flags());

}

static int mesh_vis(pcb_mesh_t *mesh, pcb_mesh_dir_t dir)
{
	size_t n;
	pcb_coord_t end;

	mesh_draw_label(mesh, dir, PCB_MM_TO_COORD(0.1), "object edge");

	mesh_trace("%s edges:\n", dir == PCB_MESH_HORIZONTAL ? "horizontal" : "vertical");
	for(n = 0; n < vtc0_len(&mesh->line[dir].edge); n++) {
		mesh_trace(" %mm", mesh->line[dir].edge.array[n]);
		mesh_draw_line(mesh, dir, mesh->line[dir].edge.array[n], PCB_MM_TO_COORD(0.1), PCB_MM_TO_COORD(0.5), PCB_MM_TO_COORD(0.1));
	}
	mesh_trace("\n");

	mesh_draw_label(mesh, dir, PCB_MM_TO_COORD(2), "density ranges");

	mesh_trace("%s ranges:\n", dir == PCB_MESH_HORIZONTAL ? "horizontal" : "vertical");
	for(n = 0; n < vtr0_len(&mesh->line[dir].dens); n++) {
		pcb_range_t *r = &mesh->line[dir].dens.array[n];
		mesh_trace(" [%mm..%mm=%mm]", r->begin, r->end, r->data[0].c);
		mesh_draw_range(mesh, dir, r->begin, r->end, PCB_MM_TO_COORD(2)+r->data[0].c/2, PCB_MM_TO_COORD(0.05));
	}
	mesh_trace("\n");

	mesh_trace("%s result:\n", dir == PCB_MESH_HORIZONTAL ? "horizontal" : "vertical");
	end = (dir == PCB_MESH_HORIZONTAL) ? PCB->MaxWidth : PCB->MaxHeight;
	for(n = 0; n < vtc0_len(&mesh->line[dir].result); n++) {
		mesh_trace(" %mm", mesh->line[dir].result.array[n]);
		mesh_draw_line(mesh, dir, mesh->line[dir].result.array[n], 0, end, PCB_MM_TO_COORD(0.03));
	}
	mesh_trace("\n");
	return 0;
}

static void mesh_auto_add_even(vtc0_t *v, pcb_coord_t c1, pcb_coord_t c2, pcb_coord_t d)
{
	long num = (c2 - c1) / d;

	if (num < 1)
		return;

	d = (c2 - c1)/(num+1);
	for(c1 += d; c1 < c2; c1 += d)
		vtc0_append(v, c1);
}

static pcb_coord_t mesh_auto_add_interp(vtc0_t *v, pcb_coord_t c, pcb_coord_t d1, pcb_coord_t d2, pcb_coord_t dd)
{
	if (dd > 0) {
		for(; d1 <= d2; d1 += dd) {
			c += d1;
			vtc0_append(v, c);
		}
		return c;
	}
	else {
		for(; d1 <= d2; d1 -= dd) {
			c -= d1;
			vtc0_append(v, c);
		}
		return c;
	}

}

static void mesh_auto_add_smooth(vtc0_t *v, pcb_coord_t c1, pcb_coord_t c2, pcb_coord_t d1, pcb_coord_t d, pcb_coord_t d2)
{
	pcb_coord_t len = c2 - c1, begin = c1, end = c2, glen;
	int lines;

	/* ramp up (if there's room) */
	if (d > d1) {
		lines = (d / d1) + 0;
		glen = lines * d;
		if (glen < len/4)
			begin = mesh_auto_add_interp(v, c1, d1, d, (d-d1)/lines);
	}

	/* ramp down (if there's room) */
	if (d > d2) {
		lines = (d / d2) + 0;
		glen = lines * d;
		if (glen < len/4)
			end = mesh_auto_add_interp(v, c2, d2, d, -(d-d2)/lines);
	}

	/* middle section: linear */
	mesh_auto_add_even(v, begin, end, d);
}

static int mesh_auto_build(pcb_mesh_t *mesh, pcb_mesh_dir_t dir)
{
	size_t n;
	pcb_coord_t c1, c2;
	pcb_coord_t d1, d, d2;

	mesh_trace("build:\n");

	/* left edge, before the first known line */
	if (!mesh->noimpl) {
		c1 = 0;
		c2 = mesh->line[dir].edge.array[0];
		mesh_find_range(&mesh->line[dir].dens, (c1+c2)/2, &d, &d1, &d2);
		if (mesh->smooth)
			mesh_auto_add_smooth(&mesh->line[dir].result, c1, c2, d1, d, d2);
		else
			mesh_auto_add_even(&mesh->line[dir].result, c1, c2, d);
	}

	/* normal, between known lines */
	for(n = 0; n < vtc0_len(&mesh->line[dir].edge); n++) {
		c1 = mesh->line[dir].edge.array[n];
		c2 = mesh->line[dir].edge.array[n+1];

		vtc0_append(&mesh->line[dir].result, c1);

		if (c2 - c1 < mesh->dens_obj / 2)
			continue; /* don't attempt to insert lines where it won't fit */

		mesh_find_range(&mesh->line[dir].dens, (c1+c2)/2, &d, &d1, &d2);
		if (c2 - c1 < d * 2)
			continue; /* don't attempt to insert lines where it won't fit */

		mesh_trace(" %mm..%mm %mm,%mm,%mm\n", c1, c2, d1, d, d2);

		if (mesh->noimpl)
			continue;

		/* place mesh lines between c1 and c2 */
		if (mesh->smooth)
			mesh_auto_add_smooth(&mesh->line[dir].result, c1, c2, d1, d, d2);
		else
			mesh_auto_add_even(&mesh->line[dir].result, c1, c2, d);
	}

	/* right edge, after the last known line */
	if (!mesh->noimpl) {
		c1 = mesh->line[dir].edge.array[vtc0_len(&mesh->line[dir].edge)-1];
		c2 = (dir == PCB_MESH_HORIZONTAL) ? PCB->MaxHeight : PCB->MaxWidth;
		mesh_find_range(&mesh->line[dir].dens, (c1+c2)/2, &d, &d1, &d2);
		if (mesh->smooth)
			mesh_auto_add_smooth(&mesh->line[dir].result, c1, c2, d1, d, d2);
		else
			mesh_auto_add_even(&mesh->line[dir].result, c1, c2, d);
	}

	mesh_trace("\n");
	return 0;
}

int mesh_auto(pcb_mesh_t *mesh, pcb_mesh_dir_t dir)
{
	vtc0_truncate(&mesh->line[dir].edge, 0);
	vtr0_truncate(&mesh->line[dir].dens, 0);
	vtc0_truncate(&mesh->line[dir].result, 0);

	mesh_gen_obj(mesh, dir);
	mesh_sort(mesh, dir);
	mesh_auto_build(mesh, dir);

	if (mesh->ui_layer != NULL)
		mesh_vis(mesh, dir);

	return 0;
}

int mesh_z(pcb_mesh_t *mesh)
{
	return 0;
}

static void mesh_layer_reset()
{
	if (mesh.ui_layer != NULL)
		pcb_uilayer_free(mesh.ui_layer);
	mesh.ui_layer = pcb_uilayer_alloc(mesh_ui_cookie, "mesh", "#007733");
}

static void ia_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	PCB_DAD_FREE(ia.dlg);
	memset(&ia, 0, sizeof(ia));
}

static void ia_gen_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	mesh_layer_reset();

	mesh.layer = CURRENT;

	mesh.dens_obj = ia.dlg[ia.dens_obj].default_val.coord_value;
	mesh.dens_gap = ia.dlg[ia.dens_gap].default_val.coord_value;
	mesh.min_space = ia.dlg[ia.min_space].default_val.coord_value;
	mesh.smooth = ia.dlg[ia.smooth].default_val.int_value;
	mesh.noimpl = ia.dlg[ia.noimpl].default_val.int_value;;

	if (ia.dlg[ia.hor].default_val.int_value)
		mesh_auto(&mesh, PCB_MESH_HORIZONTAL);
	if (ia.dlg[ia.ver].default_val.int_value)
		mesh_auto(&mesh, PCB_MESH_VERTICAL);

	mesh_z(&mesh);

	free(mesh.ui_name);
	mesh.ui_name = pcb_strdup_printf("mesh 0: %s", mesh.layer->name);
	mesh.ui_layer->name = mesh.ui_name;
	pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);

	pcb_gui->invalidate_all();
}

int pcb_mesh_interactive(void)
{
	int n;
	const char *bnds[] = { "PEC", "PMC", "MUR", "PML_8", NULL };
	const char *bnd_names[] = { "xmin", "xmax", "ymin", "ymax", "zmin", "zmax" };
	const char *subslines[] = { "0", "1", "3", "5", NULL };
	pcb_coord_t def = PCB_MM_TO_COORD(0.1);

	PCB_DAD_BEGIN_VBOX(ia.dlg);
		PCB_DAD_BEGIN_HBOX(ia.dlg);
			PCB_DAD_BEGIN_VBOX(ia.dlg);
				PCB_DAD_COMPFLAG(ia.dlg, PCB_HATF_FRAME);
				PCB_DAD_LABEL(ia.dlg, "XY-mesh");
				PCB_DAD_BEGIN_HBOX(ia.dlg);
					PCB_DAD_COORD(ia.dlg, "");
						ia.dens_obj = PCB_DAD_CURRENT(ia.dlg);
						PCB_DAD_MINMAX(ia.dlg, 0, PCB_MM_TO_COORD(5));
					PCB_DAD_LABEL(ia.dlg, "copper dens.");
					PCB_DAD_HELP(ia.dlg, "mesh density over copper");
				PCB_DAD_END(ia.dlg);

				PCB_DAD_BEGIN_HBOX(ia.dlg);
					PCB_DAD_COORD(ia.dlg, "");
						ia.dens_gap = PCB_DAD_CURRENT(ia.dlg);
						PCB_DAD_MINMAX(ia.dlg, 0, PCB_MM_TO_COORD(5));
					PCB_DAD_LABEL(ia.dlg, "gap dens.");
					PCB_DAD_HELP(ia.dlg, "mesh density over gaps");
				PCB_DAD_END(ia.dlg);

				PCB_DAD_BEGIN_HBOX(ia.dlg);
					PCB_DAD_COORD(ia.dlg, "");
						ia.min_space = PCB_DAD_CURRENT(ia.dlg);
						PCB_DAD_MINMAX(ia.dlg, 0, PCB_MM_TO_COORD(5));
					PCB_DAD_LABEL(ia.dlg, "min. spacing");
					PCB_DAD_HELP(ia.dlg, "minimum distance between mesh lines");
				PCB_DAD_END(ia.dlg);

				PCB_DAD_BEGIN_HBOX(ia.dlg);
					PCB_DAD_BOOL(ia.dlg, "");
						ia.smooth = PCB_DAD_CURRENT(ia.dlg);
					PCB_DAD_LABEL(ia.dlg, "smooth mesh");
					PCB_DAD_HELP(ia.dlg, "avoid jumps between different mesh densities,\nuse smooth (gradual) changes");
				PCB_DAD_END(ia.dlg);

				PCB_DAD_BEGIN_HBOX(ia.dlg);
					PCB_DAD_BOOL(ia.dlg, "");
						ia.hor = PCB_DAD_CURRENT(ia.dlg);
					PCB_DAD_LABEL(ia.dlg, "horizontal");
					PCB_DAD_HELP(ia.dlg, "enable adding horizontal mesh lines");
				PCB_DAD_END(ia.dlg);

				PCB_DAD_BEGIN_HBOX(ia.dlg);
					PCB_DAD_BOOL(ia.dlg, "");
						ia.ver = PCB_DAD_CURRENT(ia.dlg);
					PCB_DAD_LABEL(ia.dlg, "vertical");
					PCB_DAD_HELP(ia.dlg, "enable adding vertical mesh lines");
				PCB_DAD_END(ia.dlg);

				PCB_DAD_BEGIN_HBOX(ia.dlg);
					PCB_DAD_BOOL(ia.dlg, "");
						ia.noimpl = PCB_DAD_CURRENT(ia.dlg);
					PCB_DAD_LABEL(ia.dlg, "omit implicit");
					PCB_DAD_HELP(ia.dlg, "add only the mesh lines for boundaries,\nomit in-material meshing");
				PCB_DAD_END(ia.dlg);
			PCB_DAD_END(ia.dlg);

			PCB_DAD_BEGIN_VBOX(ia.dlg);
				PCB_DAD_COMPFLAG(ia.dlg, PCB_HATF_FRAME);
				PCB_DAD_LABEL(ia.dlg, "Z-mesh");

				PCB_DAD_BEGIN_HBOX(ia.dlg);
					PCB_DAD_ENUM(ia.dlg, subslines);
						ia.subslines = PCB_DAD_CURRENT(ia.dlg);
					PCB_DAD_LABEL(ia.dlg, "num in substrate");
					PCB_DAD_HELP(ia.dlg, "number of mesh lines in substrate");
				PCB_DAD_END(ia.dlg);

				PCB_DAD_BEGIN_HBOX(ia.dlg);
					PCB_DAD_BOOL(ia.dlg, "");
						ia.air_top = PCB_DAD_CURRENT(ia.dlg);
					PCB_DAD_LABEL(ia.dlg, "in air top");
					PCB_DAD_HELP(ia.dlg, "add mesh lines in air above the top of the board");
				PCB_DAD_END(ia.dlg);

				PCB_DAD_BEGIN_HBOX(ia.dlg);
					PCB_DAD_BOOL(ia.dlg, "");
						ia.air_bot = PCB_DAD_CURRENT(ia.dlg);
					PCB_DAD_LABEL(ia.dlg, "in air bottom");
					PCB_DAD_HELP(ia.dlg, "add mesh lines in air below the bottom of the board");
				PCB_DAD_END(ia.dlg);


				PCB_DAD_BEGIN_HBOX(ia.dlg);
					PCB_DAD_COORD(ia.dlg, "");
						ia.dens_air = PCB_DAD_CURRENT(ia.dlg);
						PCB_DAD_MINMAX(ia.dlg, 0, PCB_MM_TO_COORD(5));
					PCB_DAD_LABEL(ia.dlg, "dens. air");
					PCB_DAD_HELP(ia.dlg, "mesh line density (spacing) in air");
				PCB_DAD_END(ia.dlg);

				PCB_DAD_BEGIN_HBOX(ia.dlg);
					PCB_DAD_COORD(ia.dlg, "");
						ia.max_air = PCB_DAD_CURRENT(ia.dlg);
						PCB_DAD_MINMAX(ia.dlg, 0, PCB_MM_TO_COORD(5));
					PCB_DAD_LABEL(ia.dlg, "air thickness");
					PCB_DAD_HELP(ia.dlg, "how far out to mesh in air");
				PCB_DAD_END(ia.dlg);

				PCB_DAD_BEGIN_HBOX(ia.dlg);
					PCB_DAD_BOOL(ia.dlg, "");
						ia.smoothz = PCB_DAD_CURRENT(ia.dlg);
					PCB_DAD_LABEL(ia.dlg, "smooth mesh");
					PCB_DAD_HELP(ia.dlg, "avoid jumps between different mesh densities,\nuse smooth (gradual) changes");
				PCB_DAD_END(ia.dlg);

			PCB_DAD_END(ia.dlg);
		PCB_DAD_END(ia.dlg);

		PCB_DAD_BEGIN_HBOX(ia.dlg);
			PCB_DAD_BEGIN_VBOX(ia.dlg);
				PCB_DAD_COMPFLAG(ia.dlg, PCB_HATF_FRAME);
				PCB_DAD_LABEL(ia.dlg, "Boundary");
				for(n = 0; n < 6; n+=2) {
					char name[64];
					sprintf(name, "%s %s", bnd_names[n], bnd_names[n+1]);
					PCB_DAD_BEGIN_HBOX(ia.dlg);
						PCB_DAD_ENUM(ia.dlg, bnds);
							ia.bnd[n] = PCB_DAD_CURRENT(ia.dlg);
						PCB_DAD_LABEL(ia.dlg, name);
						PCB_DAD_ENUM(ia.dlg, bnds);
							ia.bnd[n+1] = PCB_DAD_CURRENT(ia.dlg);
					PCB_DAD_END(ia.dlg);
				}
			PCB_DAD_END(ia.dlg);

			PCB_DAD_BEGIN_VBOX(ia.dlg);
				PCB_DAD_BUTTON(ia.dlg, "Generate mesh!");
					PCB_DAD_CHANGE_CB(ia.dlg, ia_gen_cb);
			PCB_DAD_END(ia.dlg);
		PCB_DAD_END(ia.dlg);

	PCB_DAD_END(ia.dlg);

	PCB_DAD_NEW(ia.dlg, "mesher", "Generate mesh", &ia, 0, ia_close_cb);

	PCB_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.dens_obj, coord_value, PCB_MM_TO_COORD(0.15));
	PCB_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.dens_gap, coord_value, PCB_MM_TO_COORD(0.5));
	PCB_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.min_space, coord_value, PCB_MM_TO_COORD(0.1));
	PCB_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.smooth, int_value, 1);
	PCB_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.hor, int_value, 1);
	PCB_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.ver, int_value, 1);
	PCB_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.noimpl, int_value, 0);
	PCB_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.subslines, int_value, 3);
	PCB_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.air_top, int_value, 1);
	PCB_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.air_bot, int_value, 1);
	PCB_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.dens_air, coord_value, PCB_MM_TO_COORD(0.1));
	PCB_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.smoothz, int_value, 1);
	PCB_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.max_air, coord_value, PCB_MM_TO_COORD(1));
	return 0;
}

const char pcb_acts_mesh[] = "mesh()";
const char pcb_acth_mesh[] = "generate a mesh for simulation";
int pcb_act_mesh(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_mesh_interactive();
	return 0;
}
