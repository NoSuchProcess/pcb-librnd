#include <gtk/gtk.h>
#include "hid.h"

void ghid_config_window_show(void *gport);

void ghid_config_handle_units_changed(void *gport);

/* temporary back reference to hid_gtk */
void ghid_pack_mode_buttons(void);
int ghid_command_entry_is_active(void);
void ghid_init_drawing_widget(GtkWidget * widget, void *gport);
void ghid_command_use_command_window_sync(void);
gboolean ghid_preview_expose(GtkWidget * widget, GdkEventExpose * ev, pcb_hid_expose_t expcall, const pcb_hid_expose_ctx_t *ctx);
