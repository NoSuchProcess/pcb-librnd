/*
    xmc2c - convert gimp exported xmc to C source X bitmap mouse cursor
    Copyright (C) 2022 Tibor 'Igor2' Palinkas

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    http://www.repo.hu/projects/librnd
*/


/* Converts an xmc file exported from gimp to C source. Reads xmc on stdin,
   prints output on stdout. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static long read_int32(char *s)
{
	return (unsigned long)s[0] | (unsigned long)s[1] << 8 | (unsigned long)s[2] << 16 | (unsigned long)s[3] << 24;
}

static void dump(const unsigned char *s, long len)
{
	long n;
	printf("\t");
	for(n = 0; n < len; n++,s++) {
		printf("0x%02x", *s);
		if (n != len-1)
			printf(", ");
		if ((n % 12) == 11)
			printf("\n\t");
	}
}

int main(int argc, char *argv[])
{
	const char *name = "foo";
	char hdr[64], *s;
	long w, h, hotx, hoty, len, n, bp;
	unsigned char *icon, *mask, *i, *m;

	if (argv[1] != NULL)
		name = argv[1];

	if (fread(hdr, 64, 1, stdin) != 1) {
		fprintf(stderr, "error reading the header\n");
		return 1;
	}

	if (strncmp(hdr, "Xcur", 4) != 0) {
		fprintf(stderr, "Broken header\n");
		return 1;
	}

	s = &hdr[0x2c];
	w = read_int32(s); s += 4;
	h = read_int32(s); s += 4;
	hotx = read_int32(s); s += 4;
	hoty = read_int32(s); s += 4;

/*	printf("size=%ld %ld  hot %ld %ld\n", w, h, hotx, hoty);*/

	/* read pixels */
	len = w*h;
	i = icon = calloc(len/8, 1);
	m = mask = calloc(len/8, 1);
	for(n = bp = 0; n < len; n++) {
		int r, g, b, a, v;
		r = fgetc(stdin);
		g = fgetc(stdin);
		b = fgetc(stdin);
		a = fgetc(stdin);

		if ((r == EOF) || (g == EOF) || (b == EOF) || (a == EOF)) {
			fprintf(stderr, "premature EOF in pixel data\n");
			return 1;
		}

		v = r || g || b;
		a = !!a;
		*i = ((*i) >> 1) | (v ? 0x80 : 0);
		*m = ((*m) >> 1) | (a ? 0x80 : 0);
		bp++;
		if (bp == 8) {
			bp = 0;
			i++;
			m++;
		}
	}

	printf("#define %sIcon_width  %ld\n", name, w);
	printf("#define %sIcon_height %ld\n", name, h);
	printf("static unsigned char %sIcon_bits[] = {\n", name);
	dump(icon, len/8);
	printf("};\n");

	printf("#define %sMask_width  %ld\n", name, w);
	printf("#define %sMask_height %ld\n", name, h);
	printf("static unsigned char %sMask_bits[] = {\n", name);
	dump(mask, len/8);
	printf("};\n");

	free(mask);
	free(icon);
	return 0;
}
