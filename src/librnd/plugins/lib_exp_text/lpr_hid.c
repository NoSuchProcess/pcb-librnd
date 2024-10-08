/*
   This file is part of pcb-rnd and was part of gEDA/PCB but lacked proper
   copyright banner at the fork. It probably has the same copyright as
   gEDA/PCB as a whole in 2011.
*/

#include <librnd/config.h>

#include <stdio.h>
#include <stdlib.h>

#include <librnd/core/plugins.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/safe_fs.h>

#include <librnd/hid/hid.h>
#include <librnd/hid/hid_nogui.h>
#include <librnd/hid/hid_init.h>
#include <librnd/hid/hid_attrib.h>

#include "lpr_hid.h"

const char *lpr_cookie = "lpr HID";
static rnd_hid_t lpr_hid;




static const rnd_export_opt_t base_lpr_options[] = {

/* %start-doc options "98 lpr Printing Options"
@ftable @code
@item --lprcommand <string>
Command to use for printing. Defaults to @code{lpr}. This can be used to produce
PDF output with a virtual PDF printer. Example: @*
@code{--lprcommand "lp -d CUPS-PDF-Printer"}.
@end ftable
@noindent In addition, all @ref{Postscript Export} options are valid.
%end-doc
*/
	{"lprcommand", "Command to use for printing",
	 RND_HATT_STRING, 0, 0, {0, "lpr", 0}, 0}
#define HA_lprcommand 0
};

#define NUM_OPTIONS (sizeof(base_lpr_options)/sizeof(base_lpr_options[0]))

static rnd_export_opt_t *lpr_options = 0;
static int num_lpr_options = 0;
static rnd_hid_attr_val_t *lpr_values;

static void lpr_maybe_set_value(const char *name, double new_val)
{
	int n;

	if (new_val <= 0)
		return;

	for(n = 0; n < num_lpr_options; n++) {
		if (strcmp(name, lpr_options[n].name) == 0) {
			lpr_values[n].dbl = new_val;
			break;
		}
	}
}

static const double *lpr_xcalib, *lpr_ycalib;
static void lpr_ps_init(rnd_hid_t *ps_hid)
{
	if (lpr_options == 0) {
		const rnd_export_opt_t *ps_opts = ps_hid->get_export_options(ps_hid, &num_lpr_options, NULL, NULL);
		lpr_options = calloc(num_lpr_options, sizeof(rnd_export_opt_t));
		memcpy(lpr_options, ps_opts, num_lpr_options * sizeof(rnd_export_opt_t));
		memcpy(lpr_options, base_lpr_options, sizeof(base_lpr_options));
		if (lpr_hid.argument_array == NULL) {
			lpr_values = calloc(num_lpr_options, sizeof(rnd_hid_attr_val_t));
			lpr_hid.argument_array = lpr_values;
		}

		rnd_hid_load_defaults(&lpr_hid, lpr_options, num_lpr_options);

		lpr_maybe_set_value("xcalib", (lpr_xcalib != NULL) ? *lpr_xcalib : 1.0);
		lpr_maybe_set_value("ycalib", (lpr_ycalib != NULL) ? *lpr_ycalib : 1.0);
	}
}

static const rnd_export_opt_t *lpr_get_export_options(rnd_hid_t *hid, int *n, rnd_design_t *dsg, void *appspec)
{
	const char *val;



	/* We initialize the default value in this manner because the GUI
	   HID's may want to free() this string value and replace it with a
	   new one based on how a user fills out a print dialog. */
	val = lpr_values[HA_lprcommand].str;
	if ((val == NULL) || (*val == '\0')) {
		free((char *)lpr_values[HA_lprcommand].str);
		lpr_values[HA_lprcommand].str = rnd_strdup("lpr");
	}

	if (n)
		*n = num_lpr_options;
	return lpr_options;
}


static void (*rnd_lpr_hid_export_to_file)(rnd_design_t *dsg, FILE *, rnd_hid_attr_val_t *, rnd_xform_t *, void *) = NULL;
static void lpr_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	FILE *f;
	const char *filename;

	if (!options) {
		lpr_get_export_options(hid, 0, design, appspec);
		options = lpr_values;
	}

	filename = options[HA_lprcommand].str;

	rnd_trace("LPR: open %s\n", filename);
	f = rnd_popen(NULL, filename, "w");
	if (!f) {
		perror(filename);
		return;
	}

	rnd_lpr_hid_export_to_file(design, f, options, NULL, appspec);

	rnd_pclose(f);
}

static int lpr_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	lpr_get_export_options(hid, 0, NULL, NULL);
	rnd_export_register_opts2(hid, lpr_options, num_lpr_options, lpr_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}

static int lpr_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nlpr exporter command line arguments:\n\n");
	rnd_hid_usage(base_lpr_options, sizeof(base_lpr_options) / sizeof(base_lpr_options[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x lpr [lpr options] foo.pcb\n\n");
	return 0;
}


void rnd_lpr_uninit(void)
{
	rnd_hid_remove_hid(&lpr_hid);
	free(lpr_hid.argument_array);
	free(lpr_options);
	lpr_hid.argument_array = NULL;
}

int rnd_lpr_init(rnd_hid_t *ps_hid, void (*ps_ps_init)(rnd_hid_t *), void (*hid_export_to_file)(rnd_design_t *dsg, FILE *, rnd_hid_attr_val_t *, rnd_xform_t *, void *), const double *xcalib, const double *ycalib)
{
	RND_API_CHK_VER;

	rnd_lpr_hid_export_to_file = hid_export_to_file;
	lpr_xcalib = xcalib;
	lpr_ycalib = ycalib;

	memset(&lpr_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&lpr_hid);
	ps_ps_init(&lpr_hid);

	lpr_hid.struct_size = sizeof(rnd_hid_t);
	lpr_hid.name = "lpr";
	lpr_hid.description = "Postscript print";
	lpr_hid.printer = 1;

	lpr_hid.get_export_options = lpr_get_export_options;
	lpr_hid.do_export = lpr_do_export;
	lpr_hid.parse_arguments = lpr_parse_arguments;
	lpr_hid.argument_array = NULL;

	lpr_hid.usage = lpr_usage;

	lpr_ps_init(ps_hid);

	rnd_hid_register_hid(&lpr_hid);
	rnd_hid_load_defaults(&lpr_hid, base_lpr_options, NUM_OPTIONS);

	return 0;
}


