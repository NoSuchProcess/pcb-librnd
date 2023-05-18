/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
 *  Copyright (C) 2016..2019 Tibor 'Igor2' Palinkas
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
 *
 */

#include <librnd/rnd_config.h>
#include <string.h>
#include <librnd/hid/hid_attrib.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/hidlib.h>

rnd_hid_attr_node_t *rnd_hid_attr_nodes = 0;

void rnd_export_register_opts2(rnd_hid_t *hid, const rnd_export_opt_t *a, int n, const char *cookie, int copy)
{
	rnd_hid_attr_node_t *ha;

	/* printf("%d attributes registered\n", n); */
	ha = malloc(sizeof(rnd_hid_attr_node_t));
	ha->next = rnd_hid_attr_nodes;
	rnd_hid_attr_nodes = ha;
	ha->opts = a;
	ha->n = n;
	ha->cookie = cookie;
	ha->hid = hid;
}

void rnd_export_uninit(void)
{
	rnd_hid_attr_node_t *ha, *next;
	for (ha = rnd_hid_attr_nodes; ha; ha = next) {
		next = ha->next;
		if (ha->cookie != NULL)
			fprintf(stderr, "WARNING: attribute %s by %s is not uninited, check your plugins' uninit!\n", ha->opts->name, ha->cookie);
		free(ha);
	}
	rnd_hid_attr_nodes = NULL;
}

void rnd_export_remove_opts_by_cookie(const char *cookie)
{
	rnd_hid_attr_node_t *ha, *next, *prev = NULL;
	for (ha = rnd_hid_attr_nodes; ha; ha = next) {
		next = ha->next;
		if (ha->cookie == cookie) {
			if (prev == NULL)
				rnd_hid_attr_nodes = next;
			else
				prev->next = next;
			free(ha);
		}
		else
			prev = ha;
	}
}

static void opt2attr(rnd_hid_attr_val_t *dst, const rnd_export_opt_t *src)
{
	switch (src->type) {
		case RND_HATT_LABEL:
		case RND_HATT_INTEGER:
		case RND_HATT_COORD:
		case RND_HATT_BOOL:
		case RND_HATT_REAL:
		case RND_HATT_ENUM:
		case RND_HATT_UNIT:
			*dst = src->default_val;
			break;
		case RND_HATT_STRING:
			free((char *)dst->str);
			dst->str = src->default_val.str == NULL ? NULL : rnd_strdup(src->default_val.str);
			break;
		default:
		if (RND_HATT_IS_COMPOSITE(src->type)) /* function callback */
			break;
		rnd_message(RND_MSG_ERROR, "Invalid attribute type %d for attribute %s\n", src->type, src->name);
		abort();
	}
}

void rnd_hid_load_defaults(rnd_hid_t *hid, const rnd_export_opt_t *opts, int len)
{
	int n;
	rnd_hid_attr_val_t *values = hid->argument_array;

	for(n = 0; n < len; n++)
		opt2attr(&values[n], opts + n);
}


