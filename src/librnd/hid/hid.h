#ifndef RND_HID_H
#define RND_HID_H

#include <stdarg.h>
#include <liblihata/dom.h>
#include <libfungw/fungw.h>

#include <librnd/config.h>
#include <librnd/core/error.h>
#include <librnd/core/global_typedefs.h>
#include <librnd/core/box.h>

/* attribute dialog properties */
typedef enum rnd_hat_property_e {
	RND_HATP_GLOBAL_CALLBACK,
	RND_HATP_max
} rnd_hat_property_t;


typedef enum {
	RND_HID_MOUSE_PRESS,
	RND_HID_MOUSE_RELEASE,
	RND_HID_MOUSE_MOTION,
	RND_HID_MOUSE_POPUP  /* "right click", open context-specific popup */
} rnd_hid_mouse_ev_t;

/* Human Interface Device */

/*

The way the HID layer works is that you instantiate a HID device
structure, and invoke functions through its members.  Code in the
application may *not* rely on HID internals (*anything* other than what's
defined in this file).  Code in the HID layers *may* rely on data and
functions in librnd (like, design size and such), but not on anything
in application core.

Coordinates are ALWAYS in librnd internal units rnd_coord_t. Positive X is
right, positive Y is down, unless flip is activated.  Angles are
degrees, with 0 being right (positive X) and 90 being up (negative Y) - unless
flip is activated. All zoom, scaling, panning, and conversions are hidden
inside the HIDs.

The main structure is rnd_hid_t.
*/

/* Line end cap styles.  The cap *always* extends beyond the
   coordinates given, by half the width of the line. */
typedef enum {
	rnd_cap_invalid = -1,
	rnd_cap_square,        /* square pins or pads when drawn using a line */
	rnd_cap_round          /* for normal traces, round pins */
} rnd_cap_style_t;

/* The HID may need something more than an "int" for colors, timers,
   etc.  So it passes/returns one of these, which is castable to a
   variety of things.  */
typedef union {
	long lval;
	void *ptr;
} rnd_hidval_t;

typedef struct {
	rnd_coord_t width;     /* as set by set_line_width */
	rnd_cap_style_t cap;   /* as set by set_line_cap */
	int xor;               /* as set by set_draw_xor */
	rnd_hid_t *hid;        /* the HID that owns this gc */


	/* Spare: see doc/developer/spare.txt */
	void (*spare_f1)(void), (*spare_f2)(void);
	long spare_l1, spare_l2, spare_l3, spare_l4;
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
	double spare_d1, spare_d2, spare_d3, spare_d4;
	rnd_coord_t spare_c1, spare_c2, spare_c3, spare_c4;
} rnd_core_gc_t;


#define RND_HIDCONCAT(a,b) a##b

/* File Watch flags */
/* Based upon those in dbus/dbus-connection.h */
typedef enum {
	RND_WATCH_READABLE = 1 << 0,  /* As in POLLIN */
	RND_WATCH_WRITABLE = 1 << 1,  /* As in POLLOUT */
	RND_WATCH_ERROR = 1 << 2,     /* As in POLLERR */
	RND_WATCH_HANGUP = 1 << 3     /* As in POLLHUP */
} rnd_watch_flags_t;

typedef enum rnd_composite_op_s {
	RND_HID_COMP_RESET,         /* reset (allocate and clear) the sketch canvas */
	RND_HID_COMP_POSITIVE,      /* draw subsequent objects in positive, with color, no XOR allowed */
	RND_HID_COMP_POSITIVE_XOR,  /* draw subsequent objects in positive, with color, XOR allowed */
	RND_HID_COMP_NEGATIVE,      /* draw subsequent objects in negative, ignore color */
	RND_HID_COMP_FLUSH          /* combine the sketch canvas onto the output buffer and discard the sketch canvas */
} rnd_composite_op_t;

typedef enum rnd_burst_op_s {
	RND_HID_BURST_START,
	RND_HID_BURST_END
} rnd_burst_op_t;

typedef enum rnd_hid_attr_ev_e {
	RND_HID_ATTR_EV_WINCLOSE = 2, /* window closed (window manager close) */
	RND_HID_ATTR_EV_CODECLOSE     /* closed by the code, including standard close buttons */
} rnd_hid_attr_ev_t;

