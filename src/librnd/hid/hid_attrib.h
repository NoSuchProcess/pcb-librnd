#ifndef RND_HID_ATTRIB_H
#define RND_HID_ATTRIB_H

#include <genlist/gendlist.h>
#include <librnd/hid/hid.h>
#include <librnd/core/color.h>

/* Used for HID attributes (exporting and printing, mostly).
   HA_boolean uses lng, HA_enum sets lng to the index and
   str to the enumeration string.  RND_HATT_LABEL just shows the
   default str. */
struct rnd_hid_attr_val_s {
	long lng;
	const char *str;
	double dbl;
	rnd_coord_t crd;
	rnd_color_t clr;
	void (*func)();
};

typedef enum rnd_hatt_compflags_e {
	RND_HATF_FRAME       = 1,  /* box/table has a visible frame around it */
	RND_HATF_TIGHT       = 2,  /* box/table/button has minimal padding and spacing */
	RND_HATF_SCROLL      = 4,  /* box/table is scrollable */
	RND_HATF_HIDE_TABLAB = 8,  /* hide tab labes of a TABBED - the tab mechanism works, but tab names are not displayed and are not clickable */
	RND_HATF_CLR_STATIC  = 8,  /* color that can not be changed */
	RND_HATF_LEFT_TAB    = 16, /* display tab labels of TABBED on the left instead of on top (default) */
	RND_HATF_TREE_COL    = 32, /* first column of a RND_HATT_TREE is a tree */
	RND_HATF_EXPFILL     = 64, /* for hbox and vbox: expand and fill */
	RND_HATF_HIDE        = 128,/* widget is initially hidden */
	RND_HATF_TOGGLE      = 256,/* for buttons and picbuttons: use a toggle button instead of a push button */
	RND_HATF_TEXT_TRUNCATED = 512, /* label: do not set widget size for text, truncate text if widget is smaller */
	RND_HATF_TEXT_VERTICAL  = 1024,/* label: rotate text 90 degrees so it can be read from the right */
	RND_HATF_PRV_DESIGN   = 2048,  /* indicates that a preview widget is showing a section of the design so it needs to be redrawn when the (top window) design is redrawn */
	RND_HATF_WIDTH_CHR   = 4096,   /* ->geo_width is specified in charactes */
	RND_HATF_HEIGHT_CHR  = 8192,   /* ->geo_width is specified in charactes */
	RND_HATF_INIT_FOCUS  = 16384,  /* this widget has (keyboard) focus on widget creation */
	RND_HATF_PRV_GFLIP   = 32768,  /* global design flip determines preview flip */
	RND_HATF_PRV_LFLIP   = 65536,  /* local flip determines preview flip; when set RND_HATF_PRV_GFLIP is ignored */
	RND_HATF_PRG_SMALL   = 131072, /* allow progress bar to be as small as possible */
	RND_HATF_TREE_NO_AUTOEXP = 262144,/* do not auto-expand or auto-colleps on tree row activation */
	RND_HATF_TREE_MULTI  = 524288  /* [4.1.0] allow the tree widget to select multiple rows */
} rnd_hatt_compflags_t;

typedef enum rnd_hid_attr_type_e {
	/* atomic entry types */
	RND_HATT_LABEL,
	RND_HATT_INTEGER,
	RND_HATT_REAL,
	RND_HATT_STRING,              /* WARNING: string, must be malloc()'d, can't be a (const char *) */
	RND_HATT_BOOL,
	RND_HATT_ENUM,
	RND_HATT_UNIT,
	RND_HATT_COORD,
	RND_HATT_BUTTON,              /* push button; default value is the label */
	RND_HATT_TREE,                /* tree/list/table view; number of columns: rnd_hatt_table_cols; data is in field 'wdata' */
	RND_HATT_PROGRESS,            /* progress bar; displays dbl between 0 and 1 */
	RND_HATT_PREVIEW,             /* preview/render widget; callbacks in 'wdata' */
	RND_HATT_PICTURE,             /* static picture from xpm - picture data in wdata */
	RND_HATT_PICBUTTON,           /* button with static picture from xpm - picture data in wdata */
	RND_HATT_COLOR,               /* color pick (user input: select a color) */
	RND_HATT_TEXT,                /* plain text editor; data is in 'wdata' as rnd_hid_text_t */
	RND_HATT_SUBDIALOG,           /* caller operated subdialog (docked dialog); available from librnd 3.1.0 */

	/* groups (e.g. boxes) */
	RND_HATT_BEGIN_HBOX = 100,    /* NOTE: RND_HATT_IS_COMPOSITE() depends on it */
	RND_HATT_BEGIN_VBOX,
	RND_HATT_BEGIN_HPANE,         /* horizontal split and offer two vboxes; the split ratio is dbl between 0 and 1, that describes the left side's size */
	RND_HATT_BEGIN_VPANE,         /* vertical split and offer two vboxes; the split ratio is dbl between 0 and 1, that describes the left side's size */
	RND_HATT_BEGIN_TABLE,         /* wdata_aux1 is the number of columns */
	RND_HATT_BEGIN_TABBED,        /* tabbed view (e.g. notebook); ->wdata stores the tab names and a NULL; default_val's integer value is the index of the current tab */
	RND_HATT_BEGIN_COMPOUND,      /* subtree emulating a single widget; (rnd_hid_compound_t *) stored in END's wdata */

	RND_HATT_END = 200            /* close one level of RND_HATT_* */
} rnd_hid_attr_type_t;

