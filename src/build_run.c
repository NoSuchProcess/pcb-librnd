/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,2006 Thomas Nau
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#include "config.h"
#include <signal.h>
#include <genvector/gds_char.h>
#include "conf_core.h"
#include "board.h"
#include "build_run.h"
#include "hid_init.h"
#include "plug_io.h"
#include "compat_misc.h"

extern void pcb_main_uninit(void);
/** pcb_quit_app:
 *  Quits application
 */
void pcb_quit_app(void)
{
	/*
	 * save data if necessary.  It not needed, then don't trigger EmergencySave
	 * via our atexit() registering of pcb_emergency_save().  We presumably wanted to
	 * exit here and thus it is not an emergency.
	 */
	if (PCB->Changed && conf_core.editor.save_in_tmp)
		pcb_emergency_save();
	else
		pcb_disable_emergency_save();

	if (pcb_gui->do_exit == NULL) {
		pcb_main_uninit();
		exit(0);
	}
	else
		pcb_gui->do_exit(pcb_gui);
}

/** pcb_get_info_program:
 *  Returns a string that has a bunch of information about this program.
 */
char *pcb_get_info_program(void)
{
	static gds_t info;
	static int first_time = 1;

	if (first_time) {
		first_time = 0;
		gds_init(&info);
		gds_append_str(&info, "This is pcb-rnd " PCB_VERSION " (" PCB_REVISION ")" "\n an interactive ");
		gds_append_str(&info, "printed circuit board editor\n");
	}
	return info.array;
}

/** pcb_get_info_copyright:
 *  Returns a string that has a bunch of information about the copyrights.
 */
char *pcb_get_info_copyright(void)
{
	static gds_t info;
	static int first_time = 1;

	if (first_time) {
		first_time = 0;
		gds_init(&info);

		gds_append_str(&info, "Recent pcb-rnd copyright:\n");
		gds_append_str(&info, "Copyright (C) Tibor Palinkas 2013..2017 (pcb-rnd)\n");
		gds_append_str(&info, "For a more complete list of pcb-rnd authors and\ncontributors, see http://repo.hu/cgi-bin/pcb-rnd-people.cgi\n\n");

		gds_append_str(&info, "Historical copyright:\n");
		gds_append_str(&info, "pcb-rnd was originally forked from gEDA/PCB:\n");
		gds_append_str(&info, "Copyright (C) Thomas Nau 1994..1997\n");
		gds_append_str(&info, "Copyright (C) harry eaton 1998..2007\n");
		gds_append_str(&info, "Copyright (C) C. Scott Ananian 2001\n");
		gds_append_str(&info, "Copyright (C) DJ Delorie 2003..2008\n");
		gds_append_str(&info, "Copyright (C) Dan McMahill 2003..2008\n\n");
	}
	return info.array;
}

/** pcb_get_info_websites:
 *  Returns a string that has a bunch of information about the websites.
 */
char *pcb_get_info_websites(const char **url_out)
{
	static gds_t info;
	static int first_time = 1;
	static const char *URL = "http://pcb-rnd.repo.hu\n";

	if (first_time) {
		first_time = 0;
		gds_init(&info);

		gds_append_str(&info, "For more information see:\n");
		gds_append_str(&info, URL);
	}

	if (url_out != NULL)
		*url_out = URL;

	return info.array;
}

/** pcb_get_info_comments:
 *  Returns a string as the concatenation of pcb_get_info_program() and pcb_get_info_websites()
 */
char *pcb_get_info_comments(void)
{
	static gds_t info;
	static int first_time = 1;
	char *tmp;

	if (first_time) {
		first_time = 0;
		gds_init(&info);

		tmp = pcb_get_info_program();
		gds_append_str(&info, tmp);
		tmp = pcb_get_info_websites(NULL);
		gds_append_str(&info, tmp);
	}
	return info.array;
}


