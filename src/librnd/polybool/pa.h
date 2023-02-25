/* internals */

struct rnd_cvc_list_s {
	double angle;
	rnd_vnode_t *parent;
	rnd_cvc_list_t *prev, *next, *head;
	char poly, side;
};

typedef enum {
	PA_PLS_UNKNWN  = 0,
	PA_PLS_INSIDE  = 1,
	PA_PLS_OUTSIDE = 2,
	PA_PLS_ISECTED = 3
} pa_pline_status_t;

typedef enum {
	PA_PTS_UNKNWN  = 0,
	PA_PTS_INSIDE  = 1,
	PA_PTS_OUTSIDE = 2,
	PA_PTS_SHARED  = 3,
	PA_PTS_SHARED2 = 4
} pa_plinept_status_t;

#define TOUCHES 99

#define NODE_LABEL(n)  ((n)->Flags.pstatus)
#define LABEL_NODE(n,l) ((n)->Flags.pstatus = (l))


