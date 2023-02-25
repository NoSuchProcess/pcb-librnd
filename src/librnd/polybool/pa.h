/* internals */

struct rnd_cvc_list_s {
	double angle;
	rnd_vnode_t *parent;
	rnd_cvc_list_t *prev, *next, *head;
	char poly, side;
};

typedef enum {
	PA_PLL_UNKNWN  = 0,
	PA_PLL_INSIDE  = 1,
	PA_PLL_OUTSIDE = 2,
	PA_PLL_ISECTED = 3
} pa_pline_label_t;

typedef enum {
	PA_PTL_UNKNWN  = 0,
	PA_PTL_INSIDE  = 1,
	PA_PTL_OUTSIDE = 2,
	PA_PTL_SHARED  = 3,
	PA_PTL_SHARED2 = 4
} pa_plinept_label_t;

#define PA_ISC_TOUCHES 99



