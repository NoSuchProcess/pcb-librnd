#ifndef OPENGL_DEBUG_H
#define OPENGL_DEBUG_H

#include "opengl.h"

#ifdef OPENGL_DEBUG

#include <stdarg.h>

typedef const GLubyte * GLubyteCP;

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
		GLenum cap = (arg1); \
		glEnable(cap); \
		gld_print("glEnable(%d=%s)", cap, #arg1); \
	} while(0)

#define  glStencilFunc(arg1, arg2, arg3) \
	do { \
		GLenum func = (arg1); \
		GLint ref = (arg2); \
		GLuint mask = (arg3); \
		glStencilFunc(func, ref, mask); \
		gld_print("glStencilFunc(%d %d %u)", func, ref, mask); \
	} while(0)

#define  glStencilOp(arg1, arg2, arg3) \
	do { \
		GLenum fail = (arg1); \
		GLenum zfail = (arg2); \
		GLenum zpass = (arg3); \
		glStencilOp(fail, zfail, zpass); \
		gld_print("glStencilOp(%d, %d, %d)", fail, zfail, zpass); \
	} while(0)

#define  glClearStencil(arg1) \
	do { \
		GLint s = (arg1); \
		glClearStencil(s); \
		gld_print("glClearStencil(%d)", s); \
	} while(0)

#define  glStencilMask(arg1) \
	do { \
		GLuint mask = (arg1); \
		glStencilMask(mask); \
		gld_print("glStencilMask(%d)", mask); \
	} while(0)

#define  glDeleteBuffers(arg1, arg2) \
	do { \
		GLsizei n = (arg1); \
		const GLuint * buffers = (arg2); \
		glDeleteBuffers(n, buffers); \
		gld_print("glDeleteBuffers(%ld, %u)", (long)n, buffers); \
	} while(0)

#define  glGetProgramInfoLog(arg1, arg2, arg3, arg4) \
	do { \
		GLuint program = (arg1); \
		GLsizei bufSize = (arg2); \
		GLsizei * length = (arg3); \
		GLchar * infoLog = (arg4); \
		glGetProgramInfoLog(program, bufSize, length, infoLog); \
		gld_print("glGetProgramInfoLog(%d, %ld, %p, '%s')", program, bufSize, length, infoLog); \
	} while(0)

#define  glViewport(arg1, arg2, arg3, arg4) \
	do { \
		GLint x = (arg1); \
		GLint y = (arg2); \
		GLsizei width = (arg3); \
		GLsizei height = (arg4); \
		glViewport(x, y, width, height); \
		gld_print("glViewport(%d, %d, %ld, %ld)", x, y, (long)width, (long)height); \
	} while(0)

#define  glLogicOp(arg1) \
	do { \
		GLenum opcode = (arg1); \
		glLogicOp(opcode); \
		gld_print("glLogicOp(%d)", opcode); \
	} while(0)

#define  glDeleteShader(arg1) \
	do { \
		GLuint shader = (arg1); \
		glDeleteShader(shader); \
		gld_print("glDeleteShader(%u)", shader); \
	} while(0)

#define  glBindTexture(arg1, arg2) \
	do { \
		GLenum target = (arg1); \
		GLuint texture = (arg2); \
		glBindTexture(target, texture); \
		gld_print("glBindTexture(%d, %u)", target, texture); \
	} while(0)

#define  glTexParameterf(arg1, arg2, arg3) \
	do { \
		GLenum target = (arg1); \
		GLenum pname = (arg2); \
		GLfloat param = (arg3); \
		glTexParameterf(target, pname, param); \
		gld_print("glTexParameterf(%d, %d, %f)", target, pname, param); \
	} while(0)

#define  glScalef(arg1, arg2, arg3) \
	do { \
		GLfloat x = (arg1); \
		GLfloat y = (arg2); \
		GLfloat z = (arg3); \
		glScalef(x, y, z); \
		gld_print("glScalef(%f, %f, %f)", x, y, z); \
	} while(0)

#define  glColorPointer(arg1, arg2, arg3, arg4) \
	do { \
		GLint size = (arg1); \
		GLenum type = (arg2); \
		GLsizei stride = (arg3); \
		const GLvoid * ptr = (arg4); \
		glColorPointer(size, type, stride, ptr); \
		gld_print("glColorPointer(%d, %d, %ld, %p)", size, type, (long)stride, ptr); \
	} while(0)

#define  glColor3f(arg1, arg2, arg3) \
	do { \
		GLfloat red = (arg1); \
		GLfloat green = (arg2); \
		GLfloat blue = (arg3); \
		glColor3f(red, green, blue); \
		gld_print("glColor3f(%f, %f, %f)", red, green, blue); \
	} while(0)

#define  glAlphaFunc(arg1, arg2) \
	do { \
		GLenum func = (arg1); \
		GLclampf ref = (arg2); \
		glAlphaFunc(func, ref); \
		gld_print("glAlphaFunc(%d, %f)", func, ref); \
	} while(0)

