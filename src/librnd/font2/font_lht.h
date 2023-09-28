/*
 *                            COPYRIGHT
 *
 *  librnd, modular 2D CAD framework - lihata parser support - Ringdove font v2
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

#include <librnd/core/compat_misc.h>
#include <librnd/font2/font.h>
#include <genht/hash.h>

/* calls provided by the caller:

   Return non-zero on error:
    int PARSE_COORD(rnd_coord_t *dst, lht_node src);
    int PARSE_DOUBLE(double *dst, lht_node src);

   Prints error and returns special value recognized by PARSE_* if
   name does not exist in hash:
    lht_node_t *HASH_GET(lht_node_t *hash, const char *name);

   Prints error for nd:
    RND_LHT_ERROR(lht_node_t *nd, const char *fmt, ...)

*/

/* Parsing depends on font->filever: when seeing v2+ features, a warning
   is generated if font->filever is 1; font->filever==0 means unknown,
   no warning is issued. */
static int rnd_font_lht_parse_glyph(rnd_glyph_t *glp, lht_node_t *nd);
static int rnd_font_lht_parse_font(rnd_font_t *font, lht_node_t *nd);

/*** Implementation */

static int rnd_font_lht_parse_glyph(rnd_glyph_t *glp, lht_node_t *nd)
{
	lht_node_t *grp, *obj, *n;
	lht_dom_iterator_t it;
	int err = 0;

	err |= PARSE_COORD(&glp->width,  HASH_GET(nd, "width"));
	err |= PARSE_COORD(&glp->height, HASH_GET(nd, "height"));
	err |= PARSE_COORD(&glp->xdelta, HASH_GET(nd, "delta"));

	grp = lht_dom_hash_get(nd, "objects");
	for(obj = lht_dom_first(&it, grp); obj != NULL; obj = lht_dom_next(&it)) {
		rnd_coord_t x1, y1, x2, y2, th, r;
		double sa, da;
		rnd_glyph_atom_t *a;

		if (strncmp(obj->name, "line.", 5) == 0) {
			err |= PARSE_COORD(&x1, HASH_GET(obj, "x1"));
			err |= PARSE_COORD(&y1, HASH_GET(obj, "y1"));
			err |= PARSE_COORD(&x2, HASH_GET(obj, "x2"));
			err |= PARSE_COORD(&y2, HASH_GET(obj, "y2"));
			err |= PARSE_COORD(&th, HASH_GET(obj, "thickness"));
			if (err != 0)
				return -1;

			a = vtgla_alloc_append(&glp->atoms, 1);
			a->line.type = RND_GLYPH_LINE;
			a->line.x1 = x1;
			a->line.y1 = y1;
			a->line.x2 = x2;
			a->line.y2 = y2;
			a->line.thickness = th;
		}
		else if (strncmp(obj->name, "simplearc.", 10) == 0) {
			err |= PARSE_COORD(&x1,  HASH_GET(obj, "x"));
			err |= PARSE_COORD(&y1,  HASH_GET(obj, "y"));
			err |= PARSE_COORD(&r,   HASH_GET(obj, "r"));
			err |= PARSE_COORD(&th,  HASH_GET(obj, "thickness"));
			err |= PARSE_DOUBLE(&sa, HASH_GET(obj, "astart"));
			err |= PARSE_DOUBLE(&da, HASH_GET(obj, "adelta"));
			if (err != 0)
				return -1;

			a = vtgla_alloc_append(&glp->atoms, 1);
			a->arc.type = RND_GLYPH_ARC;
			a->arc.cx = x1;
			a->arc.cy = y1;
			a->arc.r = r;
			a->arc.start = sa;
			a->arc.delta = da;
			a->arc.thickness = th;
		}
		else if (strncmp(obj->name, "simplepoly.", 11) == 0) {
			int len, h, i;

			if (obj->type != LHT_LIST) {
				RND_LHT_ERROR(obj, "Symbol error: simplepoly is not a list! (ignoring this poly)\n");
				continue;
			}
			for(len = 0, n = obj->data.list.first; n != NULL; len++, n = n->next) ;
			if ((len % 2 != 0) || (len < 6)) {
				RND_LHT_ERROR(obj, "Symbol error: sumplepoly has wrong number of points (%d, expected an even integer >= 6)! (ignoring this poly)\n", len);
				continue;
			}

			a = vtgla_alloc_append(&glp->atoms, 1);
			a->poly.type = RND_GLYPH_POLY;
			vtc0_enlarge(&a->poly.pts, len);
			a->poly.pts.used = len;

			h = len/2;
			for(i = 0, n = obj->data.list.first; n != NULL; n = n->next,i++) {
				PARSE_COORD(&x1, n);
				n = n->next;
				PARSE_COORD(&y1, n);
				a->poly.pts.array[i]   = x1;
				a->poly.pts.array[h+i] = y1;
			}
		}
	}

	glp->valid = 1;
	return 0;
}

static int load_kern_key(const char *start, const char *end)
{
	/* single character cases, left or right */
	if (end == start+1)
		return *start;
	if ((end == NULL) && (start[0] != '\0') && (start[1] == '\0'))
		return *start;

	if (*start == '&') {
		char *e;
		long val = strtol(start+1, &e, 10);
		if (end != NULL) {
			if (e != end)
				return 0; /* non-numeric suffix on left side */
		}
		else {
			if (*e != '\0')
				return 0; /* non-numeric suffix on right side */
		}
		if ((val < 1) || (val > 254))
			return 0;
		return val;
	}

	return 0; /* unknown format */
}

