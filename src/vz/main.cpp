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
        *VGA screen scale support
		*logger use for message outputs

	2008-09-23:
		*vzTVSpec rework

	2007-03-11:
		*Program version controled from "vzVersion.h"

	2007-03-10:
		*FBO support leak detect
		*more gracefull cleanup

	2007-03-06:
		*GLUT goodbye!

	2007-02-24:
		*FBO using.

	2006-12-04:
		*screenshot dumping feature reintroducing.

	2006-11-26:
		*Hard sync scheme.
		*OpenGL extension scheme load changes.

	2006-11-20:
		*try to enable multisamping 

	2005-06-24:
		*Added feature 'use_offscreen_buffer' that use GL extension
		"GL_EXT_pixel_buffer_object" for creating offscreen buffers. 
		Dramaticaly decreased kernel load time in configuration
		FX5300+'use_offscreen_buffer'!!! FX5500(AGP)+'use_offscreen_buffer'
		has no effect.

    2005-06-08: Code cleanup


*/
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <time.h>
#include "vzVersion.h"
#include "vzGlExt.h"

#include "vzMain.h"
#include "vzOutput.h"
#include "vzImage.h"
#include "vzTVSpec.h"
#include "vzTTFont.h"
#include "vzLogger.h"

#include "tcpserver.h"
#include "serserver.h"


#pragma comment(lib, "winmm.lib")

/*
----------------------------------------------------------
	Main Program Info
----------------------------------------------------------
*/
vzTVSpec tv;

/*
----------------------------------------------------------
	Globals program attributes

----------------------------------------------------------
*/
int f_exit = 0;;

void* scene = NULL;	// scene loaded
char* scene_file = NULL;

char* config_file = "vz.conf";
void* config = NULL; // config loaded

HANDLE scene_lock;
void* functions = NULL; // list of loaded function
void* output_module = NULL; // output module
char start_path[1024];
char* application;
char screenshot_file[1024];

/* -------------------------------------------------------- */

static void timestamp_screenshot()
{
	time_t ltime;
	struct tm *rtime;

	/* request time */
	time( &ltime );

	/* local */
	rtime = localtime( &ltime );

	/* format name */
	strftime
	(
		screenshot_file , 
		sizeof(screenshot_file),
		"./vz-sc-%Y%m%d_%H%M%S.tga",
		rtime 
	);
};

/* ----------------------------------------------------------
	Sync rendering & frame counting
---------------------------------------------------------- */

static struct
{
	HINSTANCE instance;
	HDC hdc;
	HWND wnd;
	HGLRC glrc;
	HANDLE lock;
} vz_window_desc;

static HANDLE global_frame_event;
static unsigned long global_frame_no = 0;
//static unsigned long stop_global_frames_counter = 0;
static long skip_draw = 0;
static long not_first_at = 0;

/*----------------------------------------------------------
	sync srcs
----------------------------------------------------------*/

static HANDLE sync_render_handle = INVALID_HANDLE_VALUE;
static HANDLE internal_sync_generator_handle = INVALID_HANDLE_VALUE;

static unsigned long WINAPI internal_sync_generator(void* fc_proc_addr)
{
	frames_counter_proc fc = (frames_counter_proc)fc_proc_addr;
	
	// endless loop
	while(0 == f_exit)
	{
		// sleep
		Sleep((1000 * tv.TV_FRAME_PS_DEN) / tv.TV_FRAME_PS_NOM);

		// call frame counter
		if (fc) fc();
	};

	return 0;
};

static void frames_counter()
{
	// reset event
	ResetEvent(global_frame_event);

	// increment frame counter
	global_frame_no++;

	// rise evennt
	PulseEvent(global_frame_event);
};

