/* Copy edge tree on pline_dup() instead of re-creating the tree on geometric basis */
#define PB_OPTIMIZE_TREE_COPY

/* When 1, use the new dicer behind the old, compatibility api */
#define PA_USE_NEW_DICER 1

/* When 1: each rtree add verifies the entry is not already added */
#define PA_VERIFY_RTREE_ADD 0

/* When 1: do not skip plines that are not intersecting but include
   every bit of the input in a large PB2 model */
#define PB2_DISABLE_PLINE_INPUT_OPTIMIZATION 0

/* EPSILON^2 for endpoint matching; the bool algebra code is not
   perfect and causes tiny self intersections at the end of sharp
   spikes. Accept at most 10 nanometer of such intersection */
#define RND_POLY_ENDP_EPSILON 100

/* debug control */
#define DEBUG_CLIP 0
#define DEBUG_SLICE 0

/* dump sorted dicer frame iscs before and after redundancy check */
#define DEBUG_CLIP_DUMP_LOOP 0

/* enable compiling pa_dump_pa() for gdb (but no automatic dump from the code) */
#define DEBUG_PA_DUMP_PA 1

#undef DEBUG
