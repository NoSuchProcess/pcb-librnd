#ifndef OPENGL_DEBUG_H
#define OPENGL_DEBUG_H

#include "opengl.h"

#ifdef OPENGL_DEBUG

#include <stdarg.h>

typedef const GLubyte * GLubyteCP;

#define gld_print_ret_make(type, type_fmt) \
RND_INLINE type gld_print_ ## type (type res, const char *fmt, ...) \
{ \
	va_list ap; \
	printf("GL: "); \
	va_start(ap, fmt); \
	vprintf(fmt, ap); \
	va_end(ap); \
	printf(" = " type_fmt "\n", res); \
	return res; \
} \

RND_INLINE void gld_print(const char *fmt, ...)
{
	va_list ap;
	printf("GL: ");
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap); \
	printf("\n"); \
}


gld_print_ret_make(GLuint, "%u");
gld_print_ret_make(GLint, "%d");
gld_print_ret_make(GLubyteCP, "'%s'");

#define glCreateProgram() \
	gld_print_GLuint(glCreateProgram(), "glCreateProgram()", 0)

#define glCreateShader(arg1) \
	gld_print_GLuint(glCreateShader(arg1), "glCreateShader(%d)", arg1)


#define  glGetString(arg1) \
	gld_print_GLubyteCP(glGetString(arg1), "glGetString(%d = %s)", arg1, #arg1)
//----FIXME----: GLAPI const GLubyte *APIENTRY glGetString (GLenum name);

#define  glGetUniformLocation(arg1, arg2) \
	gld_print_GLint(glGetUniformLocation(arg1, arg2), "glGetUniformLocation(%u, '%s')", arg1, arg2)




#define glTexCoordPointer(s_,t_,st_,p_) \
	do { \
		GLint size__ = (s_); \
		GLenum type__ = (t_); \
		GLsizei stride__ = (st_); \
		const GLvoid *ptr__ = (p_); \
		glTexCoordPointer(size__, type__, stride__, ptr__); \
		gld_print("glCreateProgram(%d, %d, %ld, %p)", size__, type__, stride__, ptr__); \
	} while(0) \

#define glClearColor(r_,g_,b_,a_) \
	do { \
		GLfloat r__= (r_), g__ = (g_), b__ = (b_), a__ = (a_); \
		glClearColor(r__, g__, b__, a__); \
		gld_print("glClearColor(%f, %f, %f, %f)", r__, g__, b__, a__); \
	} while(0) \



#define glUniform4f(arg1, arg2, arg3, arg4, arg5) \
	do { \
		GLint location__ = (arg1); \
		GLfloat v0__ = (arg2); \
		GLfloat v1__ = (arg3); \
		GLfloat v2__ = (arg4); \
		GLfloat v3__ = (arg5); \
		glUniform4f(location__, v0__, v1__, v2__, v3__); \
		gld_print("glUniform4f(%d, %f, %f, %f, %f)", location__, v0__, v1__, v2__, v3__); \
	} while(0)

#define  glPushAttrib(arg1) \
	do { \
		GLbitfield mask__ = (arg1); \
		glPushAttrib(mask__); \
		gld_print("glPushAttrib(%d)", mask__); \
	} while(0)

#define  glPopAttrib() \
	do { \
		glPopAttrib(); \
		gld_print("glPopAttrib()", 0); \
	} while(0)

#define  glPushMatrix() \
	do { \
		glPushMatrix(); \
		gld_print("glPushMatrix(...)"); \
	} while(0)

#define  glPopMatrix() \
	do { \
		glPopMatrix(); \
		gld_print("glPopMatrix()", 0); \
	} while(0)

#define  glLoadIdentity() \
	do { \
		glLoadIdentity(); \
		gld_print("glLoadIdentity()", 0); \
	} while(0)

#define  glEnable(arg1) \
	do { \
		GLenum cap__ = (arg1); \
		glEnable(cap__); \
		gld_print("glEnable(%d=%s)", cap__, #arg1); \
	} while(0)

#define  glStencilFunc(arg1, arg2, arg3) \
	do { \
		GLenum func__ = (arg1); \
		GLint ref__ = (arg2); \
		GLuint mask__ = (arg3); \
		glStencilFunc(func__, ref__, mask__); \
		gld_print("glStencilFunc(%d %d %u)", func__, ref__, mask__); \
	} while(0)

#define  glStencilOp(arg1, arg2, arg3) \
	do { \
		GLenum fail__ = (arg1); \
		GLenum zfail__ = (arg2); \
		GLenum zpass__ = (arg3); \
		glStencilOp(fail__, zfail__, zpass__); \
		gld_print("glStencilOp(%d, %d, %d)", fail__, zfail__, zpass__); \
	} while(0)

#define  glClearStencil(arg1) \
	do { \
		GLint s__ = (arg1); \
		glClearStencil(s__); \
		gld_print("glClearStencil(%d)", s__); \
	} while(0)

#define  glStencilMask(arg1) \
	do { \
		GLuint mask__ = (arg1); \
		glStencilMask(mask__); \
		gld_print("glStencilMask(%d)", mask__); \
	} while(0)

#define  glDeleteBuffers(arg1, arg2) \
	do { \
		GLsizei n__ = (arg1); \
		const GLuint * buffers__ = (arg2); \
		glDeleteBuffers(n__, buffers__); \
		gld_print("glDeleteBuffers(%ld, %u)", (long)n__, buffers__); \
	} while(0)

#define  glGetProgramInfoLog(arg1, arg2, arg3, arg4) \
	do { \
		GLuint program__ = (arg1); \
		GLsizei bufSize__ = (arg2); \
		GLsizei * length__ = (arg3); \
		GLchar * infoLog__ = (arg4); \
		glGetProgramInfoLog(program__, bufSize__, length__, infoLog__); \
		gld_print("glGetProgramInfoLog(%d, %ld, %p, '%s')", program__, bufSize__, length__, infoLog__); \
	} while(0)

#define  glViewport(arg1, arg2, arg3, arg4) \
	do { \
		GLint x__ = (arg1); \
		GLint y__ = (arg2); \
		GLsizei width__ = (arg3); \
		GLsizei height__ = (arg4); \
		glViewport(x__, y__, width__, height__); \
		gld_print("glViewport(%d, %d, %ld, %ld)", x__, y__, (long)width__, (long)height__); \
	} while(0)

#define  glLogicOp(arg1) \
	do { \
		GLenum opcode__ = (arg1); \
		glLogicOp(opcode__); \
		gld_print("glLogicOp(%d)", opcode__); \
	} while(0)

#define  glDeleteShader(arg1) \
	do { \
		GLuint shader__ = (arg1); \
		glDeleteShader(shader__); \
		gld_print("glDeleteShader(%u)", shader__); \
	} while(0)

#define  glBindTexture(arg1, arg2) \
	do { \
		GLenum target__ = (arg1); \
		GLuint texture__ = (arg2); \
		glBindTexture(target__, texture__); \
		gld_print("glBindTexture(%d, %u)", target__, texture__); \
	} while(0)

#define  glTexParameterf(arg1, arg2, arg3) \
	do { \
		GLenum target__ = (arg1); \
		GLenum pname__ = (arg2); \
		GLfloat param__ = (arg3); \
		glTexParameterf(target__, pname__, param__); \
		gld_print("glTexParameterf(%d, %d, %f)", target__, pname__, param__); \
	} while(0)

#define  glScalef(arg1, arg2, arg3) \
	do { \
		GLfloat x__ = (arg1); \
		GLfloat y__ = (arg2); \
		GLfloat z__ = (arg3); \
		glScalef(x__, y__, z__); \
		gld_print("glScalef(%f, %f, %f)", x__, y__, z__); \
	} while(0)

#define  glColorPointer(arg1, arg2, arg3, arg4) \
	do { \
		GLint size__ = (arg1); \
		GLenum type__ = (arg2); \
		GLsizei stride__ = (arg3); \
		const GLvoid * ptr = (arg4); \
		glColorPointer(size__, type__, stride__, ptr); \
		gld_print("glColorPointer(%d, %d, %ld, %p)", size__, type__, (long)stride__, ptr); \
	} while(0)

#define  glColor3f(arg1, arg2, arg3) \
	do { \
		GLfloat red__ = (arg1); \
		GLfloat green__ = (arg2); \
		GLfloat blue__ = (arg3); \
		glColor3f(red__, green__, blue__); \
		gld_print("glColor3f(%f, %f, %f)", red__, green__, blue__); \
	} while(0)

#define  glAlphaFunc(arg1, arg2) \
	do { \
		GLenum func__ = (arg1); \
		GLclampf ref__ = (arg2); \
		glAlphaFunc(func__, ref__); \
		gld_print("glAlphaFunc(%d, %f)", func__, ref__); \
	} while(0)

#define  glVertexAttribPointer(arg1, arg2, arg3, arg4, arg5, arg6) \
	do { \
		GLuint index__ = (arg1); \
		GLint size__ = (arg2); \
		GLenum type__ = (arg3); \
		GLboolean normalized__ = (arg4); \
		GLsizei stride__ = (arg5); \
		const void *pointer = (arg6); \
		glVertexAttribPointer(index__, size__, type__, normalized__, stride__, pointer); \
		gld_print("glVertexAttribPointer(%u, %d, %d, %d, %ld, %p)", index__, size__, type__, normalized__, (long)stride__, pointer); \
	} while(0)

#define  glDisable(arg1) \
	do { \
		GLenum cap__ = (arg1); \
		glDisable(cap__); \
		gld_print("glDisable(%d=%s)", cap__, #arg1); \
	} while(0)

#define  glTexImage2D(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) \
	do { \
		GLenum target__ = (arg1); \
		GLint level__ = (arg2); \
		GLint internalformat__ = (arg3); \
		GLsizei width__ = (arg4); \
		GLsizei height__ = (arg5); \
		GLint border__ = (arg6); \
		GLenum format__ = (arg7); \
		GLenum type__ = (arg8); \
		const void *pixels = (arg9); \
		glTexImage2D(target__, level__, internalformat__, width__, height__, border__, format__, type__, pixels); \
		gld_print("glTexImage2D(%d, %d, %d, %ld, %ld, %d, %d, %d, %p)", target__, level__, internalformat__, (long)width__, (long)height__, border__, format__, type__, pixels); \
	} while(0)

#define  glGenVertexArrays(arg1, arg2) \
	do { \
		GLsizei n__ = (arg1); \
		GLuint * arrays__ = (arg2); \
		glGenVertexArrays(n__, arrays__); \
		gld_print("glGenVertexArrays(%ld, %u)", (long)n__, arrays__); \
	} while(0)

#define  glGetShaderiv(arg1, arg2, arg3) \
	do { \
		GLuint shader__ = (arg1); \
		GLenum pname__ = (arg2); \
		GLint * param__s = (arg3); \
		glGetShaderiv(shader__, pname__, param__s); \
		gld_print("glGetShaderiv(%u, %d, %p)", shader__, pname__, param__s); \
	} while(0)

#define  glOrtho(arg1, arg2, arg3, arg4, arg5, arg6) \
	do { \
		GLdouble left__ = (arg1); \
		GLdouble right__ = (arg2); \
		GLdouble bottom__ = (arg3); \
		GLdouble top__ = (arg4); \
		GLdouble near_val__ = (arg5); \
		GLdouble far_val__ = (arg6); \
		glOrtho(left__, right__, bottom__, top__, near_val__, far_val__); \
		gld_print("glOrtho(%f, %f, %f, %f, %f, %f)", left__, right__, bottom__, top__, near_val__, far_val__); \
	} while(0)

#define  glEnableClientState(arg1) \
	do { \
		GLenum cap__ = (arg1); \
		glEnableClientState(cap__); \
		gld_print("glEnableClientState(%d = %s)", cap__, #arg1); \
	} while(0)

#define  glColorMask(arg1, arg2, arg3, arg4) \
	do { \
		GLboolean red__ = (arg1); \
		GLboolean green__ = (arg2); \
		GLboolean blue__ = (arg3); \
		GLboolean alpha__ = (arg4); \
		glColorMask(red__, green__, blue__, alpha__); \
		gld_print("glColorMask(%d, %d, %d, %d)", red__, green__, blue__, alpha__); \
	} while(0)

#define  glShaderSource(arg1, arg2, arg3, arg4) \
	do { \
		GLuint shader__ = (arg1); \
		GLsizei count__ = (arg2); \
		const GLchar * const *string__ = (arg3); \
		const GLint * length__ = (arg4); \
		glShaderSource(shader__, count__, string__, length__); \
		gld_print("glShaderSource(%u, %ld, '%s', %p)", shader__, (long)count__, *string__, length__); \
	} while(0)

#define  glDrawArrays(arg1, arg2, arg3) \
	do { \
		GLenum mode__ = (arg1); \
		GLint first__ = (arg2); \
		GLsizei count__ = (arg3); \
		glDrawArrays(mode__, first__, count__); \
		gld_print("glDrawArrays(%d, %d, %ld)", mode__, first__, (long)count__); \
	} while(0)

#define  glVertexPointer(arg1, arg2, arg3, arg4) \
	do { \
		GLint size__ = (arg1); \
		GLenum type__ = (arg2); \
		GLsizei stride__ = (arg3); \
		const GLvoid * ptr__ = (arg4); \
		glVertexPointer(size__, type__, stride__, ptr__); \
		gld_print("glVertexPointer(%d, %d, %ld, %p)", size__, type__, (long)stride__, ptr__); \
	} while(0)


#define  glGetIntegerv(arg1, arg2) \
	do { \
		GLenum pname__ = (arg1); \
		GLint * data__ = (arg2); \
		glGetIntegerv(pname__, data__); \
		gld_print("glGetIntegerv(%d = %s, %d)", pname__, #arg1, *data__); \
	} while(0)

#define  glLinkProgram(arg1) \
	do { \
		GLuint program__ = (arg1); \
		glLinkProgram(program__); \
		gld_print("glLinkProgram(%u)", program__); \
	} while(0)

#define  glBindVertexArray(arg1) \
	do { \
		GLuint array__ = (arg1); \
		glBindVertexArray(array__); \
		gld_print("glBindVertexArray(%u)", array__); \
	} while(0)

#define  glDetachShader(arg1, arg2) \
	do { \
		GLuint program__ = (arg1); \
		GLuint shader__ = (arg2); \
		glDetachShader(program__, shader__); \
		gld_print("glDetachShader(%u, %u)", program__, shader__); \
	} while(0)

#define  glGenTextures(arg1, arg2) \
	do { \
		GLsizei n__ = (arg1); \
		GLuint * texture__s = (arg2); \
		glGenTextures(n__, texture__s); \
		gld_print("glGenTextures(%ld, %p)", n__, texture__s); \
	} while(0)

#define  glGetProgramiv(arg1, arg2, arg3) \
	do { \
		GLuint program__ = (arg1); \
		GLenum pname__ = (arg2); \
		GLint * param__s = (arg3); \
		glGetProgramiv(program__, pname__, param__s); \
		gld_print("glGetProgramiv(%u, %d, %p)", program__, pname__, param__s); \
	} while(0)

#define  glClear(arg1) \
	do { \
		GLbitfield mask__ = (arg1); \
		glClear(mask__); \
		gld_print("glClear(%d)", mask__); \
	} while(0)

#define  glUseProgram(arg1) \
	do { \
		GLuint program__ = (arg1); \
		glUseProgram(program__); \
		gld_print("glUseProgram(%u)", program__); \
	} while(0)

#define  glFlush() \
	do { \
		glFlush(); \
		gld_print("glFlush()", 0); \
	} while(0)

#define  glBufferData(arg1, arg2, arg3, arg4) \
	do { \
		GLenum target__ = (arg1); \
		GLsizeiptr size__ = (arg2); \
		const void *data__ = (arg3); \
		GLenum usage__ = (arg4); \
		glBufferData(target__, size__, data__, usage__); \
		gld_print("glBufferData(%d, %ld, %p, %d = %s)", target__, (long)size__, data__, usage__, #arg4); \
	} while(0)

#define  glAttachShader(arg1, arg2) \
	do { \
		GLuint program__ = (arg1); \
		GLuint shader__ = (arg2); \
		glAttachShader(program__, shader__); \
		gld_print("glAttachShader(%u, %u)", program__, shader__); \
	} while(0)

#define  glBindBuffer(arg1, arg2) \
	do { \
		GLenum target__ = (arg1); \
		GLuint buffer__ = (arg2); \
		glBindBuffer(target__, buffer__); \
		gld_print("glBindBuffer(%d, %u)", target__, buffer__); \
	} while(0)

#define  glMatrixMode(arg1) \
	do { \
		GLenum mode__ = (arg1); \
		glMatrixMode(mode__); \
		gld_print("glMatrixMode(%d = %s)", mode__, #arg1); \
	} while(0)

#define  glDisableClientState(arg1) \
	do { \
		GLenum cap__ = (arg1); \
		glDisableClientState(cap__); \
		gld_print("glDisableClientState(%d = %s)", cap__, #arg1); \
	} while(0)

#define  glGetShaderInfoLog(arg1, arg2, arg3, arg4) \
	do { \
		GLuint shader__ = (arg1); \
		GLsizei bufSize__ = (arg2); \
		GLsizei * length__ = (arg3); \
		GLchar * infoLog__ = (arg4); \
		glGetShaderInfoLog(shader__, bufSize__, length__, infoLog__); \
		gld_print("glGetShaderInfoLog(%u, %ld, %ld, %p)", shader__, (long)bufSize__, (long)length__, infoLog__); \
	} while(0)

#define  glEnableVertexAttribArray(arg1) \
	do { \
		GLuint index__ = (arg1); \
		glEnableVertexAttribArray(index__); \
		gld_print("glEnableVertexAttribArray(%u)", index__); \
	} while(0)

#define  glCompileShader(arg1) \
	do { \
		GLuint shader__ = (arg1); \
		glCompileShader(shader__); \
		gld_print("glCompileShader(%u)", shader__); \
	} while(0)

#define  glTranslatef(arg1, arg2, arg3) \
	do { \
		GLfloat x__ = (arg1); \
		GLfloat y__ = (arg2); \
		GLfloat z__ = (arg3); \
		glTranslatef(x__, y__, z__); \
		gld_print("glTranslatef(%f, %f, %f)", x__, y__, z__); \
	} while(0)

#define  glGenBuffers(arg1, arg2) \
	do { \
		GLsizei n__ = (arg1); \
		GLuint * buffer__s = (arg2); \
		glGenBuffers(n__, buffer__s); \
		gld_print("glGenBuffers(%ld, %p)", (long)n__, buffer__s); \
	} while(0)

#define  glDeleteProgram(arg1) \
	do { \
		GLuint program__ = (arg1); \
		glDeleteProgram(program__); \
		gld_print("glDeleteProgram(%u)", program__); \
	} while(0)

#define  glBlendFunc(arg1, arg2) \
	do { \
		GLenum sfactor__ = (arg1); \
		GLenum dfactor__ = (arg2); \
		glBlendFunc(sfactor__, dfactor__); \
		gld_print("glBlendFunc(%d = %s, %d = %s)", sfactor__, #arg1, dfactor__, #arg2); \
	} while(0)

#endif

#endif