/*----------------------------------------------------------
	render loop
----------------------------------------------------------*/
static void vz_scene_render(void);
static void vz_scene_display(void);
static unsigned long WINAPI sync_render(void* data)
{
	while(0 == f_exit)
	{
		if(WAIT_OBJECT_0 == WaitForSingleObject(global_frame_event, INFINITE))
//		WaitForSingleObject(global_frame_event, /* INFINITE */ 1000);
		{
			/* render picture */
			if(not_first_at)
			{
				// reset flag of forcing redraw
				skip_draw = 0;

				// notify about redisplay
				vz_scene_render();
			};

			/* notify to redraw bg or direct render screen */
			vz_scene_display();
		};
	};

	return 0;
};

/*----------------------------------------------------------
	FBO init/setup
----------------------------------------------------------*/
#include "fb_bg_text_data.h"

static struct
{
	unsigned int index;
	unsigned int fb;
	unsigned int stencil_rb;
	unsigned int color_tex[2];
	unsigned int color_tex_width;
	unsigned int color_tex_height;
	unsigned int depth_rb;
	unsigned int stencil_depth_rb;
	unsigned int bg_tex;
	unsigned int bg_tex_width;
	unsigned int bg_tex_height;
} fbo;

inline unsigned long POT(unsigned long v)
{
	unsigned long i;
	for(i=1;(i!=0x80000000)&&(i<v);i<<=1);
	return i;
};

static int init_fbo()
{	
/*
	http://developer.apple.com/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_offscreen/chapter_5_section_5.html
	http://www.gamedev.net/community/forums/topic.asp?topic_id=356364&whichpage=1&#2337640
	http://www.gamedev.ru/community/opengl/articles/framebuffer_object?page=3
	http://www.opengl.org/registry/specs/EXT/packed_depth_stencil.txt
*/
	
	/* check if FBO supported (extensions loaded) */
	if (NULL == glGenFramebuffersEXT)
		return -1;

	/* init fbo texture width/height */
	fbo.color_tex_width = POT(tv.TV_FRAME_WIDTH);
	fbo.color_tex_height = POT(tv.TV_FRAME_HEIGHT);

	/* generate framebuffer */
	glGenFramebuffersEXT(1, &fbo.fb);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo.fb);

	// initialize color texture
	for(int i = 0; i<2; i++)
	{

		glGenTextures(1, &fbo.color_tex[i]);
		glBindTexture(GL_TEXTURE_2D, fbo.color_tex[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D
		(
			GL_TEXTURE_2D, 
			0, 
			GL_RGBA8, 
			fbo.color_tex_width, fbo.color_tex_height, 
			0, 
			GL_BGRA_EXT, GL_UNSIGNED_BYTE, 
			NULL
		);

		glFramebufferTexture2DEXT
		(
			GL_FRAMEBUFFER_EXT, 
			GL_COLOR_ATTACHMENT0_EXT + i, 
			GL_TEXTURE_2D, 
			fbo.color_tex[i], 
			0
		);
	};

	glGenRenderbuffersEXT(1, &fbo.stencil_depth_rb);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, fbo.stencil_depth_rb);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, fbo.color_tex_width, fbo.color_tex_height);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,  GL_RENDERBUFFER_EXT, fbo.stencil_depth_rb);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, fbo.stencil_depth_rb);

	/* create background texture */
	glGenTextures(1, &fbo.bg_tex);
	glBindTexture(GL_TEXTURE_2D, fbo.bg_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D
	(
		GL_TEXTURE_2D, 
		0, 
		GL_RGBA8, 
		(fbo.bg_tex_width = 16), (fbo.bg_tex_height = 16),
		0, 
		GL_BGRA_EXT, GL_UNSIGNED_BYTE, 
		fb_bg_text_data
	);


	glBindTexture(GL_TEXTURE_2D, 0);

	fbo.index = 0;
	glDrawBuffer (GL_COLOR_ATTACHMENT0_EXT + (0 + fbo.index) );
	glReadBuffer (GL_COLOR_ATTACHMENT0_EXT + (1 - fbo.index) );

	return 0;
};


/*----------------------------------------------------------
	main OpenGL operations
----------------------------------------------------------*/

static long render_time = 0;
static long rendered_frames = 0;
static long last_title_update = 0;

