#ifndef RND_HID_GTK_CONF_H
#define RND_HID_GTK_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_BOOLEAN listen;                          /* @usage Listen for actions on stdin. */
			RND_CFT_STRING  bg_image;                        /* @usage File name of an image to put into the background of the GUI canvas. The image is read via GdkPixbuf library. It can be any size, and will be automatically scaled to fit the canvas. */

			RND_CFT_BOOLEAN compact_horizontal;              /* OBSOLETE: ignored; use central appearance/compact instead */
			RND_CFT_BOOLEAN compact_vertical;                /* OBSOLETE: ignored; use central appearance/compact instead */
			RND_CFT_INTEGER history_size;                    /* OBSOLETE: ignored; use plugins/lib_hid_common/cli_history/slots instead */
			RND_CFT_INTEGER n_mode_button_columns;           /* OBSOLETE: always have horizontal mode buttons */

			RND_CFT_REAL icon_scale;                         /* scale icon buttons and icons by this factor (normal size = 1); need restart to apply */

			const struct {
				RND_CFT_BOOLEAN to_design;                    /* OBSOLETE: use plugins/dialogs/auto_save_window_geometry/to_design instead */
				RND_CFT_BOOLEAN to_project;                   /* OBSOLETE: use plugins/dialogs/auto_save_window_geometry/to_project instead */
				RND_CFT_BOOLEAN to_user;                      /* OBSOLETE: use plugins/dialogs/auto_save_window_geometry/to_user instead */
			} auto_save_window_geometry;

			const struct {
				RND_CFT_INTEGER top_x;                        /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER top_y;                        /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER top_width;                    /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER top_height;                   /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */

				RND_CFT_INTEGER log_x;                        /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER log_y;                        /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER log_width;                    /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER log_height;                   /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */

				RND_CFT_INTEGER drc_x;                        /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER drc_y;                        /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER drc_width;                    /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER drc_height;                   /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */

				RND_CFT_INTEGER library_x;                    /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER library_y;                    /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER library_width;                /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER library_height;               /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */

				RND_CFT_INTEGER keyref_x;                     /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER keyref_y;                     /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER keyref_width;                 /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER keyref_height;                /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */

				RND_CFT_INTEGER netlist_x;                    /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER netlist_y;                    /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER netlist_height;               /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER netlist_width;                /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */

				RND_CFT_INTEGER pinout_x;                     /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER pinout_y;                     /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER pinout_height;                /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
				RND_CFT_INTEGER pinout_width;                 /* OBSOLETE: use plugins/dialogs/window_geometry/ instead */
			} window_geometry;

			const struct {
				RND_CFT_BOOLEAN transient_modal;              /* modal dialogs are transient to the main window */
				RND_CFT_BOOLEAN transient_modeless;           /* modeless dialogs are transient to the main window */
				RND_CFT_BOOLEAN auto_present;                 /* present (pop up to the top) new dialogs automatically */
			} dialog;

			const struct {
				RND_CFT_STRING  fg_red;                       /* foreground color to be used with <red> text in markup */
				RND_CFT_STRING  fg_green;                     /* foreground color to be used with <green> text in markup */
				RND_CFT_STRING  fg_blue;                      /* foreground color to be used with <blue> text in markup */
			} markup;
		} hid_gtk;
	} plugins;
} conf_hid_gtk_t;

extern conf_hid_gtk_t rnd_gtk_conf_hid;

#endif
