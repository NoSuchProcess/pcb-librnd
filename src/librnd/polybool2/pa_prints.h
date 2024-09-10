#ifndef POLYBOOL2_PA_PRINTS_H
#define POLYBOOL2_PA_PRINTS_H

#include <librnd/rnd_config.h>

#define GENPRINTS_INLINE RND_INLINE

#define GENPRINTS_EXTRA_TYPE_DISPATCH \
	case PTYPE(' ', 'C'): { \
		rnd_coord_t c = va_arg(ap, rnd_coord_t); \
		len += printer(&c, format, put_ch, pctx); \
	} break;


#include <genprints/genprints.h>
#include <librnd/core/rnd_printf.h>

#define pa_coord_is_mm(crd) ((crd) > 20000)

/*** types ***/

RND_INLINE long Pprint_vnodep(const void *val, Pfmt_t *format, void (*put_ch)(void *pctx, int ch), void *pctx)
{
	char tmp[128];
	int len;
	rnd_vnode_t *v = *((rnd_vnode_t **)val);

	if (pa_coord_is_mm(v->point[0]) || pa_coord_is_mm(v->point[1]))
		len = rnd_sprintf(tmp, "%.012mm;%.012mm", v->point[0], v->point[1]);
	else
		len = rnd_sprintf(tmp, "%ld;%ld", v->point[0], v->point[1]);

	P_PRINT_STR(tmp);

	return len;
}

RND_INLINE long Pprint_coord(const void *val, Pfmt_t *format, void (*put_ch)(void *pctx, int ch), void *pctx)
{
	char tmp[128];
	int len;
	rnd_coord_t c = *((rnd_coord_t *)val);

	if (pa_coord_is_mm(c))
		len = rnd_sprintf(tmp, "%.012mm", c);
	else
		len = rnd_sprintf(tmp, "%ld", (long)c);

	P_PRINT_STR(tmp);

	return len;
}

#define Pvnodep(val)       PMAGIC, PTYPE(' ', 'p'), Pprint_vnodep, NULL, (rnd_vnode_t *)(val)
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

#endif
