/*
 *                            COPYRIGHT
 *
 *  librnd, modular 2D CAD framework - vector font rendering (v2)
 *  librnd Copyright (C) 2022,2023 Tibor 'Igor2' Palinkas
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

#include "config.h"
#include <ctype.h>
#include <genht/hash.h>
#include <librnd/core/compat_misc.h>
#include <librnd/hid/hid.h>
#include <librnd/core/globalconst.h>
#include <librnd/core/error.h>
#include <librnd/core/xform_mx.h>
#include <librnd/core/vtc0.h>
#include <librnd/font2/font.h>

/*#define FONT2_DEBUG 1*/

/* Evaluates to 1 if opts has any mirroring */
#define OPTS_IS_MIRRORED(opts) \
	((opts & RND_FONT_MIRROR_Y) || (opts & RND_FONT_MIRROR_X))

#define STOP_AT_NL ((c.chr == '\n') && (opts & RND_FONT_STOP_AT_NL))

#ifdef FONT2_DEBUG
/* force tab support for debug */
#define is_tab(opts, chr) \
	((chr) == '\t')
#else
#define is_tab(opts, chr) \
	(((chr) == '\t') && ((opts) & RND_FONT_HTAB))
#endif

#define is_valid(font, opts, chr) \
		(is_tab(opts, chr) || (((chr) <= RND_FONT_MAX_GLYPHS) && (font)->glyph[(chr)].valid))

static rnd_font_wcache_t rnd_font_tmp_wc;

RND_INLINE rnd_coord_t rnd_font_advance_tab(rnd_font_t *font, rnd_font_render_opts_t opts, rnd_coord_t x)
{
	rnd_coord_t tabsize;
	rnd_glyph_t *gt = &font->glyph['\t'];

	if ((font->tab_width_cache <= 0) && (font->tab_width <= 0))
		rnd_message(RND_MSG_WARNING, "librnd font: missing tab_width in font; improvising a value\n");

	if (font->tab_width_cache <= 0)
		font->tab_width_cache = font->tab_width;

	if (font->tab_width_cache <= 0) {
		/* heuristics if there was no explicit tab_width */
		if (font->glyph['M'].valid)
			font->tab_width_cache = font->glyph['M'].width * 4;
		else if (font->glyph[' '].valid)
			font->tab_width_cache = font->glyph[' '].width * 4;
		else if (font->height > 0)
			font->tab_width_cache = font->height * 4;
		else
			font->tab_width_cache = RND_MM_TO_COORD(3);
	}

	if (gt->valid) {
		if (gt->advance_valid)
			x += gt->advance;
		else
			x += gt->width + gt->xdelta;
	}

	tabsize = font->tab_width_cache;
	return ((x + tabsize)/tabsize)*tabsize;
}

/* Add glyph advance to current and return the new position, for valid glyphs */
RND_INLINE rnd_coord_t rnd_font_advance_valid(rnd_font_t *font, rnd_font_render_opts_t opts, rnd_coord_t current, int chr, int chr2, rnd_glyph_t *g)
{
	if (is_tab(opts, chr))
		return rnd_font_advance_tab(font, opts, current);

	if (font->kerning_tbl_valid) {
		htkc_key_t key;
		rnd_coord_t offs;
		
		key.left = chr;
		key.right = chr2;
		offs = htkc_get(&font->kerning_tbl, key);
		if (offs != 0) current += offs; /* apply kerning in advance to the normal mechanism */
	}

	if (g->advance_valid)
		return current + g->advance;
	return current + g->width + g->xdelta;
}

/* Add glyph advance to current and return the new position, for the unknown glyph */
RND_INLINE rnd_coord_t rnd_font_advance_invalid(rnd_font_t *font, rnd_font_render_opts_t opts, rnd_coord_t current)
{
	const rnd_box_t *unk_gl = &font->unknown_glyph;
	return current + ((unk_gl->X2 - unk_gl->X1) * 6 / 5);
}

/* Add glyph advance to current and return the new position, for chr */
RND_INLINE rnd_coord_t rnd_font_advance(rnd_font_t *font, rnd_font_render_opts_t opts, int chr, int chr2, rnd_coord_t x)
{
	if (is_tab(opts, chr))
		return rnd_font_advance_tab(font, opts, x);

	if ((chr > 0) && (chr <= RND_FONT_MAX_GLYPHS)) {
		rnd_glyph_t *g = &font->glyph[chr];
		if (g->valid)
			return rnd_font_advance_valid(font, opts, x, chr, chr2, g);
	}
	return rnd_font_advance_invalid(font, opts, x);
}

#define RND_FONT2_INVALID_ENTITY_CHAR 255

/* Iterate over a string - handling &entity; sequences */
typedef struct {
	const rnd_font_t *font;
	int chr, chr_next;
	const unsigned char *pos_next;
	rnd_font_render_opts_t opts;
} cursor_t;

/* Return the next character, which is either a single byte of input or is
   an &entity; that is looked up in the font's entity table. Fills in len
   with the input length (1 or full entity length). */
RND_INLINE int cursor_next_ent(cursor_t *c, int *len)
{
	const unsigned char *s = c->pos_next, *end;


	/* if entity is enabled and we have a & it may be the start of an entity... */
	if ((c->opts & RND_FONT_ENTITY) && (*s == '&')) {
		for(end = s+1; isalnum(*end); end++) ;
		if (*end == ';') {
			char name[PCB_FONT2_ENTITY_MAX_LENGTH+1];
			int res, elen = end - (s+1);

			if (elen > PCB_FONT2_ENTITY_MAX_LENGTH) {
				*len = elen+2;
				rnd_message(RND_MSG_ERROR, "Text error: entity name too long around '%s'\n", s);
				return RND_FONT2_INVALID_ENTITY_CHAR;
			}
			memcpy(name, s+1, elen);
			name[elen] = '\0';
			*len = elen+2;

			if ((elen == 3) && (strcmp(name, "amp") == 0))
				return '&';

			if (!c->font->entity_tbl_valid)
				return RND_FONT2_INVALID_ENTITY_CHAR; /* no table to work from */

			res = htsi_get((htsi_t *)&c->font->entity_tbl, name);
			if (res > 0)
				return res;
			return RND_FONT2_INVALID_ENTITY_CHAR;
		}
		/* else: not an entity; fall through to go char by char */
	}

	/* plain char */
	*len = 1;
	return *s;
}

RND_INLINE void cursor_next(cursor_t *c)
{
	int len;

	c->chr = cursor_next_ent(c, &len);
	if (c->chr != 0) {
		c->pos_next += len;
		c->chr_next = cursor_next_ent(c, &len);
	}
	else
		c->chr_next = 0;
}

