#include <stdio.h>
#include <locale.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/rnd_bool.h>

int main(int argc, char *argv[])
{
	rnd_printf_slot[0] = "%{ ()}mq";
	setlocale(LC_ALL, "C");
	rnd_units_init();
	rnd_fprintf(stdout, argv[1], argv[2]);
	printf("\n");
	rnd_units_uninit();
	return 0;
}
