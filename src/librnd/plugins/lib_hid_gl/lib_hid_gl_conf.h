#ifndef RND_STROKE_CONF_H
#define RND_STROKE_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			const struct backend {
				RND_CFT_BOOLEAN disable_direct;    /* do not use the opengl direct draw backend (client state imlementation); needs restart to take effect */
				RND_CFT_BOOLEAN disable_vao;       /* do not use the opengl vao draw backend (vertex array object imlementation); needs restart to take effect */
				RND_CFT_LIST preference;           /* ordered list of opengl draw backends; first one that initializes without error will be chosen for rendering; needs restart to take effect */
			} backend;
		} lib_hid_gl;
	} plugins;
} conf_lib_hid_gl_t;

#endif