RND_INLINE void cursor_first(const rnd_font_t *font, rnd_font_render_opts_t opts, cursor_t *c, const unsigned char *string)
{
	c->font = font;
	c->opts = opts;
	c->pos_next = string;
#ifdef FONT2_DEBUG
	c->opts |= RND_FONT_ENTITY;
#endif
	cursor_next(c);
}


rnd_coord_t rnd_font_string_width(rnd_font_t *font, rnd_font_render_opts_t opts, double scx, const unsigned char *string)
{
	rnd_coord_t w = 0;
	cursor_t c;

	if (string == NULL)
		return 0;

	for(cursor_first(font, opts, &c, string); (c.chr != 0) && !STOP_AT_NL; cursor_next(&c))
		w = rnd_font_advance(font, opts, c.chr, c.chr_next, w);
	return rnd_round((double)w * scx);
}

rnd_coord_t rnd_font_string_height(rnd_font_t *font, rnd_font_render_opts_t opts, double scy, const unsigned char *string)
{
	rnd_coord_t h;
	cursor_t c;

	if (string == NULL)
		return 0;

	h = font->max_height;

	for(cursor_first(font, opts, &c, string); c.chr != 0 && !STOP_AT_NL; cursor_next(&c))
		if (c.chr == '\n')
			h += font->max_height;

	return rnd_round((double)h * scy);
}

RND_INLINE void cheap_text_line(rnd_xform_mx_t mx, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_font_draw_atom_cb cb, void *cb_ctx)
{
	rnd_glyph_atom_t a;

	a.type = RND_GLYPH_LINE;
	a.line.x1 = rnd_round(rnd_xform_x(mx, x1, y1));
	a.line.y1 = rnd_round(rnd_xform_y(mx, x1, y1));
	a.line.x2 = rnd_round(rnd_xform_x(mx, x2, y2));
	a.line.y2 = rnd_round(rnd_xform_y(mx, x2, y2));
	a.line.thickness = -1;

	cb(cb_ctx, &a);
}

/* Decreased level-of-detail: draw only a few lines for the whole text */
RND_INLINE int draw_text_cheap(rnd_font_t *font, rnd_font_render_opts_t opts, rnd_xform_mx_t mx, rnd_coord_t dx, rnd_coord_t dy, const unsigned char *string, double scx, double scy, rnd_font_tiny_t tiny, rnd_font_draw_atom_cb cb, void *cb_ctx)
{
	rnd_coord_t w, h = rnd_font_string_height(font, opts, scy, string);

	if (tiny == RND_FONT_TINY_HIDE) {
		if (h <= rnd_render->coord_per_pix*6) /* <= 6 pixel high: unreadable */
			return 1;
	}
	else if (tiny == RND_FONT_TINY_CHEAP) {
		if (h <= rnd_render->coord_per_pix*2) { /* <= 1 pixel high: draw a single line in the middle */
			w = rnd_font_string_width(font, opts, 1.0, string); /* scx=1 because mx xformation will scale it up anyway */
			h = rnd_font_string_height(font, opts, 1.0, string);
			cheap_text_line(mx, 0+dx, h/2+dy, w+dx, h/2+dy, cb, cb_ctx);
			return 1;
		}
		else if (h <= rnd_render->coord_per_pix*4) { /* <= 4 pixel high: draw a mirrored Z-shape */
			w = rnd_font_string_width(font, opts, 1.0, string); /* scx=1 because mx xformation will scale it up anyway */
			h = rnd_font_string_height(font, opts, 1.0, string);
			h /= 4;
			cheap_text_line(mx, dx, h+dy,   w+dx, h+dy,   cb, cb_ctx);
			cheap_text_line(mx, dx, h+dy,   w+dx, h*3+dy, cb, cb_ctx);
			cheap_text_line(mx, dx, h*3+dy, w+dx, h*3+dy, cb, cb_ctx);
			return 1;
		}
	}

	return 0;
}

RND_INLINE void draw_atom(const rnd_glyph_atom_t *a, rnd_xform_mx_t mx, rnd_coord_t dx, rnd_coord_t dy, double scx, double scy, double rotdeg, rnd_font_render_opts_t opts, rnd_coord_t thickness, rnd_coord_t min_line_width, rnd_font_draw_atom_cb cb, void *cb_ctx)
{
	long nx, ny, h;
	int too_large;
	rnd_glyph_atom_t res;
	rnd_coord_t tmp[2*RND_FONT2_MAX_SIMPLE_POLY_POINTS];

	res.type = a->type;
	switch(a->type) {
		case RND_GLYPH_LINE:
			res.line.x1 = rnd_round(rnd_xform_x(mx, a->line.x1+dx, a->line.y1+dy));
			res.line.y1 = rnd_round(rnd_xform_y(mx, a->line.x1+dx, a->line.y1+dy));
			res.line.x2 = rnd_round(rnd_xform_x(mx, a->line.x2+dx, a->line.y2+dy));
			res.line.y2 = rnd_round(rnd_xform_y(mx, a->line.x2+dx, a->line.y2+dy));
			res.line.thickness = rnd_round(a->line.thickness * (scx+scy) / 4.0);

			if (res.line.thickness < min_line_width)
				res.line.thickness = min_line_width;
			if (thickness > 0)
				res.line.thickness = thickness;

			break;

		case RND_GLYPH_ARC:
			res.arc.cx = rnd_round(rnd_xform_x(mx, a->arc.cx + dx, a->arc.cy + dy));
			res.arc.cy = rnd_round(rnd_xform_y(mx, a->arc.cx + dx, a->arc.cy + dy));
			res.arc.r  = rnd_round(a->arc.r * scx);
			res.arc.start = a->arc.start + rotdeg;
			res.arc.delta = a->arc.delta;
			res.arc.thickness = rnd_round(a->arc.thickness * (scx+scy) / 4.0);
			if (OPTS_IS_MIRRORED(opts)) {
				res.arc.start = RND_SWAP_ANGLE(res.arc.start);
				res.arc.delta = RND_SWAP_DELTA(res.arc.delta);
			}
			if (res.arc.thickness < min_line_width)
				res.arc.thickness = min_line_width;
			if (thickness > 0)
				res.arc.thickness = thickness;
			break;

		case RND_GLYPH_POLY:
			h = a->poly.pts.used/2;

			if (a->poly.pts.used >= RND_FONT2_MAX_SIMPLE_POLY_POINTS*2) {
				too_large = 1;
				rnd_message(RND_MSG_ERROR, "Can't render glyph atom: simple poly with too many points %d >= %d\n", a->poly.pts.used, RND_FONT2_MAX_SIMPLE_POLY_POINTS);
			}
			else
				too_large = 0;

			if ((opts & RND_FONT_THIN_POLY) || too_large) {
				rnd_coord_t lpx, lpy, tx, ty;

				res.line.thickness = -1;
				res.line.type = RND_GLYPH_LINE;

				tx = a->poly.pts.array[h-1];
				ty = a->poly.pts.array[2*h-1];
				lpx = rnd_round(rnd_xform_x(mx, tx + dx, ty + dy));
				lpy = rnd_round(rnd_xform_y(mx, tx + dx, ty + dy));

				for(nx = 0, ny = h; nx < h; nx++,ny++) {
					rnd_coord_t npx, npy, px = a->poly.pts.array[nx], py = a->poly.pts.array[ny];
					npx = rnd_round(rnd_xform_x(mx, px + dx, py + dy));
					npy = rnd_round(rnd_xform_y(mx, px + dx, py + dy));
					res.line.x1 = lpx; res.line.y1 = lpy;
					res.line.x2 = npx; res.line.y2 = npy;
					cb(cb_ctx, &res);
					lpx = npx;
					lpy = npy;
				}
				return; /* don't let the original cb() at the end run */
			}
			res.poly.pts.used = res.poly.pts.alloced = a->poly.pts.used;
			res.poly.pts.array = tmp;
			for(nx = 0, ny = h; nx < h; nx++,ny++) {
				rnd_coord_t px = a->poly.pts.array[nx], py = a->poly.pts.array[ny];
				res.poly.pts.array[nx] = rnd_round(rnd_xform_x(mx, px + dx, py + dy));
				res.poly.pts.array[ny] = rnd_round(rnd_xform_y(mx, px + dx, py + dy));
			}
			break;
	}

	cb(cb_ctx, &res);
}

