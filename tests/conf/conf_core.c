#include <librnd/core/conf.h>
#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>
#include <librnd/core/conf_hid.h>


conf_core_t conf_core;

void pcb_conf_core_postproc(void) { }
void conf_core_uninit_pre(void) { }
void conf_core_uninit(void) { }

void conf_core_init(void)
{
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_core, field,isarray,type_name,cpath,cname,desc,flags);
#include "conf_core_fields.h"
	pcb_conf_core_postproc();
}
