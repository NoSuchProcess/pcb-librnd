#ifndef RND_GTK_DLG_COMMAND_H
#define RND_GTK_DLG_COMMAND_H

#include <gtk/gtk.h>
#include <glib.h>
#include <librnd/core/rnd_bool.h>
#include <librnd/core/global_typedefs.h>

typedef struct rnd_gtk_command_s {
	GtkWidget *command_combo_box, *prompt_label;
	GtkEntry *command_entry;
	gboolean command_entry_status_line_active;

	void (*post_entry)(void);
	void (*pre_entry)(void);

	/* internal */
	GMainLoop *rnd_gtk_entry_loop;
	gchar *command_entered;
	void (*hide_status)(void*,int); /* called with status_ctx when the status line needs to be hidden */
	void *status_ctx;
} rnd_gtk_command_t;

void rnd_gtk_handle_user_command(rnd_hidlib_t *hl, rnd_gtk_command_t *ctx, rnd_bool raise);
char *rnd_gtk_command_entry_get(rnd_gtk_command_t *ctx, const char *prompt, const char *command);

/* Update the prompt text before the command entry - call it when any of conf_core.rc.cli_* change */
void rnd_gtk_command_update_prompt(rnd_gtk_command_t *ctx);

const char *rnd_gtk_cmd_command_entry(rnd_gtk_command_t *ctx, const char *ovr, int *cursor);

/* cancel editing and quit the main loop - NOP if not active*/
void rnd_gtk_cmd_close(rnd_gtk_command_t *ctx);

void rnd_gtk_command_combo_box_entry_create(rnd_gtk_command_t *ctx, void (*hide_status)(void*,int), void *status_ctx);

#endif
