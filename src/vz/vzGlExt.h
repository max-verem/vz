/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2005 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1stereo.tv, 2005.

    ViZualizator is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    ViZualizator is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ViZualizator; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

ChangeLog:

	2006-11-26:
		Code start.

*/

#ifndef VZGLEXT_H
#define VZGLEXT_H

#ifdef VZGLEXT_EXPORTS
#define VZGLEXT_API __declspec(dllexport)
#else
#define VZGLEXT_API __declspec(dllimport)
#pragma comment(lib, "vzGlExt.lib") 
#endif

// #include <GL/glut.h>

/* 
http://oss.sgi.com/projects/ogl-sample/ABI/glext.h
*/

#ifndef GL_STREAM_READ
#define GL_READ_ONLY						0x88B8
#define GL_WRITE_ONLY						0x88B9
#define GL_READ_WRITE						0x88BA
#define GL_BUFFER_ACCESS					0x88BB
#define GL_BUFFER_MAPPED					0x88BC
#define GL_BUFFER_MAP_POINTER				0x88BD
#define GL_STREAM_DRAW						0x88E0
#define GL_STREAM_READ						0x88E1
#define GL_STREAM_COPY						0x88E2
#define GL_STATIC_DRAW						0x88E4
#define GL_STATIC_READ						0x88E5
#define GL_STATIC_COPY						0x88E6
#define GL_DYNAMIC_DRAW						0x88E8
#define GL_DYNAMIC_READ						0x88E9
#define GL_DYNAMIC_COPY						0x88EA
#endif // GL_STREAM_READ

#ifndef GL_PIXEL_PACK_BUFFER_ARB
#define GL_PIXEL_UNPACK_BUFFER_ARB			0x88EC
#define GL_PIXEL_PACK_BUFFER_ARB			0x88EB
#endif /* GL_PIXEL_PACK_BUFFER_ARB */


#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE_BIT					0x20000000
#define GL_MULTISAMPLE						0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE			0x809E
#define GL_SAMPLE_ALPHA_TO_ONE				0x809F
#define GL_SAMPLE_COVERAGE					0x80A0
#define GL_SAMPLE_BUFFERS					0x80A8
#define GL_SAMPLES							0x80A9
#define GL_SAMPLE_COVERAGE_VALUE			0x80AA
#define GL_SAMPLE_COVERAGE_INVERT			0x80AB
#endif /* GL_MULTISAMPLE */

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

/* extensions functions types */
extern VZGLEXT_API void (WINAPI *glBlendFuncSeparateEXT)(GLenum sfactorRGB,GLenum dfactorRGB,GLenum sfactorAlpha,GLenum dfactorAlpha);
extern VZGLEXT_API void (WINAPI *glBindBuffer)(GLenum ,GLenum);
extern VZGLEXT_API void (WINAPI *glGenBuffers)(GLsizei, GLuint *);
extern VZGLEXT_API void (WINAPI *glDeleteBuffers)(GLsizei, GLuint *);
extern VZGLEXT_API unsigned char (WINAPI *glUnmapBuffer)(GLenum);
extern VZGLEXT_API void (WINAPI *glBufferData)(GLenum, GLuint , const GLvoid *, GLenum);
extern VZGLEXT_API GLvoid* (WINAPI * glMapBuffer)(GLenum, GLenum);

/* init function */
VZGLEXT_API void vzGlExtInit();


#endif /* VZGLEXT_H */