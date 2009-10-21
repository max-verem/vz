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
    2008-09-24:
        *logger use for message outputs

	2006-11-26:
		Code start.

*/
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "vzGlExt.h"
#include "vzLogger.h"

BOOL APIENTRY DllMain
(
	HANDLE hModule, 
	DWORD  ul_reason_for_call, 
	LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

// strings for automated loading
#define GL_REG_EXT(EXT_NAME, FUNC_NAME) {EXT_NAME, #FUNC_NAME, &FUNC_NAME}
#define GL_REG_EXT_FBO(FUNC_NAME) GL_REG_EXT("GL_EXT_framebuffer_object", FUNC_NAME)
#define GL_REG_EXT_PBO(FUNC_NAME) GL_REG_EXT("GL_EXT_pixel_buffer_object", FUNC_NAME)
static void* _gl_extensions_list[][3] = 
{
	/*
		http://oss.sgi.com/projects/ogl-sample/registry/EXT/blend_func_separate.txt
	*/
	{"GL_EXT_blend_func_separate","glBlendFuncSeparateEXT",&glBlendFuncSeparateEXT},

    /*
        http://oss.sgi.com/projects/ogl-sample/registry/EXT/blend_equation_separate.txt
    */
    {"GL_EXT_blend_equation_separate", "glBlendEquationSeparateEXT", &glBlendEquationSeparateEXT},

	/* 
		pixel buffer extensions:
		http://oss.sgi.com/projects/ogl-sample/registry/EXT/pixel_buffer_object.txt
	*/
	GL_REG_EXT_PBO(glBindBuffer),
	GL_REG_EXT_PBO(glUnmapBuffer),
	GL_REG_EXT_PBO(glMapBuffer),
	GL_REG_EXT_PBO(glGenBuffers),
	GL_REG_EXT_PBO(glDeleteBuffers),
	GL_REG_EXT_PBO(glBufferData),

	/*
		framebuffer buffer extensions:
		http://oss.sgi.com/projects/ogl-sample/registry/EXT/framebuffer_object.txt
	*/
	GL_REG_EXT_FBO(glIsRenderbufferEXT),
	GL_REG_EXT_FBO(glBindRenderbufferEXT),
	GL_REG_EXT_FBO(glDeleteRenderbuffersEXT),
	GL_REG_EXT_FBO(glGenRenderbuffersEXT),
	GL_REG_EXT_FBO(glRenderbufferStorageEXT),
	GL_REG_EXT_FBO(glGetRenderbufferParameterivEXT),
	GL_REG_EXT_FBO(glIsFramebufferEXT),
	GL_REG_EXT_FBO(glBindFramebufferEXT),
	GL_REG_EXT_FBO(glDeleteFramebuffersEXT),
	GL_REG_EXT_FBO(glGenFramebuffersEXT),
	GL_REG_EXT_FBO(glCheckFramebufferStatusEXT),
	GL_REG_EXT_FBO(glFramebufferTexture1DEXT),
	GL_REG_EXT_FBO(glFramebufferTexture2DEXT),
	GL_REG_EXT_FBO(glFramebufferTexture3DEXT),
	GL_REG_EXT_FBO(glFramebufferRenderbufferEXT),
	GL_REG_EXT_FBO(glGetFramebufferAttachmentParameterivEXT),
	GL_REG_EXT_FBO(glGenerateMipmapEXT),

	/* stop list */
	{NULL,NULL,NULL}
};

// extensions functions types
VZGLEXT_API void (WINAPI *glBlendFuncSeparateEXT)(GLenum sfactorRGB,GLenum dfactorRGB,GLenum sfactorAlpha,GLenum dfactorAlpha) = NULL;
VZGLEXT_API void (WINAPI *glBlendEquationSeparateEXT)(GLenum modeRGB, GLenum modeAlpha) = NULL;
VZGLEXT_API void (WINAPI *glBindBuffer)(GLenum ,GLenum) = NULL;
VZGLEXT_API void (WINAPI *glGenBuffers)(GLsizei, GLuint *) = NULL;
VZGLEXT_API void (WINAPI *glDeleteBuffers)(GLsizei, GLuint *) = NULL;
VZGLEXT_API unsigned char (WINAPI *glUnmapBuffer)(GLenum) = NULL;
VZGLEXT_API void (WINAPI *glBufferData)(GLenum, GLuint , const GLvoid *, GLenum) = NULL;
VZGLEXT_API GLvoid* (WINAPI * glMapBuffer)(GLenum, GLenum) = NULL;
VZGLEXT_API GLuint  (WINAPI *glIsRenderbufferEXT)(GLuint renderbuffer) = NULL;
VZGLEXT_API void (WINAPI *glBindRenderbufferEXT)(GLenum target, GLuint renderbuffer) = NULL;
VZGLEXT_API void (WINAPI *glDeleteRenderbuffersEXT)(GLsizei n, GLenum, GLuint *renderbuffers) = NULL;
VZGLEXT_API void (WINAPI *glGenRenderbuffersEXT)(GLsizei n, GLuint *renderbuffers) = NULL;
VZGLEXT_API void (WINAPI *glRenderbufferStorageEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) = NULL;
VZGLEXT_API void (WINAPI *glGetRenderbufferParameterivEXT)(GLenum target, GLenum pname, int *params) = NULL;
VZGLEXT_API GLuint (WINAPI *glIsFramebufferEXT)(GLuint framebuffer) = NULL;
VZGLEXT_API void (WINAPI *glBindFramebufferEXT)(GLenum target, GLuint framebuffer) = NULL;
VZGLEXT_API void (WINAPI *glDeleteFramebuffersEXT)(GLsizei n, const GLuint *framebuffers) = NULL;
VZGLEXT_API void (WINAPI *glGenFramebuffersEXT)(GLsizei n, GLuint *framebuffers) = NULL;
VZGLEXT_API GLuint  (WINAPI *glCheckFramebufferStatusEXT)(GLenum target) = NULL;
VZGLEXT_API void (WINAPI *glFramebufferTexture1DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, int level) = NULL;
VZGLEXT_API void (WINAPI *glFramebufferTexture2DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, int level) = NULL;
VZGLEXT_API void (WINAPI *glFramebufferTexture3DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, int level, int zoffset) = NULL;
VZGLEXT_API void (WINAPI *glFramebufferRenderbufferEXT)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) = NULL;
VZGLEXT_API void (WINAPI *glGetFramebufferAttachmentParameterivEXT)(GLenum target, GLenum attachment, GLenum pname, int *params) = NULL;
VZGLEXT_API void (WINAPI *glGenerateMipmapEXT)(GLenum target) = NULL;

