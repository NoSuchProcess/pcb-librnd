#ifndef RND_DIALOGS_CONF_H
#define RND_DIALOGS_CONF_H

#include <librnd/core/conf.h>

/* dialog config is here because of hidlib: each hidlib user shall have its
   own dialogs plugin */

/* WARNING: part of the public API, can be extended only at the end */

typedef struct {
	const struct {
		const struct {
			const struct {
				RND_CFT_BOOLEAN to_design;
				RND_CFT_BOOLEAN to_project;
				RND_CFT_BOOLEAN to_user;
			} auto_save_window_geometry;
			const struct {
				const struct {
					RND_CFT_INTEGER x;
					RND_CFT_INTEGER y;
					RND_CFT_INTEGER width;
					RND_CFT_INTEGER height;
				} example_template;
			} window_geometry;
			const struct {
				RND_CFT_BOOLEAN save_as_format_guess; /* enable format guessing by default in the 'save as' dialog */
			} file_select_dialog;
			const struct {
				RND_CFT_BOOLEAN dont_ask; /* don't ever ask, just go ahead and overwrite existing files */
			} file_overwrite_dialog;
		} dialogs;

		const struct {
			const struct {
				RND_CFT_STRING file;       /* Path to the history file (empty/unset means history is not preserved) */
				RND_CFT_INTEGER slots;     /* Number of commands to store in the history */
			} cli_history;
			const struct {
				RND_CFT_INTEGER dirname_maxlen;  /* shorten directory names longer than this in path buttons */
			} fsd;
		} lib_hid_common;

	} plugins;
} conf_dialogs_t;

#endif
