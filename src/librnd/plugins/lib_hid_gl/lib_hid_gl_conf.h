#ifndef RND_STROKE_CONF_H
#define RND_STROKE_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			const struct backend {
				RND_CFT_BOOLEAN disable_direct;
				RND_CFT_BOOLEAN disable_vao;
				RND_CFT_LIST preference;
			} backend;
		} lib_hid_gl;
	} plugins;
} conf_lib_hid_gl_t;

#endif
