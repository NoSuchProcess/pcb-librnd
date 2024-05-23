/* Internal API */
#include <stddef.h>
#include <genlist/gendlist.h>
#include <genvector/vtp0.h>
#include "pa.h"

#ifndef NDEBUG
#	define PB2_UID           long uid;
#	define PB2_UID_SET(obj)  (obj)->uid = (++(ctx)->uid)
#	define PB2_UID_CLR(obj)  (obj)->uid = -1
#	define PB2_UID_GET(obj)  ((long)(obj)->uid)
#else
#	define PB2_UID
#	define PB2_UID_SET(obj)
#	define PB2_UID_CLR(obj)
#	define PB2_UID_GET(obj) ((long)(obj))
#endif

typedef struct pb2_seg_s pb2_seg_t;
typedef struct pb2_curve_s pb2_curve_t;
typedef struct pb2_cgout_s pb2_cgout_t;
typedef struct pb2_cgnode_s pb2_cgnode_t;
typedef struct pb2_face_s pb2_face_t;
typedef struct pb2_ctx_s pb2_ctx_t;

struct pb2_seg_s {
	rnd_rtree_box_t bbox; /* for rtree; first x;y are the actual points */

	/* TODO("stub: this may need to be a vector if there are multiple B polys later; we probably don't need counters if we also keep discarded segments and count them as well") */
	unsigned short cntA, cntB; /* how many times this segment is in polygon A or B (count of overlapping segs) */

	unsigned shape_type:4; /* one of rnd_vnode_curve_type_t */
	unsigned discarded:1;
	unsigned risky:1;      /* endpoint moved to rounded coords - risk of new intersection */
	unsigned in_graph:1;  /* set to 1 when curve is inserted into the curve graph */
	unsigned gr_start:1;  /* set to 1 when in_graph is 1 and ->start is connected to a node */
	unsigned gr_end:1;    /* set to 1 when in_graph is 1 and ->end is connected to a node */

	rnd_vector_t start; /* startpoint */
	rnd_vector_t end;   /* endpoint; during step 1 there is no ->link_curve yet */

	union {
		struct { /* circular arc */
			rnd_vector_t center; /* part of the specification: saved */
			/* cached fields (not saved) */
			rnd_ucoord_t r[6]; /* pa_big_coord_t really */
			/* angle of point from the center [0..4); pa_big_coord_t aim = aim_int.aim_frac */
			rnd_ucoord_t aim_frac[2]; /* fraction part of aim ("angle") for start and endpoint */
			char aim_int[2]; /* integer part of the aim ("angle") for start and endpoint */
		} arc;
	} shape;

	gdl_elem_t link;      /* within a curve */
	pb2_seg_t *next_all;  /* within ctx */
	pb2_seg_t *nexts, *nexte; /* endpoint list hash in step 1 */
	PB2_UID
};


/*** curve graph ***/

struct pb2_cgout_s { /* outgoing edge from a node; not allocated separately, part of curve */
	pb2_curve_t *curve;
	pb2_cgnode_t *node;
	unsigned reverse:1;     /* when 1, end->start so this out is part of curve->out_end */
	unsigned corner_mark:1; /* when 1, the corner/angle between this curve and the next is marked */
	int nd_idx;             /* index of this out within cgnode->edges[] */
	pa_angle_t angle;
	PB2_UID
};

struct pb2_cgnode_s {
	rnd_rtree_box_t bbox;  /* for rtree; first x;y are the actual points */
	int num_edges;
	gdl_elem_t link;       /* in ctx->cgnodes */
	PB2_UID
	pb2_cgout_t edges[1];  /* overallocated to contain all edges, sorted by angle */
};

struct pb2_curve_s {
	gdl_list_t segs; /* list of segments making up this curve (of pb2_seg_t *) */
	pb2_cgout_t *out_start, *out_end;
	pb2_face_t *face[2]; /* a curve has 0, 1 or 2 adjacent faces (0 for stubs) */

	unsigned pruned:1;    /* used in step 4 and 5 when collecting curves for output faces; step 1 also marks dangling stubs pruned */
	unsigned face_1_implicit:1; /* when 1, ->face[1] is added as a wrapping face (so ->face[1] doesn't have actual segments listed on this curve) */

	unsigned face_destroy:1;  /* temporarily mark in step 4: curve is waiting for its faces to be destroyed */

	gdl_elem_t link; /* curves are on a linked list in ctx */
	PB2_UID
};

#define pb2_seg_parent_curve(seg) ( \
	(seg->link.parent == NULL) ? NULL : \
	((pb2_curve_t *)((char *)seg->link.parent - offsetof(pb2_curve_t, segs))) \
	)

/*** face graph ***/
struct pb2_face_s {
	rnd_rtree_box_t bbox;
	double area;


	/* polarity within input A, B and the output; 0 means hole, 1 means fill */
	unsigned inA:1;
	unsigned inB:1;
	unsigned out:1;

	unsigned pol_valid:1; /* ->out has been computed */
	unsigned destroy:1;   /* has a pruned curve and needs to be destroyed in step 5 */

	rnd_vector_t polarity_pt;    /* where the polarity test got made; remember this only for debugging purposes (there won't be too many faces anyway) */
	rnd_vector_t polarity_dir;   /* relative direction of the virtual piont from polarity_pt (direction only: the VP is infinitely close) */

	pb2_face_t *children, *next; /* output face tree */


	/* step 6 temporary data */
	struct {
		long hit; /* number of times the ray has hit this face */
		gdl_elem_t link;
	} step6;

	long num_curves;

	gdl_elem_t link; /* within ctx->faces */
	PB2_UID
	pb2_cgout_t *outs[1];  /* over-allocated to contain a pointer to each outgoing edge making up this face, in order */
};


/*** context ***/
struct pb2_ctx_s {
	rnd_poly_bool_op_t op;
	unsigned has_B:1;           /* set by the caller if there's a 'B' poly (binop) */

	/* step 1 output: segs and curves */
	rnd_rtree_t seg_tree;       /* haystack of segments */
	gdl_list_t curves;          /* unordered list of all curves */
	pb2_seg_t *all_segs;        /* head of singly linked list of all segments ever allocated */

	/* step 2 output: curve graph and faces */
	gdl_list_t faces;           /* unordered list of all faces */
	gdl_list_t cgnodes;         /* unordered list of all curve graph nodes */

	/* step 6 output: face stack */
	pb2_face_t root;            /* root face is a negative face representing the outside world; positive islands are children of the root face */

	/* internal: cache/temporary storage */
	TODO("TODO: could be overlapped in an union, per step")
	vtp0_t iscs, splits;  /* step1 */
	vtp0_t outtmp;        /* step2 */

	/* debug */
	const void *input_A, *input_B; /* for debug draw */

	PB2_UID               /* next UID to use */
};

/* Enable dumping all data and/or draw the world in svg after each step of the
   algo (useful for debugging) */
extern int pb2_debug_dump_steps, pb2_debug_draw_steps;

/* The caller loads ctx with poly 'A' and 'B' and loads ctx->op then calls
   pb2_exec to perform clipping and clean up ctx. */
int pb2_exec(pb2_ctx_t *ctx, rnd_polyarea_t **res);

/*** For filling up ctx with content ***/
void pb2_1_map_seg_line(pb2_ctx_t *ctx, const rnd_vector_t p1, const rnd_vector_t p2, char poly_id, int isected);


/*** For unit testing ***/
extern int pb2_face_polarity_at_verbose;
int pb2_face_polarity_at(pb2_ctx_t *ctx, rnd_vector_t pt, rnd_vector_t direction);

