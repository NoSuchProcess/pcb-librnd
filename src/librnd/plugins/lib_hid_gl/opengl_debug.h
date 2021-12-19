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

static inline void gld_print(const char *fmt, ...)
{
	va_list ap;
	printf("GL: ");
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap); \
	printf("\n"); \
}


gld_print_ret_make(GLuint, "%u");

#define glCreateProgram() \
	gld_print_GLuint(glCreateProgram(), "glCreateProgram()", 0)

#define glCreateShader(arg1) \
	gld_print_GLuint(glCreateShader(arg1), "glCreateShader(%d)", arg1)


#define glTexCoordPointer(s_,t_,st_,p_) \
	do { \
		GLint size = (s_); \
		GLenum type = (t_); \
		GLsizei stride = (st_); \
		const GLvoid *ptr = (p_); \
		glTexCoordPointer(size,type,stride,ptr); \
		gld_print("glCreateProgram(%d, %d, %ld, %p)", size, type, stride, ptr); \
	} while(0) \

#define glClearColor(r_,g_,b_,a_) \
	do { \
		GLfloat r=(r_), g=(g_), b=(b_), a=(a_); \
		glClearColor(r,g,b,a); \
		gld_print("glClearColor(%f, %f, %f, %f)", r, g, b, a); \
	} while(0) \



#define glUniform4f(arg1, arg2, arg3, arg4, arg5) \
	do { \
		GLint location = (arg1); \
		GLfloat v0 = (arg2); \
		GLfloat v1 = (arg3); \
		GLfloat v2 = (arg4); \
		GLfloat v3 = (arg5); \
		glUniform4f(location, v0, v1, v2, v3); \
		gld_print("glUniform4f(%d, %f, %f, %f, %f)", location, v0, v1, v2, v3); \
	} while(0)

#define  glPushAttrib(arg1) \
	do { \
		GLbitfield mask = (arg1); \
		glPushAttrib(mask); \
		gld_print("glPushAttrib(%d)", mask); \
	} while(0)
