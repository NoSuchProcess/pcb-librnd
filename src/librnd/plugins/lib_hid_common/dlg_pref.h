#ifndef RND_DLG_PREF_H
#define RND_DLG_PREF_H

typedef struct pref_ctx_s pref_ctx_t;

#include <librnd/core/conf.h>
#include <librnd/core/conf_hid.h>
#include <librnd/hid/hid_dad.h>
#include <librnd/core/actions.h>

typedef struct pref_conflist_s pref_confitem_t;
struct pref_conflist_s {
	const char *label;
	const char *confpath;
	int wid;
	pref_confitem_t *cnext; /* linked list for conf callback - should be NULL initially */
};

typedef enum rnd_pref_tab_flag_e { /* bitfield */
	RND_PREFTAB_NEEDS_ROLE = 1,
	RND_PREFTAB_AUTO_FREE_DATA = 2       /* free tab data when plugin is unloaded */
} rnd_pref_tab_flag_t;

typedef struct rnd_pref_tab_hook_s rnd_pref_tab_hook_t;
struct rnd_pref_tab_hook_s {
	const char *tab_label;
	unsigned long flags;                /* bitfield of rnd_pref_tab_flag_t */

	void (*open_cb)(pref_ctx_t *ctx);   /* called right after the dialog box is created */
	void (*close_cb)(pref_ctx_t *ctx);  /* called from the dialog box is close_cb event */
	void (*create_cb)(pref_ctx_t *ctx); /* called while the dialog box is being created: create widgets in current tab */

	void (*design_replaced_cb)(pref_ctx_t *ctx); /* called if the design got replaced (e.g. new board loaded) */
	void (*meta_changed_cb)(pref_ctx_t *ctx);    /* called if the design metadata changed */

	void (*spare_f1)(); void (*spare_f2)(); void (*spare_f3)(); void (*spare_f4)();
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
	long spare_l1, spare_l2, spare_l3, spare_l4;
};

#define RND_PREF_MAX_TAB 32

typedef struct {
	int wtree, wintree, wdesc, wname, wmainp, wnattype, wfilter;
	int wnatval[RND_CFN_max+1], wsrc[RND_CFN_max+1];
	rnd_conf_native_t *selected_nat;
	int selected_idx;
	int spare_i1, spare_i2, spare_i3, spare_i4;
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
} pref_conf_t;

typedef struct {
	int wlist;
	int lock;
	int spare_i1, spare_i2, spare_i3, spare_i4;
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
} pref_key_t;

typedef struct {
	int wlist, wdesc, wload, wunload, wreload, wexport;
	int spare_i1, spare_i2, spare_i3, spare_i4;
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
} pref_menu_t;

typedef struct {
	int wmaster, wboard, wproject, wuser;
	int spare_i1, spare_i2, spare_i3, spare_i4;
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
} pref_win_t;

struct pref_ctx_s {
	RND_DAD_DECL_NOINIT(dlg)
	int wtab, wrole, wrolebox;
	int active; /* already open - allow only one instance */

	struct {
		const rnd_pref_tab_hook_t *hooks;
		void *tabdata;
	} tab[RND_PREF_MAX_TAB];
	int tabs;       /* number of app-specific tabs used */
	int tabs_total; /* number of tabs used (app-specific and built-in combined) */
	unsigned int tabs_inited:1; /* whether tabs (including app-specific tabs) are inited */

	rnd_conf_role_t role; /* where changes are saved to */

	pref_confitem_t *rnd_conf_lock; /* the item being changed - should be ignored in a conf change callback */
	vtp0_t auto_free; /* free() each item on close */

	void (*spare_f1)(); void (*spare_f2)(); void (*spare_f3)(); void (*spare_f4)();
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
	long spare_l1, spare_l2, spare_l3, spare_l4;

	/* builtin tabs - potentially variable size but apps won't need to read them */
	pref_win_t win;
	pref_key_t key;
	pref_menu_t menu;
	pref_conf_t conf;
};

extern pref_ctx_t pref_ctx;

/* Create label-input widget pair for editing a conf item, or create whole
   list of them */
void rnd_pref_create_conf_item(pref_ctx_t *ctx, pref_confitem_t *item, void (*change_cb)(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr));
void rnd_pref_create_conftable(pref_ctx_t *ctx, pref_confitem_t *list, void (*change_cb)(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr));

/* Set the config node from the current widget value of a conf item, or
   create whole list of them; the table version returns whether the item is found. */
void rnd_pref_dlg2conf_item(pref_ctx_t *ctx, pref_confitem_t *item, rnd_hid_attribute_t *attr);
rnd_bool rnd_pref_dlg2conf_table(pref_ctx_t *ctx, pref_confitem_t *list, rnd_hid_attribute_t *attr);

/* Remove conf change binding - shall be called when widgets are removed
   (i.e. on dialog box close) */
void rnd_pref_conflist_remove(pref_ctx_t *ctx, pref_confitem_t *list);

extern rnd_conf_hid_id_t pref_hid;


/*** public API for the caller ***/

lht_node_t *rnd_pref_dlg2conf_pre(rnd_design_t *hidlib, pref_ctx_t *ctx);
void rnd_pref_dlg2conf_post(rnd_design_t *hidlib, pref_ctx_t *ctx);

void rnd_pref_init_func_dummy(pref_ctx_t *ctx, int tab);


/* In event callbacks no context is available; return context baed on hidlib */
pref_ctx_t *rnd_pref_get_ctx(rnd_design_t *hidlib);

#define PREF_INIT_FUNC rnd_pref_init_func_dummy

#define PREF_INIT(ctx, hooks_) \
	do { \
		ctx->tab[PREF_TAB].hooks = hooks_; \
		PREF_INIT_FUNC(ctx, PREF_TAB-1); \
	} while(0)

#define PREF_TABDATA(ctx)   (ctx->tab[PREF_TAB].tabdata)


/*** internal API for librnd ***/
void rnd_dlg_pref_init(int pref_tab, void (*first_init)(pref_ctx_t *ctx, int tab));
void rnd_dlg_pref_uninit(void);

extern const char rnd_acts_Preferences[];
extern const char rnd_acth_Preferences[];
fgw_error_t rnd_act_Preferences(fgw_arg_t *res, int argc, fgw_arg_t *argv);

extern const char rnd_acts_dlg_confval_edit[];
extern const char rnd_acth_dlg_confval_edit[];
extern fgw_error_t rnd_act_dlg_confval_edit(fgw_arg_t *res, int argc, fgw_arg_t *argv);

#endif
