/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include <librnd/rnd_config.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <genvector/gds_char.h>

#include <librnd/core/unit.h>
#include <librnd/core/hidlib.h>
#include <librnd/hid/hid.h>
#include <librnd/hid/grid.h>
#include <librnd/core/conf.h>
#include <librnd/core/rnd_conf.h>
#include <librnd/core/conf_hid.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/rnd_bool.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/conf_hid.h>

static const char grid_cookie[] = "librnd grid";
static int grid_idx_lock;

rnd_coord_t rnd_grid_fit(rnd_coord_t x, rnd_coord_t grid_spacing, rnd_coord_t grid_offset)
{
	x -= grid_offset;
	x = grid_spacing * rnd_round((double) x / grid_spacing);
	x += grid_offset;
	return x;
}

rnd_bool_t rnd_grid_parse(rnd_grid_t *dst, const char *src)
{
	const char *nsep;
	char *sep3, *sep2, *sep, *tmp, *size, *ox = NULL, *oy = NULL, *unit = NULL;
	rnd_bool succ;

	nsep = strchr(src, ':');
	if (nsep != NULL)
		src = nsep+1;
	else
		dst->name = NULL;

	/* remember where size starts */
	while(isspace(*src)) src++;
	sep = size = tmp = rnd_strdup(src);

	/* find optional offs */
	sep2 = strchr(sep, '@');
	if (sep2 != NULL) {
		sep = sep2;
		*sep = '\0';
		sep++;
		ox = sep;
		sep3 = strchr(sep, ',');
		if (sep3 != NULL) {
			*sep3 = '\0';
			sep3++;
			oy = sep;
		}
	}

	/* find optional unit switch */
	sep2 = strchr(sep, '!');
	if (sep2 != NULL) {
		sep = sep2;
		*sep = '\0';
		sep++;
		unit = sep;
	}

	/* convert */
	dst->size = rnd_get_value(size, NULL, NULL, &succ);
	if ((!succ) || (dst->size < 0))
		goto error;

	if (ox != NULL) {
		dst->ox = rnd_get_value(ox, NULL, NULL, &succ);
		if (!succ)
			goto error;
	}
	else
		dst->ox = 0;

	if (oy != NULL) {
		dst->oy = rnd_get_value(oy, NULL, NULL, &succ);
		if (!succ)
			goto error;
	}
	else
		dst->oy = 0;

	if (unit != NULL) {
		dst->unit = rnd_get_unit_struct(unit);
		if (dst->unit == NULL)
			goto error;
	}
	else
		dst->unit = NULL;

	/* success */
	free(tmp);

	if (nsep != NULL)
		dst->name = rnd_strndup(src, nsep-src-1);
	else
		dst->name = NULL;
	return rnd_true;

	error:;
	free(tmp);
	return rnd_false;
}

rnd_bool_t rnd_grid_append_print(gds_t *dst, const rnd_grid_t *src)
{
	if (src->size <= 0)
		return rnd_false;
	if (src->name != NULL) {
		gds_append_str(dst, src->name);
		gds_append(dst, ':');
	}
	rnd_append_printf(dst, "%$.08mH", src->size);
	if ((src->ox != 0) || (src->oy != 0))
		rnd_append_printf(dst, "@%$.08mH,%$.08mH", src->ox, src->oy);
	if (src->unit != NULL) {
		gds_append(dst, '!');
		gds_append_str(dst, src->unit->suffix);
	}
	return rnd_true;
}

char *rnd_grid_print(const rnd_grid_t *src)
{
	gds_t tmp;
	gds_init(&tmp);
	if (!rnd_grid_append_print(&tmp, src)) {
		gds_uninit(&tmp);
		return NULL;
	}
	return tmp.array; /* do not uninit tmp */
}

void rnd_grid_set(rnd_design_t *hidlib, const rnd_grid_t *src)
{
	rnd_hid_set_grid(hidlib, src->size, rnd_true, src->ox, src->oy);
	if (src->unit != NULL)
		rnd_hid_set_unit(hidlib, src->unit);
}

void rnd_grid_free(rnd_grid_t *dst)
{
	free(dst->name);
	dst->name = NULL;
}

