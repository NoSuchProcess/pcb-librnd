/* Returns pointer to current working directory.  If 'path' is not NULL, then
   the current working directory is copied to the array pointed to by 'path' */
char *rnd_get_wd(char *path);

/* alternative to system() for proper argument passing */
int rnd_spawnvp(const char **argv);


/* Creates a new temporary file name. Warning: race condition: doesn't create
the file! */
char *rnd_tempfile_name_new(const char *name);

/* remove temporary file and _also_ free the memory for name
 * (this fact is a little confusing) */
int rnd_tempfile_unlink(char *name);

/* Return non-zero if fn is an absolute path */
int rnd_is_path_abs(const char *fn);

/* [3.2.0] Return dirname(1) of path in a newly allocated string */
char *rnd_dirname(const char *path);


/* [3.2.0] same as rnd_dirname(), but returns the real-path of the parent
   directory of path; that is, if path points to a symlink, it's the real
   path of the symlink's parent dir (so path is never resolved) */
char *rnd_dirname_real(const char *path);


/*** Only for internal usage from within librnd ***/

/* Return 1 if path is a file that can be opened for read */
int rnd_file_readable_(const char *path);

/* Query multiple stat fields at once. Returns 0 on success. Output
   fields must not be NULL. */
int rnd_file_stat_(const char *path, int *is_dir, long *size, double *mtime);