typedef enum rnd_hid_fsd_flags_e { /* bitwise OR, used by file selection dialog (fsd) */
	RND_HID_FSD_READ = 1,            /* when set let the user pick existing files only ("load"); if unset, both esiting and non-existing files are accepted */
	RND_HID_FSD_MULTI = 32           /* [4.1.0] let the user select multiple files - when RND_HID_FSD_READ is also specified*/
} rnd_hid_fsd_flags_t;

typedef struct {
	const char *name;
	const char *mime;
	const char **pat; /* NULL terminated array of file name patterns */

	/* Spare: see doc/developer/spare.txt */
	void (*spare_f1)(void), (*spare_f2)(void);
	long spare_l1, spare_l2, spare_l3, spare_l4;
	void *spare_p1, *spare_p2;
	double spare_d1, spare_d2;
	rnd_coord_t spare_c1, spare_c2;
} rnd_hid_fsd_filter_t;

extern const rnd_hid_fsd_filter_t rnd_hid_fsd_filter_any[];

/* Optional fields of a menu item; all non-NULL fields are strdup'd in the HID. */
typedef struct rnd_menu_prop_s {
	const char *action;
	const char *accel;
	const char *tip;        /* tooltip */
	const char *checked;
	const char *update_on;
	const rnd_color_t *foreground;
	const rnd_color_t *background;
	const char *cookie;     /* used for cookie based removal */

	/* Spare: see doc/developer/spare.txt */
	long spare_l1, spare_l2, spare_l3, spare_l4;
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
} rnd_menu_prop_t;

typedef enum {
	RND_HID_DOCK_TOP_LEFT,       /*  hbox on the top, below the menubar */
	RND_HID_DOCK_TOP_RIGHT,      /*  hbox on the top, next to the menubar */
	RND_HID_DOCK_TOP_INFOBAR,    /*  vbox for horizontally aligned important messages, above ("on top of") the drawing area for critical warnings - may have bright background color */
	RND_HID_DOCK_LEFT,
	RND_HID_DOCK_BOTTOM,
	RND_HID_DOCK_FLOAT,          /* separate window */

	RND_HID_DOCK_max
} rnd_hid_dock_t;

extern int rnd_dock_is_vert[RND_HID_DOCK_max];    /* 1 if a new dock box (parent of a new sub-DAD) should be a vbox, 0 if hbox */
extern int rnd_dock_has_frame[RND_HID_DOCK_max];  /* 1 if a new dock box (parent of a new sub-DAD) should be framed */

typedef enum rnd_set_crosshair_e {
	RND_SC_DO_NOTHING = 0,
	RND_SC_WARP_POINTER,
	RND_SC_PAN_VIEWPORT
} rnd_set_crosshair_t;

/* This is the main HID structure.  */
struct rnd_hid_s {
	/* The size of this structure.  We use this as a compatibility
	   check; a HID built with a different hid.h than we're expecting
	   should have a different size here.  */
	int struct_size;

	/* The name of this HID.  This should be suitable for
	   command line options, multi-selection menus, file names,
	   etc. */
	const char *name;

	/* Likewise, but allowed to be longer and more descriptive.  */
	const char *description;

	/* If set, this is the GUI HID.  Exactly one of these three
	   (gui, printer, exporter) flags must be set; setting "gui" lets the
	   expose callback optimize and coordinate itself.  */
	unsigned gui:1;

	/* If set, this is the printer-class HID.  The application
	   may use this to do command-line printing, without having
	   instantiated any GUI HIDs.  Only one printer HID is normally
	   defined at a time. Exactly one of these three
	   (gui, printer, exporter) flags must be set  */
	unsigned printer:1;

	/* If set, this HID provides an export option, and should be used as
	   part of the File->Export menu option.  Examples are PNG, Gerber,
	   and EPS exporters. Exactly one of these three
	   (gui, printer, exporter) flags must be set  */
	unsigned exporter:1;

	/* Export plugin should not be listed in the export dialog; GUI plugin
	   should not be auto-selected */
	unsigned hide_from_gui:1;

	/* If set, draw the mask layer inverted. Normally the mask is a filled
	   rectangle over the design with cutouts at pins/pads. The HIDs
	   use render in normal mode, gerber renders in inverse mode. */
	unsigned mask_invert:1;

	/* Always draw layers in compositing mode - no base layer */
	unsigned force_compositing:1;

	/* When 1, HID supports markup (e.g. color) in DAD text widgets  */
	unsigned supports_dad_text_markup:1;

