#include <stdio.h>
#include <locale.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/rnd_bool.h>
#include <librnd/core/hidlib.h>
#include <librnd/core/rnd_conf.h>

int main(int argc, char *argv[])
{
	rnd_printf_slot[0] = "%{ ()}mq";

	/* manual init sequence required due to broken linker on OSX */
	rnd_multi_get_current();
	rnd_hidlib_conf_init();


	setlocale(LC_ALL, "C");
	rnd_units_init();
	rnd_fprintf(stdout, argv[1], argv[2]);
	printf("\n");
	rnd_units_uninit();
	return 0;
}
