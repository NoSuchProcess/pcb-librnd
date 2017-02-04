/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Alain Vigne
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 *
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* FIXME: Find a way to get rid of GHidPort *out and gui.h */
#include "gui.h"

/* FIXME: could be more generic with a pcb_color_s structure depending on Toolkit :
 * GdkColor for GTK2
 * GdkRGBA  for GTK3
 */
/*#include "colors.h"*/

const gchar *ghid_get_color_name(GdkRGBA * color)
{
	static char tmp[16];

	if (!color)
		return "#000000";

	/*sprintf(tmp, "#%2.2x%2.2x%2.2x", (color->red >> 8) & 0xff, (color->green >> 8) & 0xff, (color->blue >> 8) & 0xff); */
	tmp = gdk_rgba_to_string(color);
	/* Note that this string representation may lose some precision, since r, g and b
	   are represented as 8-bit integers.
	   If this is a concern, you should use a different representation.
	 */

	return tmp;
}

void ghid_map_color_string(const char *color_string, GdkRGBA * color)
{
	/*FIXME: Get rid of GdkColormap and GHidPort */
	static GdkColormap *colormap = NULL;
	GHidPort *out = &ghid_port;
	/* Needs a
	   GdkVisual *visual
	   to replace GdkColormap, then cairo will handle colors
	 */

	if (!color || !out->top_window)
		return;
	if (colormap == NULL)
		colormap = gtk_widget_get_colormap(out->top_window);
	if (color->red || color->green || color->blue)
		gdk_colormap_free_colors(colormap, color, 1);
	/*if false then ? */
	gdk_rgba_parse(color, color_string);
	gdk_color_alloc(colormap, color);
}
