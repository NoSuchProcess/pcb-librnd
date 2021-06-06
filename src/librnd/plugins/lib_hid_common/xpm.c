#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <librnd/rnd_config.h>
#include <librnd/core/actions.h>

#include "xpm/question.xpm"
#include "xpm/warning.xpm"
#include "xpm/online_help.xpm"

const char **rnd_dlg_xpm_by_name(const char *name)
{
	if (strcmp(name, "question") == 0) return question_xpm;
	if (strcmp(name, "warning") == 0) return warning_xpm;
	if (strcmp(name, "online_help") == 0) return online_help_xpm;
	return NULL;
}

fgw_error_t rnd_act_dlg_xpm_by_name(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *name = "";
	static const char *rnd_acts_dlg_xpm_by_name = "need xpm name";

	RND_ACT_MAY_CONVARG(1, FGW_STR, dlg_xpm_by_name, name = argv[1].val.str);

	res->type = FGW_PTR;
	res->val.ptr_void = rnd_dlg_xpm_by_name(name);

	return 0;
}
