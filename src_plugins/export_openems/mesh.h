#include "layer.h"
#include "vtc0.h"
#include "vtr0.h"

typedef struct {
	vtc0_t user_line; /* input: lines forced by the user */
	vtr0_t user_dens; /* input: density forced by the user */
	vtc0_t edge;      /* input: around object edge - lines that must be in the mesh */
	vtr0_t dens;      /* input: density ranges; data[0].c is the target density */
	vtc0_t result;    /* resulting line coordinates */
} pcb_mesh_lines_t;

typedef enum {
	PCB_MESH_HORIZONTAL, /* variable y coord (horizontal lines) */
	PCB_MESH_VERTICAL,   /* variable x coord (vertical lines) */
	PCB_MESH_Z,
	PCB_MESH_max
} pcb_mesh_dir_t;

typedef struct {
	pcb_layer_t *layer;                    /* input layer (objects are picked up from this layer) */
	pcb_layer_t *ui_layer_xy, *ui_layer_z; /* optional UI layers to draw the mesh on */
	char *ui_name_xy;                      /* name of the UI layer */
	pcb_coord_t dens_obj, dens_gap;        /* target density: distance between mesh lines above objects and above gaps */
	pcb_coord_t min_space;                 /* make sure there's always at least this much space between two mesh lines */
	pcb_coord_t def_subs_thick;            /* default substrate thickness */
	pcb_coord_t def_copper_thick;          /* default copper thickness */
	pcb_mesh_lines_t line[PCB_MESH_max];   /* actual lines of the mesh */
	const char *bnd[6];                    /* temporary: boundary conditions */
	unsigned smooth:1;                     /* if set, avoid jumps in the meshing by gradually changing meshing distance */
	unsigned noimpl:1;                     /* when set, do not add extra implicit mesh lines, keep the explicit ones only */
} pcb_mesh_t;

extern const char pcb_acts_mesh[];
extern const char pcb_acth_mesh[];
int pcb_act_mesh(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);

/* Get one of the configured meshes */
pcb_mesh_t *pcb_mesg_get(const char *name);
