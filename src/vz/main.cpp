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

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <time.h>
#include "vzGlExt.h"

#include "vzMain.h"
#include "vzOutput.h"
#include "vzImage.h"
#include "vzTVSpec.h"
#ifdef _DEBUG
#include "vzTTFont.h"
#endif

#include "tcpserver.h"

/*
----------------------------------------------------------
	Main Program Info
----------------------------------------------------------
*/
const char* VZ_TITLE = "ViZualizator";
float VZ_VERSION_NUMBER = 1.0f;
const char* VZ_VERSION_SUFFIX = "rc8";
vzTVSpec tv;

/*
----------------------------------------------------------
	Globals program attributes

----------------------------------------------------------
*/
void* scene = NULL;	// scene loaded
char* scene_file = NULL;

char* config_file = "vz.conf";
void* config = NULL; // config loaded

HANDLE scene_lock;
void* functions = NULL; // list of loaded function
void* output_module = NULL; // output module
char start_path[1024];
char* application;
//int main_stage;
char screenshot_file[1024];
//int use_offscreen_buffer = 0;

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
static HANDLE global_frame_event;
static unsigned long global_frame_no = 0;
//static unsigned long stop_global_frames_counter = 0;
static long skip_draw = 0;
static long not_first_at = 0;
static unsigned long WINAPI internal_sync_generator(void* fc_proc_addr)
{
	frames_counter_proc fc = (frames_counter_proc)fc_proc_addr;
	
	// endless loop
	while(1)
	{
		// sleep
		Sleep(tv.TV_FRAME_DUR_MS);

#ifdef DEBUG_HARD_SYNC
		printf("[%-10d] internal_sync_generator\n", timeGetTime());
#endif /* DEBUG_HARD_SYNC */

		// call frame counter
		if (fc) fc();
	};
};

static void frames_counter()
{
#ifdef DEBUG_HARD_SYNC
printf("[%-10d] frames_counter\n", timeGetTime());
#endif /* DEBUG_HARD_SYNC */

	// reset event
	ResetEvent(global_frame_event);

	// increment frame counter
	global_frame_no++;

	// rise evennt
	PulseEvent(global_frame_event);
};

static unsigned long WINAPI sync_render(void* data)
{
	int r,p;

	while(1)
	{
		p = 0;
		r = WaitForSingleObject(global_frame_event, 15) ;/* INFINITE); */
		if(WAIT_OBJECT_0 == r)
			p = 1;
		else
		{
			if
			(
				(NULL != output_module)
				&&
				(vzOuputRenderSlots(output_module) > 0)
			)
				p = 1;
		};

		/* check if we need to pulse */
		if(p)
		{
			ResetEvent(global_frame_event);

#ifdef DEBUG_HARD_SYNC
			printf("[%-10d] sync_render\n", timeGetTime());
#endif /* DEBUG_HARD_SYNC */

			if(not_first_at)
			{
				// reset flag of forcing redraw
				skip_draw = 0;

				// notify about redisplay
				glutPostRedisplay();
			};
		};
	};
};


/*----------------------------------------------------------
	GLUT operatives
----------------------------------------------------------*/
static long render_time = 0;
static long rendered_frames = 0;
static long last_title_update = 0;

static void vz_glut_display(void)
{
	int force_render = 0, force_rendered = 0;

	// check if redraw should processed
	// by internal needs
	if (skip_draw)
		return;

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

#ifdef DEBUG_HARD_SYNC
		printf("[%-10d] vz_glut_display\n", timeGetTime());
#endif /* DEBUG_HARD_SYNC */

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

#ifdef RENDER_TO_AUX_BUFFERS
		/* fist case */
		if(1 == rendered_frames)
		{glReadBuffer (GL_AUX0); glDrawBuffer (GL_AUX1);};

		/* copy pixels */
		int b1,b2;

		glGetIntegerv(GL_DRAW_BUFFER, &b1);
		glGetIntegerv(GL_READ_BUFFER, &b2);

		/* copy to front */
		glReadBuffer (b1); glDrawBuffer (GL_FRONT); 
		glClear(GL_COLOR_BUFFER_BIT); glCopyPixels (0,0,tv.TV_FRAME_WIDTH,tv.TV_FRAME_HEIGHT,GL_COLOR);

		/* swap buffers */
		glDrawBuffer (b2);
		glReadBuffer (b1); 

#endif /* RENDER_TO_AUX_BUFFERS */

		// and swap buffers
		glutSwapBuffers();

		// save time of draw start
		long draw_stop_time = timeGetTime();
		render_time = draw_stop_time - draw_start_time;

		// check if we need to make a screenshot
		if(*screenshot_file)
		{
			char* error_log;
			vzImage* screenshot = vzImageNewFromVB(tv.TV_FRAME_WIDTH,tv.TV_FRAME_HEIGHT);
			vzImageSaveTGA(screenshot_file,screenshot,&error_log);
			*screenshot_file = 0;
			vzImageFree(screenshot);
		};

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
	};

	// mark flag about first frame
	not_first_at = 1;
};

static void vz_glut_idle()
{
	// check if we need to update frames rendering info
	if((global_frame_no - last_title_update) > tv.TV_FRAME_PS)
	{
		char buf[100];
		sprintf
		(
			buf,
			"Frame %ld, drawn in %ld miliseconds, distance in frames %ld",
			global_frame_no,
			render_time,
			global_frame_no - rendered_frames
		);
		glutSetWindowTitle(buf);
		last_title_update = global_frame_no;
	};

	Sleep(1);
};