#define  glVertexAttribPointer(arg1, arg2, arg3, arg4, arg5, arg6) \
	do { \
		GLuint index = (arg1); \
		GLint size = (arg2); \
		GLenum type = (arg3); \
		GLboolean normalized = (arg4); \
		GLsizei stride = (arg5); \
		const void *pointer = (arg6); \
		glVertexAttribPointer(index, size, type, normalized, stride, pointer); \
		gld_print("glVertexAttribPointer(%u, %d, %d, %d, %ld, %p)", index, size, type, normalized, (long)stride, pointer); \
	} while(0)

#define  glDisable(arg1) \
	do { \
		GLenum cap = (arg1); \
		glDisable(cap); \
		gld_print("glDisable(%d=%s)", cap, #arg1); \
	} while(0)

#define  glTexImage2D(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) \
	do { \
		GLenum target = (arg1); \
		GLint level = (arg2); \
		GLint internalformat = (arg3); \
		GLsizei width = (arg4); \
		GLsizei height = (arg5); \
		GLint border = (arg6); \
		GLenum format = (arg7); \
		GLenum type = (arg8); \
		const void *pixels = (arg9); \
		glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels); \
		gld_print("glTexImage2D(%d, %d, %d, %ld, %ld, %d, %d, %d, %p)", target, level, internalformat, (long)width, (long)height, border, format, type, pixels); \
	} while(0)

#define  glGenVertexArrays(arg1, arg2) \
	do { \
		GLsizei n = (arg1); \
		GLuint * arrays = (arg2); \
		glGenVertexArrays(n, arrays); \
		gld_print("glGenVertexArrays(%ld, %u)", (long)n, arrays); \
	} while(0)

#define  glGetShaderiv(arg1, arg2, arg3) \
	do { \
		GLuint shader = (arg1); \
		GLenum pname = (arg2); \
		GLint * params = (arg3); \
		glGetShaderiv(shader, pname, params); \
		gld_print("glGetShaderiv(%u, %d, %p)", shader, pname, params); \
	} while(0)

#define  glOrtho(arg1, arg2, arg3, arg4, arg5, arg6) \
	do { \
		GLdouble left = (arg1); \
		GLdouble right = (arg2); \
		GLdouble bottom = (arg3); \
		GLdouble top = (arg4); \
		GLdouble near_val = (arg5); \
		GLdouble far_val = (arg6); \
		glOrtho(left, right, bottom, top, near_val, far_val); \
		gld_print("glOrtho(%f, %f, %f, %f, %f, %f)", left, right, bottom, top, near_val, far_val); \
	} while(0)