#define rnd_font_mx(mx, x0, y0, scx, scy, rotdeg, opts) \
do { \
	rnd_xform_mx_translate(mx, x0, y0); \
	if (OPTS_IS_MIRRORED(opts)) \
		rnd_xform_mx_scale(mx, (opts & RND_FONT_MIRROR_X) ? -1 : 1, (opts & RND_FONT_MIRROR_Y) ? -1 : 1); \
	rnd_xform_mx_rotate(mx, rotdeg); \
	rnd_xform_mx_scale(mx, scx, scy); \
} while(0)

RND_INLINE rnd_coord_t rnd_font_draw_glyph(rnd_font_t *font, rnd_coord_t x, rnd_coord_t y, int chr, int chr_next, rnd_xform_mx_t mx, double scx, double scy, double rotdeg, rnd_font_render_opts_t opts, rnd_coord_t thickness, rnd_coord_t min_line_width, rnd_font_draw_atom_cb cb, void *cb_ctx)
{
	/* draw atoms if symbol is valid and data is present */
	if (is_valid(font, opts, chr)) {
		long n;
		rnd_glyph_t *g = &font->glyph[chr];
		rnd_coord_t xt = x, xnext;
		int inhibit = 0;

		xnext = rnd_font_advance_valid(font, opts, x, chr, chr_next, g);

		if (is_tab(opts, chr)) {
			if (!(opts & RND_FONT_INVIS_TAB)) {
				/* position visible tab character in the middle */
				xt += (xnext - x)/2 - (font)->glyph[(chr)].width;
			}
			else
				inhibit = 1;
		}

		if (!inhibit && (font)->glyph[(chr)].valid)
			for(n = 0; n < g->atoms.used; n++)
				draw_atom(&g->atoms.array[n], mx, xt, y,  scx, scy, rotdeg, opts, thickness, min_line_width, cb, cb_ctx);

		/* move on to next cursor position */
		x = xnext;
	}
	else {
		/* the default symbol is a filled box */
		rnd_coord_t p[8];
		rnd_glyph_atom_t tmp;

		p[0+0] = rnd_round(rnd_xform_x(mx, font->unknown_glyph.X1 + x, font->unknown_glyph.Y1 + y));
		p[4+0] = rnd_round(rnd_xform_y(mx, font->unknown_glyph.X1 + x, font->unknown_glyph.Y1 + y));
		p[0+1] = rnd_round(rnd_xform_x(mx, font->unknown_glyph.X2 + x, font->unknown_glyph.Y1 + y));
		p[4+1] = rnd_round(rnd_xform_y(mx, font->unknown_glyph.X2 + x, font->unknown_glyph.Y1 + y));
		p[0+2] = rnd_round(rnd_xform_x(mx, font->unknown_glyph.X2 + x, font->unknown_glyph.Y2 + y));
		p[4+2] = rnd_round(rnd_xform_y(mx, font->unknown_glyph.X2 + x, font->unknown_glyph.Y2 + y));
		p[0+3] = rnd_round(rnd_xform_x(mx, font->unknown_glyph.X1 + x, font->unknown_glyph.Y2 + y));
		p[4+3] = rnd_round(rnd_xform_y(mx, font->unknown_glyph.X1 + x, font->unknown_glyph.Y2 + y));

		/* draw move on to next cursor position */
		if (opts & RND_FONT_THIN_POLY) {
			tmp.line.type = RND_GLYPH_LINE;
			tmp.line.thickness = -1;

			tmp.line.x1 = p[0+0]; tmp.line.y1 = p[4+0];
			tmp.line.x2 = p[0+1]; tmp.line.y2 = p[4+1];
			cb(cb_ctx, &tmp);

			tmp.line.x1 = p[0+1]; tmp.line.y1 = p[4+1];
			tmp.line.x2 = p[0+2]; tmp.line.y2 = p[4+2];
			cb(cb_ctx, &tmp);

			tmp.line.x1 = p[0+2]; tmp.line.y1 = p[4+2];
			tmp.line.x2 = p[0+3]; tmp.line.y2 = p[4+3];
			cb(cb_ctx, &tmp);

			tmp.line.x1 = p[0+3]; tmp.line.y1 = p[4+3];
			tmp.line.x2 = p[0+0]; tmp.line.y2 = p[4+0];
			cb(cb_ctx, &tmp);
		}
		else {
			tmp.poly.type = RND_GLYPH_POLY;
			tmp.poly.pts.used = tmp.poly.pts.alloced = 8;
			tmp.poly.pts.array = p;
			cb(cb_ctx, &tmp);
		}
		x = rnd_font_advance_invalid(font, opts, x);
	}

	return x;
}

RND_INLINE rnd_coord_t rnd_font_step_y(rnd_font_t *font)
{
	if (font->line_height_cache <= 0)
		font->line_height_cache = (font->line_height > 0) ? font->line_height : font->max_height*1.5;
	return font->line_height_cache;
}

/* skip to the next line or break on end of string */ 
#define next_line() \
	if ((c.chr == 0) || (opts & RND_FONT_STOP_AT_NL)) \
		break; \
	x = 0; \
	y += rnd_font_step_y(font) + extra_y; \
	lineno++; \
	setup_halign(align, &lineno, &x, &y, &extra_glyph, &extra_spc); \
	cursor_next(&c);