static int rnd_font_lht_parse_font(rnd_font_t *font, lht_node_t *nd)
{
	lht_node_t *grp, *sym, *tabw, *ents, *ent, *kerning, *kern, *lineh;
	lht_dom_iterator_t it;
	int err = 0;

	if (nd->type != LHT_HASH) {
		RND_LHT_ERROR(nd, "font must be a hash\n");
		return -1;
	}

	err |= PARSE_COORD(&font->max_height, HASH_GET(nd, "cell_height"));
	err |= PARSE_COORD(&font->max_width,  HASH_GET(nd, "cell_width"));

	lineh = lht_dom_hash_get(nd, "line_height");
	if (lineh != NULL) {
		if ((font->filever > 0) && (font->filever < 2))
			RND_LHT_ERROR(tabw, "line height is unexpected below font file version 2.\n");
		PARSE_COORD(&font->line_height, lineh);
	}
	else
		font->line_height = 0;


	tabw = lht_dom_hash_get(nd, "tab_width");
	if (tabw != NULL) {
		if ((font->filever > 0) && (font->filever < 2))
			RND_LHT_ERROR(tabw, "tab_width is unexpected below font file version 2.\n");
		PARSE_COORD(&font->tab_width, tabw);
	}
	else
		font->tab_width = 0;

	ents = lht_dom_hash_get(nd, "entities");
	if (ents != NULL) {
		if (ents->type == LHT_HASH) {
			if ((font->filever > 0) && (font->filever < 2))
				RND_LHT_ERROR(ents, "entities is unexpected below font file version 2.\n");
			htsi_init(&font->entity_tbl, strhash, strkeyeq);
			font->entity_tbl_valid = 1;
			for(ent = lht_dom_first(&it, ents); ent != NULL; ent = lht_dom_next(&it)) {
				if (ent->type == LHT_TEXT) {
					char *end;
					long idx = strtol(ent->data.text.value, &end, 10);
					if ((*end == '\0') && (idx > 0) && (idx < 255))
						htsi_set(&font->entity_tbl, rnd_strdup(ent->name), idx);
					else
						RND_LHT_ERROR(ent, "entity value must be a decimal integer between 1 and 254 inclusive\n");
				}
			}
		}
		else
			RND_LHT_ERROR(ents, "entities shall be a hash\n");
	}

	kerning = lht_dom_hash_get(nd, "kerning");
	if (kerning != NULL) {
		if (kerning->type == LHT_HASH) {
			if ((font->filever > 0) && (font->filever < 2))
				RND_LHT_ERROR(kerning, "entities is unexpected below font file version 2.\n");
			htkc_init(&font->kerning_tbl, htkc_keyhash, htkc_keyeq);
			font->kerning_tbl_valid = 1;
			for(kern = lht_dom_first(&it, kerning); kern != NULL; kern = lht_dom_next(&it)) {
				if (kern->type == LHT_TEXT) {
					rnd_coord_t offs = 0;
					err |= PARSE_COORD(&offs,  kern);
					if (offs != 0) {
						htkc_key_t key;
						char *sep = strchr(kern->name+1, '-'); /* +1 so if '-' is the left char it is preserved */
						if (sep != NULL) {
							key.left = load_kern_key(kern->name, sep);
							key.right = load_kern_key(sep+1, NULL);
							if ((key.left > 0) && (key.right > 0))
								htkc_set(&font->kerning_tbl, key, offs);
							else
								RND_LHT_ERROR(kern, "invalid character in kerning key\n");
						}
						else
							RND_LHT_ERROR(kern, "kerning key must be char-char pair\n");
					}
					else
						RND_LHT_ERROR(kern, "kerning value must be a non-zero coord\n");
				}
			}
		}
		else
			RND_LHT_ERROR(kerning, "kerning shall be a hash\n");
	}


	grp = lht_dom_hash_get(nd, "symbols");

	for(sym = lht_dom_first(&it, grp); sym != NULL; sym = lht_dom_next(&it)) {
		int chr;
		if (sym->type != LHT_HASH)
			continue;
		if (*sym->name == '&') {
			char *end;
			chr = strtol(sym->name+1, &end, 16);
			if (*end != '\0') {
				RND_LHT_ERROR(sym, "Ignoring glyph with invalid glyph name '%s'.\n", sym->name);
				continue;
			}
		}
		else
			chr = *sym->name;
		if ((chr >= 0) && (chr < sizeof(font->glyph) / sizeof(font->glyph[0])))
			rnd_font_lht_parse_glyph(font->glyph+chr, sym);
	}

	return err;
}

static int rnd_font_load(rnd_design_t *design, rnd_font_t *dst, const char *fn, int quiet)
{
	int res = 0;
	char *errmsg = NULL, *realfn;
	lht_doc_t *doc = NULL;

	realfn = rnd_fopen_check(design, fn, "r");
	if (realfn != NULL)
		doc = lht_dom_load(realfn, &errmsg);
	free(realfn);

	if (doc == NULL) {
		if (!quiet)
			rnd_message(RND_MSG_ERROR, "Error loading '%s': %s\n", fn, errmsg);
		free(errmsg);
		return -1;
	}

	dst->filever = 0;
	if ((doc->root->type != LHT_LIST) || (strncmp(doc->root->name, "pcb-rnd-font-v", 14) != 0)) {
		if (!quiet)
			rnd_message(RND_MSG_ERROR, "Error loading font: %s is not a font lihata.\n", fn);
		res = -1;
	}
	else {
		dst->filever = atoi(doc->root->name+14);
		if ((dst->filever < 1) || (dst->filever > 2)) {
			if (!quiet)
				rnd_message(RND_MSG_ERROR, "Error loading font: %s version %d is out of range (version not supported).\n", fn, dst->filever);
			res = -1;
		}
		else
			rnd_font_lht_parse_font(dst, doc->root->data.list.first);
	}

	free(errmsg);
	lht_dom_uninit(doc);
	return res;
}
