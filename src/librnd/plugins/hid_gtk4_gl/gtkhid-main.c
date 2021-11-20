#include "config.h"

#include <stdlib.h>

#include <librnd/core/plugins.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/hidlib_conf.h>

#include <librnd/plugins/lib_hid_gl/draw_gl.h>

#include <librnd/plugins/lib_gtk4_common/compat.h>
#include <librnd/plugins/lib_gtk_common/glue_common.h>
#include <librnd/plugins/lib_gtk_common/glue_hid.h>

const char *ghid_gl_cookie = "gtk4 hid, gl";

rnd_hid_t gtk4_gl_hid;

extern void ghid_gl_install(rnd_gtk_impl_t *impl, rnd_hid_t *hid);

void ghid_gl_install(rnd_gtk_impl_t *impl, rnd_hid_t *hid)
{
TODO("remove this and link gtkhid-gl instead");
fprintf(stderr, "No GL rendering for gtk4 yet\n");
}

int gtk4_gl_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_gtkg_glue_common_init(ghid_gl_cookie);
	ghid_gl_install(&ghidgui->impl, hid);
	return rnd_gtk_parse_arguments(hid, argc, argv);
}

int pplg_check_ver_hid_gtk4_gl(int ver_needed) { return 0; }

void pplg_uninit_hid_gtk4_gl(void)
{
	rnd_event_unbind_allcookie(ghid_gl_cookie);
	rnd_conf_hid_unreg(ghid_gl_cookie);
	drawgl_uninit();
}


int pplg_init_hid_gtk4_gl(void)
{
	RND_API_CHK_VER;

	rnd_gtk_glue_hid_init(&gtk4_gl_hid);

	gtk4_gl_hid.parse_arguments = gtk4_gl_parse_arguments;

	gtk4_gl_hid.name = "gtk4_gl";
	gtk4_gl_hid.description = "Gtk4 - The Gimp Toolkit, with opengl rendering";

	rnd_hid_register_hid(&gtk4_gl_hid);

	return 0;
}
