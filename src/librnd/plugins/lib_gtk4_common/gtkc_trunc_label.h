#include <gtk/gtk.h>

/* Alternative, simplified label implementation:
    - supports truncated text (default) - widget can be narrower than text length and is clipped without ellipsis
    - supports 90 degree rotated vertical text (both truncated and normal)
    - supports non-truncated text
   Does not support:
    - wrap
    - baseline
    - css
*/

G_DECLARE_FINAL_TYPE(GtkcTrucLabel, gtkc_trunc_label, TRUNC_LABEL, WIDGET, GtkWidget)

#define GTKC_TYPE_TRUNC_LABEL     (gtkc_trunc_label_get_type ())
#define GTKC_TRUNC_LABEL(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTKC_TYPE_TRUNC_LABEL, GtkcTruncLabel))
#define GTKC_IS_TRUNC_LABEL(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTKC_TYPE_TRUNC_LABEL))

typedef struct GtkcTruncLabel_s GtkcTruncLabel;

GtkWidget *gtkc_trunc_label_new(const char *text);

/* When enabled, rotate text 90 degrees CCW (off by default) */
void gtkc_trunc_set_rotated(GtkcTruncLabel *tl, gboolean rotated);

/* When enabled, do not truncate text, widget minimum size requires
   text to fit (off by default). This is a workaround for GtkLabel+css
   rotation layout bug */
void gtkc_trunc_set_notruncate(GtkcTruncLabel *self, gboolean notruncate);