static void vz_scene_render(void)
{
	int force_render = 0, force_rendered = 0;

	/* check if extensions loaded */
	if(!(glExtInitDone)) 
		return;

	// check if redraw should processed
	// by internal needs
	if (skip_draw)
		return;

	WaitForSingleObject(vz_window_desc.lock, INFINITE);
	if(0 == wglMakeCurrent(vz_window_desc.hdc, vz_window_desc.glrc))
	{
		DWORD err = GetLastError();
		wglMakeCurrent(NULL, NULL);
		ReleaseMutex(vz_window_desc.lock);
		return;
	};
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo.fb);

	// scene redrawed
	skip_draw = 1;

	/* check if we need to draw frame */
	if
	(
		(NULL == output_module)
		||
		(
			(NULL != output_module)
			&&
			(vzOuputRenderSlots(output_module) > 0)
		)
	)
		force_render = 1;

	while(force_render)
	{
		/* reset force render flag */
		force_render = 0;
		force_rendered++;

		// save time of draw start
		long draw_start_time = timeGetTime();

		// output module tricks
		if(output_module)
			vzOuputPreRender(output_module);

		rendered_frames++;

		// lock scene
		WaitForSingleObject(scene_lock,INFINITE);

		// draw scene
		if (scene)
			vzMainSceneDisplay(scene,global_frame_no);
		else
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		};

		// flush all 
		glFlush();

		if(output_module)
			vzOuputPostRender(output_module);

		// save time of draw start
		long draw_stop_time = timeGetTime();
		render_time = draw_stop_time - draw_start_time;

		ReleaseMutex(scene_lock);

		/* check if we need to draw more frames */
		if
		(
			(NULL != output_module)
			&&
			(vzOuputRenderSlots(output_module) > 0)
		)
			force_render = 1;

		/* check if we need to terminate this loop */
		if(force_rendered > VZOUTPUT_MAX_BUFS)
			force_rendered = 0;

		fbo.index = 1 - fbo.index;
		glDrawBuffer (GL_COLOR_ATTACHMENT0_EXT + (0 + fbo.index) );
		glReadBuffer (GL_COLOR_ATTACHMENT0_EXT + (1 - fbo.index) );
	};

	/* unbind conext */
	wglMakeCurrent(NULL, NULL);
	ReleaseMutex(vz_window_desc.lock);
};