	/* it is possible to create DAD dialogs before the GUI_INIT event is emitted
	   (e.g. draw progress bar while loading a command line file) */
	unsigned allow_dad_before_init:1;

 /* when 1 and this hid is rnd_render, do not change rnd_render even if an export plugin or GUI is calling the render code */
	unsigned override_render:1;

	/* Called by core when the global design context changes (e.g. design changed)
	   Optional: The HID may store the design pointer so it doesn't have to call
	   rnd_multi_get_current() repeatedly. This function is called first when
	   core desing changes (switched to a different design in a multi-design app
	   or loaded a new design in a single-design app).
	   Apps should NOT call this directly, use rnd_multi_switch_to() instead. */
	void (*set_design)(rnd_hid_t *hid, rnd_design_t *design);

	/* Returns a set of resources describing options the export or print
	   HID supports.  In GUI mode, the print/export dialogs use this to
	   set up the selectable options.  In command line mode, these are
	   used to interpret command line options.  If n_ret_ is non-NULL,
	   the number of attributes is stored there. Note: the table is read-only
	   and persistent.
	   When called for starting up the GUI or initialize the export for -x,
	   dsg is NULL and appspec is NULL.
	   When called for actual exporting, dsg is not NULL and appspec is NULL or an
	   application specific context to control export details.
	   */
	const rnd_export_opt_t *(*get_export_options)(rnd_hid_t *hid, int *n_ret, rnd_design_t *dsg, void *appspec);

	/* Exports (or print) the current design (also passed as an argument so that
	   pure export plugins don't have to implement set_design). The options given
	   represent the choices made from the options returned from
	   get_export_options.  Call with options_ == NULL to start the
	   primary GUI (create a main window, print, export, etc).
	   Appspec is application specific custom config (used by app-specific
	   exporters only)  */
	void (*do_export)(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec);

	/* OPTIONAL: called to check if a given design with the given appspec
	   could be exported by the given hid. If NULL, true is assumed.
	   Not called on GUI, only on exporters and printers */
	rnd_bool (*can_export)(rnd_hid_t *hid, rnd_design_t *design, void *appspec);

	/* Export plugins: if not NULL, rnd_hid_parse_command_line() sets up opt
	   value backing memory from this array */
	rnd_hid_attr_val_t *argument_array;

	/* uninit a GUI hid */
	void (*uninit)(rnd_hid_t *hid);

	/* uninit a GUI hid */
	void (*do_exit)(rnd_hid_t *hid);

	/* Process pending events, flush output, but never block. Called from busy
	   loops from time to time. */
	void (*iterate)(rnd_hid_t *hid);

	/* Parses the command line.  Call this early for whatever HID will be
	   the primary HID, as it will set all the registered attributes.
	   The HID should remove all arguments, leaving any possible file
	   names behind. Returns 0 on success, positive for recoverable errors
	   (no change in argc or argv) and negative for unrecoverable errors.  */
	int (*parse_arguments)(rnd_hid_t *hid, int *argc, char ***argv);

	/* This may be called to ask the GUI to force a redraw of a given area */
	void (*invalidate_lr)(rnd_hid_t *hid, rnd_coord_t left, rnd_coord_t right, rnd_coord_t top, rnd_coord_t bottom);
	void (*invalidate_all)(rnd_hid_t *hid);
	void (*notify_crosshair_change)(rnd_hid_t *hid, rnd_bool changes_complete);
	void (*notify_mark_change)(rnd_hid_t *hid, rnd_bool changes_complete);

	/* During redraw or print/export cycles, this is called once per layer group
	   (physical layer); pusrpose/purpi are the extracted purpose field and its
	   keyword/function version; layer is the first layer in the group.
	   TODO: The group may be -1 until the layer rewrite is finished.
	   If it returns false (zero), the HID does not want that layer, and none of
	   the drawing functions should be called.  If it returns rnd_true (nonzero),
	   the items in that layer [group] should be drawn using the various drawing
	   functions.  In addition to the copper layer groups, you may select virtual
	   layers. The is_empty argument is a hint - if set, the layer is empty, if
	   zero it may be non-empty. Exports the current design, as set with
	   ->set_design(), but design is also passed so exporters don't need to implement
	   ->set_design(). */
	int (*set_layer_group)(rnd_hid_t *hid, rnd_design_t *design, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform);

