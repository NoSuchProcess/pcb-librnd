#ifndef PCB_HID_ATTRIB_H
#define PCB_HID_ATTRIB_H

#include "hid.h"

/* Used for HID attributes (exporting and printing, mostly).
   HA_boolean uses int_value, HA_enum sets int_value to the index and
   str_value to the enumeration string.  HID_Label just shows the
   default str_value.  HID_Mixed is a real_value followed by an enum,
   like 0.5in or 100mm. */
typedef struct HID_Attr_Val_s {
	int int_value;
	const char *str_value;
	double real_value;
	Coord coord_value;
} HID_Attr_Val;

enum hids { HID_Label, HID_Integer, HID_Real, HID_String,
	HID_Boolean, HID_Enum, HID_Mixed, HID_Path,
	HID_Unit, HID_Coord
};

typedef struct HID_Attribute_s {
	char *name;
	/* If the help_text is this, usage() won't show this option */
#define ATTR_UNDOCUMENTED ((char *)(1))
	char *help_text;
	enum hids type;
	int min_val, max_val;				/* for integer and real */
	HID_Attr_Val default_val;		/* Also actual value for global attributes.  */
	const char **enumerations;
	/* If set, this is used for global attributes (i.e. those set
	   statically with REGISTER_ATTRIBUTES below) instead of changing
	   the default_val.  Note that a HID_Mixed attribute must specify a
	   pointer to HID_Attr_Val here, and HID_Boolean assumes this is
	   "char *" so the value should be initialized to zero, and may be
	   set to non-zero (not always one).  */
	void *value;
	int hash;										/* for detecting changes. */
} HID_Attribute;

extern void hid_register_attributes(HID_Attribute *, int, const char *cookie, int copy);
#define REGISTER_ATTRIBUTES(a, cookie) HIDCONCAT(void register_,a) ()\
{ hid_register_attributes(a, sizeof(a)/sizeof(a[0]), cookie, 0); }

/* remove all attributes and free the list */
void hid_attributes_uninit(void);

typedef struct HID_AttrNode {
	struct HID_AttrNode *next;
	HID_Attribute *attributes;
	int n;
	const char *cookie;
} HID_AttrNode;

extern HID_AttrNode *hid_attr_nodes;

void hid_save_settings(int);
void hid_load_settings(void);

#endif