typedef struct {
	rnd_coord_t boxw, boxh;
	rnd_font_align_t halign, valign;
	rnd_font_wcache_t *wcache;
} align_t;

#define WCACHE_GLOB_WIDTH ((align->boxw <= 0) ? align->wcache->array[0] : align->boxw)
#define WCACHE_GLOB_HEIGHT (align->boxh)
#define WCACHE_LINE_WIDTH align->wcache->array[(*lineno + 1) * 4]
#define WCACHE_LINE_CHARS align->wcache->array[(*lineno + 1) * 4 + 1]
#define WCACHE_LINE_SPCS  align->wcache->array[(*lineno + 1) * 4 + 2]
#define WCACHE_NUM_LINES  ((align->wcache->used / 4)-1)
#define WCACHE_NET_HEIGHT (WCACHE_NUM_LINES * font->line_height_cache)

RND_INLINE void setup_valign(rnd_font_t *font, rnd_font_render_opts_t opts, align_t *align, int *lineno, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t *extra_y)
{
	if ((opts & RND_FONT_BASELINE) && (font->baseline != 0))
		*y = -font->baseline;
	else
		*y = 0;

	if ((align == NULL) || (align->boxh <= 0)) return;

	rnd_font_step_y(font); /* make sure we have the line_height cache updated */

	switch(align->valign) {
		case RND_FONT_ALIGN_START: return;
		case RND_FONT_ALIGN_invalid: return;
		case RND_FONT_ALIGN_CENTER:    *y += (WCACHE_GLOB_HEIGHT - WCACHE_NET_HEIGHT) / 2; break;
		case RND_FONT_ALIGN_END:       *y += (WCACHE_GLOB_HEIGHT - WCACHE_NET_HEIGHT); break;
		case RND_FONT_ALIGN_WORD_JUST:
		case RND_FONT_ALIGN_JUST:
			if ((WCACHE_NUM_LINES-1) > 0)
				*extra_y = (WCACHE_GLOB_HEIGHT - WCACHE_NET_HEIGHT) / (WCACHE_NUM_LINES-1);
			break;
	}
}

RND_INLINE void setup_halign(align_t *align, int *lineno, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t *extra_glyph, rnd_coord_t *extra_spc)
{
	if ((align == NULL) || (((*lineno+1)*4+3) >= align->wcache->used)) return;

	switch(align->halign) {
		case RND_FONT_ALIGN_START: return;
		case RND_FONT_ALIGN_invalid: return;
		case RND_FONT_ALIGN_CENTER:    *x = (WCACHE_GLOB_WIDTH - WCACHE_LINE_WIDTH) / 2; break;
		case RND_FONT_ALIGN_END:       *x = (WCACHE_GLOB_WIDTH - WCACHE_LINE_WIDTH); break;
		case RND_FONT_ALIGN_WORD_JUST:
			*x = 0;
			if (WCACHE_LINE_SPCS > 0)
				*extra_spc = (WCACHE_GLOB_WIDTH - WCACHE_LINE_WIDTH) / WCACHE_LINE_SPCS;
			else
				*extra_spc = 0;
			break;
		case RND_FONT_ALIGN_JUST:
			*x = 0;
			if ((WCACHE_LINE_CHARS-1) > 0)
				*extra_spc = *extra_glyph = (WCACHE_GLOB_WIDTH - WCACHE_LINE_WIDTH) / (WCACHE_LINE_CHARS-1);
			else
				*extra_spc = *extra_glyph = 0;
			break;
	}
}

RND_INLINE void rnd_font_draw_string_(rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_render_opts_t opts, rnd_coord_t thickness, rnd_coord_t min_line_width, rnd_font_tiny_t tiny, rnd_coord_t extra_glyph, rnd_coord_t extra_spc, align_t *align, rnd_font_draw_atom_cb cb, void *cb_ctx)
{
	rnd_xform_mx_t mx = RND_XFORM_MX_IDENT;
	cursor_t c;
	int lineno = 0;
	rnd_coord_t x = 0, y = 0, extra_y = 0;

	rnd_font_mx(mx, x0, y0, scx, scy, rotdeg, opts);

#ifdef FONT2_DEBUG
	opts |= RND_FONT_MULTI_LINE;
#endif
	setup_valign(font, opts, align, &lineno, &x, &y, &extra_y);
	setup_halign(align, &lineno, &x, &y, &extra_glyph, &extra_spc);

	/* Text too small at this zoom level: cheap draw */
	if (tiny != RND_FONT_TINY_ACCURATE) {
		if (opts & RND_FONT_MULTI_LINE) {
			for(cursor_first(font, opts, &c, string);;) {
				const unsigned char *start = c.pos_next;
				if (!draw_text_cheap(font, opts | RND_FONT_STOP_AT_NL, mx, x, y, start, scx, scy, tiny, cb, cb_ctx))
					goto render_accurately;
				for(; (c.chr != 0) && (c.chr != '\n'); cursor_next(&c)) ;
				next_line();
			}
			return; /* skip expensive rendering */
		}
		else {
			if (draw_text_cheap(font, opts, mx, 0, 0, string, scx, scy, tiny, cb, cb_ctx))
				return;
		}
	}

	render_accurately:;
	if (opts & RND_FONT_MULTI_LINE) {
		for(cursor_first(font, opts, &c, string);;) {
			for(; (c.chr != 0) && (c.chr != '\n'); cursor_next(&c)) {
				x = rnd_font_draw_glyph(font, x, y, c.chr, c.chr_next, mx, scx, scy, rotdeg, opts, thickness, min_line_width, cb, cb_ctx);
				x += (isspace(c.chr)) ? extra_spc :  extra_glyph; /* for justify */
			}
			next_line();
		}
	}
	else {
		for(cursor_first(font, opts, &c, string); c.chr != 0; cursor_next(&c)) {
			x = rnd_font_draw_glyph(font, x, 0, c.chr, c.chr_next, mx, scx, scy, rotdeg, opts, thickness, min_line_width, cb, cb_ctx);
			x += (isspace(c.chr)) ? extra_spc :  extra_glyph; /* for justify */
		}
	}
}

#undef next_line

void rnd_font_draw_string(rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_render_opts_t opts, rnd_coord_t thickness, rnd_coord_t min_line_width, rnd_font_tiny_t tiny, rnd_font_draw_atom_cb cb, void *cb_ctx)
{
	rnd_font_draw_string_(font, string, x0, y0, scx, scy, rotdeg, opts, thickness, min_line_width, tiny, 0, 0, NULL, cb, cb_ctx);
}

