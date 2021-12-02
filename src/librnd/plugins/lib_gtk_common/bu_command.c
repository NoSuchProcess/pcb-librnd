/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* This file was originally written by Bill Wilson for the PCB Gtk port and
   got heavily reworked for pcb-rnd */

/* provides an interface for getting user input for executing a command. */


#include <librnd/rnd_config.h>
#include "rnd_gtk.h"
#include "bu_command.h"
#include <librnd/core/hidlib_conf.h>

#include <gdk/gdkkeysyms.h>

#include <librnd/core/actions.h>

#include "hid_gtk_conf.h"
#include <librnd/plugins/lib_hid_common/cli_history.h>

/* Put an allocated string on the history list and combo text list
   if it is not a duplicate.  The combo box is just a shadow of the
   real, common history, shared with other HIDs. The combo box strings
   take "const gchar *", which are allocated within the common history. */

static void rnd_gtk_chist_append(void *ctx_, const char *cmd)
{
	rnd_gtk_command_t *ctx = (rnd_gtk_command_t *)ctx_;
	gtkc_combo_box_text_append_text(ctx->command_combo_box, cmd);
}

static void rnd_gtk_chist_remove(void *ctx_, int idx)
{
	rnd_gtk_command_t *ctx = (rnd_gtk_command_t *)ctx_;
	gtkc_combo_box_text_remove(ctx->command_combo_box, idx);
}

static void command_history_add(rnd_gtk_command_t *ctx, gchar *cmd)
{
	rnd_clihist_append(cmd, ctx, rnd_gtk_chist_append, rnd_gtk_chist_remove);
}


/* Called when user hits "Enter" key in command entry.  The action to take
   depends on where the combo box is.  If it's in the command window, we can
   immediately execute the command and carry on.  If it's in the status
   line hbox, then we need stop the command entry g_main_loop from running
   and save the allocated string so it can be returned from
   rnd_gtk_command_entry_get() */
static void command_entry_activate_cb(GtkWidget *widget, gpointer data)
{
	rnd_gtk_command_t *ctx = data;
	gchar *command;
	const gchar *cmd = gtkc_entry_get_text(GTK_ENTRY(ctx->command_entry));

	if (cmd != NULL) {
		while ((*cmd == ' ') || (*cmd == '\t'))
			cmd++;
		command = g_strdup(cmd);
	}
	gtkc_entry_set_text(ctx->command_entry, "");

	if (*command)
		command_history_add(ctx, command);

	if (ctx->rnd_gtk_entry_loop && g_main_loop_is_running(ctx->rnd_gtk_entry_loop)) /* should always be */
		g_main_loop_quit(ctx->rnd_gtk_entry_loop);
	ctx->command_entered = command; /* Caller will free it */
}

static rnd_bool command_key_press_cb(GtkWidget *widget, long mods, long key_raw, long kv, void *udata)
{
	rnd_gtk_command_t *ctx = udata;

	if (kv == RND_GTK_KEY(Tab)) {
		rnd_cli_tab(ghidgui->hidlib);
		return TRUE;
	}

	/* escape key handling */
	if (kv == RND_GTK_KEY(Escape)) {
		rnd_gtk_cmd_close(ctx);
		return TRUE;
	}

	return FALSE;
}

static rnd_bool command_key_release_cb(GtkWidget *widget, long mods, long key_raw, long kv, void *udata)
{
	rnd_gtk_command_t *ctx = udata;

	if (ctx->command_entry_status_line_active)
		rnd_cli_edit(ghidgui->hidlib);
	return TRUE;
}

/* Create the command_combo_box.  The command_combo_box
   lives in the bottom_hbox either shown or hidden.
   Since it's never destroyed, the combo history strings never need
   rebuilding. */
void rnd_gtk_command_combo_box_entry_create(rnd_gtk_command_t *ctx, void (*hide_status)(void*,int), void *status_ctx)
{
	ctx->status_ctx = status_ctx;
	ctx->hide_status = hide_status;
	ctx->command_combo_box = gtkc_combo_box_entry_new_text();
	ctx->command_entry = gtkc_combo_box_get_entry(ctx->command_combo_box);

	gtkc_entry_set_width_chars(ctx->command_entry, 40);
	gtk_entry_set_activates_default(ctx->command_entry, TRUE);

	g_signal_connect(G_OBJECT(ctx->command_entry), "activate", G_CALLBACK(command_entry_activate_cb), ctx);

	g_object_ref(G_OBJECT(ctx->command_combo_box)); /* so can move it */
	rnd_clihist_init();
	rnd_clihist_sync(ctx, rnd_gtk_chist_append);

	gtkc_bind_key_press(ctx->command_entry,  rnd_gtkc_xy_ev(&ctx->kpress, command_key_press_cb, ctx));
	gtkc_bind_key_release(ctx->command_entry, rnd_gtkc_xy_ev(&ctx->krelease, command_key_release_cb, ctx));
}