#define  glEnableClientState(arg1) \
	do { \
		GLenum cap = (arg1); \
		glEnableClientState(cap); \
		gld_print("glEnableClientState(%d = %s)", cap, #arg1); \
	} while(0)

#define  glColorMask(arg1, arg2, arg3, arg4) \
	do { \
		GLboolean red = (arg1); \
		GLboolean green = (arg2); \
		GLboolean blue = (arg3); \
		GLboolean alpha = (arg4); \
		glColorMask(red, green, blue, alpha); \
		gld_print("glColorMask(%f, %f, %f, %f)", red, green, blue, alpha); \
	} while(0)

#define  glShaderSource(arg1, arg2, arg3, arg4) \
	do { \
		GLuint shader = (arg1); \
		GLsizei count = (arg2); \
		const GLchar * const *string = (arg3); \
		const GLint * length = (arg4); \
		glShaderSource(shader, count, string, length); \
		gld_print("glShaderSource(%u, %ld, '%s', %p)", shader, (long)count, *string, length); \
	} while(0)

#define  glDrawArrays(arg1, arg2, arg3) \
	do { \
		GLenum mode = (arg1); \
		GLint first = (arg2); \
		GLsizei count = (arg3); \
		glDrawArrays(mode, first, count); \
		gld_print("glDrawArrays(%d, %d, %ld)", mode, first, (long)count); \
	} while(0)

#define  glVertexPointer(arg1, arg2, arg3, arg4) \
	do { \
		GLint size = (arg1); \
		GLenum type = (arg2); \
		GLsizei stride = (arg3); \
		const GLvoid * ptr = (arg4); \
		glVertexPointer(size, type, stride, ptr); \
		gld_print("glVertexPointer(%d, %d, %ld, %p)", size, type, (long)stride, ptr); \
	} while(0)


#define  glGetIntegerv(arg1, arg2) \
	do { \
		GLenum pname = (arg1); \
		GLint * data = (arg2); \
		glGetIntegerv(pname, data); \
		gld_print("glGetIntegerv(%d = %s, %d)", pname, #arg1, *data); \
	} while(0)

#define  glLinkProgram(arg1) \
	do { \
		GLuint program = (arg1); \
		glLinkProgram(program); \
		gld_print("glLinkProgram(%u)", program); \
	} while(0)

#define  glBindVertexArray(arg1) \
	do { \
		GLuint array = (arg1); \
		glBindVertexArray(array); \
		gld_print("glBindVertexArray(%u)", array); \
	} while(0)

#define  glDetachShader(arg1, arg2) \
	do { \
		GLuint program = (arg1); \
		GLuint shader = (arg2); \
		glDetachShader(program, shader); \
		gld_print("glDetachShader(%u, %u)", program, shader); \
	} while(0)

#define  glGenTextures(arg1, arg2) \
	do { \
		GLsizei n = (arg1); \
		GLuint * textures = (arg2); \
		glGenTextures(n, textures); \
		gld_print("glGenTextures(%ld, %p)", n, textures); \
	} while(0)

#define  glGetProgramiv(arg1, arg2, arg3) \
	do { \
		GLuint program = (arg1); \
		GLenum pname = (arg2); \
		GLint * params = (arg3); \
		glGetProgramiv(program, pname, params); \
		gld_print("glGetProgramiv(%u, %d, %p)", program, pname, params); \
	} while(0)

#define  glClear(arg1) \
	do { \
		GLbitfield mask = (arg1); \
		glClear(mask); \
		gld_print("glClear(%d)", mask); \
	} while(0)

#define  glUseProgram(arg1) \
	do { \
		GLuint program = (arg1); \
		glUseProgram(program); \
		gld_print("glUseProgram(%u)", program); \
	} while(0)

#define  glFlush() \
	do { \
		glFlush(); \
		gld_print("glFlush()", 0); \
	} while(0)

#define  glBufferData(arg1, arg2, arg3, arg4) \
	do { \
		GLenum target = (arg1); \
		GLsizeiptr size = (arg2); \
		const void *data = (arg3); \
		GLenum usage = (arg4); \
		glBufferData(target, size, data, usage); \
		gld_print("glBufferData(%d, %ld, %p, %d = %s)", target, (long)size, data, usage, #arg4); \
	} while(0)

#define  glAttachShader(arg1, arg2) \
	do { \
		GLuint program = (arg1); \
		GLuint shader = (arg2); \
		glAttachShader(program, shader); \
		gld_print("glAttachShader(%u, %u)", program, shader); \
	} while(0)

#define  glBindBuffer(arg1, arg2) \
	do { \
		GLenum target = (arg1); \
		GLuint buffer = (arg2); \
		glBindBuffer(target, buffer); \
		gld_print("glBindBuffer(%d, %u)", target, buffer); \
	} while(0)

#define  glMatrixMode(arg1) \
	do { \
		GLenum mode = (arg1); \
		glMatrixMode(mode); \
		gld_print("glMatrixMode(%d = %s)", mode, #arg1); \
	} while(0)

#define  glDisableClientState(arg1) \
	do { \
		GLenum cap = (arg1); \
		glDisableClientState(cap); \
		gld_print("glDisableClientState(%d = %s)", cap, #arg1); \
	} while(0)

#define  glGetShaderInfoLog(arg1, arg2, arg3, arg4) \
	do { \
		GLuint shader = (arg1); \
		GLsizei bufSize = (arg2); \
		GLsizei * length = (arg3); \
		GLchar * infoLog = (arg4); \
		glGetShaderInfoLog(shader, bufSize, length, infoLog); \
		gld_print("glGetShaderInfoLog(%u, %ld, %ld, %p)", shader, (long)bufSize, (long)length, infoLog); \
	} while(0)

#define  glEnableVertexAttribArray(arg1) \
	do { \
		GLuint index = (arg1); \
		glEnableVertexAttribArray(index); \
		gld_print("glEnableVertexAttribArray(%u)", index); \
	} while(0)

#define  glCompileShader(arg1) \
	do { \
		GLuint shader = (arg1); \
		glCompileShader(shader); \
		gld_print("glCompileShader(%u)", shader); \
	} while(0)

#define  glTranslatef(arg1, arg2, arg3) \
	do { \
		GLfloat x = (arg1); \
		GLfloat y = (arg2); \
		GLfloat z = (arg3); \
		glTranslatef(x, y, z); \
		gld_print("glTranslatef(%f, %f, %f)", x, y, z); \
	} while(0)

#define  glGenBuffers(arg1, arg2) \
	do { \
		GLsizei n = (arg1); \
		GLuint * buffers = (arg2); \
		glGenBuffers(n, buffers); \
		gld_print("glGenBuffers(%ld, %p)", (long)n, buffers); \
	} while(0)

#define  glDeleteProgram(arg1) \
	do { \
		GLuint program = (arg1); \
		glDeleteProgram(program); \
		gld_print("glDeleteProgram(%u)", program); \
	} while(0)

#define  glBlendFunc(arg1, arg2) \
	do { \
		GLenum sfactor = (arg1); \
		GLenum dfactor = (arg2); \
		glBlendFunc(sfactor, dfactor); \
		gld_print("glBlendFunc(%d = %s, %d = %s)", sfactor, #arg1, dfactor, #arg2); \
	} while(0)

#endif

#endif
