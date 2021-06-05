
/*** basic GUI integration ***/

/* Append a new command to the list after the user has entered/executed it */
typedef void rnd_clihist_append_cb_t(void *ctx, const char *cmd);
typedef void rnd_clihist_remove_cb_t(void *ctx, int idx);
void rnd_clihist_append(const char *cmd, void *ctx, rnd_clihist_append_cb_t *append, rnd_clihist_remove_cb_t *remove);


/* Initialize the command history, load the history from disk if needed */
void rnd_clihist_init(void);

/* Call a series of append's to get the GUI in sync with the list */
void rnd_clihist_sync(void *ctx, rnd_clihist_append_cb_t *append);


/*** Cursor operation ***/

/* Move the cursor 'up' (backward in history) and return the next entry's cmd */
const char *rnd_clihist_prev(void);

/* Move the cursor 'down' (forward in history) and return the next entry's cmd */
const char *rnd_clihist_next(void);

/* Reset the cursor (pointing to void, behind the most recent entry), while the
   user is typing a new entry */
void rnd_clihist_reset(void);


/*** Misc/internal ***/
/* Load the last saved list from disk */
void rnd_clihist_load(void);

/* Save the command history to disk - called automatically on uninit */
void rnd_clihist_save(void);

/* Free all memory used by the history */
void rnd_clihist_uninit(void);

