/* internals */

struct rnd_cvc_list_s {
	double angle;
	rnd_vnode_t *parent;
	rnd_cvc_list_t *prev, *next, *head;
	char poly, side;
};

#define ISECTED 3
#define UNKNWN  0
#define INSIDE  1
#define OUTSIDE 2
#define SHARED 3
#define SHARED2 4

#define TOUCHES 99

#define NODE_LABEL(n)  ((n)->Flags.status)
#define LABEL_NODE(n,l) ((n)->Flags.status = (l))


