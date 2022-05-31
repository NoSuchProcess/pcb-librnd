/* Temporary API for pre-4.0.0 multi-sheet support in sch-rnd 

   DO NOT USE THIS API outside of sch-rnd.

*/

typedef struct rnd_conf_state_s rnd_conf_state_t;


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
