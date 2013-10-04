#include <stdio.h>
#include "arg.h"
#include "log.h"
#include "dep.h"
#include "tmpasm.h"

/* Runs when a custom command line argument is found
 returns true if no furhter argument processing should be done */
int hook_custom_arg(const char *key, const char *value)
{
	return 0;
}


/* Runs before anything else */
int hook_preinit()
{
	return 0;
}

/* Runs after initialization */
int hook_postinit()
{
	return 0;
}

/* Runs after all arguments are read and parsed */
int hook_postarg()
{
	return 0;
}

/* Runs when things should be detected for the host system */
int hook_detect_host()
{
	return 0;
}

/* Runs when things should be detected for the target system */
int hook_detect_target()
{
	require("fstools/mkdir", 0, 1);
	require("libs/gui/gtk2", 0, 1);
	require("libs/gui/gd", 0, 1);

	/* for the toporouter: */
	require("libs/sul/glib", 0, 1);

	/* generic utils for Makefiles */
	require("fstools/rm",  0, 1);
	require("fstools/ar",  0, 1);
	require("fstools/cp",  0, 1);
	require("fstools/ln",  0, 1);
	require("sys/ext_exe", 0, 1);
	return 0;
}

#ifdef GENCALL
/* If enabled, generator implements ###call *### and generator_callback is
   the callback function that will be executed upon ###call### */
void generator_callback(char *cmd, char *args)
{
	printf("* generator_callback: '%s' '%s'\n", cmd, args);
}
#endif

/* Runs after detection hooks, should generate the output (Makefiles, etc.) */
int hook_generate()
{
	db_mkdir("/local");

	printf("Generating Makefile.conf\n", tmpasm("..", "Makefile.conf.in", "Makefile.conf"));

	printf("Generating gts/Makefile\n", tmpasm("../gts", "Makefile.in", "Makefile"));
	printf("Generating pcb/Makefile\n", tmpasm("../src", "Makefile.in", "Makefile"));
	return 0;
}

/* Runs before everything is uninitialized */
void hook_preuninit()
{
}

/* Runs at the very end, when everything is already uninitialized */
void hook_postuninit()
{
}