static void vz_scene_display(void)
{
	float X, Y;
	float W, H;

	float kW = ((float)tv.vga_width) / tv.TV_FRAME_WIDTH;
	float kH = ((float)tv.vga_height) / tv.TV_FRAME_HEIGHT;

	/* check if extensions loaded */
	if(!(glExtInitDone)) 
		return;

	WaitForSingleObject(vz_window_desc.lock, INFINITE);
	if(0 == wglMakeCurrent(vz_window_desc.hdc, vz_window_desc.glrc))
	{
		DWORD err = GetLastError();
		wglMakeCurrent(NULL, NULL);
		ReleaseMutex(vz_window_desc.lock);
		return;
	};

	// check if we need to make a screenshot
	if(*screenshot_file)
	{
		char* error_log;
		vzImage* screenshot = vzImageNewFromVB(tv.TV_FRAME_WIDTH,tv.TV_FRAME_HEIGHT);
		vzImageSaveTGA(screenshot_file,screenshot,&error_log);
		*screenshot_file = 0;
		vzImageFree(screenshot);
	};

	/* unbind offscreen buffer */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	/* clear */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT /*| GL_STENCIL_BUFFER_BIT */); 

	/* draw safe areas markers */
	if(NULL != tv.sa)
	{
		for(int i = 0; i<2; i++)
		{
			if(0 == i)
				/* red */
				glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
			else
				/* green */
				glColor4f(0.0f, 0.0f, 1.0f, 1.0f);

			glBegin(GL_LINE_LOOP);

			glVertex3f(kW * tv.sa->x[i], kH * tv.sa->y[i], 0.0f);
			glVertex3f(kW * (tv.TV_FRAME_WIDTH - tv.sa->x[i]), kH * tv.sa->y[i], 0.0f);
			glVertex3f(kW * (tv.TV_FRAME_WIDTH - tv.sa->x[i]), kH * (tv.TV_FRAME_HEIGHT - tv.sa->y[i]), 0.0f);
			glVertex3f(kW * tv.sa->x[i], kH * (tv.TV_FRAME_HEIGHT - tv.sa->y[i]), 0.0f);

			glEnd();
		};
	};

	/* draw texture */
	{
		// begin drawing
		glEnable(GL_TEXTURE_2D);

		/* calc texture geoms */
		W = (float)fbo.color_tex_width;
		H = (float)fbo.color_tex_height; 
		W *= kW;
		H *= kH;

		// draw rendered image

		X = 0.0f; Y = 0.0f;
		glBindTexture(GL_TEXTURE_2D, fbo.color_tex[fbo.index]);

		// Draw a quad (ie a square)
		glBegin(GL_QUADS);

		glColor4f(1.0f,1.0f,1.0f,1.0f);

		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(X, Y, 0.0f);

		glTexCoord2f(0.0f, 1.0f);
		glVertex3f(X, Y + H, 0.0f);

		glTexCoord2f(1.0f, 1.0f);
		glVertex3f(X + W, Y + H, 0.0f);

		glTexCoord2f(1.0f, 0.0f);
		glVertex3f(X + W, Y, 0.0f);

		glEnd(); // Stop drawing QUADS 

		glBindTexture(GL_TEXTURE_2D, 0);
	};

	/* draw background */
	{
		X = 0.0f, Y = 0.0f;
		glBindTexture(GL_TEXTURE_2D, fbo.bg_tex);

		// Draw a quad (ie a square)
		glBegin(GL_QUADS);

		glColor4f(1.0f,1.0f,1.0f,1.0f);

		glTexCoord2f(
			0.0f,
			0.0f);
		glVertex3f(
			X, 
			Y, 
			0.0f);

		glTexCoord2f(
			0.0f, 
			(float)(tv.TV_FRAME_HEIGHT/fbo.bg_tex_height));
		glVertex3f(
			X,
			Y + (float)(tv.TV_FRAME_HEIGHT),
			0.0f);

		glTexCoord2f(
			(float)(tv.TV_FRAME_WIDTH/fbo.bg_tex_width), 
			(float)(tv.TV_FRAME_HEIGHT/fbo.bg_tex_height));

		glVertex3f(
			X + (float)(tv.TV_FRAME_WIDTH),
			Y + (float)(tv.TV_FRAME_HEIGHT),
			0.0f);

		glTexCoord2f(
			(float)(tv.TV_FRAME_WIDTH/fbo.bg_tex_width), 
			0.0f);
		glVertex3f(
			X + (float)(tv.TV_FRAME_WIDTH), 
			Y, 
			0.0f);

		glEnd(); // Stop drawing QUADS

		glBindTexture(GL_TEXTURE_2D, 0);

		glDisable(GL_TEXTURE_2D);

	};

	// and swap buffers
	glFlush();
	SwapBuffers(vz_window_desc.hdc);

	// mark flag about first frame
	not_first_at = 1;

	/* unbind conext */
	wglMakeCurrent(NULL, NULL);
	ReleaseMutex(vz_window_desc.lock);

	/* check if we need to update frames rendering info */
	if((global_frame_no - last_title_update) > ((unsigned)(tv.TV_FRAME_PS_NOM / tv.TV_FRAME_PS_DEN)))
	{
		char buf[100];
		sprintf
		(
			buf,
			"Frame: %ld; drawn in: %ld miliseconds; distance in frames: %ld; "
			"tv mode: %s; vga scale: %d%%",
			global_frame_no,
			render_time,
			global_frame_no - rendered_frames,
			tv.NAME,
			100 >> tv.VGA_SCALE
		);
		last_title_update = global_frame_no;
		SetWindowText(vz_window_desc.wnd, buf);
	};
};

