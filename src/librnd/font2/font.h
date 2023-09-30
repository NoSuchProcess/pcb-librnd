/*
 *                            COPYRIGHT
 *
 *  librnd, modular 2D CAD framework - vector font rendering v2
 *  librnd Copyright (C) 2022, 2023 Tibor 'Igor2' Palinkas
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
 *    Project page: http://repo.hu/projects/librnd
 *    lead developer: http://repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#ifndef RND_FONT_H
#define RND_FONT_H

#include <genht/htsi.h>
#include <librnd/core/global_typedefs.h>
#include <librnd/core/box.h>
#include <librnd/font2/glyph.h>
#include <librnd/font2/htkc.h>

/* Note: char 0 can not be used (string terminator). If RND_FONT_ENTITY is
   enabled, char 255 is used internally to indicate unknown entities */
#define RND_FONT_MAX_GLYPHS 255
#define RND_FONT_DEFAULT_CELLSIZE 50

/* [4.1.0] maximum number of points in a simple polygon (used in glyphs) */
#define RND_FONT2_MAX_SIMPLE_POLY_POINTS 256

/* [4.1.0] maximum length of an entity name (the sequence between & and ;) */
#define PCB_FONT2_ENTITY_MAX_LENGTH 32


typedef long int rnd_font_id_t;      /* safe reference */

typedef struct rnd_font_s {          /* complete set of symbols */
	rnd_coord_t max_height, max_width; /* maximum glyph width and height; calculated per glyph; saved as cell_height and cell_width */
	rnd_coord_t height, cent_height;   /* total height, font-wise (distance between lowest point of the lowest glyph and highest point of the tallest glyph); cent_ variant is centerline height in stroke font */
	rnd_box_t unknown_glyph;           /* drawn when a glyph is not found (filled box) */
	rnd_glyph_t glyph[RND_FONT_MAX_GLYPHS+1];
	char *name;                        /* not unique */
	rnd_font_id_t id;                  /* unique for safe reference */
	rnd_coord_t tab_width;             /* [4.1.0, filever 2] \t positions when rendering with RND_FONT_HTAB; calculated from 'M' when unspecified */
	rnd_coord_t line_height;           /* [4.1.0, filever 2] y stepping for multiline text; if not available (0 in pre-v2 font): 1.1*max_height */
	rnd_coord_t baseline;              /* [4.1.0, filever 2] y offset from the top to the baseline - see comment at RND_FONT_BASELINE */

	htsi_t entity_tbl;                 /* [4.1.0, filever 2] key: entity name without the "&" and ";" wrapping; value: [1..245] glyph index */
	htkc_t kerning_tbl;                /* [4.1.0, filever 2] key: character pair; value: signed coord added to advance of the first char */

	/* cached fields - these are not directly in the file */
	unsigned entity_tbl_valid:1;       /* [4.1.0] */
	unsigned kerning_tbl_valid:1;      /* [4.1.0] */
	char filever;                      /* [4.1.0] 0 for unknown/legacy, 1 for lht v1 or 2 for lht v2 */
	rnd_coord_t tab_width_cache;       /* [4.1.0] either ->tab_width or if that's missing, computed by a heuristic only once per font */
	rnd_coord_t line_height_cache;     /* [4.1.0] effective y stepping for multiline text; either ->line_height or ->max_height*1.1 */

	/* Spare: see doc/developer/spare.txt */
	void (*spare_f1)(void), (*spare_f2)(void);
	long spare_l1, spare_l2, spare_l3, spare_l4;
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
	double spare_d1, spare_d2, spare_d3, spare_d4;
	rnd_coord_t spare_c1, spare_c2, spare_c3, spare_c4;
} rnd_font_t;


typedef enum {                 /* bitfield - order matters for backward compatibility */
	RND_FONT_MIRROR_NO = 0,      /* do not mirror (but 0 and 1 are cleared) */
	RND_FONT_MIRROR_Y = 1,       /* change Y coords (mirror over the X axis) */
	RND_FONT_MIRROR_X = 2,       /* change X coords (mirror over the Y axis) */
	RND_FONT_HTAB = 4,           /* [4.1.0] render horizontal tab */
	RND_FONT_ENTITY = 8,         /* [4.1.0] interpret &entity; sequences and render them single glyph */
	RND_FONT_MULTI_LINE = 16,    /* [4.1.0] support rendering into multiple lines (split at \n) */
	RND_FONT_STOP_AT_NL = 32,    /* [4.1.0] stop rendering at the first newline */
	RND_FONT_THIN_POLY = 64,     /* [4.1.0] render polygons with their outline only */
	RND_FONT_BASELINE = 128      /* [4.1.0] normally the origin of a text object is the top-left of the glyph coord system; when this is enabled and the font has a baseline, the glyphs are moved up so the origin is at the custom baseline of the font (bbox is modified too) */
} rnd_font_render_opts_t;

typedef enum rnd_font_tiny_e { /* How to draw text that is too tiny to be readable */
	RND_FONT_TINY_HIDE,          /* do not draw it at all */
	RND_FONT_TINY_CHEAP,         /* draw a cheaper, simplified approximation that shows there's something there */
	RND_FONT_TINY_ACCURATE       /* always draw text accurately, even if it will end up unreadable */
} rnd_font_tiny_t;