void rnd_font_draw_string_justify(rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_render_opts_t opts, rnd_coord_t thickness, rnd_coord_t min_line_width, rnd_font_tiny_t tiny, rnd_coord_t extra_glyph, rnd_coord_t extra_spc, rnd_font_draw_atom_cb cb, void *cb_ctx)
{
	rnd_font_draw_string_(font, string, x0, y0, scx, scy, rotdeg, opts, thickness, min_line_width, tiny, extra_glyph, extra_spc, NULL, cb, cb_ctx);
}

static void wcache_update(rnd_font_t *font, rnd_font_render_opts_t opts, rnd_font_wcache_t *wcache, const unsigned char *string);
void rnd_font_draw_string_align(rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_render_opts_t opts, rnd_coord_t thickness, rnd_coord_t min_line_width, rnd_font_tiny_t tiny, rnd_coord_t boxw, rnd_coord_t boxh, rnd_font_align_t halign, rnd_font_align_t valign, rnd_font_wcache_t *wcache, rnd_font_draw_atom_cb cb, void *cb_ctx)
{
	align_t a;

	a.boxw = boxw;
	a.boxh = boxh;
	a.halign = halign;
	a.valign = valign;
	if (wcache == NULL) {
		/* reuse the allocation but flush the cache - not really reentrant in multithread */
		a.wcache = &rnd_font_tmp_wc;
		a.wcache->used = 0;
	}
	else
		a.wcache = wcache;

	if ((a.wcache->used < 4) || (a.wcache->array[3] != opts))
		wcache_update(font, opts, a.wcache, string);

	rnd_font_draw_string_(font, string, x0, y0, scx, scy, rotdeg, opts, thickness, min_line_width, tiny, 0, 0, &a, cb, cb_ctx);

	if (wcache == NULL)
		rnd_font_uninit_wcache(a.wcache); /* free memory of the temporary cache */
}

/*** wcache ***/
void rnd_font_uninit_wcache(rnd_font_wcache_t *wcache)
{
	vtc0_uninit(wcache);
}

void rnd_font_inval_wcache(rnd_font_wcache_t *wcache)
{
	wcache->used = 0;
}

static void wcache_update(rnd_font_t *font, rnd_font_render_opts_t opts, rnd_font_wcache_t *wcache, const unsigned char *string)
{
	cursor_t c;
	rnd_coord_t x = 0;
	int spc = 0, len = 0;

	/* first entry should be the dimension of the widest line */
	wcache->used = 0;
	vtc0_append(wcache, 0); /* pixel width */
	vtc0_append(wcache, 0); /* character length */
	vtc0_append(wcache, 0); /* number of whitepsace (space and tab) */
	vtc0_append(wcache, opts);

	for(cursor_first(font, opts, &c, string); ; cursor_next(&c)) {
		if ((c.chr == 0) || (c.chr == '\n')) { /* line termination */
			vtc0_append(wcache, x);   /* pixel width */
			vtc0_append(wcache, len); /* character length */
			vtc0_append(wcache, spc); /* number of whitepsace (space and tab) */
			vtc0_append(wcache, 0);   /* padding (opts in the first record) */

			/* update first entry (absolte maximums) */
			if (x > wcache->array[0])   wcache->array[0] = x;
			if (len > wcache->array[1]) wcache->array[1] = len;
			if (spc > wcache->array[2]) wcache->array[2] = spc;
			x = 0;
			spc = len = 0;
		}
		else {
			len++;
			if (isspace(c.chr)) spc++;
			x = rnd_font_advance(font, opts, c.chr, c.chr_next, x);
		}

		if (c.chr == 0)
			break;
	}
}


/*** bbox ***/

/* Calculates accurate centerline bbox */
RND_INLINE void font_arc_bbox(rnd_box_t *dst, rnd_glyph_arc_t *a)
{
	double ca1, ca2, sa1, sa2, minx, maxx, miny, maxy, delta;
	rnd_angle_t ang1, ang2;

	/* first put angles into standard form: ang1 < ang2, both angles between 0 and 720 */
	delta = RND_CLAMP(a->delta, -360, 360);

	if (delta > 0) {
		ang1 = rnd_normalize_angle(a->start);
		ang2 = rnd_normalize_angle(a->start + delta);
	}
	else {
		ang1 = rnd_normalize_angle(a->start + delta);
		ang2 = rnd_normalize_angle(a->start);
	}

	if (ang1 > ang2)
		ang2 += 360;

	/* Make sure full circles aren't treated as zero-length arcs */
	if (delta == 360 || delta == -360)
		ang2 = ang1 + 360;

	/* calculate sines, cosines */
	sa1 = sin(RND_M180 * ang1); ca1 = cos(RND_M180 * ang1);
	sa2 = sin(RND_M180 * ang2); ca2 = cos(RND_M180 * ang2);

	minx = MIN(ca1, ca2); miny = MIN(sa1, sa2);
	maxx = MAX(ca1, ca2); maxy = MAX(sa1, sa2);

	/* Check for extreme angles */
	if ((ang1 <= 0 && ang2 >= 0) || (ang1 <= 360 && ang2 >= 360))
		maxx = 1;
	if ((ang1 <= 90 && ang2 >= 90) || (ang1 <= 450 && ang2 >= 450))
		maxy = 1;
	if ((ang1 <= 180 && ang2 >= 180) || (ang1 <= 540 && ang2 >= 540))
		minx = -1;
	if ((ang1 <= 270 && ang2 >= 270) || (ang1 <= 630 && ang2 >= 630))
		miny = -1;

	/* Finally, calculate bounds, converting sane geometry into rnd geometry */
	dst->X1 = a->cx - a->r * maxx;
	dst->X2 = a->cx - a->r * minx;
	dst->Y1 = a->cy + a->r * miny;
	dst->Y2 = a->cy + a->r * maxy;
}

void rnd_font_arc_bbox(rnd_box_t *dst, rnd_glyph_arc_t *a)
{
	font_arc_bbox(dst, a);
}


