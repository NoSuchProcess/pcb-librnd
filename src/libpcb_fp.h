#ifndef LIBPCB_FP_H
#define LIBPCB_FP_H
typedef enum {
	PCB_FP_INVALID,
	PCB_FP_DIR,
	PCB_FP_FILE,
	PCB_FP_PARAMETRIC
} pcb_fp_type_t;

/* List all symbols, optionally recursively, from CWD/subdir. For each symbol
   or subdir, call the callback. Ignore file names starting with .*/
int pcb_fp_list(const char *subdir, int recurse,  int (*cb)(void *cookie, const char *subdir, const char *name, pcb_fp_type_t type), void *cookie);

/* Decide about the type of a footprint file by reading the content*/
pcb_fp_type_t pcb_fp_file_type(const char *fn);

/* duplicates the name and splits it into a basename and params;
   params is NULL if the name is not parametric (and "" if parameter list is empty)
   returns 1 if name is parametric, 0 if file element.
   The caller shall free only *basename at the end.
   */
int pcb_fp_dupname(const char *name, char **basename, char **params);


/* walk the search_path for finding the first footprint for basename (shall not contain "(") */
char *pcb_fp_search(const char *search_path, const char *basename, int parametric);

/* Open a footprint for reading; if the footprint is parametric, it's run
   prefixed with libshell (or executed directly, if libshell is NULL).
   If name is not an absolute path, search_path is searched for the first match.
   The user has to supply a state integer that will be used by pcb_fp_fclose().
 */
FILE *pcb_fp_fopen(const char *libshell, const char *search_path, const char *name, int *state);

/* Close the footprint file opened by pcb_fp_fopen(). */
void pcb_fp_fclose(FILE *f, int *st);

#endif
