#ifndef CONFTEST_CONF_CORE_H
#define CONFTEST_CONF_CORE_H

#include <librnd/core/conf.h>
#include <librnd/core/color.h>

typedef struct {
	const struct {
		RND_CFT_LIST library_search_paths;
		RND_CFT_STRING backup_name;
	} rc;
	const struct {
		struct {
			RND_CFT_COLOR layer[64];
		} color;
	} appearance;
} conf_core_t;

extern conf_core_t conf_core;

void conf_core_init(void);
void conf_core_uninit_pre(void);

#endif
