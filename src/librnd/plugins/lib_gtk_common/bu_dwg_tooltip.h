#include <gtk/gtk.h>
gboolean rnd_gtk_dwg_tooltip_check_object(rnd_design_t *hl, GtkWidget *drawing_area, rnd_coord_t crosshairx, rnd_coord_t crosshairy);
void rnd_gtk_dwg_tooltip_cancel_update(void);
void rnd_gtk_dwg_tooltip_queue(GtkWidget *drawing_area, GSourceFunc cb, void *ctx);

