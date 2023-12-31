#include "compat.h"

#ifdef GDK_WINDOWING_X11
#	include <gdk/x11/gdkx.h>
#endif
#ifdef GDK_WINDOWING_WAYLAND
#	include <gdk/wayland/gdkwayland.h>
#endif

static inline void gtkc_display_get_pointer(GdkDisplay *gdisplay, int *pointer_x, int *pointer_y)
{
#ifdef GDK_WINDOWING_X11
	if (GDK_IS_X11_DISPLAY(gdisplay)) {
		Display *display = GDK_DISPLAY_XDISPLAY(gdisplay);
		Window tmp, rootwin = gdk_x11_display_get_xrootwindow(gdisplay);
		int res, itmp;
		unsigned int utmp;

		if (gtkc_resolve_X()) return;

		res = gtkc_XQueryPointer(display, rootwin, &tmp, &tmp, pointer_x, pointer_y, &itmp, &itmp, &utmp);
		if (!res)
			*pointer_x = *pointer_y = 0;
	}
#endif
}

static inline void gtkc_display_warp_pointer(GdkDisplay *gdisplay, int pointer_x, int pointer_y)
{
#ifdef GDK_WINDOWING_X11
	if (GDK_IS_X11_DISPLAY(gdisplay)) {
		Display *display = GDK_DISPLAY_XDISPLAY(gdisplay);
		Window tmp, rootwin = gdk_x11_display_get_xrootwindow(gdisplay);
		int res, itmp;
		unsigned int utmp;

		if (gtkc_resolve_X()) return;

		res = gtkc_XQueryPointer(display, rootwin, &tmp, &tmp, &itmp, &itmp, &itmp, &itmp, &utmp);
		if (res) {
			gtkc_XWarpPointer(display, 0, rootwin, 0, 0, 0, 0, pointer_x, pointer_y);
			return;
		}
	}
#endif
}


#include <librnd/plugins/lib_gtk_common/ui_crosshair.c>
