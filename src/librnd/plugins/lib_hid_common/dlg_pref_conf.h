#ifndef RND_DLG_PREF_CONF_H
#define RND_DLG_PREF_CONF_H

#include <librnd/core/conf.h>

void rnd_dlg_pref_conf_close(pref_ctx_t *ctx);
void rnd_dlg_pref_conf_create(pref_ctx_t *ctx);
void rnd_dlg_pref_conf_open(pref_ctx_t *ctx, const char *tabarg);

void rnd_pref_dlg_conf_changed_cb(pref_ctx_t *ctx, rnd_conf_native_t *cfg, int arr_idx); /* global conf change */

#endif