RND_INLINE void rnd_font_string_bbox_(rnd_coord_t cx[4], rnd_coord_t cy[4], int compat, rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_render_opts_t opts, rnd_coord_t thickness, rnd_coord_t min_line_width, int scale)
{
	cursor_t c;
	rnd_coord_t minx, miny, maxx, maxy, tx, ty, min_unscaled_radius;
	rnd_bool first_time = rnd_true;
	rnd_xform_mx_t mx = RND_XFORM_MX_IDENT;

	if (string == NULL) {
		int n;
		for(n = 0; n < 4; n++) {
			cx[n] = x0;
			cy[n] = y0;
		}
		return;
	}

#ifdef FONT2_DEBUG
	opts |= RND_FONT_MULTI_LINE;
#endif

	rnd_font_mx(mx, x0, y0, scx, scy, rotdeg, opts);

	minx = miny = maxx = maxy = tx = ty = 0;

	if ((opts & RND_FONT_BASELINE) && (font->baseline != 0))
		miny = maxy = ty = -font->baseline;

	/* We are initially computing the bbox of the un-scaled text but
	   min_line_width is interpreted on the scaled text. */
	if (compat)
		min_unscaled_radius = (min_line_width * (100.0 / (double)scale)) / 2;
	else
		min_unscaled_radius = (min_line_width * (2/(scx+scy))) / 2;

	/* calculate size of the bounding box */
	for(cursor_first(font, opts, &c, string); (c.chr != 0) && !STOP_AT_NL; cursor_next(&c)) {
		if (c.chr == '\n') {
			tx = 0;
			ty += rnd_font_step_y(font);
		}
		else if (is_valid(font, opts, c.chr)) {
			int inhibit = 0;
			rnd_coord_t txnext;
			long n;
			rnd_glyph_t *g = &font->glyph[c.chr];

			txnext = rnd_font_advance_valid(font, opts, tx, c.chr, c.chr_next, g);

			if (is_tab(opts, c.chr)) {
				if (!(opts & RND_FONT_INVIS_TAB)) {
					/* position visible tab character in the middle */
					tx += (txnext - tx)/2 - (font)->glyph[(c.chr)].width;
				}
				else
					inhibit = 1;
			}

			if (!inhibit && g->valid) {
				for(n = 0; n < g->atoms.used; n++) {
					rnd_glyph_atom_t *a = &g->atoms.array[n];
					rnd_coord_t unscaled_radius;
					rnd_box_t b;

					switch(a->type) {
						case RND_GLYPH_LINE:
								/* Clamp the width of text lines at the minimum thickness.
								   NB: Divide 4 in thickness calculation is comprised of a factor
								       of 1/2 to get a radius from the center-line, and a factor
								       of 1/2 because some stupid reason we render our glyphs
								       at half their defined stroke-width. 
								   WARNING: need to keep it for pcb-rnd backward compatibility
								*/
							unscaled_radius = RND_MAX(min_unscaled_radius, a->line.thickness / 4);

							if (first_time) {
								minx = maxx = a->line.x1 + tx;
								miny = maxy = a->line.y1 + ty;
								first_time = rnd_false;
							}

							minx = MIN(minx, a->line.x1 - unscaled_radius + tx);
							miny = MIN(miny, a->line.y1 - unscaled_radius + ty);
							minx = MIN(minx, a->line.x2 - unscaled_radius + tx);
							miny = MIN(miny, a->line.y2 - unscaled_radius + ty);
							maxx = MAX(maxx, a->line.x1 + unscaled_radius + tx);
							maxy = MAX(maxy, a->line.y1 + unscaled_radius + ty);
							maxx = MAX(maxx, a->line.x2 + unscaled_radius + tx);
							maxy = MAX(maxy, a->line.y2 + unscaled_radius + ty);
						break;

						case RND_GLYPH_ARC:
							font_arc_bbox(&b, &a->arc);

							if (compat)
								unscaled_radius = a->arc.thickness / 2;
							else
								unscaled_radius = RND_MAX(min_unscaled_radius, a->arc.thickness / 2);

							minx = MIN(minx, b.X1 + tx - unscaled_radius);
							miny = MIN(miny, b.Y1 + ty - unscaled_radius);
							maxx = MAX(maxx, b.X2 + tx + unscaled_radius);
							maxy = MAX(maxy, b.Y2 + ty + unscaled_radius);
							break;

						case RND_GLYPH_POLY:
							{
								int i, h = a->poly.pts.used/2;
								rnd_coord_t *px = &a->poly.pts.array[0], *py = &a->poly.pts.array[h];

								for(i = 0; i < h; i++,px++,py++) {
									minx = MIN(minx, *px + tx);
									miny = MIN(miny, *py + ty);
									maxx = MAX(maxx, *px + tx);
									maxy = MAX(maxy, *py + ty);
								}
							}
							break;
					}
				}
			}
			tx = txnext;
		}
		else { /* invalid char */
			rnd_box_t *ds = &font->unknown_glyph;

			minx = MIN(minx, ds->X1 + tx);
			miny = MIN(miny, ds->Y1 + ty);
			minx = MIN(minx, ds->X2 + tx);
			miny = MIN(miny, ds->Y2 + ty);
			maxx = MAX(maxx, ds->X1 + tx);
			maxy = MAX(maxy, ds->Y1 + ty);
			maxx = MAX(maxx, ds->X2 + tx);
			maxy = MAX(maxy, ds->Y2 + ty);

			tx = rnd_font_advance_invalid(font, opts, tx);
		}
	}

	/* it is enough to do the transformations only once, on the raw bounding box:
	   calculate the transformed coordinates of all 4 corners of the raw
	   (non-axis-aligned) bounding box */
	cx[0] = rnd_round(rnd_xform_x(mx, minx, miny));
	cy[0] = rnd_round(rnd_xform_y(mx, minx, miny));
	cx[1] = rnd_round(rnd_xform_x(mx, maxx, miny));
	cy[1] = rnd_round(rnd_xform_y(mx, maxx, miny));
	cx[2] = rnd_round(rnd_xform_x(mx, maxx, maxy));
	cy[2] = rnd_round(rnd_xform_y(mx, maxx, maxy));
	cx[3] = rnd_round(rnd_xform_x(mx, minx, maxy));
	cy[3] = rnd_round(rnd_xform_y(mx, minx, maxy));
}

void rnd_font_string_bbox(rnd_coord_t cx[4], rnd_coord_t cy[4], rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_render_opts_t opts, rnd_coord_t thickness, rnd_coord_t min_line_width)
{
	rnd_font_string_bbox_(cx, cy, 0, font, string, x0, y0, scx, scy, rotdeg, opts, thickness, min_line_width, 0);
}

void rnd_font_string_bbox_pcb_rnd(rnd_coord_t cx[4], rnd_coord_t cy[4], rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_render_opts_t opts, rnd_coord_t thickness, rnd_coord_t min_line_width, int scale)
{
	rnd_font_string_bbox_(cx, cy, 1, font, string, x0, y0, scx, scy, rotdeg, opts, thickness, min_line_width, scale);
}