/* --------------------------------------------------------------------------

	OpenGL window

-------------------------------------------------------------------------- */
static void vz_window_reshape(int w, int h)
{
	w = (tv.TV_FRAME_WIDTH * tv.TV_FRAME_PAR_NOM) / tv.TV_FRAME_PAR_DEN;
	h = tv.TV_FRAME_HEIGHT;

	glViewport (0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D (0.0, (GLdouble) w, 0.0, (GLdouble) h);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
};

static int vz_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message)
	{
		case WM_ERASEBKGND:
			return 1;
			break;

		case WM_PAINT:
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_COMMAND: 
//			switch ( LOWORD ( wparam ) )
//			{
//			};
			break;

		case WM_KEYDOWN:
			switch (wparam)
			{
				case 'c':
				case 'C':
					//  Alarm scene clear !!!!!

					// lock scene
					WaitForSingleObject(scene_lock,INFINITE);

					if(scene)
						vzMainSceneFree(scene);
					scene = NULL;

					// unlock scene
					ReleaseMutex(scene_lock);

					break;

				case 's':
				case 'S':
					/* create a screen shot */
					WaitForSingleObject(scene_lock,INFINITE);
					if(scene)
						timestamp_screenshot();
					ReleaseMutex(scene_lock);
					break;
				};
			break;
		
		case WM_SIZE: 
			/* Resize the window with the new width and height. */
			WaitForSingleObject(vz_window_desc.lock, INFINITE);
			wglMakeCurrent(vz_window_desc.hdc, vz_window_desc.glrc);
			vz_window_reshape(LOWORD(lparam), HIWORD(lparam));
			wglMakeCurrent(NULL, NULL);
			ReleaseMutex(vz_window_desc.lock);
			break;

		default:
			return DefWindowProc(hwnd, message, wparam, lparam);
	};

	Sleep(10);

	return 0;
};

static int vz_destroy_window()
{
	if(0 != vz_window_desc.glrc)
	{
		wglMakeCurrent(NULL,NULL);
		wglDeleteContext(vz_window_desc.glrc);
	};

	if(0 != vz_window_desc.hdc)
		ReleaseDC(vz_window_desc.wnd, vz_window_desc.hdc);

	return 0;
};

