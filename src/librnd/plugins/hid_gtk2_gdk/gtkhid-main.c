#include "config.h"

#include <stdlib.h>

#include <librnd/core/plugins.h>
#include <librnd/hid/hid_init.h>

#include <librnd/plugins/lib_gtk2_common/compat.h>
#include <librnd/plugins/lib_gtk_common/glue_common.h>
#include <librnd/plugins/lib_gtk_common/glue_hid.h>

const char *ghid_cookie = "gtk2 hid, gdk";

rnd_hid_t gtk2_gdk_hid;

extern void ghid_gdk_install(rnd_gtk_impl_t *impl, rnd_hid_t *hid);

int gtk2_gdk_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_gtkg_glue_common_init(ghid_cookie);
	ghid_gdk_install(&ghidgui->impl, hid);
	return rnd_gtk_parse_arguments(hid, argc, argv);
}

int pplg_check_ver_hid_gtk2_gdk(int ver_needed) { return 0; }

void pplg_uninit_hid_gtk2_gdk(void)
{
	rnd_gtkg_glue_common_uninit(ghid_cookie);
}

int pplg_init_hid_gtk2_gdk(void)
{
	RND_API_CHK_VER;

	rnd_gtk_glue_hid_init(&gtk2_gdk_hid);

	gtk2_gdk_hid.parse_arguments = gtk2_gdk_parse_arguments;

	gtk2_gdk_hid.name = "gtk2_gdk";
	gtk2_gdk_hid.description = "Gtk2 - The Gimp Toolkit, with GDK software pixmap rendering";

	rnd_hid_register_hid(&gtk2_gdk_hid);

	return 0;
}
