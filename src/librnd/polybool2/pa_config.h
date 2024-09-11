/* Copy edge tree on pline_dup() instead of re-creating the tree on geometric basis */
#define PB_OPTIMIZE_TREE_COPY

/* When 1: each rtree add verifies the entry is not already added */
#define PA_VERIFY_RTREE_ADD 0

/* When 1: do not skip plines that are not intersecting but include
   every bit of the input in a large PB2 model */
#define PB2_DISABLE_PLINE_INPUT_OPTIMIZATION 0

/* EPSILON^2 for endpoint matching in validation code to overcome rounding
   errors. Accept at most 2 units (nanometer) of such intersection. */
#define RND_POLY_VALID_ENDP_EPSILON 4


/* debug control */
/* enable compiling pa_dump_pa() for gdb (but no automatic dump from the code) */
#define DEBUG_PA_DUMP_PA 1



#define DEBUG_CLIP 0
#define DEBUG_SLICE 0

/* dump sorted dicer frame iscs before and after redundancy check */
#define DEBUG_CLIP_DUMP_LOOP 0

/*** these can be overridden runtime, these are the initial default values */

/* Enable debug trace of pb2 steps - a lot of textual output with coords */
#define DEBUG_DUMP_STEPS 0

/* Enable debug draw in polybool format after steps */
#define DEBUG_DRAW_STEPS 0

/* Dump each input and result into poly files readable by the tester */
#define DEBUG_DUMP_BOOLOPS 0

/* Dump each input and output pline of rnd_pline_dup_with_offset_round() in poly debug format */
#define DEBUG_DUMP_OFFSET_ROUND 0

/* Print extra trace of the ray when determining face polarity */
#define DEBUG_STEP3_FACE_POLARITY_VERBOSE 0


/* When compield with debug (for assert()) if this is 0, run a check on the
   output of each operation */
#define DEBUG_DISABLE_AUTOCHECK 0

/* If set to 1, do not create the edge rtree in step 7 */
#define DEBUG_INHIBIT_EDGE_TREE 0

#undef DEBUG