static int vz_create_window()
{
	/* clear window desc struct */
	memset(&vz_window_desc, 0, sizeof(vz_window_desc));

	/* setup module instance */
	vz_window_desc.instance = GetModuleHandle(NULL);

	/* prepare window class */
	WNDCLASSEX wndClass;
	ZeroMemory              ( &wndClass, sizeof(wndClass) );
	wndClass.cbSize         = sizeof(WNDCLASSEX); 
	wndClass.style          = CS_OWNDC;
	wndClass.lpfnWndProc    = (WNDPROC)vz_window_proc;
	wndClass.cbClsExtra     = 0;
	wndClass.cbWndExtra     = 0;
	wndClass.hInstance      = vz_window_desc.instance;
	wndClass.hIcon          = 0;
	wndClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
	wndClass.lpszMenuName   = 0;
	wndClass.lpszClassName  = VZ_TITLE;
	wndClass.hIconSm        = 0;

	/* register the window class */
	if (RegisterClassEx(&wndClass) == 0)
	{
		logger_printf(1, "ERROR: Failed to register the window class!");
		return -1;
	};

	/* preapre styles */
	DWORD dwStyle = 
		WS_CAPTION | WS_SYSMENU |
		WS_OVERLAPPED |								// Creates an overlapping window
		WS_CLIPCHILDREN |							// Doesn't draw within child windows
		WS_CLIPSIBLINGS;							// Doesn't draw within sibling windows
	DWORD dwExStyle = 
		WS_EX_APPWINDOW |							// Top level window
        WS_EX_WINDOWEDGE;							// Border with a raised edge
   
	/* prepare dimensions */
	RECT rMain;
	rMain.left		= 0;
	rMain.right     = tv.vga_width;
	rMain.top       = 0;
	rMain.bottom    = tv.vga_height;  
	AdjustWindowRect ( &rMain, dwStyle, 0);

	/* Attempt to create the actual window. */
	vz_window_desc.wnd = CreateWindowEx
	(
		dwExStyle,									// Extended window styles
		VZ_TITLE,									// Class name
		VZ_TITLE,									// Window title (caption)
		dwStyle,									// Window styles
		0, 0,										// Window position
		rMain.right - rMain.left,
		rMain.bottom - rMain.top,					// Size of window
		0,											// No parent window
		NULL,										// Menu
		vz_window_desc.instance,					// Instance
		0											// Pass nothing to WM_CREATE
	);					
	if(0 == vz_window_desc.wnd ) 
    {
		logger_printf(1, "ERROR: Unable to create window! [%d]", GetLastError());
		vz_destroy_window();
		return -2;
    };

	/* Try to get a device context. */
	vz_window_desc.hdc = GetDC(vz_window_desc.wnd);
	if (!vz_window_desc.hdc ) 
    {
		vz_destroy_window();
		logger_printf(1, "ERROR: Unable to get a device context!");
		return -3;
    };

	/* Settings for the OpenGL window. */
	PIXELFORMATDESCRIPTOR pfd; 
	ZeroMemory (&pfd, sizeof(pfd));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);		// Size Of This Pixel Format Descriptor
	pfd.nVersion     = 1;							// The version of this data structure
	pfd.dwFlags      = 
		PFD_DRAW_TO_WINDOW |						// Buffer supports drawing to window
		PFD_SUPPORT_OPENGL |						// Buffer supports OpenGL drawing
		PFD_DOUBLEBUFFER;							// Supports double buffering
	pfd.dwLayerMask  = PFD_MAIN_PLANE;				// We want the standard mask (this is ignored anyway)
	pfd.iPixelType   = PFD_TYPE_RGBA;				// RGBA color format
	pfd.cColorBits   = 32;							// OpenGL color depth
	pfd.cAlphaBits   = 8;
	pfd.cDepthBits   = 16;							// Specifies the depth of the depth buffer
	pfd.iLayerType   = PFD_MAIN_PLANE;				// Ignored
	pfd.cAccumBits   = 0;							// No special bitplanes needed
	pfd.cStencilBits = 8;							// We desire 8 stencil bits

	/* Attempts to find the pixel format supported by a 
	device context that is the best match 
	to a given pixel format specification. */
	GLuint PixelFormat = ChoosePixelFormat(vz_window_desc.hdc, &pfd);
	if(0 == PixelFormat)
	{
		vz_destroy_window();
		logger_printf(1, "ERROR: Unable to find a suitable pixel format");
		return -4;
    };

	/* Sets the specified device context's pixel format to 
	the format specified by the PixelFormat. */
	if(!SetPixelFormat(vz_window_desc.hdc, PixelFormat, &pfd))
    {
		vz_destroy_window();
		logger_printf(1, "ERROR: Unable to set the pixel format");
		return -5;
    };

	/* Create an OpenGL rendering context. */
	vz_window_desc.glrc = wglCreateContext(vz_window_desc.hdc);
	if(0 == vz_window_desc.glrc)
    {
		vz_destroy_window();
		logger_printf(1, "ERROR: Unable to create an OpenGL rendering context");
		return -6;
    };

	/* Makes the specified OpenGL rendering context the calling thread's current rendering context. */
	if(!wglMakeCurrent (vz_window_desc.hdc, vz_window_desc.glrc))
    {
		vz_destroy_window();
		logger_printf(1, "ERROR: Unable to activate OpenGL rendering context");
		return -7;
    };

	/* load opengl Extensions */
	vzGlExtInit();

	/* init FBO */
	if (0 != init_fbo())
	{
		vz_destroy_window();
		logger_printf(1, "ERROR: FBO not supported by hardware");
		return -7;
    };


	/* clear */
	glClearColor (0.0, 0.0, 0.0, 0.0);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_ALPHA_TEST);
	glEnable (GL_BLEND);
	glShadeModel(GL_SMOOTH);

	/* reshape window */
	vz_window_reshape(0, 0);

	/* Settings to ensure that the window is the topmost window. */
	ShowWindow(vz_window_desc.wnd, SW_SHOW);
	SetForegroundWindow(vz_window_desc.wnd);
	SetFocus(vz_window_desc.wnd);

	/* unbind gl */
	wglMakeCurrent(NULL, NULL);

	return 0;
};

