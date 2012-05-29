/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2011 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1stereo.tv, 2011.

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
*/
#define _CRT_SECURE_NO_WARNINGS

#include "memleakcheck.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <time.h>
#include <direct.h>
#include <errno.h>
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
#include "udpserver.h"

#include "main.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib") 

//#define DEBUG_TIMINGS
#ifdef DEBUG_TIMINGS
#define OUTPUT_TIMINGS(PREF) logger_printf(1, "main: " PREF ": %s:%d", __FILE__, __LINE__);
#else
#define OUTPUT_TIMINGS(PREF)
#endif

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

void* layers[VZ_MAX_LAYERS];
char* startup_scene_file = NULL;
HANDLE layers_lock;

char* config_file = "vz.conf";
void* config = NULL; // config loaded

void* functions = NULL; // list of loaded function
void* output_context = NULL; // output module
char start_path[1024];
char* application;
char screenshot_file[1024];

/* -------------------------------------------------------- */
int CMD_screenshot(char* filename, char** error_log)
{
	/* notify */
	logger_printf(0, "CMD_screenshot [%s]", filename);

	// lock scene
	WaitForSingleObject(layers_lock, INFINITE);

	// copy filename
	strcpy(screenshot_file, filename);

	// unlock scene
	ReleaseMutex(layers_lock);

	return 0;
};

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

typedef struct
{
    HINSTANCE instance;
    HDC hdc;
    HWND wnd;
    HGLRC glrc;
    HANDLE lock;
} vz_window_t;

static vz_window_t vz_window_desc;

static int vz_window_lockgl(vz_window_t* w)
{
    if(WaitForSingleObject(w->lock, INFINITE))
    {
        logger_printf(1, "vz_window_lockgl: WaitForSingleObject failed");
        return -EFAULT;
    };

    if(!wglMakeCurrent(w->hdc, w->glrc))
    {
        logger_printf(1, "vz_window_lockgl: wglMakeCurrent failed");
        return -EFAULT;
    };

    return 0;
};

static int vz_window_unlockgl(vz_window_t* w)
{
    int r = 0;

    if(!wglMakeCurrent(NULL, NULL))
    {
        r = -EFAULT;
        logger_printf(1, "vz_window_unlockgl: wglMakeCurrent failed");
    };

    if(!ReleaseMutex(w->lock))
    {
        r = -EFAULT;
        logger_printf(1, "vz_window_lockgl: ReleaseMutex failed");
    };

    return r;
};

static HANDLE global_frame_event;
static unsigned long global_frame_no = 0;

#define ENSURE_OPENGL_CONTEXT(BLOCK)                                \
    if(vz_window_lockgl(&vz_window_desc))                           \
        logger_printf(1, "ENSURE_OPENGL_CONTEXT FAILED at %s:%d",   \
            __FILE__, __LINE__);                                    \
    else                                                            \
    {                                                               \
        BLOCK;                                                      \
        vz_window_unlockgl(&vz_window_desc);                        \
    };

/* ---------------------------------------------------------
    scene load/unload
-----------------------------------------------------------*/
int CMD_layer_unload(long idx)
{
    int r = 0;

    /* notify */
    logger_printf(0, "CMD_layer_unload [%ld]", idx);

    // lock scene
    WaitForSingleObject(layers_lock, INFINITE);

    if(idx < 0 || idx >= VZ_MAX_LAYERS)
    {
        r = -1;
        logger_printf(2, "Unable to unload layer [%ld] - OUT OF LAYER", idx);
    }
    else
    {
        void* scene_tmp = NULL;

        /* free loaded scene */
        if(layers[idx])
        {
            scene_tmp = layers[idx];
            layers[idx] = NULL;

            // unlock scene
            ReleaseMutex(layers_lock);

            ENSURE_OPENGL_CONTEXT(vzMainSceneRelease(scene_tmp););

            vzMainSceneFree(scene_tmp);

            return r;
        };
    };

    // unlock scene
    ReleaseMutex(layers_lock);

    return r;
};

