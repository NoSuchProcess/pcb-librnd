#include <gtk/gtk.h>

/* Alternative, simplified, visual-only scrollbar implementation */

G_DECLARE_FINAL_TYPE(GtkcScrollbar, gtkc_scrollbar, SCROLLBAR, WIDGET, GtkWidget)

#define GTKC_TYPE_SCROLLBAR     (gtkc_scrollbar_get_type ())
#define GTKC_SCROLLBAR(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTKC_TYPE_SCROLLBAR, GtkcScrollbar))
#define GTKC_IS_SCROLLBAR(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTKC_TYPE_SCROLLBAR))

typedef struct _GtkcScrollbar GtkcScrollbar;

GtkWidget *gtkc_scrollbar_new(GtkOrientation orientation);

void gtkc_scrollbar_set_range(GtkcScrollbar *scb, double min, double max, double win);
void gtkc_scrollbar_set_val(GtkcScrollbar *scb, double val);
void gtkc_scrollbar_set_val_normal(GtkcScrollbar *scb, double val01); /* clamps to 0..1-nwin */
double gtkc_scrollbar_get_val(GtkcScrollbar *scb);
double gtkc_scrollbar_get_val_normal(GtkcScrollbar *scb);
void gtkc_scrollbar_get_range(GtkcScrollbar *scb, double *min, double *max, double *win);


