#ifndef RND_LTF_PREVIEW_H
#define RND_LTF_PREVIEW_H
#include <genlist/gendlist.h>

typedef struct rnd_ltf_preview_s {
	void *hid_ctx;
	rnd_hid_attribute_t *attr;
	rnd_hid_expose_ctx_t exp_ctx;

	Window window;
	Widget pw;

	rnd_coord_t x, y;            /* PCB coordinates of upper right corner of window */
	rnd_coord_t x1, y1, x2, y2;  /* PCB extends of desired drawing area  */
	double zoom;                 /* PCB units per screen pixel */
	int v_width, v_height;       /* widget dimensions in pixels */

	rnd_hid_expose_ctx_t ctx;
	rnd_bool (*mouse_ev)(void *widget, void *draw_data, rnd_hid_mouse_ev_t kind, rnd_coord_t x, rnd_coord_t y);
	void (*pre_close)(struct rnd_ltf_preview_s *pd);
	rnd_hid_expose_t overlay_draw;
	unsigned resized:1;
	unsigned pan:1;
	unsigned expose_lock:1;
	unsigned redraw_with_design:1;
	unsigned flip_local:1;
	unsigned flip_global:1;
	unsigned flip_x:1; /* local flip, when flip_local is enabled */
	unsigned flip_y:1; /* local flip, when flip_local is enabled */
	int pan_ox, pan_oy;
	rnd_coord_t pan_opx, pan_opy;
	gdl_elem_t link; /* in the list of all previews in ltf_previews */
} rnd_ltf_preview_t;

void rnd_ltf_preview_callback(Widget da, rnd_ltf_preview_t *pd, XmDrawingAreaCallbackStruct *cbs);

/* Query current widget sizes and use pd->[xy]* to recalculate and apply the
   zoom level before a redraw */
void rnd_ltf_preview_zoom_update(rnd_ltf_preview_t *pd);

/* Double buffered redraw using the current zoom, calling core function for
   generic preview */
void rnd_ltf_preview_redraw(rnd_ltf_preview_t *pd);

/* Convert widget pixel px;py back to preview render coordinates */
void rnd_ltf_preview_getxy(rnd_ltf_preview_t *pd, int px, int py, rnd_coord_t *dst_x, rnd_coord_t *dst_y);

/* invalidate (redraw) all preview widgets whose current view overlaps with
   the screen box; if screen is NULL, redraw all */
void rnd_ltf_preview_invalidate(const rnd_box_t *screen);


void rnd_ltf_preview_add(rnd_ltf_preview_t *prv);
void rnd_ltf_preview_del(rnd_ltf_preview_t *prv);
#endif
