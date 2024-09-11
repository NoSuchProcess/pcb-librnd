#ifndef POLYBOOL2_PA_PRINTS_H
#define POLYBOOL2_PA_PRINTS_H

#include <librnd/rnd_config.h>

#ifdef RND_API_VER
#	include <librnd/core/error.h>
#	include <librnd/core/rnd_printf.h>
#endif

#define GENPRINTS_INLINE RND_INLINE

#define GENPRINTS_EXTRA_TYPE_DISPATCH \
	case PTYPE(' ', 'C'): { \
		rnd_coord_t c = va_arg(ap, rnd_coord_t); \
		len += printer(&c, format, put_ch, pctx); \
	} break;


#include <genprints/genprints.h>

#define pa_coord_is_mm(crd) ((crd) > 20000)

/*** types ***/

RND_INLINE long P_print_vect_(rnd_vector_t v, Pfmt_t *format, void (*put_ch)(void *pctx, int ch), void *pctx)
{
	char tmp[128];
	int len;

#ifdef RND_API_VER
	if (pa_coord_is_mm(v[0]) || pa_coord_is_mm(v[1]))
		len = rnd_sprintf(tmp, "%.012mm;%.012mm", v[0], v[1]);
	else
#endif
		len = sprintf(tmp, "%ld;%ld", (long)v[0], (long)v[1]);

	P_PRINT_STR(tmp);

	return len;

}


RND_INLINE long Pprint_vectp(const void *val, Pfmt_t *format, void (*put_ch)(void *pctx, int ch), void *pctx)
{
	rnd_vector_t *v = *((rnd_vector_t **)val);
	return P_print_vect_(*v, format, put_ch, pctx);
}

RND_INLINE long Pprint_vnodep(const void *val, Pfmt_t *format, void (*put_ch)(void *pctx, int ch), void *pctx)
{
	rnd_vnode_t *v = *((rnd_vnode_t **)val);

	return P_print_vect_(v->point, format, put_ch, pctx);
}

RND_INLINE long Pprint_coord(const void *val, Pfmt_t *format, void (*put_ch)(void *pctx, int ch), void *pctx)
{
	char tmp[128];
	int len;
	rnd_coord_t c = *((rnd_coord_t *)val);

#ifdef RND_API_VER
	if (pa_coord_is_mm(c))
		len = rnd_sprintf(tmp, "%.012mm", c);
	else
#endif
		len = sprintf(tmp, "%ld", (long)c);

	P_PRINT_STR(tmp);

	return len;
}

#define Pvnodep(val)       PMAGIC, PTYPE(' ', 'p'), Pprint_vnodep, NULL, (rnd_vnode_t *)(val)
#define Pvect(val)         PMAGIC, PTYPE(' ', 'p'), Pprint_vectp,  NULL, (rnd_coord_t *)(val)
#define Pcoord(val)        PMAGIC, PTYPE(' ', 'C'), Pprint_coord, NULL, (rnd_coord_t)(val)
#define Pcoord2(x, y)      Pcoord(x), ";", Pcoord(y)
#define Pdbl2(x, y)        Pdbl(x), ";", Pdbl(y)



/*** print calls ***/

static void Pput_ch_stderr(void *pctx, int ch)
{
	fputc(ch, stderr);
}

RND_INLINE long pa_trace(const char *first, ...)
{
	long len;

	va_list ap;
	va_start(ap, first);

	len = vcprints(Pput_ch_stderr, NULL, first, ap);

	va_end(ap);
	return len;
}

#ifdef RND_API_VER
RND_INLINE void pa_error(const char *msg)
{
	rnd_message(RND_MSG_ERROR, "%s", msg);
}
#else
/* pa_error() is supplied by the user */
	extern void pa_error(const char *msg);

#	define rnd_fopen(ctx, fn, mode) fopen(fn, mode)
#endif

#endif