static void vz_glut_reshape(int w, int h)
{
	w = tv.TV_FRAME_WIDTH, h = tv.TV_FRAME_HEIGHT;
	glViewport (0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D (0.0, (GLdouble) w, 0.0, (GLdouble) h);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
};

static void vz_glut_mouse(int button, int state, int x, int y)
{
};

static void vz_glut_keyboard(unsigned char key, int x, int y)
{
	switch(key)
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
};


static void vz_glut_start(int argc, char** argv)
{
	char *multisampling = vzConfigParam(config,"vzMain","multisampling");

	printf("main: Initialization GLUT...\n");

	glutInit(&argc, argv);
	glutInitDisplayMode (GLUT_RGBA | GLUT_DOUBLE | GLUT_STENCIL | GLUT_ALPHA | ((multisampling)?GLUT_MULTISAMPLE:0) );

	glutInitWindowSize (tv.TV_FRAME_WIDTH, tv.TV_FRAME_HEIGHT); 
	glutInitWindowPosition (10, 10);
	glutCreateWindow (argv[0]);

	printf("main: GLUT created window\n");

	// request extensions
	vzGlExtInit();

	// init OpenGL features
	glClearColor (0.0, 0.0, 0.0, 0.0);
//	glShadeModel (GL_FLAT);
	glShadeModel(GL_SMOOTH);

	// enable alpha function
	glEnable(GL_ALPHA_TEST);

	// enable depth tests
	glEnable(GL_DEPTH_TEST);

	// enable blending
	glEnable (GL_BLEND);

	/* check/show multisampling */
	{
		int m,s,b;
		m = glIsEnabled(GL_MULTISAMPLE);
		glGetIntegerv(GL_SAMPLES, &s);
		glGetIntegerv(GL_SAMPLE_BUFFERS, &b);
		printf("GL_MULTISAMPLE=%d GL_SAMPLES=%d, GL_SAMPLE_BUFFERS=%d\n", m, s, b);
	};

	glShadeModel(GL_SMOOTH);

	// GLUT prep
	printf("Assigning GLUT functions... ");
	glutDisplayFunc(vz_glut_display); 
	glutReshapeFunc(vz_glut_reshape); 
	glutMouseFunc(vz_glut_mouse);
	glutKeyboardFunc(vz_glut_keyboard);
	glutIdleFunc(vz_glut_idle);
	printf("OK\n");

	printf("Main Loop starting...\n");

	glutMainLoop();

};

static void vz_exit(void)
{
	printf("Main loop finished");
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

	// hello message
	printf("%s (vz-%.2f-%s) [controller]\n",VZ_TITLE, VZ_VERSION_NUMBER,VZ_VERSION_SUFFIX);

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
			printf("Usage: vz.exe [ -f <scene file> ] [-c <config file> ] [GLUT parameters ....]\n");
			return 0;
		}
		else
			parse_args = 0;
	};

	// loading config
	printf("Loading config file '%s'...",config_file);
	config = vzConfigOpen(config_file);
	printf("Loaded!\n");

	// clear screenshot filename
	screenshot_file[0] = 0;
	
	// setting tv system
	vzConfigTVSpec(config,"tvspec", &tv);

	// syncs
	global_frame_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	ResetEvent(global_frame_event);
	scene_lock = CreateMutex(NULL,FALSE,NULL);

	// init functions
	functions = vzMainNewFunctionsList(config);

	// init output module
	char *output_module_name = vzConfigParam(config,"main","output");
	if(output_module_name)
	{
		printf("Loading output module '%s'...",output_module_name);
		output_module = vzOutputNew(config,output_module_name,&tv);
		if (!(vzOutputReady(output_module)))
		{
			printf("Failed\n");
			vzOutputFree(output_module);
		}
		else
			printf("OK\n");
	};


	// try to create sync thread in output module
	int output_module_sync = 0;
	if(output_module)
		output_module_sync = vzOutputSync(output_module, frames_counter);

	// check if flag. if its not OK - start internal
	if(output_module_sync == 0)
	{
		HANDLE l = CreateThread
		(
			NULL,
			1024,
			internal_sync_generator,
			frames_counter, // params
			0,
			NULL
		);

		SetThreadPriority(l , THREAD_PRIORITY_HIGHEST);
	};

	/* sync render */
	{
		HANDLE l = CreateThread
		(
			NULL,
			1024,
			sync_render,
			NULL, // params
			0,
			NULL
		);

		SetThreadPriority(l , THREAD_PRIORITY_HIGHEST);
	};

	// check if we need automaticaly load scene
	if(scene_file)
	{
		// init scene
		scene = vzMainSceneNew(functions,config,&tv);
		if (!vzMainSceneLoad(scene, scene_file))
		{
			printf("Error!!! Unable to load scene '%s'\n",scene_file);
			vzMainSceneFree(scene);
		};
		scene_file = NULL;
	};

	// start tcpserver
	unsigned long thread;
	HANDLE tcpserver_handle = CreateThread(0, 0, tcpserver, config, 0, &thread);

	/* exit handler */
	atexit(vz_exit);

	// start glut loop
	argc++;
	argv--;*argv = application;
	vz_glut_start(argc,argv);

	// is it ever finished?
	// need to read about this

	return 0;
};