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
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "vzGlExt.h"

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
static void* _gl_extensions_list[][3] = 
{
	{"GL_EXT_blend_func_separate","glBlendFuncSeparateEXT",&glBlendFuncSeparateEXT},
	{"GL_EXT_pixel_buffer_object","glBindBuffer",&glBindBuffer},
	{"GL_EXT_pixel_buffer_object","glUnmapBuffer",&glUnmapBuffer},
	{"GL_EXT_pixel_buffer_object","glMapBuffer", &glMapBuffer},
	{"GL_EXT_pixel_buffer_object","glGenBuffers",&glGenBuffers},
	{"GL_EXT_pixel_buffer_object","glDeleteBuffers",&glDeleteBuffers},
	{"GL_EXT_pixel_buffer_object","glBufferData",&glBufferData},
	{NULL,NULL,NULL}
};

// extensions functions types
VZGLEXT_API void (WINAPI *glBlendFuncSeparateEXT)(GLenum sfactorRGB,GLenum dfactorRGB,GLenum sfactorAlpha,GLenum dfactorAlpha) = NULL;
VZGLEXT_API void (WINAPI *glBindBuffer)(GLenum ,GLenum) = NULL;
VZGLEXT_API void (WINAPI *glGenBuffers)(GLsizei, GLuint *) = NULL;
VZGLEXT_API void (WINAPI *glDeleteBuffers)(GLsizei, GLuint *) = NULL;
VZGLEXT_API unsigned char (WINAPI *glUnmapBuffer)(GLenum) = NULL;
VZGLEXT_API void (WINAPI *glBufferData)(GLenum, GLuint , const GLvoid *, GLenum) = NULL;
VZGLEXT_API GLvoid* (WINAPI * glMapBuffer)(GLenum, GLenum) = NULL;


// function
VZGLEXT_API void vzGlExtInit()
{
	char* gl_extensions = (char*)glGetString(GL_EXTENSIONS);

	// check if string not null
	if(!(gl_extensions))
	{
		printf("vzGlExt: 'glGetString' Failed!\n");
		return;
	}
	else
		printf("vzGlExt: OpenGL extensions supported: %s\n", gl_extensions);

	// notify about loading extensions
	printf("vzGlExt: Loading extensions...\n");

	/* enum extensions */
	for(int i=0;_gl_extensions_list[i][0];i++)
	{
		printf("\t'%s':",(char*)_gl_extensions_list[i][0]);
		if(strstr(gl_extensions,(char*)_gl_extensions_list[i][0]))
		{
			printf("loading '%s'...",(char*)_gl_extensions_list[i][1] );
			*((unsigned long*)_gl_extensions_list[i][2]) = (unsigned long)wglGetProcAddress((char*)_gl_extensions_list[i][1]);
			printf("%s", (_gl_extensions_list[i][2])?"OK":"FAILED");
		}
		else
			printf("not supported");
		printf("\n");
	};
};



