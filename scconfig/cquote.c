/*
    pcb-rnd - quote file and pack it in a C string (ANSI C code)
    Copyright (C) 2016  Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

		Project page: http://repo.hu/projects/scconfig
		Contact via email: scconfig [at] igor2.repo.hu
*/

#include <stdio.h>

void copy(const char *inds)
{
	int c, nl = 0, qt = 1, ind = 1;
	while((c = getc(stdin)) != EOF) {
		if (ind) {
			printf("%s", inds);
			if (!nl)
				printf("   ");
			ind = 0;
		}
		if (nl) {
			printf("NL ");
			nl = 0;
		}
		if (qt) {
			printf("\"");
			qt = 0;
		}
		switch(c) {
			case '\t': printf("	");  break;
			case '\n': printf("\"\n"); nl = qt = ind = 1; break;
			case '\r': break;
			case '\\': printf("\\\\");  break;
			case '"': printf("\\\"");  break;
			default:
				if ((c < 32) || (c>126))
					printf("\\%3o", c);
				else
					putc(c, stdout);
		}
	}
	if (!qt)
		printf("\"");
	if (nl) {
		if (ind)
			printf("%s", inds);
		printf("NL");
	}
	printf(";\n");
}



int main(int argc, char *argv[])
{
	char *varname = "quoted_file";
	char *inds = "\t";
	char *cmd, *arg;
	int n;

	for(n = 1; n < argc; n++) {
		cmd = argv[n];
		arg = argv[n+1];
		while(*cmd == '-') cmd++;
		switch(*cmd) {
			case 'n': varname = arg; n++; break;
			case 'i': inds = arg; n++; break;
		}
	}

	printf("#define NL \"\\n\"\n");

	printf("const char *%s = \\\n", varname);
	copy(inds);
	return 0;
}