rnd_glyph_line_t *rnd_font_new_line_in_glyph(rnd_glyph_t *glyph, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t thickness);
rnd_glyph_arc_t *rnd_font_new_arc_in_glyph(rnd_glyph_t *glyph, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r, rnd_angle_t start, rnd_angle_t delta, rnd_coord_t thickness);
rnd_glyph_poly_t *rnd_font_new_poly_in_glyph(rnd_glyph_t *glyph, long num_points);

/* Free all font content in f; doesn't free f itself */
void rnd_font_free(rnd_font_t *f);

/* Remove all content (atoms and geometry, except for ->height) of
   the glyph and mark it invalid; doesn't free g itself */
void rnd_font_free_glyph(rnd_glyph_t *g);


/* Very rough (but quick) estimation on the full size of the text */
rnd_coord_t rnd_font_string_width(rnd_font_t *font, rnd_font_render_opts_t opts, double scx, const unsigned char *string);
rnd_coord_t rnd_font_string_height(rnd_font_t *font, rnd_font_render_opts_t opts, double scy, const unsigned char *string);


typedef void (*rnd_font_draw_atom_cb)(void *cb_ctx, const rnd_glyph_atom_t *a);

/* Render a text object atom by atom using cb/cb_ctx */
void rnd_font_draw_string(rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_render_opts_t opts, rnd_coord_t thickness, rnd_coord_t min_line_width, rnd_font_tiny_t tiny, rnd_font_draw_atom_cb cb, void *cb_ctx);

/* Same as draw_string but insert extra_glyph empty space after each glyph
   except for whitepsace, where extra_spc is inserted */
void rnd_font_draw_string_justify(rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_render_opts_t opts, rnd_coord_t thickness, rnd_coord_t min_line_width, rnd_font_tiny_t tiny, rnd_coord_t extra_glyph, rnd_coord_t extra_spc, rnd_font_draw_atom_cb cb, void *cb_ctx);

/* Calculate all 4 corners of the transformed (e.g. rotated) box in cx;cy */
void rnd_font_string_bbox(rnd_coord_t cx[4], rnd_coord_t cy[4], rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_render_opts_t opts, rnd_coord_t thickness, rnd_coord_t min_line_width);
void rnd_font_string_bbox_pcb_rnd(rnd_coord_t cx[4], rnd_coord_t cy[4], rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_render_opts_t opts, rnd_coord_t thickness, rnd_coord_t min_line_width, int scale);

/* transforms symbol coordinates so that the left edge of each symbol
   is at the zero position. The y coordinates are moved so that min(y) = 0 */
void rnd_font_normalize(rnd_font_t *f);
void rnd_font_normalize_pcb_rnd(rnd_font_t *f);

/* v1 font is missing height and unknown glyph, this fixes that up */
void rnd_font_fix_v1(rnd_font_t *f);


/* Count the number of characters in s that wouldn't render with the given font */
int rnd_font_invalid_chars(rnd_font_t *font, rnd_font_render_opts_t opts, const unsigned char *s);

void rnd_font_copy(rnd_font_t *dst, const rnd_font_t *src);
void rnd_font_copy_tables(rnd_font_t *dst, const rnd_font_t *src); /* [4.1.0] */


/*** [4.1.0] render in a box with alignment ***/
typedef struct vtc0_s rnd_font_wcache_t;

typedef enum {
	RND_FONT_ALIGN_START,     /* left or top */
	RND_FONT_ALIGN_CENTER,
	RND_FONT_ALIGN_END,       /* right or bottom */
	RND_FONT_ALIGN_WORD_JUST, /* extra space inserted between words or lines */
	RND_FONT_ALIGN_JUST,      /* extra space inserted between characters; only for horizontal */
	RND_FONT_ALIGN_invalid
} rnd_font_align_t;

/* Same as rnd_font_draw_string but aligned in a box. Unrotated box width and
   height are passed as boxw and boxh. Alignment of text lines within the box
   is specified using halign and valign. Per line widths are cached for a
   specific text string if wcache is not NULL - wcache needs to be invalidated
   when text string changes, use rnd_font_uninit_wcache */
void rnd_font_draw_string_align(rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_render_opts_t opts, rnd_coord_t thickness, rnd_coord_t min_line_width, rnd_font_tiny_t tiny, rnd_coord_t boxw, rnd_coord_t boxh, rnd_font_align_t halign, rnd_font_align_t valign, rnd_font_wcache_t *wcache, rnd_font_draw_atom_cb cb, void *cb_ctx);

/* Free all memory of wcache */
void rnd_font_uninit_wcache(rnd_font_wcache_t *wcache);

/* Keep memory allocation but invalidate the cache so it is recalculated upon
   next call. */
void rnd_font_inval_wcache(rnd_font_wcache_t *wcache);

/*** embedded (internal) font ***/

typedef struct embf_line_s {
	int x1, y1, x2, y2, th;
} embf_line_t;

typedef struct embf_font_s {
	int delta;
	embf_line_t *lines;
	int num_lines;
} embf_font_t;

void rnd_font_load_internal(rnd_font_t *font, embf_font_t *embf_font, int len, rnd_coord_t embf_minx, rnd_coord_t embf_miny, rnd_coord_t embf_maxx, rnd_coord_t embf_maxy);

#endif