int CMD_layer_load(char* filename, long idx)
{
    int r = 0;

    /* notify */
    logger_printf(0, "CMD_layer_load [%s,%ld]", filename, idx);

    // lock scene
    WaitForSingleObject(layers_lock, INFINITE);

    if(idx < 0 || idx >= VZ_MAX_LAYERS)
    {
        r = -1;
        logger_printf(2, "Unable to load scene [%s] - OUT OF LAYER", filename);
    }
    else
    {
        void* scene_tmp = NULL;

        /* free loaded scene */
        scene_tmp = layers[idx];
        layers[idx] = NULL;

        ReleaseMutex(layers_lock);

        if(scene_tmp)
        {
            /* release scene */
            ENSURE_OPENGL_CONTEXT(vzMainSceneRelease(scene_tmp););

            /* free scene */
            vzMainSceneFree(scene_tmp);
        };

        /* create a new scene */
        scene_tmp = vzMainSceneNew(functions, config, &tv);

        /* check if scene loaded correctly */
        if (!vzMainSceneLoad(scene_tmp, filename))
        {
            logger_printf(2, "Unable to load scene [%s]", filename);

            vzMainSceneFree(scene_tmp);

            r = -2;
            scene_tmp = NULL;
        };

        /* init scene */
        ENSURE_OPENGL_CONTEXT(vzMainSceneInit(scene_tmp););

        WaitForSingleObject(layers_lock, INFINITE);

        layers[idx] = scene_tmp;
    };

    // unlock scene
    ReleaseMutex(layers_lock);

    return r;
};

/*----------------------------------------------------------
	render loop
----------------------------------------------------------*/
static int vz_scene_render(int rendered_limit);
static void vz_scene_display(void);
static unsigned long WINAPI sync_render(void* data)
{
    int l = (int)data;
    DWORD d;

    do
    {
        /* render frames */
        vz_scene_render(l);

        /* notify to redraw bg or direct render screen */
        vz_scene_display();

        /* one more check before waiting */
        if(f_exit) break;

        /* wait for sync */
        d = WaitForSingleObject(global_frame_event, INFINITE);

        /* break loop on error */
        if(d) break;
    }
    while(!f_exit);

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
    glErrorLog(glGenFramebuffersEXT(1, &fbo.fb););
    glErrorLog(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo.fb););

	// initialize color texture
	for(int i = 0; i<2; i++)
	{

        glErrorLog(glGenTextures(1, &fbo.color_tex[i]););
        glErrorLog(glBindTexture(GL_TEXTURE_2D, fbo.color_tex[i]););
        glErrorLog(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR););
        glErrorLog(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR););
        glErrorLog(glTexImage2D
		(
			GL_TEXTURE_2D, 
			0, 
			GL_RGBA8, 
			fbo.color_tex_width, fbo.color_tex_height, 
			0, 
			GL_BGRA_EXT, GL_UNSIGNED_BYTE, 
			NULL
        ););

        glErrorLog(glFramebufferTexture2DEXT
		(
			GL_FRAMEBUFFER_EXT, 
			GL_COLOR_ATTACHMENT0_EXT + i, 
			GL_TEXTURE_2D, 
			fbo.color_tex[i], 
			0
        ););
	};

    glErrorLog(glGenRenderbuffersEXT(1, &fbo.stencil_depth_rb););
    glErrorLog(glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, fbo.stencil_depth_rb););
    glErrorLog(glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, fbo.color_tex_width, fbo.color_tex_height););
    glErrorLog(glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,  GL_RENDERBUFFER_EXT, fbo.stencil_depth_rb););
    glErrorLog(glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, fbo.stencil_depth_rb););

    /* create background texture */
    glErrorLog(glGenTextures(1, &fbo.bg_tex););
    glErrorLog(glBindTexture(GL_TEXTURE_2D, fbo.bg_tex););
    glErrorLog(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR););
    glErrorLog(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR););
    glErrorLog(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT););
    glErrorLog(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT););
    glErrorLog(glTexImage2D
	(
		GL_TEXTURE_2D, 
		0, 
		GL_RGBA8, 
		(fbo.bg_tex_width = 16), (fbo.bg_tex_height = 16),
		0, 
		GL_BGRA_EXT, GL_UNSIGNED_BYTE, 
		fb_bg_text_data
    ););


    glErrorLog(glBindTexture(GL_TEXTURE_2D, 0););

	fbo.index = 0;
    glErrorLog(glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT + (0 + fbo.index) ););
    glErrorLog(glReadBuffer(GL_COLOR_ATTACHMENT0_EXT + (1 - fbo.index) ););

	return 0;
};