static void rnd_font_normalize_(rnd_font_t *f, int fix_only, int compat)
{
	long i, n, m, h;
	rnd_glyph_t *g;
	rnd_coord_t *px, *py, fminy, fmaxy, fminyw, fmaxyw;
	rnd_coord_t totalminy = RND_MAX_COORD;
	rnd_box_t b;

	/* calculate cell width and height (is at least RND_FONT_DEFAULT_CELLSIZE)
	   maximum cell width and height
	   minimum x and y position of all lines */
	if (!fix_only) {
		f->max_width = RND_FONT_DEFAULT_CELLSIZE;
		f->max_height = RND_FONT_DEFAULT_CELLSIZE;
	}
	fminy = fmaxy = 0;
	fminyw = fmaxyw = 0;

	for(i = 0, g = f->glyph; i <= RND_FONT_MAX_GLYPHS; i++, g++) {
		rnd_coord_t minx, miny, maxx, maxy, minxw, minyw, maxxw, maxyw;

		/* skip if index isn't used or symbol is empty (e.g. space) */
		if (!g->valid || (g->atoms.used == 0))
			continue;

		minx = miny = minxw = minyw = RND_MAX_COORD;
		maxx = maxy = maxxw = maxyw = 0;
		for(n = 0; n < g->atoms.used; n++) {
			rnd_glyph_atom_t *a = &g->atoms.array[n];
			switch(a->type) {
				case RND_GLYPH_LINE:
					minx = MIN(minx, a->line.x1); minxw = MIN(minxw, a->line.x1 - a->line.thickness/2);
					miny = MIN(miny, a->line.y1); minyw = MIN(minyw, a->line.y1 - a->line.thickness/2);
					minx = MIN(minx, a->line.x2); minxw = MIN(minxw, a->line.x2 + a->line.thickness/2);
					miny = MIN(miny, a->line.y2); minyw = MIN(minyw, a->line.y2 + a->line.thickness/2);
					maxx = MAX(maxx, a->line.x1); maxxw = MAX(maxxw, a->line.x1 - a->line.thickness/2);
					maxy = MAX(maxy, a->line.y1); maxyw = MAX(maxyw, a->line.y1 - a->line.thickness/2);
					maxx = MAX(maxx, a->line.x2); maxxw = MAX(maxxw, a->line.x2 + a->line.thickness/2);
					maxy = MAX(maxy, a->line.y2); maxyw = MAX(maxyw, a->line.y2 + a->line.thickness/2);
					break;
				case RND_GLYPH_ARC:
					font_arc_bbox(&b, &a->arc);
					minx = MIN(minx, b.X1); minxw = MIN(minxw, b.X1 - a->arc.thickness/2);
					miny = MIN(miny, b.Y1); minyw = MIN(minyw, b.Y1 - a->arc.thickness/2);
					maxx = MAX(maxx, b.X2); maxxw = MAX(maxxw, b.X2 + a->arc.thickness/2);
					maxy = MAX(maxy, b.Y2); maxyw = MAX(maxyw, b.Y2 + a->arc.thickness/2);
					break;
				case RND_GLYPH_POLY:
					h = a->poly.pts.used/2;
					px = &a->poly.pts.array[0];
					py = &a->poly.pts.array[h];
					for(m = 0; m < h; m++,px++,py++) {
						minx = MIN(minx, *px); minxw = MIN(minxw, *px);
						miny = MIN(miny, *py); minyw = MIN(minyw, *py);
						maxx = MAX(maxx, *px); maxxw = MAX(maxxw, *px);
						maxy = MAX(maxy, *py); maxyw = MAX(maxyw, *py);
					}
					break;
		}
	}

		/* move glyphs to left edge */
		if (minx != 0) {
			for(n = 0; n < g->atoms.used; n++) {
				rnd_glyph_atom_t *a = &g->atoms.array[n];
				switch(a->type) {
					case RND_GLYPH_LINE:
						a->line.x1 -= minx;
						a->line.x2 -= minx;
						break;
					case RND_GLYPH_ARC:
						a->arc.cx -= minx;
						break;
					case RND_GLYPH_POLY:
						h = a->poly.pts.used/2;
						px = &a->poly.pts.array[0];
						for(m = 0; m < h; m++,px++)
							*px -= minx;
						break;
				}
			}
		}

		if (!fix_only) {
			/* set symbol bounding box with a minimum cell size of (1,1) */
			g->width = maxx - minx + 1;
			g->height = maxy + 1;

			/* check per glyph min/max  */
			f->max_width = MAX(f->max_width, g->width);
			f->max_height = MAX(f->max_height, g->height);
		}

		/* check total min/max  */
		fminy = MIN(fminy, miny);
		fmaxy = MAX(fmaxy, maxy);
		fminyw = MIN(fminyw, minyw);
		fmaxyw = MAX(fmaxyw, maxyw);

		totalminy = MIN(totalminy, miny);
	}

	f->cent_height = fmaxy - fminy;
	f->height = fmaxyw - fminyw;

	/* move coordinate system to the upper edge (lowest y on screen) */
	if (!fix_only && (totalminy != 0)) {
		for(i = 0, g = f->glyph; i <= RND_FONT_MAX_GLYPHS; i++, g++) {

			if (!g->valid) continue;
			g->height -= totalminy;

			for(n = 0; n < g->atoms.used; n++) {
				rnd_glyph_atom_t *a = &g->atoms.array[n];
				switch(a->type) {
					case RND_GLYPH_LINE:
						a->line.y1 -= totalminy;
						a->line.y2 -= totalminy;
						break;
					case RND_GLYPH_ARC:
						a->arc.cy -= totalminy;
						break;
					case RND_GLYPH_POLY:
						h = a->poly.pts.used/2;
						py = &a->poly.pts.array[h];
						for(m = 0; m < h; m++,py++)
							*py -= totalminy;
						break;
				}
			}
		}
	}

	/* setup the box for the unknown glyph */
	f->unknown_glyph.X1 = f->unknown_glyph.Y1 = 0;
	f->unknown_glyph.X2 = f->unknown_glyph.X1 + f->max_width;
	f->unknown_glyph.Y2 = f->unknown_glyph.Y1 + f->max_height;
}

void rnd_font_fix_v1(rnd_font_t *f)
{
	rnd_font_normalize_(f, 1, 0);
}


void rnd_font_normalize(rnd_font_t *f)
{
	rnd_font_normalize_(f, 0, 0);
}

void rnd_font_normalize_pcb_rnd(rnd_font_t *f)
{
	rnd_font_normalize_(f, 0, 1);
}

void rnd_font_load_internal(rnd_font_t *font, embf_font_t *embf_font, int len, rnd_coord_t embf_minx, rnd_coord_t embf_miny, rnd_coord_t embf_maxx, rnd_coord_t embf_maxy)
{
	int n, l;
	memset(font, 0, sizeof(rnd_font_t));
	font->max_width  = embf_maxx - embf_minx;
	font->max_height = embf_maxy - embf_miny;

	for(n = 0; n < len; n++) {
		if (embf_font[n].delta != 0) {
			rnd_glyph_t *g = font->glyph + n;
			embf_line_t *lines = embf_font[n].lines;

			vtgla_enlarge(&g->atoms, embf_font[n].num_lines);
			for(l = 0; l < embf_font[n].num_lines; l++) {
				rnd_glyph_atom_t *a = &g->atoms.array[l];
				a->line.type = RND_GLYPH_LINE;
				a->line.x1 = RND_MIL_TO_COORD(lines[l].x1);
				a->line.y1 = RND_MIL_TO_COORD(lines[l].y1);
				a->line.x2 = RND_MIL_TO_COORD(lines[l].x2);
				a->line.y2 = RND_MIL_TO_COORD(lines[l].y2);
				a->line.thickness = RND_MIL_TO_COORD(lines[l].th);
			}
			g->atoms.used = embf_font[n].num_lines;

			g->valid = 1;
			g->xdelta = RND_MIL_TO_COORD(embf_font[n].delta);
		}
	}
	rnd_font_normalize_(font, 0, 1); /* embedded font doesn't have arcs or polygons, loading in compatibility mode won't change anything */
}

