#define gld_print_ret_make(type, type_fmt) \
static inline type gld_print_ ## type (type res, const char *fmt, ...) \
{ \
	va_list ap; \
	printf("GL: "); \
	va_start(ap, fmt); \
	vprintf(fmt, ap); \
	va_end(ap); \
	printf(" = " type_fmt "\n", res); \
	return res; \
} \

gld_print_ret_make(GLuint, "%u");

#define glCreateProgram() \
	gld_print_GLuint(glCreateProgram(), "glCreateProgram()", 0)