	/* Tell the GUI the layer last selected has been finished with. */
	void (*end_layer)(rnd_hid_t *hid);

	/*** Drawing Functions. ***/

	/* Make an empty gc (graphics context, which is like a pen).  */
	rnd_hid_gc_t (*make_gc)(rnd_hid_t *hid);
	void (*destroy_gc)(rnd_hid_gc_t gc);

	/* Composite layer drawing: manipulate the sketch canvas and set
	   positive or negative drawing mode. The canvas covers the screen box. */
	void (*set_drawing_mode)(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen);

	/* Announce start/end of a render burst for a specific screen screen box;
	   A GUI hid should set the coord_per_pix value here for proper optimization. */
	void (*render_burst)(rnd_hid_t *hid, rnd_burst_op_t op, const rnd_box_t *screen);

	/*** gc vs. rnd_hid_t *: rnd_core_gc_t contains ->hid, so these calls don't
	     need to get it as first arg. ***/

	/* Sets a color. Can be one of the special colors like rnd_color_drill.
	   (Always use the drill color to draw holes and slots).
	   You may assume this is cheap enough to call inside the redraw
	   callback, but not cheap enough to call for each item drawn. */
	void (*set_color)(rnd_hid_gc_t gc, const rnd_color_t *color);

	/* Sets the line style.  While calling this is cheap, calling it with
	   different values each time may be expensive, so grouping items by
	   line style is helpful.  */
	void (*set_line_cap)(rnd_hid_gc_t gc, rnd_cap_style_t style);
	void (*set_line_width)(rnd_hid_gc_t gc, rnd_coord_t width);
	void (*set_draw_xor)(rnd_hid_gc_t gc, int xor);