#define RND_HATT_IS_COMPOSITE(type) \
	(((type) >= RND_HATT_BEGIN_HBOX) && ((type) < RND_HATT_END))

#define RND_HAT_IS_STR(type) (type == RND_HATT_STRING)

/* alternative field names in struct rnd_hid_attribute_s */
#define rnd_hatt_flags       hatt_flags
#define rnd_hatt_table_cols  wdata_aux1

struct rnd_hid_attribute_s {
	const char *name;
	const char *help_text;
	rnd_hid_attr_type_t type;
	double min_val, max_val; /* for integer and real */
	rnd_hid_attr_val_t val; /* Also actual value for global attributes. */

	/* Already set by the caller when creating the widget.
	   RND_HATT_ENUM: const char ** (NULL terminated list of values)
	   RND_HATT_PICTURE & RND_HATT_PICBUTTON: const char **xpm
	   RND_HATT_TREE and others: (rnd_hid_*_t *)
	   RND_HATT_PREVIEW: 
	   */
	void *wdata; /* main widget data */
	int wdata_aux1; /* auixiliary widget data - should phase out long term */

	/* dynamic API */
	unsigned changed:1; /* 0 for initial values, 1 on user change */
	unsigned empty:1;   /* set to 1 by the widget implementation if the textual value is empty, where applicable */
	void (*change_cb)(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr); /* called upon value change by the user */
	void (*right_cb)(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);  /* called upon right click by the user */
	void (*enter_cb)(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);  /* called upon the user pressed enter in a widget that handles keys */
	void *user_data; /* ignored; the caller is free to use it */
	rnd_hatt_compflags_t hatt_flags;

	/* geometry */
	int geo_width; /* when RND_HATF_WIDTH_CHR is set, width of the widget in characters, on creation-time */
	int geo_height;

	/* Spare: see doc/developer/spare.txt */
	void (*spare_f1)(void), (*spare_f2)(void);
	long spare_l1, spare_l2, spare_l3, spare_l4;
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
	double spare_d1, spare_d2;
	rnd_coord_t spare_c1, spare_c2;
};

struct rnd_export_opt_s {
	const char *name;
	const char *help_text;
	rnd_hid_attr_type_t type;
	double min_val, max_val;        /* for integer and real */
	rnd_hid_attr_val_t default_val;
	const char **enumerations; /* NULL terminated list of values for a RND_HATT_ENUM */


	/* This used to be an alternative value target but as a void * it did
	   no work on all platforms - it was not clear what different HATT types
	   should write, especially for an enum: int *, long *...  */
	/* void *value; */
};

/* Load export plugin's read-write hid->attribute_array with defaults from
   read-only opts; do this once, at init. */
void rnd_hid_load_defaults(rnd_hid_t *hid, const rnd_export_opt_t *opts, int len);

/* The selected HID or export plugin (and potentially
   dependent exporter plugins) register their options and command line
   options will be looked up in these. Plugins not participating in the
   current session won't register and the registration is lost immediately
   after the export because the application exits. Cam or dialog box direct
   exporting won't go through this. */
void rnd_export_register_opts2(rnd_hid_t *hid, const rnd_export_opt_t *a, int n, const char *cookie, int copy);

/* Remove all attributes registered with the given cookie */
void rnd_export_remove_opts_by_cookie(const char *cookie);

/* remove all attributes and free the list */
void rnd_export_uninit(void);

typedef struct rnd_hid_attr_node_s {
	struct rnd_hid_attr_node_s *next;
	rnd_hid_t *hid;
	const rnd_export_opt_t *opts;
	int n;
	const char *cookie;
} rnd_hid_attr_node_t;

extern rnd_hid_attr_node_t *rnd_hid_attr_nodes;

void rnd_hid_usage(const rnd_export_opt_t *a, int numa);
void rnd_hid_usage_option(const char *name, const char *help);

/* Count the number of direct children, start_from the first children */
int rnd_hid_attrdlg_num_children(rnd_hid_attribute_t *attrs, int start_from, int n_attrs);

/* Invoke a simple modal attribute dialog if GUI is available */
int rnd_attribute_dialog_(const char *id, rnd_hid_attribute_t *attrs, int n_attrs, const char *title, void *caller_data, void **retovr, int defx, int defy, int minx, int miny, void **hid_ctx_out);
int rnd_attribute_dialog(const char *id, rnd_hid_attribute_t *attrs, int n_attrs, const char *title, void *caller_data);


/* Convert between compflag bit value and name */
const char *rnd_hid_compflag_bit2name(rnd_hatt_compflags_t bit);
rnd_hatt_compflags_t rnd_hid_compflag_name2bit(const char *name);

/*** When an rnd_export_opt_t item is a box, the following function is called
     from its ->func ***/

typedef enum rnd_hid_export_opt_func_action_e {
	RND_HIDEOF_USAGE, /* call_ctx is a FILE * */
	RND_HIDEOF_DAD    /* call_ctx is a rnd_hid_export_opt_func_dad_t */
} rnd_hid_export_opt_func_action_t;

typedef void (*rnd_hid_export_opt_func_t)(rnd_hid_export_opt_func_action_t act, void *call_ctx, const rnd_export_opt_t *opt, rnd_hid_attr_val_t *val);

#endif