void rnd_font_copy_tables(rnd_font_t *dst, const rnd_font_t *src)
{
	htkc_entry_t *k;
	htsi_entry_t *e;

	/* copy the kerning table */
	htkc_init(&dst->kerning_tbl, htkc_keyhash, htkc_keyeq);
	for(k = htkc_first(&src->kerning_tbl); k != NULL; k = htkc_next(&src->kerning_tbl, k))
		htkc_set(&dst->kerning_tbl, k->key, k->value);

	htsi_init(&dst->entity_tbl, strhash, strkeyeq);
	for(e = htsi_first(&src->entity_tbl); e != NULL; e = htsi_next(&src->entity_tbl, e))
		htsi_set(&dst->entity_tbl, rnd_strdup(e->key), e->value);
}

void rnd_font_copy(rnd_font_t *dst, const rnd_font_t *src)
{
	int n, m;
	rnd_glyph_t *dg;
	const rnd_glyph_t *sg;

	memcpy(dst, src, sizeof(rnd_font_t));

	/* dup each glyph (copy all atoms) */
	for(n = 0, sg = src->glyph, dg = dst->glyph; n <= RND_FONT_MAX_GLYPHS; n++,sg++,dg++) {
		rnd_glyph_atom_t *da;
		const rnd_glyph_atom_t *sa;
		long size = sizeof(rnd_glyph_atom_t) * sg->atoms.used;

		if (sg->atoms.used == 0) {
			dg->atoms.used = 0;
			continue;
		}

		dg->atoms.array = malloc(size);
		memcpy(dg->atoms.array, sg->atoms.array, size);
		dg->atoms.used = dg->atoms.alloced = sg->atoms.used;

		/* dup allocated fields of atoms */
		for(m = 0, sa = sg->atoms.array, da = dg->atoms.array; m < dg->atoms.used; m++,sa++,da++) {
			switch(sa->type) {
				case RND_GLYPH_LINE:
				case RND_GLYPH_ARC:
					break; /* no allocated fields */
				case RND_GLYPH_POLY:
					if (sa->poly.pts.used > 0) {
						size = sizeof(rnd_coord_t) * sa->poly.pts.used;
						da->poly.pts.array = malloc(size);
						memcpy(da->poly.pts.array, sa->poly.pts.array, size);
						da->poly.pts.used = da->poly.pts.alloced = sa->poly.pts.used;
					}
					else
						da->poly.pts.used = 0;
					break;
			}
		}
	}

	rnd_font_copy_tables(dst, src);

	if (src->name != NULL)
		dst->name = rnd_strdup(src->name);
}

int rnd_font_invalid_chars(rnd_font_t *font, rnd_font_render_opts_t opts, const unsigned char *string)
{
	cursor_t c;
	int ctr = 0;

	for(cursor_first(font, opts, &c, string); c.chr != 0; cursor_next(&c))
		if ((c.chr > RND_FONT_MAX_GLYPHS) || (!font->glyph[c.chr].valid))
			ctr++;

	return ctr;
}


rnd_glyph_line_t *rnd_font_new_line_in_glyph(rnd_glyph_t *glyph, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t thickness)
{
	rnd_glyph_atom_t *a = vtgla_alloc_append(&glyph->atoms, 1);
	a->line.type = RND_GLYPH_LINE;
	a->line.x1 = x1;
	a->line.y1 = y1;
	a->line.x2 = x2;
	a->line.y2 = y2;
	a->line.thickness = thickness;
	return &a->line;
}

rnd_glyph_arc_t *rnd_font_new_arc_in_glyph(rnd_glyph_t *glyph, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r, rnd_angle_t start, rnd_angle_t delta, rnd_coord_t thickness)
{
	rnd_glyph_atom_t *a = vtgla_alloc_append(&glyph->atoms, 1);
	a->arc.type = RND_GLYPH_ARC;
	a->arc.cx = cx;
	a->arc.cy = cy;
	a->arc.r = r;
	a->arc.start = start;
	a->arc.delta = delta;
	a->arc.thickness = thickness;
	return &a->arc;
}

rnd_glyph_poly_t *rnd_font_new_poly_in_glyph(rnd_glyph_t *glyph, long num_points)
{
	rnd_glyph_atom_t *a = vtgla_alloc_append(&glyph->atoms, 1);
	a->poly.type = RND_GLYPH_POLY;
	vtc0_enlarge(&a->poly.pts, num_points*2);
	return &a->poly;
}


void rnd_font_free_glyph(rnd_glyph_t *g)
{
	int n;
	rnd_glyph_atom_t *a;
	for(n = 0, a = g->atoms.array; n < g->atoms.used; n++,a++) {
		switch(a->type) {
			case RND_GLYPH_LINE:
			case RND_GLYPH_ARC:
				break; /* no allocated fields */
			case RND_GLYPH_POLY:
				vtc0_uninit(&a->poly.pts);
		}
	}
	vtgla_uninit(&g->atoms);
	g->valid = 0;
	g->advance_valid = 0;
	g->width = g->height = g->xdelta = g->advance = 0;
}

void rnd_font_free(rnd_font_t *f)
{
	int n;
	for(n = 0; n <= RND_FONT_MAX_GLYPHS; n++)
		rnd_font_free_glyph(&f->glyph[n]);

	if (f->entity_tbl_valid) {
		htsi_entry_t *e;
		for(e = htsi_first(&f->entity_tbl); e != NULL; e = htsi_next(&f->entity_tbl, e))
			free(e->key);
		htsi_uninit(&f->entity_tbl);
		f->entity_tbl_valid = 0;
	}
	if (f->kerning_tbl_valid) {
		htkc_uninit(&f->kerning_tbl);
		f->kerning_tbl_valid = 0;
	}
	
	free(f->name);
	f->name = NULL;
	f->id = -1;

	/* By freeing the cache here we may lose a memory allocation optimization,
	   but there won't be any render after the last font_free() so we avoid
	   a memleak without having to introduce a central uninit function */
	rnd_font_uninit_wcache(&rnd_font_tmp_wc);
}

