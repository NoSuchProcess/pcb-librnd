#ifndef G2D_TYPECFG_H
#define G2D_TYPECFG_H
#include <math.h>
#include <opc89.h>
#include <gengeo2d/common.h>
#include <librnd/core/globalconst.h>
#include <librnd/core/compat_misc.h>

typedef funcops double g2d_coord_t;
typedef funcops double g2d_calc_t;
typedef funcops double g2d_offs_t;
typedef funcops double g2d_angle_t;

#define G2D_COORD_MAX RND_COORD_MAX
#define G2D_COORD_TOL2 (0.01)

G2D_INLINE g2d_coord_t g2d_round_coord(g2d_calc_t x) { return rnd_round(x); }
G2D_INLINE g2d_coord_t g2d_round_coord_down(g2d_calc_t x) { return floor(x); }
G2D_INLINE g2d_coord_t g2d_round_coord_up(g2d_calc_t x) { return ceil(x); }

G2D_INLINE opfunc g2d_calc_t g2d_calc_t_div_g2d_calc_t(g2d_calc_t a, g2d_calc_t b) { return a/b; }
G2D_INLINE opfunc g2d_calc_t g2d_calc_t_mul_g2d_calc_t(g2d_calc_t a, g2d_calc_t b) { return a*b; }
G2D_INLINE opfunc g2d_calc_t g2d_calc_t_sub_g2d_calc_t(g2d_calc_t a, g2d_calc_t b) { return a-b; }
G2D_INLINE opfunc g2d_calc_t g2d_calc_t_add_g2d_calc_t(g2d_calc_t a, g2d_calc_t b) { return a+b; }
G2D_INLINE opfunc g2d_calc_t g2d_calc_t_neg(g2d_calc_t a) { return -a; }
G2D_INLINE opfunc int g2d_calc_t_eq_g2d_calc_t(g2d_calc_t a, g2d_calc_t b) { return a==b; }
G2D_INLINE opfunc int g2d_calc_t_neq_g2d_calc_t(g2d_calc_t a, g2d_calc_t b) { return a!=b; }
G2D_INLINE opfunc int g2d_calc_t_lt_g2d_calc_t(g2d_calc_t a, g2d_calc_t b) { return a<b; }
G2D_INLINE opfunc int g2d_calc_t_lte_g2d_calc_t(g2d_calc_t a, g2d_calc_t b) { return a<=b; }
G2D_INLINE opfunc int g2d_calc_t_gt_g2d_calc_t(g2d_calc_t a, g2d_calc_t b) { return a>b; }
G2D_INLINE opfunc int g2d_calc_t_gte_g2d_calc_t(g2d_calc_t a, g2d_calc_t b) { return a>=b; }
G2D_INLINE opfunc double double_convfrom_g2d_calc_t(g2d_calc_t a) { return a; }
G2D_INLINE opfunc g2d_calc_t g2d_calc_t_convfrom_double(double a) { return a; }
G2D_INLINE opfunc g2d_calc_t g2d_calc_t_convfrom_g2d_coord_t(g2d_coord_t a) { return a; }
G2D_INLINE opfunc g2d_calc_t g2d_calc_t_convfrom_g2d_angle_t(g2d_angle_t a) { return a; }
G2D_INLINE opfunc g2d_calc_t g2d_calc_t_mul_g2d_offs_t(g2d_calc_t a, g2d_offs_t b) { return a*b; }

G2D_INLINE opfunc g2d_coord_t g2d_coord_t_add_g2d_coord_t(g2d_coord_t a, g2d_coord_t b) { return a+b; }
G2D_INLINE opfunc g2d_coord_t g2d_coord_t_sub_g2d_coord_t(g2d_coord_t a, g2d_coord_t b) { return a-b; }
G2D_INLINE opfunc int g2d_coord_t_eq_g2d_coord_t(g2d_coord_t a, g2d_coord_t b) { return a==b; }
G2D_INLINE opfunc int g2d_coord_t_neq_g2d_coord_t(g2d_coord_t a, g2d_coord_t b) { return a!=b; }
G2D_INLINE opfunc int g2d_coord_t_lt_g2d_coord_t(g2d_coord_t a, g2d_coord_t b) { return a<b; }
G2D_INLINE opfunc int g2d_coord_t_lte_g2d_coord_t(g2d_coord_t a, g2d_coord_t b) { return a<=b; }
G2D_INLINE opfunc int g2d_coord_t_gt_g2d_coord_t(g2d_coord_t a, g2d_coord_t b) { return a>b; }
G2D_INLINE opfunc int g2d_coord_t_gte_g2d_coord_t(g2d_coord_t a, g2d_coord_t b) { return a>=b; }
G2D_INLINE opfunc g2d_coord_t g2d_coord_t_neg(g2d_coord_t a) { return -a; }
G2D_INLINE opfunc g2d_coord_t g2d_coord_t_convfrom_g2d_calc_t(g2d_calc_t a) { return g2d_round_coord(a); }
G2D_INLINE opfunc g2d_coord_t g2d_coord_t_convfrom_int(int a) { return a; }

