char *GetWorkingDirectory(char *);
const char *get_user_name(void);

/* mkdir() implentation, mostly for plugins, which don't have our config.h.
 * Check whether mkdir() is mkdir or _mkdir, and whether it takes one
 * or two arguments.  WIN32 mkdir takes one argument and POSIX takes
 * two.
 */
#if HAVE_MKDIR
#if MKDIR_TAKES_ONE_ARG
				 /* MinGW32 */
#include <io.h>									/* mkdir under MinGW only takes one argument */
#define MKDIR(a, b) mkdir(a)
#else
#define MKDIR(a, b) mkdir(a, b)
#endif
#else
#if HAVE__MKDIR
				 /* plain Windows 32 */
#define MKDIR(a, b) _mkdir(a)
#else
#define MKDIR(a, b) pcb_mkdir(a, b)
#define MKDIR_IS_PCBMKDIR 1
int pcb_mkdir(const char *path, int mode);
#endif
#endif

/* for access(), fork() and friends */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

int pcb_spawnvp(char **argv);
char *tempfile_name_new(char *name);
int tempfile_unlink(char *name);