/** pcb_get_info_compile_options:
 *  Returns a string that has a bunch of information about the options selected at compile time.
 */
char *pcb_get_info_compile_options(void)
{
	pcb_hid_t **hids;
	int i;
	static gds_t info;
	static int first_time = 1;

#define TAB "    "

	if (first_time) {
		first_time = 0;
		gds_init(&info);

		gds_append_str(&info, "----- Run Time Options -----\n");
		gds_append_str(&info, "GUI: ");
		if (pcb_gui != NULL) {
			gds_append_str(&info, pcb_gui->name);
			gds_append_str(&info, "\n");
		}
		else
			gds_append_str(&info, "none\n");

		gds_append_str(&info, "\n----- Compile Time Options -----\n");
		hids = pcb_hid_enumerate();
		gds_append_str(&info, "GUI:\n");
		for (i = 0; hids[i]; i++) {
			if (hids[i]->gui) {
				gds_append_str(&info, TAB);
				gds_append_str(&info, hids[i]->name);
				gds_append_str(&info, " : ");
				gds_append_str(&info, hids[i]->description);
				gds_append_str(&info, "\n");
			}
		}

		gds_append_str(&info, "Exporters:\n");
		for (i = 0; hids[i]; i++) {
			if (hids[i]->exporter) {
				gds_append_str(&info, TAB);
				gds_append_str(&info, hids[i]->name);
				gds_append_str(&info, " : ");
				gds_append_str(&info, hids[i]->description);
				gds_append_str(&info, "\n");
			}
		}

		gds_append_str(&info, "Printers:\n");
		for (i = 0; hids[i]; i++) {
			if (hids[i]->printer) {
				gds_append_str(&info, TAB);
				gds_append_str(&info, hids[i]->name);
				gds_append_str(&info, " : ");
				gds_append_str(&info, hids[i]->description);
				gds_append_str(&info, "\n");
			}
		}
	}
#undef TAB
	return info.array;
}

/** pcb_get_infostr:
 *  Returns a string that has a bunch of information about this copy of pcb.
 *  Can be used for things like "about" dialog boxes.
 */
char *pcb_get_infostr(void)
{
	static gds_t info;
	static int first_time = 1;
	char *tmp;

	if (first_time) {
		first_time = 0;
		gds_init(&info);

		tmp = pcb_get_info_program();
		gds_append_str(&info, tmp);
		tmp = pcb_get_info_copyright();
		gds_append_str(&info, tmp);
		gds_append_str(&info, "\n\n");
		gds_append_str(&info, "pcb-rnd is licensed under the terms of the GNU\n");
		gds_append_str(&info, "General Public License version 2\n");
		gds_append_str(&info, "See the LICENSE file for more information\n\n");
		tmp = pcb_get_info_websites(NULL);
		gds_append_str(&info, tmp);

		tmp = pcb_get_info_compile_options();
		gds_append_str(&info, tmp);
	}
	return info.array;
}

const char *pcb_author(void)
{
	if (conf_core.design.fab_author && conf_core.design.fab_author[0])
		return conf_core.design.fab_author;
	else
		return pcb_get_user_name();
}

/** pcb_catch_signal:
 *  Catches signals which abort the program.
 */
void pcb_catch_signal(int Signal)
{
	const char *s;

	switch (Signal) {
#ifdef SIGHUP
	case SIGHUP:
		s = "SIGHUP";
		break;
#endif
	case SIGINT:
		s = "SIGINT";
		break;
#ifdef SIGQUIT
	case SIGQUIT:
		s = "SIGQUIT";
		break;
#endif
	case SIGABRT:
		s = "SIGABRT";
		break;
	case SIGTERM:
		s = "SIGTERM";
		break;
	case SIGSEGV:
		s = "SIGSEGV";
		break;
	default:
		s = "unknown";
		break;
	}
	pcb_message(PCB_MSG_ERROR, "aborted by %s signal\n", s);
	exit(1);
}
