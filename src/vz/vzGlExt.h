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
	2007-03-06:
		*GLUT goodbye!

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

#include <gl/gl.h>
#include <gl/glu.h>
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

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

#ifndef GL_FRAMEBUFFER_EXT
#define GL_FRAMEBUFFER_EXT                     0x8D40
#define GL_RENDERBUFFER_EXT                    0x8D41
#define GL_STENCIL_INDEX1_EXT                  0x8D46
#define GL_STENCIL_INDEX4_EXT                  0x8D47
#define GL_STENCIL_INDEX8_EXT                  0x8D48
#define GL_STENCIL_INDEX16_EXT                 0x8D49
#define GL_RENDERBUFFER_WIDTH_EXT              0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT             0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT    0x8D44
#define GL_RENDERBUFFER_RED_SIZE_EXT           0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE_EXT         0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE_EXT          0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE_EXT         0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE_EXT         0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE_EXT       0x8D55
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT            0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT            0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT          0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT  0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT     0x8CD4
#define GL_COLOR_ATTACHMENT0_EXT                0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT                0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT                0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT                0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT                0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT                0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT                0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT                0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT                0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT                0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT               0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT               0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT               0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT               0x8CED
#define GL_COLOR_ATTACHMENT14_EXT               0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT               0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT                 0x8D00
#define GL_STENCIL_ATTACHMENT_EXT               0x8D20
#define GL_FRAMEBUFFER_COMPLETE_EXT                          0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT             0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT     0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT             0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT                0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT            0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT            0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT                       0x8CDD
#define GL_FRAMEBUFFER_BINDING_EXT				0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT				0x8CA7
#define GL_MAX_COLOR_ATTACHMENTS_EXT			0x8CDF
#define GL_MAX_RENDERBUFFER_SIZE_EXT			0x84E8
#define GL_NVALID_FRAMEBUFFER_OPERATION_EXT		0x0506
#define GL_DEPTH_COMPONENT16					0x81A5
#define GL_DEPTH_COMPONENT24					0x81A6
#define GL_DEPTH_COMPONENT32					0x81A7

#define GL_DEPTH_STENCIL_EXT 0x84F9
#define GL_UNSIGNED_INT_24_8_EXT 0x84FA
#define GL_DEPTH24_STENCIL8_EXT 0x88F0
#define GL_TEXTURE_STENCIL_SIZE_EXT 0x88F1

#endif /* FRAMEBUFFER_EXT */

#ifndef GL_BLEND_EQUATION_RGB_EXT
#define GL_BLEND_EQUATION_RGB_EXT               0x8009  /* same as BLEND_EQUATION */
#endif /* GL_BLEND_EQUATION_RGB_EXT */

#ifndef GL_BLEND_EQUATION_ALPHA_EXT
#define GL_BLEND_EQUATION_ALPHA_EXT             0x883D
#endif /* GL_BLEND_EQUATION_ALPHA_EXT */

#ifndef GL_BLEND_DST_RGB_EXT
#define GL_BLEND_DST_RGB_EXT                    0x80C8
#endif /* GL_BLEND_DST_RGB_EXT */

#ifndef GL_BLEND_SRC_RGB_EXT
#define GL_BLEND_SRC_RGB_EXT                    0x80C9
#endif /* GL_BLEND_SRC_RGB_EXT */

#ifndef GL_BLEND_DST_ALPHA_EXT
#define GL_BLEND_DST_ALPHA_EXT                  0x80CA
#endif /* GL_BLEND_DST_ALPHA_EXT */

#ifndef GL_BLEND_SRC_ALPHA_EXT
#define GL_BLEND_SRC_ALPHA_EXT                  0x80CB
#endif /* GL_BLEND_SRC_ALPHA_EXT */

#ifndef GL_FUNC_ADD
#define GL_FUNC_ADD                             0x8006
#endif /* GL_FUNC_ADD */

#ifndef GL_FUNC_SUBTRACT
#define GL_FUNC_SUBTRACT                        0x800A
#endif /* GL_FUNC_SUBTRACT */

#ifndef GL_FUNC_REVERSE_SUBTRACT
#define GL_FUNC_REVERSE_SUBTRACT                0x800B
#endif /* GL_FUNC_REVERSE_SUBTRACT */

#ifndef GL_FUNC_MIN
#define GL_FUNC_MIN                             0x8007
#endif /* GL_FUNC_MIN */

