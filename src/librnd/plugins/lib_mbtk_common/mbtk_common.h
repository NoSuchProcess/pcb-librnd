#include <libmbtk/libmbtk.h>
#include <libmbtk/display.h>
#include <librnd/core/rnd_bool.h>
#include <librnd/core/hidlib.h>

typedef struct rnd_mbtk_view_s {
	double coord_per_px;     /* Zoom level described as PCB units per screen pixel */

	rnd_coord_t x0, y0, width, height;

	unsigned inhibit_pan_common:1; /* when 1, do not call rnd_gtk_pan_common() */
	unsigned use_max_hidlib:1;     /* when 1, use hidlib->size_*; when 0, use the following two: */
	unsigned local_flip:1;   /* ignore hidlib's flip and use the local one */
	unsigned flip_x:1, flip_y:1; /* local version of flips when ->local_flip is enabled */
	rnd_coord_t max_width;
	rnd_coord_t max_height;

	int canvas_width, canvas_height;

	rnd_bool has_entered;
	rnd_bool panning;
	rnd_coord_t design_x, design_y;        /* design space coordinates of the mouse pointer */
	rnd_coord_t crosshair_x, crosshair_y;  /* design_space coordinates of the crosshair     */

	struct rnd_mbtk_s *ctx;

	unsigned local_hidlib:1; /* if 1, use local hidlib instead of current GUI hidlib (for local dialogs) */
	rnd_hidlib_t *hidlib;    /* remember the hidlib the dialog was opened for */
} rnd_mbtk_view_t;

typedef struct rnd_mbtk_topwin_s rnd_mbtk_topwin_t; /* opaque so that libmbtk/widgets.h doesn't need to be included here */

/* Global context for an active HID */
typedef struct rnd_mbtk_s {
	mbtk_display_t disp;
	rnd_bool drawing_allowed;     /* track if a drawing area is available for rendering */
	rnd_mbtk_view_t view;         /* top window's ddrawing area */
	rnd_hidlib_t *hidlib;
	unsigned hid_active:1;
	unsigned gui_is_up:1;
	rnd_mbtk_topwin_t *topwin;
} rnd_mbtk_t;