void rnd_gtk_cmd_close(rnd_gtk_command_t *ctx)
{
	if (!ctx->command_entry_status_line_active)
		return;

	if (ctx->rnd_gtk_entry_loop && g_main_loop_is_running(ctx->rnd_gtk_entry_loop)) /* should always be (the entry is active) */
		g_main_loop_quit(ctx->rnd_gtk_entry_loop);

	ctx->command_entered = NULL; /* We are aborting */

	/* Hide the host widget in full screen - not enough if only the entry is gone */
	if (rnd_conf.editor.fullscreen) {
		gtk_widget_hide(gtk_widget_get_parent(ctx->command_combo_box));
		gtk_widget_hide(gtk_widget_get_parent(ctx->prompt_label));
	}
}


void rnd_gtk_command_update_prompt(rnd_gtk_command_t *ctx)
{
	if (ctx->prompt_label != NULL)
		gtk_label_set_text(GTK_LABEL(ctx->prompt_label), rnd_cli_prompt(":"));
}


/* This is the command entry function called from Action Command().
   The command_combo_box is packed into the status line label hbox. */
char *rnd_gtk_command_entry_get(rnd_gtk_command_t *ctx, const char *prompt, const char *command)
{
	gint escape_sig_id, escape_sig2_id;

	/* Flag so output drawing area won't try to get focus away from us and
	   so resetting the status line label can be blocked when resize
	   callbacks are invokded from the resize caused by showing the combo box. */
	ctx->command_entry_status_line_active = TRUE;

	gtkc_entry_set_text(ctx->command_entry, command ? command : "");
	if (rnd_conf.editor.fullscreen)
		gtk_widget_show(gtk_widget_get_parent(ctx->command_combo_box));

	gtk_widget_show(ctx->command_combo_box);
	gtk_widget_show(ctx->prompt_label);
	ctx->hide_status(ctx->status_ctx, 0);

	/* Remove the top window accel group so keys intended for the entry
	   don't get intercepted by the menu system.  Set the interface
	   insensitive so all the user can do is enter a command, grab focus
	   and connect a handler to look for the escape key. */
	ctx->pre_entry();

	gtk_widget_grab_focus(GTK_WIDGET(ctx->command_entry));
	escape_sig_id = gtkc_bind_key_press(ctx->command_entry,  rnd_gtkc_xy_ev(&ctx->kpress, command_key_press_cb, ctx));
	escape_sig2_id = gtkc_bind_key_release(ctx->command_entry, rnd_gtkc_xy_ev(&ctx->krelease, command_key_release_cb, ctx));

	ctx->rnd_gtk_entry_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(ctx->rnd_gtk_entry_loop);

	g_main_loop_unref(ctx->rnd_gtk_entry_loop);
	ctx->rnd_gtk_entry_loop = NULL;

	ctx->command_entry_status_line_active = FALSE;

	/* Restore the damage we did before entering the loop. */
	gtkc_unbind_key(ctx->command_entry, &ctx->kpress, escape_sig_id);
	gtkc_unbind_key(ctx->command_entry, &ctx->krelease, escape_sig2_id);

	/* Hide/show the widgets */
	if (rnd_conf.editor.fullscreen) {
		gtk_widget_hide(gtk_widget_get_parent(ctx->command_combo_box));
		gtk_widget_hide(gtk_widget_get_parent(ctx->prompt_label));
	}
	ctx->hide_status(ctx->status_ctx, 1);

	/* Restore the status line label and give focus back to the drawing area */
	gtk_widget_hide(ctx->command_combo_box);
	gtk_widget_hide(ctx->prompt_label);
	ctx->post_entry();

	return ctx->command_entered;
}


void rnd_gtk_handle_user_command(rnd_hidlib_t *hl, rnd_gtk_command_t *ctx, rnd_bool raise)
{
	char *command;

	command = rnd_gtk_command_entry_get(ctx, rnd_cli_prompt(":"), (gchar *)"");
	if (command != NULL) {
		/* copy new command line to save buffer */
		rnd_parse_command(hl, command, rnd_false);
		g_free(command);
	}
}

const char *rnd_gtk_cmd_command_entry(rnd_gtk_command_t *ctx, const char *ovr, int *cursor)
{
	if (!ctx->command_entry_status_line_active) {
		if (cursor != NULL)
			*cursor = -1;
		return NULL;
	}
	if (ovr != NULL) {
		gtkc_entry_set_text(ctx->command_entry, ovr);
		if (cursor != NULL)
			gtk_editable_set_position(GTK_EDITABLE(ctx->command_entry), *cursor);
	}
	if (cursor != NULL)
		*cursor = gtk_editable_get_position(GTK_EDITABLE(ctx->command_entry));
	return gtkc_entry_get_text(ctx->command_entry);
}

