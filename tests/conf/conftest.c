#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include "conf.h"
#include "conf_hid.h"
#include "conf_core.h"
#include "src_plugins/debug/debug_conf.h"

int lineno = 0;
int global_notify = 0;
conf_hid_id_t hid_id;
const char *hid_cookie = "conftest cookie";

void Message(const char *Format, ...)
{
	va_list args;

	fprintf(stderr, "Error in line %d: ", lineno);

	va_start(args, Format);
	vfprintf(stderr, Format, args);
	va_end(args);

	fprintf(stderr, "\n");
}

void watch_pre(conf_native_t *cfg)
{
	printf("watch_pre:  '%s' old value\n", cfg->hash_path);
}

void watch_post(conf_native_t *cfg)
{
	printf("watch_post: '%s' new value\n", cfg->hash_path);
}

void notify_pre(conf_native_t *cfg)
{
	printf("notify_pre:  '%s' old value\n", cfg->hash_path);
}

void notify_post(conf_native_t *cfg)
{
	printf("notify_post: '%s' new value\n", cfg->hash_path);
}

conf_hid_callbacks_t watch_cbs = {watch_pre, watch_post, NULL, NULL};
conf_hid_callbacks_t global_cbs = {notify_pre, notify_post, NULL, NULL};


extern lht_doc_t *conf_root[];
void cmd_dump(char *arg)
{
	if (arg == NULL) {
		Message("Need an arg: native or lohata");
		return;
	}
	if (strncmp(arg, "native", 6) == 0) {
		arg+=7;
		while(isspace(*arg)) arg++;
		conf_dump(stdout, "", 1, arg);
	}
	else if (strncmp(arg, "lihata", 6) == 0) {
		arg+=7;
		while(isspace(*arg)) arg++;
		conf_role_t role = conf_role_parse(arg);
		if (role == CFR_invalid) {
			Message("Invalid role: '%s'", arg);
			return;
		}
		if (conf_root[role] != NULL)
			lht_dom_export(conf_root[role]->root, stdout, "");
		else
			printf("<empty>\n");
	}
	else
		Message("Invalid dump mode: '%s'", arg);
}

conf_policy_t current_policy = POL_OVERWRITE;
conf_role_t current_role = CFR_DESIGN;

void cmd_policy(char *arg)
{
	conf_policy_t np = conf_policy_parse(arg);
	if (np == POL_invalid)
		Message("Invalid/unknown policy: '%s'", arg);
	else
		current_policy = np;
}

void cmd_role(char *arg)
{
	conf_policy_t nr = conf_role_parse(arg);
	if (nr == POL_invalid)
		Message("Invalid/unknown role: '%s'", arg);
	else
		current_role = nr;
}

void cmd_prio(char *arg)
{
	char *end;
	int np = strtol(arg == NULL ? "" : arg, &end, 10);
	lht_node_t *first;

	if ((*end != '\0') || (np < 0)) {
		Message("Invalid integer prio: '%s'", arg);
		return;
	}
	first = conf_lht_get_first(current_role);
	if (first != NULL) {
		char tmp[128];
		char *end;
		end = strchr(first->name, '-');
		if (end != NULL)
			*end = '\0';
		sprintf(tmp, "%s-%d", first->name, np);
		free(first->name);
		first->name = strdup(tmp);
		conf_update(NULL);
	}
}

void cmd_set(char *arg)
{
	char *path, *val;
	int res;

	path = arg;
	val = strpbrk(path, " \t=");
	if (val == NULL) {
		Message("set needs a value");
		return;
	}
	*val = '\0';
	val++;
	while(isspace(*val) || (*val == '=')) val++;

	res = conf_set(current_role, path, -1, val, current_policy);
	printf("set result: %d\n", res);
}

void cmd_watch(char *arg, int add)
{
	conf_native_t *n = conf_get_field(arg);
	if (n == NULL) {
		Message("unknown path");
		return;
	}
	conf_hid_set_cb(n, hid_id, (add ? &watch_cbs : NULL));
}

void cmd_notify(char *arg)
{
	if (arg != NULL) {
		if (strcmp(arg, "on") == 0)
			global_notify = 1;
		else if (strcmp(arg, "off") == 0)
			global_notify = 0;
	}
	printf("Notify is %s\n", global_notify ? "on" : "off");
}

int main()
{
	char line[1024];

	hid_id = conf_hid_reg(hid_cookie, &global_cbs);

	conf_init();
	conf_core_init();
	conf_core_postproc();
	conf_load_all(NULL, NULL);

	while(fgets(line, sizeof(line), stdin)) {
		char *end = line + strlen(line) - 1;
		char *cmd, *arg;

		lineno++;

		while((end >= line) && ((*end == '\n') || (*end == '\r'))) {
			*end = '\0';
			end--;
		}
		cmd = line;
		while(isspace(*cmd)) cmd++;
		if (*cmd == '#')
			continue;
		arg = strpbrk(cmd, " \t");
		if (arg != NULL) {
			*arg = '\0';
			arg++;
			while(isspace(*arg)) arg++;
		}

		if (strcmp(cmd, "dump") == 0)
			cmd_dump(arg);
		else if (strcmp(cmd, "set") == 0)
			cmd_set(arg);
		else if (strcmp(cmd, "policy") == 0)
			cmd_policy(arg);
		else if (strcmp(cmd, "prio") == 0)
			cmd_prio(arg);
		else if (strcmp(cmd, "role") == 0)
			cmd_role(arg);
		else if (strcmp(cmd, "watch") == 0)
			cmd_watch(arg, 1);
		else if (strcmp(cmd, "unwatch") == 0)
			cmd_watch(arg, 0);
		else if (strcmp(cmd, "notify") == 0)
			cmd_notify(arg);
		else
			Message("unknown command '%s'", cmd);
	}

	conf_hid_unreg(hid_cookie);
	conf_uninit();
}