G2D_INLINE opfunc g2d_angle_t g2d_angle_t_add_g2d_angle_t(g2d_angle_t a, g2d_angle_t b) { return a+b; }
G2D_INLINE opfunc g2d_angle_t g2d_angle_t_sub_g2d_angle_t(g2d_angle_t a, g2d_angle_t b) { return a-b; }
G2D_INLINE opfunc g2d_angle_t g2d_angle_t_div_g2d_angle_t(g2d_angle_t a, g2d_angle_t b) { return a/b; }
G2D_INLINE opfunc int g2d_angle_t_eq_g2d_angle_t(g2d_angle_t a, g2d_angle_t b) { return a==b; }
G2D_INLINE opfunc int g2d_angle_t_lt_g2d_angle_t(g2d_angle_t a, g2d_angle_t b) { return a<b; }
G2D_INLINE opfunc int g2d_angle_t_lte_g2d_angle_t(g2d_angle_t a, g2d_angle_t b) { return a<=b; }
G2D_INLINE opfunc int g2d_angle_t_gt_g2d_angle_t(g2d_angle_t a, g2d_angle_t b) { return a>b; }
G2D_INLINE opfunc int g2d_angle_t_gte_g2d_angle_t(g2d_angle_t a, g2d_angle_t b) { return a>=b; }
G2D_INLINE opfunc g2d_angle_t g2d_angle_t_convfrom_double(double a) { return a; }
G2D_INLINE opfunc g2d_angle_t g2d_angle_t_convfrom_g2d_calc_t(g2d_calc_t a) { return a; }
G2D_INLINE opfunc double double_convfrom_g2d_angle_t(g2d_angle_t a) { return a; }
G2D_INLINE opfunc g2d_angle_t g2d_angle_t_neg(g2d_angle_t a) { return -a; }
G2D_INLINE opfunc g2d_angle_t g2d_angle_t_mul_g2d_offs_t(g2d_angle_t a, g2d_offs_t b) { return a*b; }

G2D_INLINE opfunc int g2d_offs_t_eq_g2d_offs_t(g2d_offs_t a, g2d_offs_t b) { return a == b; }
G2D_INLINE opfunc int g2d_offs_t_lt_g2d_offs_t(g2d_offs_t a, g2d_offs_t b) { return a < b; }
G2D_INLINE opfunc int g2d_offs_t_lte_g2d_offs_t(g2d_offs_t a, g2d_offs_t b) { return a <= b; }
G2D_INLINE opfunc int g2d_offs_t_gt_g2d_offs_t(g2d_offs_t a, g2d_offs_t b) { return a > b; }
G2D_INLINE opfunc int g2d_offs_t_gte_g2d_offs_t(g2d_offs_t a, g2d_offs_t b) { return a >= b; }
G2D_INLINE opfunc g2d_offs_t g2d_offs_t_convfrom_g2d_angle_t(g2d_angle_t a) { return a; }
G2D_INLINE opfunc g2d_offs_t g2d_offs_t_convfrom_double(double a) { return a; }
G2D_INLINE opfunc g2d_offs_t g2d_offs_t_convfrom_g2d_calc_t(g2d_calc_t a) { return a; }

G2D_INLINE g2d_calc_t g2d_sqrt(g2d_calc_t a) { return g2d_calc_t_convfrom_double(sqrt(double_convfrom_g2d_calc_t(a))); }
G2D_INLINE g2d_calc_t g2d_cos(g2d_angle_t a) { return g2d_calc_t_convfrom_double(cos(double_convfrom_g2d_angle_t(a))); }
G2D_INLINE g2d_calc_t g2d_sin(g2d_angle_t a) { return g2d_calc_t_convfrom_double(sin(double_convfrom_g2d_angle_t(a))); }
G2D_INLINE g2d_angle_t g2d_acos(g2d_calc_t a) { return g2d_angle_t_convfrom_double(acos(double_convfrom_g2d_calc_t(a))); }
G2D_INLINE g2d_angle_t g2d_asin(g2d_calc_t a) { return g2d_angle_t_convfrom_double(asin(double_convfrom_g2d_calc_t(a))); }
G2D_INLINE g2d_angle_t g2d_atan2(g2d_calc_t a, g2d_calc_t b) { return g2d_angle_t_convfrom_double(atan2(double_convfrom_g2d_calc_t(a), double_convfrom_g2d_calc_t(b))); }

#endif