	/* The usual drawing functions.  "draw" means to use segments of the
	   given width, whereas "fill" means to fill to a zero-width
	   outline.  */
	void (*draw_line)(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
	void (*draw_arc)(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t xradius, rnd_coord_t yradius, rnd_angle_t start_angle, rnd_angle_t delta_angle);
	void (*draw_rect)(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
	void (*fill_circle)(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius);
	void (*fill_polygon)(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y);
	void (*fill_polygon_offs)(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy);
	void (*fill_rect)(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);

	void (*draw_pixmap)(rnd_hid_t *hid, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t sx, rnd_coord_t sy, rnd_pixmap_t *pixmap);
	void (*uninit_pixmap)(rnd_hid_t *hid, rnd_pixmap_t *pixmap);

	/*** GUI layout functions.  Not used or defined for print/export HIDs. ***/

	/* Temporary */
	int (*shift_is_pressed)(rnd_hid_t *hid);
	int (*control_is_pressed)(rnd_hid_t *hid);
	int (*mod1_is_pressed)(rnd_hid_t *hid);

	/* Returns 0 on success, -1 on esc pressed */
	int (*get_coords)(rnd_hid_t *hid, const char *msg, rnd_coord_t *x, rnd_coord_t *y, int force);

	/* Sets the crosshair, which may differ from the pointer depending
	   on grid and pad snap.  Note that the HID is responsible for
	   hiding, showing, redrawing, etc.  The core just tells it what
	   coordinates it's actually using.  Note that this routine may
	   need to know what "librnd units" are so it can display them in mm
	   or mils accordingly.  If cursor_action_ is set, the cursor or
	   screen may be adjusted so that the cursor and the crosshair are
	   at the same point on the screen.  */
	void (*set_crosshair)(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, rnd_set_crosshair_t cursor_action);

	/* Causes func to be called at some point in the future.  Timers are
	   only good for *one* call; if you want it to repeat, add another
	   timer during the callback for the first.  user_data_ can be
	   anything, it's just passed to func.  Times are not guaranteed to
	   be accurate.  */
	rnd_hidval_t (*add_timer)(rnd_hid_t *hid, void (*func)(rnd_hidval_t user_data), unsigned long milliseconds, rnd_hidval_t user_data);
	/* Use this to stop a timer that hasn't triggered yet. */
	void (*stop_timer)(rnd_hid_t *hid, rnd_hidval_t timer);

	/* Causes func_ to be called when some condition occurs on the file
	   descriptor passed. Conditions include data for reading, writing,
	   hangup, and errors. user_data_ can be anything, it's just passed
	   to func. If the watch function returns rnd_true, the watch is kept, else
	   it is removed. */
	rnd_hidval_t (*watch_file)(rnd_hid_t *hid, int fd, unsigned int condition, rnd_bool (*func)(rnd_hidval_t watch, int fd, unsigned int condition, rnd_hidval_t user_data), rnd_hidval_t user_data);

	/* Use this to stop a file watch; must not be called from within a GUI callback! */
	void (*unwatch_file)(rnd_hid_t *hid, rnd_hidval_t watch);



	/* A generic Dynamic Attribute Dialog box (DAD). If n_attrs_ is
	   zero, attrs_(.name) must be NULL terminated. attr_dlg_run returns
	   non-zero if an error occurred (usually, this means the user cancelled
	   the dialog or something). Title is the window title, id is a human readable
	   unique ID of the dialog box.
	   The HID may choose to ignore id or it may use it for a tooltip or
	   text in a dialog box, or a help string. It is used in some events (e.g.
	   for window placement) and is strdup'd. If defx and defy are larger than 0,
	   they are hints for the default (starting) window size - can be overridden
	   by window placement. Sets opaque hid_ctx as soon as possible, in *hid_ctx_out.
	   (Hid_ctx shall save rnd_hid_t so subsequent attr_dlg_*() calls don't have
	   it as an argument) */
	void (*attr_dlg_new)(rnd_hid_t *hid, const char *id, rnd_hid_attribute_t *attrs, int n_attrs, const char *title, void *caller_data, rnd_bool modal, void (*button_cb)(void *caller_data, rnd_hid_attr_ev_t ev), int defx, int defy, int minx, int miny, void **hid_ctx_out);
	int (*attr_dlg_run)(void *hid_ctx);
	void (*attr_dlg_raise)(void *hid_ctx); /* raise the window to top */
	void (*attr_dlg_close)(void *hid_ctx); /* close the GUI but do not yet free hid_ctx (results should be available) */
	void (*attr_dlg_free)(void *hid_ctx);

	/* Set a property of an attribute dialog (typical call is between
	   attr_dlg_new() and attr_dlg_run()) */
	void (*attr_dlg_property)(void *hid_ctx, rnd_hat_property_t prop, const rnd_hid_attr_val_t *val);


	/* Disable or enable a widget of an active attribute dialog; if enabled is
	   2, the widget is put to its second enabled mode (pressed state for buttons, highlight on label) */
	int (*attr_dlg_widget_state)(void *hid_ctx, int idx, int enabled);

	/* hide or show a widget of an active attribute dialog */
	int (*attr_dlg_widget_hide)(void *hid_ctx, int idx, rnd_bool hide);

	/* Changes state of widget at idx. Returns 0 on success. Meaning of
	   arguments is widgets-specific. */
	int (*attr_dlg_widget_poke)(void *hid_ctx, int idx, int argc, fgw_arg_t argv[]);

	/* Change the current value of a widget; same as if the user chaged it,
	   except the value-changed callback is inhibited.  Val is copied (e.g. if
	   it is a string, .str is strdup'd in the call) */
	int (*attr_dlg_set_value)(void *hid_ctx, int idx, const rnd_hid_attr_val_t *val);

	/* Change the help text (tooltip) string of a widget; NULL means remove it.
	   NOTE: does _not_ change the help_text field of the attribute. */
	void (*attr_dlg_set_help)(void *hid_ctx, int idx, const char *val);

	/* top window docking: enter a new docked part by registering a
	   new subdialog or leave (remove a docked part) from a subdialog. Return 0
	   on success. */
	int (*dock_enter)(rnd_hid_t *hid, rnd_hid_dad_subdialog_t *sub, rnd_hid_dock_t where, const char *id);
	void (*dock_leave)(rnd_hid_t *hid, rnd_hid_dad_subdialog_t *sub);

	/* Removes a menu recursively */
	int (*remove_menu_node)(rnd_hid_t *hid, lht_node_t *nd);

	/* At the moment HIDs load the menu file. Some plugin code, like the toolbar
	   code needs to traverse the menu tree too. This call exposes the
	   HID-internal menu struct */
	rnd_hid_cfg_t *(*get_menu_cfg)(rnd_hid_t *hid);

	/* Update the state of all checkboxed menus whose lihata
	   node cookie matches cookie (or all checkboxed menus globally if cookie
	   is NULL) */
	void (*update_menu_checkbox)(rnd_hid_t *hid, const char *cookie);

	/* Creates a new menu or submenu from an existing (already merged) lihata node */
	int (*create_menu_by_node)(rnd_hid_t *hid, int is_popup, const char *name, int is_main, lht_node_t *parent, lht_node_t *ins_after, lht_node_t *menu_item);

	/* Pointer to the hid's configuration - useful for plugins and core wanting to install menus at anchors */
	rnd_hid_cfg_t *menu;


	/* Optional: print usage string (if accepts command line arguments)
	   Subtopic:
	     NULL    print generic help
	     string  print summary for the topic in string
	   Return 0 on success.
	*/
	int (*usage)(rnd_hid_t *hid, const char *subtopic);

	/* Optional: when non-zero, the core renderer may decide to draw cheaper
	   (simplified) approximation of some objects that would end up being too
	   small. For a GUI, this should depend on the zoom level */
	rnd_coord_t coord_per_pix;

	/* If ovr is not NULL:
	    - overwrite the command etry with ovr
	    - if cursor is not NULL, set the cursor after the overwrite
	   If ovr is NULL:
	    - if cursor is not NULL, load the value with the cursor (or -1 if not supported)
	   Return the current command entry content in a read-only string */
	const char *(*command_entry)(rnd_hid_t *hid, const char *ovr, int *cursor);

	/*** clipboard handling for GUI HIDs ***/
	/* Place string on the clipboard; return 0 on success */
	int (*clip_set)(rnd_hid_t *hid, const char *str);

	/* retrieve a string from the clipboard; return NULL when empty;
	   caller needs to free() the returned string. */
	char *(*clip_get)(rnd_hid_t *hid);

	/* run redraw-benchmark and return an FPS value (optional) */
	double (*benchmark)(rnd_hid_t *hid);

	/* (rnd_hid_cfg_keys_t *): key state */
	void *key_state;

	/*** zoom/pan calls ***/

	/* side-correct zoom to show a window of the design. If set_crosshair
	   is true, also update the crosshair to be on the center of the window */
	void (*zoom_win)(rnd_hid_t *hid, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_bool set_crosshair);

	/* Zoom relative or absolute by factor; relative means current zoom is
	   multiplied by factor */
	void (*zoom)(rnd_hid_t *hid, rnd_coord_t center_x, rnd_coord_t center_y, double factor, int relative);

	/* Pan relative/absolute by x and y; relative means x and y are added to
	   the current pan */
	void (*pan)(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int relative);

	/* Start or stop panning at x;y - stop is mode=0, start is mode=1 */
	void (*pan_mode)(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, rnd_bool mode);

	/* Load viewbox with the extents of visible pixels translated to design coords */
	void (*view_get)(rnd_hid_t *hid, rnd_box_t *viewbox);

	/*** misc GUI ***/
	/* Open the command line */
	void (*open_command)(rnd_hid_t *hid);

	/* Open a popup menu addressed by full menu path (starting with "/popups/").
	   Return 0 on success. */
	int (*open_popup)(rnd_hid_t *hid, const char *menupath);


	/* Change the mouse cursor. Cursors are registered first, either by name
	   (named cursor) or by supplying a 16*16 pixel/mask combo. The list of
	   cursors names available may depend on the HID. Indices are allocated
	   sequentially by the caller (the tool code) starting from 0. Apps
	   shouldn't call these directly. */
	void (*reg_mouse_cursor)(rnd_hid_t *hid, int idx, const char *name, const unsigned char *pixel, const unsigned char *mask);
	void (*set_mouse_cursor)(rnd_hid_t *hid, int idx);

	/* change top window title any time the after the GUI_INIT event */
	void (*set_top_title)(rnd_hid_t *hid, const char *title);

	/* this field is used by that HID implementation to store its data */
	void *hid_data;

	/* convert hid_ctx into design ptr; only valid within a DAD callback. This
	   is different from rnd_multi_get_current() because this returns the design associated
	   with the dialog, which (for multi-instance local dialogs) may be different
	   from the design what's currently show by the GUI */
	rnd_design_t *(*get_dad_design)(void *hid_ctx);

	/*** (these should be upper, but the struct has to be extended at spares
	     for binary compatibility) ***/

	/* [4.1.0] Move focus to the widget addressed by idx. Returns 0 on success. */
	int (*attr_dlg_widget_focus)(void *hid_ctx, int idx);

	/* Spare: see doc/developer/spare.txt */
	void (*spare_f2)(void), (*spare_f3)(void), (*spare_f4)(void), (*spare_f5)(void), (*spare_f6)(void);
	long spare_l1, spare_l2, spare_l3, spare_l4, spare_l5, spare_l6, spare_l7, spare_l8;
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4, *spare_p5, *spare_p6;
	double spare_d1, spare_d2, spare_d3, spare_d4, spare_d5, spare_d6;
	rnd_coord_t spare_c1, spare_c2, spare_c3, spare_c4;
};

typedef void (*rnd_hid_expose_cb_t)(rnd_hid_gc_t gc, rnd_hid_expose_ctx_t *e);

struct rnd_hid_expose_ctx_s {
	rnd_design_t *design;            /* what's being exposed */
	rnd_box_t view;                  /* which part is being exposed */
	rnd_hid_expose_cb_t expose_cb;   /* function that is called on expose to get things drawn */
	void *draw_data;                 /* user data for the expose function */

	/* Spare: see doc/developer/spare.txt */
	void (*spare_f1)(void), (*spare_f2)(void);
	long spare_l1, spare_l2, spare_l3, spare_l4;
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
	double spare_d1, spare_d2, spare_d3, spare_d4;
	rnd_coord_t coord_per_pix;       /* [4.1.0] current zoom level */
	rnd_coord_t spare_c2, spare_c3, spare_c4;
};

typedef void (*rnd_hid_expose_t)(rnd_hid_t *hid, const rnd_hid_expose_ctx_t *ctx);
typedef void (*rnd_hid_preview_expose_t)(rnd_hid_t *hid, rnd_hid_expose_ctx_t *ctx);

/* This is initially set to a "no-gui" GUI, and later reset by
   main. It is used for on-screen GUI calls, such as dialog boxes */
/*extern rnd_hid_t *rnd_gui; - declared in core/error.h */

/* This is initially set to a "no-gui" GUI, and later reset by
   main. hid_expose_callback also temporarily set it for drawing. Normally
   matches rnd_gui, but is temporarily changed while exporting. It is used
   for drawing calls, mainly by draw*.c */
extern rnd_hid_t *rnd_render;

/* This is either NULL or points to the current HID that is being called to
   do the exporting. The GUI HIDs set and unset this var.*/
extern rnd_hid_t *rnd_exporter;

/* This is either NULL or points to the current rnd_action_t that is being
   called. The action launcher sets and unsets this variable. */
extern const rnd_action_t *rnd_current_action;

/* The GUI may set this to be approximately the physical size of a pixel,
   to allow for near-misses in selection and changes in drawing items
   smaller than a screen pixel.  */
extern int rnd_pixel_slop;

/*** Glue for GUI/CLI dialogs: wrappers around actions */

/* Prompts the user to enter a string, returns the string.  If
   default_string isn't NULL, the form is pre-filled with this
   value. "msg" is printed above the query. The optional title
   is the window title.
   Returns NULL on cancel. The caller needs to free the returned string */
char *rnd_hid_prompt_for(rnd_design_t *hl, const char *msg, const char *default_string, const char *title);

/* Present a dialog box with a message and variable number of buttons. If icon
   is not NULL, attempt to draw the named icon on the left. The vararg part is
   one or more buttons, as a list of "char *label, int retval", terminated with
   NULL. */
int rnd_hid_message_box(rnd_design_t *hl, const char *icon, const char *title, const char *label, ...);

/* Show modal progressbar to the user, offering cancel long running processes.
   Pass all zeros to flush display and remove the dialog.
   Returns nonzero if the user wishes to cancel the operation.  */
int rnd_hid_progress(long so_far, long total, const char *message);

/* non-zero if DAD dialogs are available currently */
#define RND_HAVE_GUI_ATTR_DLG \
	((rnd_gui != NULL) && (rnd_gui->gui) && (rnd_gui->attr_dlg_new != NULL) && (rnd_gui->attr_dlg_new != rnd_nogui_attr_dlg_new))
void rnd_nogui_attr_dlg_new(rnd_hid_t *hid, const char *id, rnd_hid_attribute_t *attrs, int n_attrs, const char *title, void *caller_data, rnd_bool modal, void (*button_cb)(void *caller_data, rnd_hid_attr_ev_t ev), int defx, int defy, int minx, int miny, void **hid_ctx_out);


int rnd_hid_dock_enter(rnd_hid_dad_subdialog_t *sub, rnd_hid_dock_t where, const char *id);
void rnd_hid_dock_leave(rnd_hid_dad_subdialog_t *sub);

#define rnd_hid_redraw(design) rnd_gui->invalidate_all(rnd_gui)

void rnd_hid_busy(rnd_design_t *design, rnd_bool is_busy);

/* Notify the GUI that data relating to the crosshair is being changed.
 *
 * The argument passed is rnd_false to notify "changes are about to happen",
 * and rnd_true to notify "changes have finished".
 *
 * Each call with a 'rnd_false' parameter must be matched with a following one
 * with a 'rnd_true' parameter. Unmatched 'rnd_true' calls are currently not permitted,
 * but might be allowed in the future.
 *
 * GUIs should not complain if they receive extra calls with 'rnd_true' as parameter.
 * They should initiate a redraw of the crosshair attached objects - which may
 * (if necessary) mean repainting the whole screen if the GUI hasn't tracked the
 * location of existing attached drawing. */
void rnd_hid_notify_crosshair_change(rnd_design_t *hl, rnd_bool changes_complete);

/* Plugin helper: timed batch updates; any plugin may trigger the update, multiple
   triggers are batched and only a single RND_EVENT_GUI_BATCH_TIMER is emitted
   after a certain time passed since the last trigger. The event is emitted
   with the last design argument passed. If there's no GUI available, no event
   is emitted. */
void rnd_hid_gui_batch_timer(rnd_design_t *design);

	/* Run the file selection dialog. Return a string the caller needs to free().
	 * title may be used as a dialog box title.  Ignored if NULL.
	 *
	 * descr is a longer help string. Not used at the moment. Ignored if NULL.
	 *
	 * default_file is the default file name.  Ignored if NULL.
	 *
	 * default_ext is the default file extension, like ".pdf".
	 * Ignored if NULL.
	 *
	 * flt is a {NULL} terminated array of file filters; HID support is optional.
	 * Ignored if NULL. If NULL and default_ext is not NULL, the HID may make
	 * up a minimalistic filter from the default_ext also allowing *.*.
	 *
	 * history_tag may be used by the GUI to keep track of file
	 * history.  Examples would be "board", "vendor", "renumber",
	 * etc.  If NULL, no specific history is kept.
	 *
	 * flags_ are the bitwise OR
	 *
	 * sub is an optional DAD sub-dialog, can be NULL; its parent_poke commands:
	 *  close          close the dialog
	 *  get_path       returns the current full path in res as an strdup'd string (caller needs to free it)
	 *  set_file_name  replaces the file name portion of the current path from arg[0].d.s
	 */
char *rnd_hid_fileselect(rnd_hid_t *hid, const char *title, const char *descr, const char *default_file, const char *default_ext, const rnd_hid_fsd_filter_t *flt, const char *history_tag, rnd_hid_fsd_flags_t flags, rnd_hid_dad_subdialog_t *sub);


/* Actual implementation of rnd_hid_fileselect(); registered by the plugin
   that implements file selection (typically lib_hid_common). The reason
   for this abstraction is that we do not need to compile the code for
   the file select dialog into core, useful in non-GUI cases. */
extern char *(*rnd_hid_fileselect_imp)(rnd_hid_t *hid, const char *title, const char *descr, const char *default_file, const char *default_ext, const rnd_hid_fsd_filter_t *flt, const char *history_tag, rnd_hid_fsd_flags_t flags, rnd_hid_dad_subdialog_t *sub);

/* If the mouse cursor is in the drawin area, set x;y silently and return;
   else show msg and let the user click in the drawing area. If force is
   non-zero and msg is non-NULL, discard the cache and force querying a
   new coord. This mode must NOT be used unless action arguments explictly
   requested it. Returns 0 on success, -1 on esc pressed */
int rnd_hid_get_coords(const char *msg, rnd_coord_t *x, rnd_coord_t *y, int force);

/* Change normal mouse cursor to id (in rnd_gui) */
void rnd_hid_set_mouse_cursor(int id);

/* Temporarily override normal mouse cursor to id (in rnd_gui). If id is -1,
   return to the last set normal cursor. */
void rnd_hid_override_mouse_cursor(int id);

#endif