VZGLEXT_API int  glExtInitDone = 0;;

// function
VZGLEXT_API void vzGlExtInit()
{
	char* msg;
	char* gl_extensions = (char*)glGetString(GL_EXTENSIONS);

	// check if string not null
	if(!(gl_extensions))
	{
		logger_printf(1, "vzGlExt: 'glGetString' Failed!");
		return;
	}
	else
		logger_printf(0, "vzGlExt: OpenGL extensions supported: %s", gl_extensions);

	// notify about loading extensions
	logger_printf(0, "vzGlExt: Loading extensions...");

	/* enum extensions */
	for(int i=0;_gl_extensions_list[i][0];i++)
	{
		if(strstr(gl_extensions,(char*)_gl_extensions_list[i][0]))
		{
			*((unsigned long*)_gl_extensions_list[i][2]) = (unsigned long)wglGetProcAddress((char*)_gl_extensions_list[i][1]);
			msg = (_gl_extensions_list[i][2])?"OK":"FAILED";
		}
		else
			msg = "not supported";
		
		logger_printf(0, "vzGlExt: '%s': %s",(char*)_gl_extensions_list[i][1], msg);
	};

	glExtInitDone = 1;
};

VZGLEXT_API GLenum vzGlExtEnumLookup(char* name)
{
    static const struct
    {
        char* name;
        GLenum val;
    }
    gl_attrs[] =
    {
        { "ZERO",                           GL_ZERO },
        { "ONE",                            GL_ONE },
        { "DST_COLOR",                      GL_DST_COLOR },
        { "ONE_MINUS_DST_COLOR",            GL_ONE_MINUS_DST_COLOR },
        { "SRC_ALPHA",                      GL_SRC_ALPHA },
        { "ONE_MINUS_SRC_ALPHA",            GL_ONE_MINUS_SRC_ALPHA },
        { "DST_ALPHA",                      GL_DST_ALPHA },
        { "ONE_MINUS_DST_ALPHA",            GL_ONE_MINUS_DST_ALPHA },
        { "CONSTANT_COLOR",                 GL_CONSTANT_COLOR },
        { "ONE_MINUS_CONSTANT_COLOR",       GL_ONE_MINUS_CONSTANT_COLOR },
        { "CONSTANT_ALPHA",                 GL_CONSTANT_ALPHA },
        { "ONE_MINUS_CONSTANT_ALPHA",       GL_ONE_MINUS_CONSTANT_ALPHA },
        { "SRC_ALPHA_SATURATE",             GL_SRC_ALPHA_SATURATE },
        { "SRC_COLOR",                      GL_SRC_COLOR },
        { "ONE_MINUS_SRC_COLOR",            GL_ONE_MINUS_SRC_COLOR },

        { "FUNC_ADD",                       GL_FUNC_ADD },
        { "FUNC_SUBTRACT",                  GL_FUNC_SUBTRACT },
        { "FUNC_REVERSE_SUBTRACT",          GL_FUNC_REVERSE_SUBTRACT },
        { "FUNC_MIN",                       GL_FUNC_MIN },
        { "FUNC_MAX",                       GL_FUNC_MAX },
        { "MIN",                            GL_FUNC_MIN },
        { "MAX",                            GL_FUNC_MAX },

        {NULL,                              GL_UNKNOWN_ATTR }
    };

    if(NULL == name)
        return GL_UNKNOWN_ATTR;

    for(int i = 0; (NULL != gl_attrs[i].name); i++)
    {
        int r = _stricmp(gl_attrs[i].name, name);
        if(!r) return gl_attrs[i].val;
    };

    return GL_UNKNOWN_ATTR;
};