#ifndef GL_FUNC_MAX
#define GL_FUNC_MAX                             0x8008
#endif /* GL_FUNC_MAX */

#define GL_CONSTANT_COLOR                       0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR             0x8002
#define GL_CONSTANT_ALPHA                       0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA             0x8004
#define GL_BLEND_COLOR                          0x8005

#define GL_UNKNOWN_ATTR                         0xFFFF

/* extensions functions types */
extern VZGLEXT_API void (WINAPI *glBlendFuncSeparateEXT)(GLenum sfactorRGB,GLenum dfactorRGB,GLenum sfactorAlpha,GLenum dfactorAlpha);
extern VZGLEXT_API void (WINAPI *glBlendEquationSeparateEXT)(GLenum modeRGB, GLenum modeAlpha);
extern VZGLEXT_API void (WINAPI *glBindBuffer)(GLenum ,GLenum);
extern VZGLEXT_API void (WINAPI *glGenBuffers)(GLsizei, GLuint *);
extern VZGLEXT_API void (WINAPI *glDeleteBuffers)(GLsizei, GLuint *);
extern VZGLEXT_API unsigned char (WINAPI *glUnmapBuffer)(GLenum);
extern VZGLEXT_API void (WINAPI *glBufferData)(GLenum, GLuint , const GLvoid *, GLenum);
extern VZGLEXT_API GLvoid* (WINAPI * glMapBuffer)(GLenum, GLenum);
extern VZGLEXT_API GLuint  (WINAPI *glIsRenderbufferEXT)(GLuint renderbuffer);
extern VZGLEXT_API void (WINAPI *glBindRenderbufferEXT)(GLenum target, GLuint renderbuffer);
extern VZGLEXT_API void (WINAPI *glDeleteRenderbuffersEXT)(GLsizei n, GLenum, GLuint *renderbuffers);
extern VZGLEXT_API void (WINAPI *glGenRenderbuffersEXT)(GLsizei n, GLuint *renderbuffers);
extern VZGLEXT_API void (WINAPI *glRenderbufferStorageEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
extern VZGLEXT_API void (WINAPI *glGetRenderbufferParameterivEXT)(GLenum target, GLenum pname, int *params);
extern VZGLEXT_API GLuint (WINAPI *glIsFramebufferEXT)(GLuint framebuffer);
extern VZGLEXT_API void (WINAPI *glBindFramebufferEXT)(GLenum target, GLuint framebuffer);
extern VZGLEXT_API void (WINAPI *glDeleteFramebuffersEXT)(GLsizei n, const GLuint *framebuffers);
extern VZGLEXT_API void (WINAPI *glGenFramebuffersEXT)(GLsizei n, GLuint *framebuffers);
extern VZGLEXT_API GLuint  (WINAPI *glCheckFramebufferStatusEXT)(GLenum target);
extern VZGLEXT_API void (WINAPI *glFramebufferTexture1DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, int level);
extern VZGLEXT_API void (WINAPI *glFramebufferTexture2DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, int level);
extern VZGLEXT_API void (WINAPI *glFramebufferTexture3DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, int level, int zoffset);
extern VZGLEXT_API void (WINAPI *glFramebufferRenderbufferEXT)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
extern VZGLEXT_API void (WINAPI *glGetFramebufferAttachmentParameterivEXT)(GLenum target, GLenum attachment, GLenum pname, int *params);
extern VZGLEXT_API void (WINAPI *glGenerateMipmapEXT)(GLenum target);
extern VZGLEXT_API int  glExtInitDone;

/* init function */
VZGLEXT_API void vzGlExtInit();
VZGLEXT_API GLenum vzGlExtEnumLookup(char* name);

#define glErrorLog(OPER)                                            \
{                                                                   \
    int glErrorLogValue;                                            \
    OPER;                                                           \
    glErrorLogValue = glGetError();                                 \
    if(GL_NO_ERROR != glErrorLogValue)                              \
        logger_printf(1,                                            \
            "glGetError 0x%.4X=\"%s\" for [" #OPER "] at  %s:%d",   \
            glErrorLogValue, gluErrorString(glErrorLogValue),       \
            __FILE__, __LINE__);                                    \
}

#ifdef _DEBUG
#define glErrorLogD(OPER) glErrorLog(OPER)
#else
#define glErrorLogD(OPER) OPER
#endif

#endif /* VZGLEXT_H */