/*----------------------------------------------------------
	main OpenGL operations
----------------------------------------------------------*/

static long render_time = 0;
static long rendered_frames = 0;
static long last_title_update = 0;

static int vz_scene_render(int rendered_limit)
{
    int idx, cnt, r, rendered_local = 0;

    /* check if extensions loaded */
    if(!(glExtInitDone)) 
        return -ENOMEM;

    r = vz_window_lockgl(&vz_window_desc);
    if(r) return r;

    /* bind buffer */
    glErrorLog(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo.fb););

    /* check if we need to draw frame */
    while(vzOutputRenderSlots(output_context) > 0 && !f_exit
        && rendered_local < rendered_limit)
    {
        rendered_local++;
        rendered_frames++;

        // save time of draw start
        long draw_start_time = timeGetTime();

        // finish all rendering
//        glFinish();

        // output module tricks
        vzOutputPreRender(output_context);

        // lock scene
        WaitForSingleObject(layers_lock,INFINITE);

OUTPUT_TIMINGS("vz_scene_render");

        /* draw layers */
        void* render_starter = NULL;
        for(idx = 0, cnt = 0; idx < VZ_MAX_LAYERS && !layers[idx]; idx++);
        if(idx < VZ_MAX_LAYERS && layers[idx])
            vzMainSceneDisplay(layers[idx], global_frame_no, VZ_MAX_LAYERS, layers);
        else
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // unlock scene
        ReleaseMutex(layers_lock);

        vzOutputPostRender(output_context);

        // flush all 
        glFlush();

OUTPUT_TIMINGS("vz_scene_render");

        // swap draw/read buffers
        fbo.index = 1 - fbo.index;
        glErrorLog(glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT + (0 + fbo.index)););
        glErrorLog(glReadBuffer(GL_COLOR_ATTACHMENT0_EXT + (1 - fbo.index)););
OUTPUT_TIMINGS("vz_scene_render");

        // save time of draw start
        long draw_stop_time = timeGetTime();

        render_time = draw_stop_time - draw_start_time;
#ifdef DEBUG_TIMINGS
        if(render_time > 30)
            logger_printf(1, "frame[%d] draws to slow: %d",
                rendered_frames, render_time);
#endif

    };

    /* unbind conext */
    vz_window_unlockgl(&vz_window_desc);