#define MAX_FILENAMES 1024
int rnd_hid_parse_command_line(int *argc, char ***argv)
{
	rnd_hid_attr_node_t *ha;
	int i, e, ok, filenames = 0;
	char *filename[MAX_FILENAMES+2], *end;
	rnd_bool succ;

	/* set defaults */
	for (ha = rnd_hid_attr_nodes; ha; ha = ha->next) {
		rnd_hid_attr_val_t *backup = NULL;
		if (ha->hid != NULL)
			backup = ha->hid->argument_array;

		if (backup == NULL) {
			rnd_message(RND_MSG_ERROR, "rnd_hid_parse_command_line(): no backup storage. Direct ->value field is not supported anymore. Your hid/export plugin (%s, %s) needs to be fixed.\n", ha->hid->name, ha->cookie);
			return -1;
		}
		rnd_hid_load_defaults(ha->hid, ha->opts, ha->n);
	}

	if (rnd_gui->get_export_options != NULL)
		rnd_gui->get_export_options(rnd_gui, NULL, NULL, NULL);

	/* parse the command line and overwrite with values read from argc/argv */
	while(*argc > 0)
	if (*argc && (*argv)[0][0] == '-' && (*argv)[0][1] == '-') {
		int bool_val;
		int arg_ofs;

		bool_val = 1;
		arg_ofs = 2;
		try_no_arg:;
		for (ha = rnd_hid_attr_nodes; ha; ha = ha->next) {
			rnd_hid_attr_val_t *backup = ha->hid->argument_array;

			for (i = 0; i < ha->n; i++)
				if (strcmp((*argv)[0] + arg_ofs, ha->opts[i].name) == 0) {
					const rnd_export_opt_t *a = ha->opts + i;
					char *ep;
					const rnd_unit_t *unit;
					switch (ha->opts[i].type) {
					case RND_HATT_LABEL:
						break;
					case RND_HATT_INTEGER:
						if ((*argv)[1] == NULL) {
							rnd_message(RND_MSG_ERROR, "export parameter error: %s requires a parameter\n", (*argv)[0]);
							return -1;
						}
						else
							backup[i].lng = strtol((*argv)[1], &end, 0);
						if (*end != '\0') {
							rnd_message(RND_MSG_ERROR, "export parameter error: %s requires an integer but got '%s'\n", (*argv)[0], (*argv)[1]);
							return -1;
						}
						(*argc)--;
						(*argv)++;
						break;
					case RND_HATT_COORD:
						if ((*argv)[1] == NULL) {
							rnd_message(RND_MSG_ERROR, "export parameter error: %s requires a parameter\n", (*argv)[0]);
							return -1;
						}
						else
							backup[i].crd = rnd_get_value((*argv)[1], NULL, NULL, &succ);
						if (!succ) {
							rnd_message(RND_MSG_ERROR, "export parameter error: %s requires a coord value but got '%s'\n", (*argv)[0], (*argv)[1]);
							return -1;
						}
						(*argc)--;
						(*argv)++;
						break;
					case RND_HATT_REAL:
						if ((*argv)[1] == NULL) {
							rnd_message(RND_MSG_ERROR, "export parameter error: %s requires a parameter\n", (*argv)[0]);
							return -1;
						}
						else
							backup[i].dbl = strtod((*argv)[1], &end);
						if (*end != '\0') {
							rnd_message(RND_MSG_ERROR, "export parameter error: %s requires a number but got '%s'\n", (*argv)[0], (*argv)[1]);
							return -1;
						}
						(*argc)--;
						(*argv)++;
						break;
					case RND_HATT_STRING:
						backup[i].str = rnd_strdup(RND_EMPTY((*argv)[1]));
						(*argc)--;
						(*argv)++;
						break;
					case RND_HATT_BOOL:
						backup[i].lng = bool_val;
						break;
					case RND_HATT_ENUM:
						if ((*argv)[1] == NULL) {
							rnd_message(RND_MSG_ERROR, "export parameter error: %s requires a parameter\n", (*argv)[0]);
							return -1;
						}
						else
							ep = (*argv)[1];
						ok = 0;
						for (e = 0; a->enumerations[e]; e++)
							if (strcmp(a->enumerations[e], ep) == 0) {
								ok = 1;
								backup[i].lng = e;
								backup[i].str = ep;
								break;
							}
						if (!ok) {
							rnd_message(RND_MSG_ERROR, "ERROR:  \"%s\" is an unknown value for the --%s option\n", (*argv)[1], a->name);
							return -1;
						}
						(*argc)--;
						(*argv)++;
						break;
					case RND_HATT_UNIT:
						if ((*argv)[1] == NULL) {
							rnd_message(RND_MSG_ERROR, "export parameter error: %s requires a parameter\n", (*argv)[0]);
							return -1;
						}
						else
							unit = rnd_get_unit_struct((*argv)[1]);
						if (unit == NULL) {
							rnd_message(RND_MSG_ERROR, "ERROR:  unit \"%s\" is unknown to pcb (option --%s)\n", (*argv)[1], a->name);
							return -1;
						}
						backup[i].lng = unit->index;
						backup[i].str = unit->suffix;
						(*argc)--;
						(*argv)++;
						break;
					default:
						break;
					}
					(*argc)--;
					(*argv)++;
					ha = 0;
					goto got_match;
				}
		}
		if (bool_val == 1 && strncmp((*argv)[0], "--no-", 5) == 0) {
			bool_val = 0;
			arg_ofs = 5;
			goto try_no_arg;
		}
		rnd_message(RND_MSG_ERROR, "unrecognized option: %s\n", (*argv)[0]);
		return -1;
	got_match:;
	}
	else {
		/* Any argument without the "--" is considered a filename. Take only
		   one filename for now */

		if (filenames > MAX_FILENAMES) {
			rnd_message(RND_MSG_ERROR, "Too manu filenames specified. Overflow at filename: %s\n", (*argv)[0]);
			return -1;
		}
		if (!rnd_app.multi_design && (filenames > 0)) {
			rnd_message(RND_MSG_ERROR, "Multiple filenames not supported. First filename was: %s; offending second filename: %s\n", filename[0], (*argv)[0]);
			return -1;
		}
		
		filename[filenames++] = (*argv)[0];
		(*argc)--;
		(*argv)++;
	}

	/* restore filename to the first argument */
	for(i = 0; i < filenames; i++)
		(*argv)[i] = filename[i];
	(*argc) = filenames;

	return 0;
}

void rnd_hid_usage_option(const char *name, const char *help)
{
	fprintf(stderr, "--%-20s %s\n", name, help);
}

void rnd_hid_usage(const rnd_export_opt_t *a, int numa)
{
	for (; numa > 0; numa--,a++) {
		const char *help;
		if (a->help_text == NULL) help = "";
		else help = a->help_text;
		if (RND_HATT_IS_COMPOSITE(a->type)) {
			rnd_hid_export_opt_func_t fnc = a->default_val.func;
			if (fnc != NULL)
				fnc(RND_HIDEOF_USAGE, stderr, a, NULL);
		}
		else {
			rnd_hid_usage_option(a->name, help);
			if ((a->type == RND_HATT_ENUM) && (a->enumerations != NULL)) {
				const char **s;
				fprintf(stderr, "  %-20s Possible enum values:\n", "");
				for(s = a->enumerations; *s != NULL; s++)
					fprintf(stderr, "  %-22s %s\n", "", *s);
			}
		}
	}
}