rnd_bool_t rnd_grid_list_jump(rnd_design_t *hidlib, int dst)
{
	const rnd_conf_listitem_t *li;
	rnd_grid_t g;
	int max = rnd_conflist_length((rnd_conflist_t *)&rnd_conf.editor.grids);

	if (dst < 0)
		dst = 0;
	if (dst >= max)
		dst = max - 1;
	if (dst < 0)
		return rnd_false;

	grid_idx_lock++;

	rnd_conf_setf(RND_CFR_DESIGN, "editor/grids_idx", -1, "%d", dst);

	li = rnd_conflist_nth((rnd_conflist_t *)&rnd_conf.editor.grids, dst);
	/* clamp */
	if (li == NULL) {
		grid_idx_lock--;
		return rnd_false;
	}

	if (!rnd_grid_parse(&g, li->payload)) {
		grid_idx_lock--;
		return rnd_false;
	}
	rnd_grid_set(hidlib, &g);
	rnd_grid_free(&g);

	grid_idx_lock--;
	return rnd_true;
}

rnd_bool_t rnd_grid_list_step(rnd_design_t *hidlib, int stp)
{
	int dst = rnd_conf.editor.grids_idx;
	if (dst < 0)
		dst = -dst-1;
	return rnd_grid_list_jump(hidlib, dst + stp);
}

void rnd_grid_inval(void)
{
	if (rnd_conf.editor.grids_idx > 0)
		rnd_conf_setf(RND_CFR_DESIGN, "editor/grids_idx", -1, "%d", -1 - rnd_conf.editor.grids_idx);
}

/*** catch editor/grid changes to update editor/grids_idx */

static void grid_conf_chg(rnd_conf_native_t *cfg, int arr_idx, void *user_data)
{
	gdl_iterator_t it;
	rnd_conf_listitem_t *ge;
	int idx = 0, found = -1;

	if (grid_idx_lock)
		return;

	grid_idx_lock++;
	rnd_conflist_foreach((rnd_conflist_t *)&rnd_conf.editor.grids, &it, ge) {
		rnd_grid_t g;
/*		g.ox = hidlib->grid_ox; g.oy = hidlib->grid_oy; */
		if (rnd_grid_parse(&g, ge->payload)) {
			if ((g.size == rnd_conf.editor.grid) /*&& (g.ox == hidlib->grid_ox) && (g.oy == hidlib->grid_oy)*/) {
				found = idx;
				rnd_grid_free(&g);
				break;
			}
			rnd_grid_free(&g);
		}
		idx++;
	}

	if (rnd_conf.editor.grids_idx != found)
		rnd_conf_setf(RND_CFR_DESIGN, "editor/grids_idx", -1, "%d", found);
	grid_idx_lock--;
}

void rnd_grid_init(void)
{
	rnd_conf_native_t *n = rnd_conf_get_field("editor/grid");
	static rnd_conf_hid_id_t grid_conf_id;

	if (n != NULL) {
		static rnd_conf_hid_callbacks_t cbs;
		grid_conf_id = rnd_conf_hid_reg(grid_cookie, NULL);
		memset(&cbs, 0, sizeof(rnd_conf_hid_callbacks_t));
		cbs.val_change_post = grid_conf_chg;
		rnd_conf_hid_set_cb(n, grid_conf_id, &cbs);
	}

}

void rnd_grid_uninit(void)
{
	rnd_conf_hid_unreg(grid_cookie);
}

/* sets cursor grid with respect to grid offset values */
void rnd_hid_set_grid(rnd_design_t *hidlib, rnd_coord_t Grid, rnd_bool align, rnd_coord_t ox, rnd_coord_t oy)
{
	if (Grid >= 1 && Grid <= RND_MAX_GRID) {
		if (align) {
			hidlib->grid_ox = ox % Grid;
			hidlib->grid_oy = oy % Grid;
		}
		hidlib->grid = Grid;
		rnd_conf_set_design("editor/grid", "%$mS", Grid);
		if (rnd_conf.editor.draw_grid)
			rnd_gui->invalidate_all(rnd_gui);
	}
}

void rnd_hid_set_unit(rnd_design_t *hidlib, const rnd_unit_t *new_unit)
{
	if (new_unit != NULL && new_unit->allow != RND_UNIT_NO_PRINT)
		rnd_conf_set(RND_CFR_DESIGN, "editor/grid_unit", -1, new_unit->suffix, RND_POL_OVERWRITE);
}