static void vz_window_event_loop()
{
	/* event loop */
	MSG msg;
	int f_exit = 0;
	while (!f_exit)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
			if (msg.message == WM_QUIT)
				f_exit = 1;
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
            };
		};
    };
};


/*
----------------------------------------------------------

  MAIN function

----------------------------------------------------------
*/

int main(int argc, char** argv)
{
	/* init timer period */
	timeBeginPeriod(1);

	/* default ttFont path */
	vzTTFontAddFontPath("fonts");

#ifdef _DEBUG
{
	vzTTFont* temp = new vzTTFont("m1-h", NULL);
	delete temp;
}
#endif /* _DEBUG */

	// analization of params
	application = *argv;
	argc--;argv++;	// skip own name
	int parse_args = 1;
	while((argc >= 2) && (parse_args))
	{
		if (strcmp(*argv,"-c") == 0)
		{
			// set config file name
			argc -= 2;argv++;config_file = *argv; argv++;
		}
		else if (strcmp(*argv,"-f") == 0)
		{
			// set scene file name
			argc -= 2;argv++;scene_file = *argv; argv++;
		}
		else if ( (strcmp(*argv,"-h") == 0) || (strcmp(*argv,"/?") == 0) )
		{
			printf("Usage: vz.exe [ -f <scene file> ] [-c <config file> ]\n");
			return 0;
		}
		else
			parse_args = 0;
	};

	/* setup logger */
	logger_setup("vz.log", 3600*24, 0);

	/* hello message */
	logger_printf(0, "%s (vz-%s) [controller]",VZ_TITLE, VZ_VERSION);

	// loading config
	logger_printf(0, "Loading config file '%s'...", config_file);
	config = vzConfigOpen(config_file);
	logger_printf(0, "Config file Loaded!");

	// clear screenshot filename
	screenshot_file[0] = 0;
	
	// setting tv system
	vzConfigTVSpec(config,"tvspec", &tv);

	// init output module
	char *output_module_name = vzConfigParam(config,"main","output");
	if(output_module_name)
	{
		logger_printf(0, "Loading output module '%s'...", output_module_name);
		output_module = vzOutputNew(config,output_module_name,&tv);
		if (!(vzOutputReady(output_module)))
		{
			logger_printf(1, "Failed to load '%s'", output_module_name);
			vzOutputFree(output_module);
		}
		else
			logger_printf(0, "Module '%s' loaded", output_module_name);
	};

	/* add font path */
	vzTTFontAddFontPath(vzConfigParam(config,"main","font_path"));

	/* create output window */
	if(0 == vz_create_window())
	{
		/* create gl lock */
		vz_window_desc.lock = CreateMutex(NULL,FALSE,NULL);

		// syncs
		global_frame_event = CreateEvent(NULL, TRUE, FALSE, NULL);
		ResetEvent(global_frame_event);
		scene_lock = CreateMutex(NULL,FALSE,NULL);

		// init functions
		functions = vzMainNewFunctionsList(config);

		// try to create sync thread in output module
		int output_module_sync = 0;
		if(output_module)
			output_module_sync = vzOutputSync(output_module, frames_counter);

		// check if flag. if its not OK - start internal
		if(output_module_sync == 0)
		{
			internal_sync_generator_handle = CreateThread
			(
				NULL,
				1024,
				internal_sync_generator,
				frames_counter, // params
				0,
				NULL
			);

			SetThreadPriority(internal_sync_generator_handle , THREAD_PRIORITY_HIGHEST);
		};

		/* sync render */
		{
			sync_render_handle = CreateThread
			(
				NULL,
				1024,
				sync_render,
				NULL, // params
				0,
				NULL
			);

			SetThreadPriority(sync_render_handle , THREAD_PRIORITY_HIGHEST);
		};

		// check if we need automaticaly load scene
		if(scene_file)
		{
			// init scene
			scene = vzMainSceneNew(functions,config,&tv);
			if (!vzMainSceneLoad(scene, scene_file))
			{
				logger_printf(1, "Unable to load scene '%s'",scene_file);
				vzMainSceneFree(scene);
			};
			scene_file = NULL;
		};

		// start tcpserver/serial server
		unsigned long thread;
		HANDLE tcpserver_handle = CreateThread(0, 0, tcpserver, config, 0, &thread);
		HANDLE serserver_handle = CreateThread(0, 0, serserver, config, 0, &thread);
		SetThreadPriority(tcpserver_handle , THREAD_PRIORITY_LOWEST);
		SetThreadPriority(serserver_handle , THREAD_PRIORITY_LOWEST);

		/* event loop */
		vz_window_event_loop();
		f_exit = 1;
		logger_printf(1, "main: vz_window_event_loop finished");

		/* stop tcpserver/serial server */
		logger_printf(1, "main: tcpserver_kill...");
		tcpserver_kill();
		logger_printf(1, "main: tcpserver_kill... DONE");

		logger_printf(1, "main: serserver_kill...");
		serserver_kill();
		logger_printf(1, "main: serserver_kill... DONE");

		logger_printf(1, "main: waiting for tcpserver_handle...");
		WaitForSingleObject(tcpserver_handle, INFINITE);
		logger_printf(1, "main: WaitForSingleObject(tcpserver_handle) finished");

		logger_printf(1, "main: waiting for serserver_handle...");
		WaitForSingleObject(serserver_handle, INFINITE);
		logger_printf(1, "main: WaitForSingleObject(serserver_handle) finished");

		CloseHandle(serserver_handle);
		CloseHandle(tcpserver_handle);

		/* stop sync render thread */
		logger_printf(1, "main: waiting for sync_render_handle...");
		WaitForSingleObject(sync_render_handle, INFINITE);
		logger_printf(1, "main: WaitForSingleObject(sync_render_handle) finished");
		CloseHandle(sync_render_handle);

		/* stop internal sync generator */
		if(INVALID_HANDLE_VALUE != internal_sync_generator_handle)
		{
			logger_printf(1, "main: waiting for internal_sync_generator_handle...");
			WaitForSingleObject(internal_sync_generator_handle, INFINITE);
			logger_printf(1, "main: WaitForSingleObject(internal_sync_generator_handle) finished");
			CloseHandle(internal_sync_generator_handle);
		};

		/* unload scene */
		if(NULL != scene)
		{
			logger_printf(1, "main: waiting for vzMainSceneFree(scene)...");
			vzMainSceneFree(scene);
			logger_printf(1, "main: vzMainSceneFree(scene) finished");
			scene = NULL;
		};

		logger_printf(1, "main: waiting for vzMainFreeFunctionsList(functions)...");
		vzMainFreeFunctionsList(functions);
		logger_printf(1, "main: vzMainFreeFunctionsList(functions) finished");

		/* destroy window */
		logger_printf(1, "main: waiting for vz_destroy_window()...");
		vz_destroy_window();
		logger_printf(1, "main: vz_destroy_window() finished...");

		/* close mutexes */
		CloseHandle(scene_lock);
		CloseHandle(vz_window_desc.lock);
	};

	/* cleanup */
	if(output_module_name)
	{
		logger_printf(1, "main: waiting for vzOutputFree(output_module)...");
		vzOutputFree(output_module);
		logger_printf(1, "main: vzOutputFree(output_module) finished...");
		output_module = NULL;
	};

	logger_printf(1, "main: Bye!");

	return 0;
};
