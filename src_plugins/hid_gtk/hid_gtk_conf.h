#ifndef PCB_HID_GTK_CONF_H
#define PCB_HID_GTK_CONF_H

#include "conf.h"

typedef struct {
	const struct plugins {
		const struct hid_gtk {
			CFT_BOOLEAN listen;                          /* @usage Listen for actions on stdin. */
			CFT_STRING  bg_image;                        /* @usage File name of an image to put into the background of the GUI canvas. The image must be a color PPM image, in binary (not ASCII) format. It can be any size, and will be automatically scaled to fit the canvas. */

			CFT_BOOLEAN compact_horizontal;
			CFT_BOOLEAN compact_vertical;
			CFT_BOOLEAN use_command_window;
			CFT_INTEGER history_size;
			CFT_INTEGER n_mode_button_columns;

			CFT_BOOLEAN auto_save_window_geometry;       /* true=save window geometry so they are preserved after a restart of pcb */
			CFT_BOOLEAN save_window_geometry_in_design;  /* true=save window geometry per design; false=save it per user */
			const struct window_geometry {
				CFT_INTEGER top_width;
				CFT_INTEGER top_height;
				CFT_INTEGER log_width;
				CFT_INTEGER log_height;
				CFT_INTEGER drc_width;
				CFT_INTEGER drc_height;
				CFT_INTEGER library_width;
				CFT_INTEGER library_height;
				CFT_INTEGER netlist_height;
				CFT_INTEGER keyref_width;
				CFT_INTEGER keyref_height;
			} window_geometry;
		} hid_gtk;
	} plugins;
} conf_hid_gtk_t;

/* Call macro op(field_name, ...) for each window geometry field */
#define GHID_WGEO_ALL(op, ...) \
do { \
	op(top_width, __VA_ARGS__); \
	op(top_height, __VA_ARGS__); \
	op(log_width, __VA_ARGS__); \
	op(log_height, __VA_ARGS__); \
	op(drc_width, __VA_ARGS__); \
	op(drc_height, __VA_ARGS__); \
	op(library_width, __VA_ARGS__); \
	op(library_height, __VA_ARGS__); \
	op(netlist_height, __VA_ARGS__); \
	op(keyref_width, __VA_ARGS__); \
	op(keyref_height, __VA_ARGS__); \
} while(0)

typedef struct window_geometry window_geometry_t;
extern window_geometry_t hid_gtk_wgeo;
extern conf_hid_gtk_t conf_hid_gtk;

#endif
