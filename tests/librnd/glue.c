/*** hidlib glue ***/

typedef struct design_s {
	rnd_hidlib_t hidlib; /* shall be the first */
} design_t;

design_t CTX;

const pup_buildin_t local_buildins[] = {
	{NULL, NULL, NULL, NULL, 0, NULL}
};

static const char *action_args[] = {
	NULL, NULL, NULL, NULL, NULL /* terminator */
};

void conf_core_init()
{
	rnd_conf_reg_field_(NULL, 1, RND_CFN_COORD, "should_never_match", "dummy", 0);
}
