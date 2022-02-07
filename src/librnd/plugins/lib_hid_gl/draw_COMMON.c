/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2009-2017 PCB Contributers (See ChangeLog for details)
 *  Copyright (C) 2017 Adrian Purser
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Low level gl rendering: code that works with any backend; included by
   draw_*.c */

static void common_set_color(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	red = r;
	green = g;
	blue = b;
	alpha = a;
}

static void common_prim_add_triangle(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat x3, GLfloat y3)
{
	/* Debug Drawing */
#if 0
	primbuf_add(GL_LINES, vertbuf.size, 6, red, green, blue, alpha);
	vertbuf_reserve_extra(6);
	vertbuf_add(x1,y1);  vertbuf_add(x2,y2);
	vertbuf_add(x2,y2);  vertbuf_add(x3,y3);
	vertbuf_add(x3,y3);  vertbuf_add(x1,y1);
#endif

	primbuf_add(GL_TRIANGLES, vertbuf.size, 3, 0, red, green, blue, alpha);
	vertbuf_reserve_extra(3);
	vertbuf_add(x1, y1);
	vertbuf_add(x2, y2);
	vertbuf_add(x3, y3);
}

static void common_prim_add_textrect(GLfloat x1, GLfloat y1, GLfloat u1, GLfloat v1,
	GLfloat x2, GLfloat y2, GLfloat u2, GLfloat v2,
	GLfloat x3, GLfloat y3, GLfloat u3, GLfloat v3,
	GLfloat x4, GLfloat y4, GLfloat u4, GLfloat v4,
	GLuint texture_id)
{
	primbuf_add(GL_TRIANGLE_FAN, vertbuf.size, 4, texture_id, red, green, blue, alpha);
	vertbuf_reserve_extra(4);
	vertbuf_add_xyuv(x1, y1, u1, v1);
	vertbuf_add_xyuv(x2, y2, u2, v2);
	vertbuf_add_xyuv(x3, y3, u3, v3);
	vertbuf_add_xyuv(x4, y4, u4, v4);
}

static void common_prim_add_line(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	primbuf_add(GL_LINES, vertbuf.size, 2, 0, red, green, blue, alpha);
	vertbuf_reserve_extra(2);
	vertbuf_add(x1, y1);
	vertbuf_add(x2, y2);
}

static void common_prim_add_rect(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	primbuf_add(GL_LINE_LOOP, vertbuf.size, 4, 0, red, green, blue, alpha);
	vertbuf_reserve_extra(4);
	vertbuf_add(x1, y1);
	vertbuf_add(x2, y1);
	vertbuf_add(x2, y2);
	vertbuf_add(x1, y2);
}

static void common_prim_reserve_triangles(int count)
{
	vertbuf_reserve_extra(count * 3);
}

RND_INLINE void draw_common_config_texture(GLuint texture_id)
{
		glBindTexture(GL_TEXTURE_2D, texture_id);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glEnable(GL_TEXTURE_2D);
}

static void common_flush(void)
{
	glFlush();
}

static long common_texture_import(unsigned char *pixels, int width, int height, int has_alpha)
{
	GLuint texture_id;

	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, has_alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, pixels);

	return texture_id;
}

static void common_texture_free(long id)
{
	GLuint tmp = id;
	glDeleteTextures(1, &tmp);
}

static void common_prim_set_marker(void)
{
	vertbuf_set_marker();
	primbuf_set_marker();
}

static void common_prim_rewind_to_marker(void)
{
	vertbuf_rewind();
	primbuf_rewind();
}


/*** version query utils for init ***/
RND_INLINE int gl_is_es(void)
{
	static const char es_prefix[] = "OpenGL ES";
	const char *ver;

	/* libepoxy says: some ES implementions (e.g. PowerVR) don't have the
	   es_prefix in their version string. For now we ignore these as
	   they need dlsym() to detect */

	ver = (const char *)glGetString(GL_VERSION);
	if (ver == NULL)
		return 0;

	return strncmp(ver, es_prefix, sizeof(es_prefix)-1) == 0;
}

RND_INLINE GLint gl_get_ver_major(void)
{
	GLint major;

#ifdef GL_MAJOR_VERSION
	glGetIntegerv(GL_MAJOR_VERSION, &major);
#endif

	if (major == 0)
		glGetIntegerv(GL_VERSION, &major);

	if (major == 0) {
		const GLubyte *verstr = glGetString(GL_VERSION);
		rnd_message(RND_MSG_DEBUG, "opengl gl_get_ver_major: you have a real ancient opengl version '%s'\n", verstr == NULL ? "<unknown>" : (const char *)verstr);
		return -1;
	}

	return major;
}
