#define ROUND(a) (long)((a) > 0 ? ((a)+0.5) : ((a)-0.5))

#define EPSILON (1E-8)

#undef min
#undef max
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

