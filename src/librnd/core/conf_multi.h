/* Temporary API for pre-4.0.0 multi-sheet support in sch-rnd 

   DO NOT USE THIS API outside of sch-rnd.

*/

#include <genlist/gendlist.h>

extern gdl_list_t rnd_designs; /* all design files currently open (in any project) */

/* Allocate storage where the global config can be saved to */
rnd_conf_state_t *rnd_conf_state_alloc(void);

/* Free storage and all fields of a conf state */
void rnd_conf_state_free(rnd_conf_state_t *cs);

/* Save global config to dst then zero out global config; dst becomes valid
   so it can be loaded later. */
void rnd_conf_state_save(rnd_conf_state_t *dst);

/* Load global config from src; zero out fields of src and mark it invalid
   so it can not be loaded twice. */
void rnd_conf_state_load(rnd_conf_state_t *src);

/* Copy shared conf from src to current global; useful for initializing for
   a new design */
void rnd_conf_state_init_from(rnd_conf_state_t *src);

/* call this before loading a new design in a multi-design app; resets
   and prepares global conf */
void rnd_conf_multi_pre_load_design(void);

/* Announce a new design after loaded or created (creates config save structs) */
void rnd_conf_state_new_design(rnd_design_t *dsg);

/* call when design is unloaded/discarded (frees config save structs) */
void rnd_conf_state_del_design(rnd_design_t *dsg);

/* Call this to get dsg's project config loaded or bound if it's already loaded.
   Assumes dsg->project is set and the project structure is created.
   Normally called after the design is loaded. */
void rnd_multi_load_prj_for_dsg(rnd_design_t *dsg);

/*** per plugin and per app custom config ***/
void rnd_conf_state_plug_reg(void *globvar, long size, const char *cookie);
void rnd_conf_state_plug_unreg_all_cookie(const char *cookie);

/* Plugin conf registration */

#define rnd_conf_plug_reg(globvar, intern__, cookie) \
	do { \
		rnd_conf_reg_intern(intern__); \
		rnd_conf_state_plug_reg(&(globvar), sizeof(globvar), cookie); \
	} while(0)


#define rnd_conf_plug_unreg(conf_path_prefix, intern__, cookie) \
	do { \
		rnd_conf_unreg_intern(intern__); \
		rnd_conf_unreg_fields(conf_path_prefix); \
		rnd_conf_state_plug_unreg_all_cookie(cookie); \
	} while(0)


/*** internal ***/

/* Called after a switch, merges config if needed */
void rnd_conf_multi_merge_after_switch(rnd_design_t *dsg);
