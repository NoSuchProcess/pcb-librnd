 /*
  *                            COPYRIGHT
  *
  *  pcb-rnd, interactive printed circuit board design
  *  (this file is based on PCB, interactive printed circuit board design)
  *  Copyright (C) 2006 Dan McMahill
  *  Copyright (C) 2019,2022 Tibor 'Igor2' Palinkas
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

#include <librnd/core/color_cache.h>

typedef struct rnd_drwpx_color_struct_s {
	int c;                   /* the descriptor used by the gd library */
	unsigned int r, g, b, a; /* so I can figure out what rgb value c refers to */
} rnd_drwpx_color_struct_t;

#ifndef FROM_DRAW_PIXMAP_C
typedef struct gdImage_s gdImage;
#endif

typedef struct rnd_drwpx_s {
	/* public: config */
	rnd_design_t *hidlib;
	double scale; /* should be 1 by default */
	double bloat;
	rnd_coord_t x_shift, y_shift;
	int ymirror, in_mono;

	/* public: result */
	long png_drawn_objs;

	/* private */
	rnd_clrcache_t color_cache;
	int color_cache_inited;
	htpp_t brush_cache;
	int brush_cache_inited;
	int w, h; /* in pixels */
	int dpi, xmax, ymax;
	rnd_drwpx_color_struct_t *black, *white;
	gdImage *im, *master_im, *comp_im, *erase_im;
	int last_color_r, last_color_g, last_color_b, last_cap;
	gdImage *lastbrush;
	int linewidth, unerase_override;

	/* private: leftovers from pcb-rnd photo mode */
	int photo_mode, is_photo_drill, is_photo_mech, doing_outline, have_outline;

	/* Spare: see doc/developer/spare.txt */
	void (*spare_f1)(void), (*spare_f2)(void);
	long enable_pixmap; /* bool: enable exporting pixmap objects */
	long spare_l2, spare_l3, spare_l4;
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
	double spare_d1, spare_d2, spare_d3, spare_d4;
} rnd_drwpx_t;

void rnd_drwpx_init(rnd_drwpx_t *pctx, rnd_design_t *hidlib);
void rnd_drwpx_uninit(rnd_drwpx_t *pctx);

void rnd_drwpx_parse_bloat(rnd_drwpx_t *pctx, const char *str);
int rnd_drwpx_set_size(rnd_drwpx_t *pctx, rnd_box_t *bbox, int dpi_in, int xmax_in, int ymax_in, int xymax_in);
int rnd_drwpx_create(rnd_drwpx_t *pctx, int use_alpha);

void rnd_drwpx_start(rnd_drwpx_t *pctx);
void rnd_drwpx_finish(rnd_drwpx_t *pctx, FILE *f, int filetype_idx);

/* Available file types (compile time configuration) */
const char *rnd_drwpx_get_file_suffix(int filetype_idx);
extern const char *rnd_drwpx_filetypes[];
int rnd_drwpx_has_any_format(void);

/* HID API standard drawing calls */
rnd_hid_gc_t rnd_drwpx_make_gc(rnd_hid_t *hid);
void rnd_drwpx_destroy_gc(rnd_hid_gc_t gc);
void rnd_drwpx_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style);
void rnd_drwpx_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width);

/* HID API standard drawing calls, plus pctx */
void rnd_drwpx_set_drawing_mode(rnd_drwpx_t *pctx, rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen);
void rnd_drwpx_set_color(rnd_drwpx_t *pctx, rnd_hid_gc_t gc, const rnd_color_t *color);
void rnd_drwpx_set_draw_xor(rnd_hid_gc_t gc, int xor_);
void rnd_drwpx_fill_rect(rnd_drwpx_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
void rnd_drwpx_draw_line(rnd_drwpx_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
void rnd_drwpx_draw_arc(rnd_drwpx_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle);
void rnd_drwpx_fill_circle(rnd_drwpx_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius);
void rnd_drwpx_fill_polygon_offs(rnd_drwpx_t *pctx, rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy);
void rnd_drwpx_draw_pixmap(rnd_drwpx_t *pctx, rnd_hid_t *hid, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t sx, rnd_coord_t sy, rnd_pixmap_t *pixmap); /* [4.0.0] */

/*** for pcb-rnd only (for historical reasons...) ***/
/* The result of a failed gdImageColorAllocate() call */
#define RND_PNG_BADC -1
