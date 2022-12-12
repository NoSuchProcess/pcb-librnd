/* 3-state plugin system; possible states of each plugin, stored in
   /local/module/PLUGIN_NAME/controls:
    "disable" = do not compile it at all
    "buildin" = enable, static link into the executable
    "plugin"  = enable, make it a dynamic link library (runtime load plugin)
*/

#define sdisable "disable"
#define sbuildin "buildin"
#define splugin  "plugin"


/* Macros to check the state */

#define plug_eq(name, val) \
	((get("/local/module/" name "/controls") != NULL) && (strcmp(get("/local/module/" name "/controls"), val) == 0))

#define plug_eq_expl(name, val) \
	((get("/local/module/" name "/explicit") != NULL) && (strcmp(get("/local/module/" name "/explicit"), val) == 0))

#define plug_is_enabled(name)  (plug_eq(name, splugin) || plug_eq(name, sbuildin))
#define plug_is_explicit(name) (plug_eq_expl(name, splugin) || plug_eq_expl(name, sbuildin))
#define plug_is_disabled(name) (plug_eq(name, sdisabled))
#define plug_is_buildin(name)  (plug_eq(name, sbuildin))
#define plug_is_plugin(name)   (plug_eq(name, splugin))

#define plug_is_explicit_buildin(name)  (plug_eq_expl(name, sbuildin))

/* auto-set tables to change control to the desired value */
const arg_auto_set_node_t arg_disable[] = {
	{"controls",    sdisable},
	{"explicit",    sdisable},
	{NULL, NULL}
};

const arg_auto_set_node_t arg_Disable[] = {
	{"controls",    sdisable},
	{NULL, NULL}
};

const arg_auto_set_node_t arg_buildin[] = {
	{"controls",    sbuildin},
	{"explicit",    sbuildin},
	{NULL, NULL}
};

const arg_auto_set_node_t arg_plugin[] = {
	{"controls",    splugin},
	{"explicit",    splugin},
	{NULL, NULL}
};


/* plugin_def implementation to create CLI args */
#define plugin3_args(name, desc) \
	{"disable-" name, "/local/module/" name,  arg_disable,   "$do not compile " desc}, \
	{"Disable-" name, "/local/module/" name,  arg_Disable,   NULL }, \
	{"buildin-" name, "/local/module/" name,  arg_buildin,   "$static link " desc " into the executable"}, \
	{"plugin-"  name, "/local/module/" name,  arg_plugin,    "$" desc " is a dynamic loadable plugin"},


/* plugin_def implementation to set default state */
#define plugin3_default(name, default_) \
	db_mkdir("/local/module/" name); \
	put("/local/module/" name "/controls", default_);

/* plugin_def implementation to print a report with the final state */
#define plugin3_stat(name, desc) \
	plugin_stat(desc, "/local/module/" name "/controls", name);
