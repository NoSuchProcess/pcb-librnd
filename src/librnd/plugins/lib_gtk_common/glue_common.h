#include <librnd/core/hid.h>
#include "rnd_gtk.h"

#include <librnd/plugins/lib_gtk_common/dlg_topwin.h>


void rnd_gtkg_glue_common_init(const char *cookie);
void rnd_gtkg_glue_common_uninit(const char *cookie);

void rnd_gtkg_draw_area_update(rnd_gtk_port_t *out, GdkRectangle *rect);

/* make sure the context is set to draw the whole widget size, which might
   be slightly larger than the original request */
#define RND_GTK_PREVIEW_TUNE_EXTENT(ctx, allocation) \
do { \
	rnd_coord_t nx1, ny1, nx2, ny2; \
	nx1 = Px(0); nx2 = Px(allocation.width); \
	ny1 = Py(0); ny2 = Py(allocation.height); \
	if (nx1 < nx2) { \
		ctx->view.X1 = nx1; \
		ctx->view.X2 = nx2; \
	} \
	else { \
		ctx->view.X1 = nx2; \
		ctx->view.X2 = nx1; \
	} \
	if (ny1 < ny2) { \
		ctx->view.Y1 = ny1; \
		ctx->view.Y2 = ny2; \
	} \
	else { \
		ctx->view.Y1 = ny2; \
		ctx->view.Y2 = ny1; \
	} \
} while(0)

/* Redraw all previews intersecting the specified screenbox (in case of lr) */
void rnd_gtk_previews_invalidate_lr(rnd_coord_t left, rnd_coord_t right, rnd_coord_t top, rnd_coord_t bottom);
void rnd_gtk_previews_invalidate_all(void);

/*** Internal calls, hid implementations won't need these ***/
void rnd_gtk_tw_ranges_scale(rnd_gtk_t *ctx);
void rnd_gtk_note_event_location(gint event_x, gint event_y, int valid);

void rnd_gtk_interface_input_signals_connect(void);
void rnd_gtk_interface_input_signals_disconnect(void);
void rnd_gtk_interface_set_sensitive(gboolean sensitive);
void rnd_gtk_port_ranges_changed(void);
void rnd_gtk_mode_cursor_main(void);
void rnd_gtk_pan_common(void);


void rnd_gtkg_init_pixmap_low(rnd_gtk_pixmap_t *gpm);
void rnd_gtkg_uninit_pixmap_low(rnd_gtk_pixmap_t *gpm);


/* extend a 8 bit color component to 16 bits and guarantee round trip */
RND_INLINE unsigned rnd_gtk_color8to16(unsigned c)
{
	unsigned res = c << 8;
	if (c > 0x7F)
		res |= 0x00FF;
	return res;
}