//    logger_printf(1, "vz_scene_render: %d", rendered_local);

    return rendered_local;
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

    if(vz_window_lockgl(&vz_window_desc)) return;

    // check if we need to make a screenshot
    if(*screenshot_file)
    {
        int r;
        char* error_log;
        vzImage* temp_image;

        // create buffer
        r = vzImageCreate(&temp_image, tv.TV_FRAME_WIDTH, tv.TV_FRAME_HEIGHT,
            VZIMAGE_PIXFMT_BGRA);
        if(!r)
        {
            /* read gl pixels to buffer - ready to use */
            glErrorLogD(glReadPixels(0, 0, tv.TV_FRAME_WIDTH, tv.TV_FRAME_HEIGHT,
                GL_BGRA_EXT, GL_UNSIGNED_BYTE, temp_image->surface););

            /* save image */
            vzImageSaveTGA(screenshot_file, temp_image, &error_log);

            /* release */
            vzImageRelease(&temp_image);
        };

        /* reset flag */
        *screenshot_file = 0;
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
			(float)(tv.vga_height/fbo.bg_tex_height));
		glVertex3f(
			X,
			Y + (float)(tv.vga_height),
			0.0f);

		glTexCoord2f(
			(float)(tv.vga_width/fbo.bg_tex_width), 
			(float)(tv.vga_height/fbo.bg_tex_height));

		glVertex3f(
			X + (float)(tv.vga_width),
			Y + (float)(tv.vga_height),
			0.0f);

		glTexCoord2f(
			(float)(tv.vga_width/fbo.bg_tex_width), 
			0.0f);
		glVertex3f(
			X + (float)(tv.vga_width), 
			Y, 
			0.0f);

		glEnd(); // Stop drawing QUADS

		glBindTexture(GL_TEXTURE_2D, 0);

		glDisable(GL_TEXTURE_2D);

	};

	// and swap buffers
	glFlush();

    /* unbind conext */
    vz_window_unlockgl(&vz_window_desc);

	/* check if we need to update frames rendering info */
	if((global_frame_no - last_title_update) > ((unsigned)(tv.TV_FRAME_PS_NOM / tv.TV_FRAME_PS_DEN)))
	{
		char buf[1024];
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
    h = tv.TV_FRAME_HEIGHT;
    w = (tv.TV_FRAME_WIDTH * tv.TV_FRAME_PAR_NOM) / tv.TV_FRAME_PAR_DEN;

	glViewport (0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D (0.0, (GLdouble) w, 0.0, (GLdouble) h);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
};

#ifdef SCENE_LOAD_LEAK_DETECT
static void* test_scene = NULL;
static _CrtMemState s;
#endif /* SCENE_LOAD_LEAK_DETECT */
static LRESULT vz_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message)
	{
		case WM_ERASEBKGND:
			return 1;
			break;

		case WM_PAINT:
			break;

        case WM_CLOSE:
            f_exit = 1;
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
#ifdef SCENE_LOAD_LEAK_DETECT
                case 'd':
                case 'D':
                    if(!test_scene)
                    {
                        _CrtMemCheckpoint(&s);
                        _CrtMemDumpStatistics(&s);
                        test_scene = vzMainSceneNew(functions, config, &tv);
                        vzMainSceneLoad(test_scene, "./weather_five-days/weather_five-days_v02_01.xml");
                    };
                    break;
                case 'f':
                case 'F':
                    if(test_scene)
                        vzMainSceneFree(test_scene);
                    _CrtMemDumpAllObjectsSince(&s);
                    _CrtMemDumpStatistics(&s);
                    test_scene = NULL;
                    break;
                case 'e':
                case 'E':
                    _CrtMemDumpAllObjectsSince(&s);
                    _CrtMemDumpStatistics(&s);
                    break;
#endif /* SCENE_LOAD_LEAK_DETECT */

				case 'c':
				case 'C':
					//  Alarm scene clear !!!!!

                    // lock layers
                    WaitForSingleObject(layers_lock,INFINITE);

                    for(int idx= 0; idx < VZ_MAX_LAYERS; idx++)
                    {
                        if(layers[idx]) vzMainSceneFree(layers[idx]);
                        layers[idx] = NULL;
                    };

                    // unlock layers
                    ReleaseMutex(layers_lock);

					break;

				case 's':
				case 'S':
                    /* create a screen shot */
                    WaitForSingleObject(layers_lock,INFINITE);
                    timestamp_screenshot();
                    ReleaseMutex(layers_lock);
					break;
				};
			break;
		
		case WM_SIZE: 
			/* Resize the window with the new width and height. */
            ENSURE_OPENGL_CONTEXT(vz_window_reshape(LOWORD(lparam), HIWORD(lparam)););
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
    pfd.dwFlags = 
        PFD_DRAW_TO_WINDOW |						// Buffer supports drawing to window
        PFD_SUPPORT_OPENGL;                         // Buffer supports OpenGL drawing
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
    if(vzGlExtInit())
    {
        vz_destroy_window();
        logger_printf(1, "ERROR: not all OpenGL extensions supported");
        return -7;
    };

	/* init FBO */
	if (0 != init_fbo())
	{
		vz_destroy_window();
		logger_printf(1, "ERROR: FBO not supported by hardware");
		return -7;
    };

    /* init shaders */
    if (vzGlExtShader())
    {
        vz_destroy_window();
        logger_printf(1, "ERROR: shaders initialization failed");
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

	while (!f_exit)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
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
    char *output_context_name;

#ifdef _DEBUG
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif /* _DEBUG */

    /* clear layers */
    for(int l = 0; l < VZ_MAX_LAYERS; l++)
        layers[l] = NULL;

	/* init timer period */
	timeBeginPeriod(1);

    /* init xml engine */
    vzMainXMLInit();

	/* default ttFont path */
    vzTTFont::init_freetype();
	vzTTFontAddFontPath("fonts");
    get_font_init();

//#define _DEBUG_IMG "http://we-manager.internal.m1stereo.tv/spool/photos/00000008.png"
//#define _DEBUG_IMG "ftp://10.1.1.17/pub/XCHG/1/test.png"
#ifdef _DEBUG_IMG
    vzImage* img = NULL;
    int r = vzImageLoad(&img, _DEBUG_IMG);
    vzImageRelease(&img);
#endif /* _DEBUG_IMG */

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
            // set startup scene file name
            argc -= 2; argv++; startup_scene_file = *argv; argv++;
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
    logger_init();

	/* hello message */
	logger_printf(0, "%s (vz-%s) [controller]",VZ_TITLE, VZ_VERSION);

	// loading config
	logger_printf(0, "Loading config file '%s'...", config_file);
	config = vzConfigOpen(config_file);
	logger_printf(0, "Config file Loaded!");

	/* setup log to console if flag defined */
	if(NULL != vzConfigParam(config, "main", "log_to_console"))
		logger_dup_to_console();

	// clear screenshot filename
	screenshot_file[0] = 0;
	
	// setting tv system
	vzConfigTVSpec(config,"tvspec", &tv);

	/* add font path */
	vzTTFontAddFontPath(vzConfigParam(config,"main","font_path"));

    /* create output */
    output_context_name = vzConfigParam(config, "main", "output");
    logger_printf(0, "Loading output modules '%s'...", output_context_name);
    vzOutputNew(&output_context, config, output_context_name, &tv);
    vzOutputGlobalContextSet(output_context);

	/* create output window */
	if(0 == vz_create_window())
	{
        int render_limit = 4;
        HANDLE sync_render_handle;

        /* create gl lock */
        vz_window_desc.lock = CreateMutex(NULL,FALSE,NULL);

        /* init layers lock */
        layers_lock = CreateMutex(NULL,FALSE,NULL);

        // syncs
        global_frame_event = CreateEvent(NULL, TRUE, FALSE, NULL);
        ResetEvent(global_frame_event);

        // init output
        vz_window_lockgl(&vz_window_desc);
        vzOutputInit(output_context, global_frame_event, &global_frame_no);
        vz_window_unlockgl(&vz_window_desc);

        // init functions
        vz_window_lockgl(&vz_window_desc);
        functions = vzMainNewFunctionsList(config);
        vz_window_unlockgl(&vz_window_desc);

        /* change working directory */
        if(vzConfigParam(config, "main", "change_root"))
            _chdir(vzConfigParam(config, "main", "change_root"));

        /* change render limit */
        if(vzConfigParam(config, "main", "render_limit"))
            render_limit = atol(vzConfigParam(config, "main", "render_limit"));

        // check if we need automaticaly load scene
        if(startup_scene_file)
        {
            CMD_layer_load(startup_scene_file, 0);
            startup_scene_file = NULL;
        };

        /* sync render */
        sync_render_handle = CreateThread
        (
            NULL,
            1024,
            sync_render,
            (void*)render_limit,
            0,
            NULL
        );
//SetThreadPriority(sync_render_handle , THREAD_PRIORITY_HIGHEST);

		// start (tcp|udp)server
		unsigned long thread;
		HANDLE tcpserver_handle = INVALID_HANDLE_VALUE;
		HANDLE udpserver_handle = INVALID_HANDLE_VALUE;
		// init winsock
#define WS_VER_MAJOR 2
#define WS_VER_MINOR 2
		WSADATA wsaData;
		if ( WSAStartup( ((unsigned long)WS_VER_MAJOR) | (((unsigned long)WS_VER_MINOR)<<8), &wsaData ) != 0 )
			logger_printf(1, "main: WSAStartup() failed");
		else
		{
			tcpserver_handle = CreateThread(0, 0, tcpserver, config, 0, &thread);
			SetThreadPriority(tcpserver_handle , THREAD_PRIORITY_LOWEST);
			udpserver_handle = CreateThread(0, 0, udpserver, config, 0, &thread);
			SetThreadPriority(udpserver_handle , THREAD_PRIORITY_LOWEST);
		};

		/* start serial server */
		HANDLE serserver_handle = CreateThread(0, 0, serserver, config, 0, &thread);
		SetThreadPriority(serserver_handle , THREAD_PRIORITY_LOWEST);

        // run output
        vz_window_lockgl(&vz_window_desc);
        vzOutputRun(output_context);
        vz_window_unlockgl(&vz_window_desc);

		/* event loop */
		vz_window_event_loop();
		f_exit = 1;
		logger_printf(1, "main: vz_window_event_loop finished");

        // stop output
        vz_window_lockgl(&vz_window_desc);
        vzOutputStop(output_context);
        vz_window_unlockgl(&vz_window_desc);

		/* stop udpserver server */
		if(INVALID_HANDLE_VALUE != udpserver_handle)
		{
			logger_printf(1, "main: udpserver_kill...");
			udpserver_kill();
			logger_printf(1, "main: udpserver_kill... DONE");
		};

		/* stop tcpserver/serial server */
		if(INVALID_HANDLE_VALUE != tcpserver_handle)
		{
			logger_printf(1, "main: tcpserver_kill...");
			tcpserver_kill();
			logger_printf(1, "main: tcpserver_kill... DONE");
		};

		logger_printf(1, "main: serserver_kill...");
		serserver_kill();
		logger_printf(1, "main: serserver_kill... DONE");

		if(INVALID_HANDLE_VALUE != udpserver_handle)
		{
			logger_printf(1, "main: waiting for udpserver_handle...");
			WaitForSingleObject(udpserver_handle, INFINITE);
			logger_printf(1, "main: WaitForSingleObject(udpserver_handle) finished");
			CloseHandle(udpserver_handle);
		};

		if(INVALID_HANDLE_VALUE != tcpserver_handle)
		{
			logger_printf(1, "main: waiting for tcpserver_handle...");
			WaitForSingleObject(tcpserver_handle, INFINITE);
			logger_printf(1, "main: WaitForSingleObject(tcpserver_handle) finished");
			CloseHandle(tcpserver_handle);
		};

		logger_printf(1, "main: waiting for serserver_handle...");
		WaitForSingleObject(serserver_handle, INFINITE);
		logger_printf(1, "main: WaitForSingleObject(serserver_handle) finished");
		CloseHandle(serserver_handle);

        /* stop sync render thread */
        logger_printf(1, "main: waiting for sync_render_handle...");
        while(WAIT_TIMEOUT == WaitForSingleObject(sync_render_handle, 100))
            SetEvent(global_frame_event);
        CloseHandle(sync_render_handle);
        CloseHandle(global_frame_event);
        logger_printf(1, "main: WaitForSingleObject(sync_render_handle) finished");

        /* unload layers */
        for(int idx = 0; idx < VZ_MAX_LAYERS; idx++)
        {
            logger_printf(1, "main: waiting for CMD_layer_unload(%d)...", idx);
            CMD_layer_unload(idx);
            logger_printf(1, "main: CMD_layer_unload(%d) finished", idx);
        };

		logger_printf(1, "main: waiting for vzMainFreeFunctionsList(functions)...");
        wglMakeCurrent(vz_window_desc.hdc, vz_window_desc.glrc);
		vzMainFreeFunctionsList(functions);
        wglMakeCurrent(NULL, NULL);
		logger_printf(1, "main: vzMainFreeFunctionsList(functions) finished");

        // release output
        vz_window_lockgl(&vz_window_desc);
        logger_printf(1, "main: waiting for vzOutputRelease(output_context)...");
        vzOutputRelease(output_context);
        logger_printf(1, "main: vzOutputRelease(output_context) finished...");
        vz_window_unlockgl(&vz_window_desc);

		/* destroy window */
		logger_printf(1, "main: waiting for vz_destroy_window()...");
		vz_destroy_window();
		logger_printf(1, "main: vz_destroy_window() finished...");

		/* close mutexes */
        CloseHandle(layers_lock);
		CloseHandle(vz_window_desc.lock);
	};

    // release output
    logger_printf(1, "main: waiting for vzOutputRelease(output_context)...");
    vzOutputFree(&output_context);
    logger_printf(1, "main: vzOutputRelease(output_context) finished...");

	logger_printf(1, "main: Bye!");
	logger_release();

    /* delete config */
    vzConfigClose(config);

    /* release freetype lib */
    get_font_release();
    vzTTFont::release_freetype();

    /* release xml engine */
    vzMainXMLRelease();

#ifdef _DEBUG
    _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
    _CrtDumpMemoryLeaks();
#endif /* _DEBUG */

	return 0;
};
