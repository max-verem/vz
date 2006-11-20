#include <GL/glut.h>


#ifndef GL_STREAM_READ
#define		GL_STREAM_READ					0x88E1
#endif // GL_STREAM_READ

#ifndef GL_PIXEL_PACK_BUFFER_ARB
#define 	GL_PIXEL_PACK_BUFFER_ARB		0x88EB
#endif // GL_PIXEL_PACK_BUFFER_ARB

#ifndef GL_PIXEL_UNPACK_BUFFER_ARB
#define 	GL_PIXEL_UNPACK_BUFFER_ARB		0x88EC
#endif // GL_PIXEL_UNPACK_BUFFER_ARB

#ifndef GL_READ_ONLY
#define		GL_READ_ONLY					0x88B8
#endif // GL_READ_ONLY

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

// extensions functions types
static int loaded_GL_EXT = 0;
static void (WINAPI *glBlendFuncSeparateEXT)(GLenum sfactorRGB,GLenum dfactorRGB,GLenum sfactorAlpha,GLenum dfactorAlpha) = NULL;
static void (WINAPI *glBindBuffer)(GLenum ,GLenum) = NULL;
static void (WINAPI *glGenBuffers)(GLsizei, GLuint *) = NULL;
static void (WINAPI *glDeleteBuffers)(GLsizei, GLuint *) = NULL;
static unsigned char (WINAPI *glUnmapBuffer)(GLenum) = NULL;
static void (WINAPI *glBufferData)(GLenum, GLuint , const GLvoid *, GLenum) = NULL;
static GLvoid* (WINAPI * glMapBuffer)(GLenum, GLenum) = NULL;

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


// function
void load_GL_EXT()
{
	// check if already loaded!
	if (loaded_GL_EXT) return;

	// set flag that process done
	loaded_GL_EXT = 1;

	char* gl_extensions = (char*)glGetString(GL_EXTENSIONS);

	// check if string not null
	if(!(gl_extensions)) return;

	// notify about loading extensions
	printf("Loading extensions...\n");

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

