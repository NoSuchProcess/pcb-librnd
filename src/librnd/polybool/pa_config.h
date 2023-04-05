/* Copy edge tree on pline_dup() instead of re-creating the tree on geometric basis */
#define PB_OPTIMIZE_TREE_COPY

/* If defined, use fixed point numbers with multi-word integers (bigint) to
   represent true coordinates of intersection points at high precision */
/*#define PA_BIGCOORD_ISC*/

/* If defined, intersection points close to edge endpoint is moved to edge
   endpoint; this removes the tiny edge corner case. The value is manhattan
   distance^2 within the isc point is moved to the endpoint */
/*#define PA_TWEAK_ISC 4*/

/* debug control */
#define DEBUG_JUMP 0
#define DEBUG_GATHER 0
#define DEBUG_ANGLE 0
#define DEBUG_CVC 0
#define DEBUG_ISC 0
#define DEBUG_DUMP 0
#undef DEBUG

/* only when DEBUG is enabled */
#define DEBUG_ALL_LABELS 1
