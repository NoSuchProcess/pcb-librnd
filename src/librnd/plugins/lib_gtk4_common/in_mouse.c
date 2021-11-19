#include "compat.h"

#define GDKC_HAND2       "pointer"
#define GDKC_WATCH       "progress"
#define GDKC_DRAPED_BOX  "all-scroll"

typedef struct {
	const char *name;
	rnd_gtkc_cursor_type_t shape;
} named_cursor_t;

static const named_cursor_t named_cursors[] = {
	{"question_arrow", "help"},
	{"left_ptr",       "default"},
	{"hand",           "grabbing"},
	{"crosshair",      "crosshair"},
	{"dotbox",         "cell"},
	{"pencil",         "context-menu"},
	{"up_arrow",       "n-resize"},
	{"ul_angle",       "nw-resize"},
	{"pirate",         "not-allowed"},
	{"xterm",          "text"},
	{"iron_cross",     "move"},
	{NULL, 0}
};

static inline GdkCursor *gdkc_cursor_new(void *ctx, const char *name)
{
	return gdk_cursor_new_from_name(name, NULL);
}

#include <librnd/plugins/lib_gtk_common/in_mouse.c>
