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
#include <GL/glut.h>


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
int main_stage;
char screenshot_file[1024];
int use_offscreen_buffer = 0;

/*
----------------------------------------------------------

	this flags define method for sincync
	of frame counting methods

----------------------------------------------------------
*/
HANDLE global_frame_event;


/*
----------------------------------------------------------
	frame counting functions
----------------------------------------------------------
*/
unsigned long global_frame_no = 0;
unsigned long stop_global_frames_counter = 0;


HANDLE internal_sync_thread;
unsigned long WINAPI internal_sync_generator(void* fc_proc_addr)
{
	frames_counter_proc fc = (frames_counter_proc)fc_proc_addr;
	
	// endless loop
	while(1)
	{
		// sleep
		Sleep(tv.TV_FRAME_DUR_MS);

		// call frame counter
		if (fc) fc();
	};
};


void frames_counter()
{
	// reset event
	ResetEvent(global_frame_event);

	// increment frame counter
	global_frame_no++;

	// rise evennt
	PulseEvent(global_frame_event);
};


/* 
----------------------------------------------------------
	GLUT operatives
----------------------------------------------------------
*/
static long skip_draw = 0;
static long draw_start_time = 0;
static long dropped_frames = 0;
static long rendered_frames = 0;
static long last_title_update = 0;
static long not_first_at = 0;

void vz_glut_display(void)
{
	// check if redraw should processed
	// by internal needs
	if (skip_draw)
		return;

	// scene redrawed
	skip_draw = 1;

	// save time of draw start
	draw_start_time = timeGetTime();

	// output module tricks
	if(!(not_first_at))
	{
		// init buffers
		if(output_module)
			vzOutputInitBuffers(output_module);
	}
	else
	{
		// preprender buffers
		if(output_module)
			vzOuputPreRender(output_module);
	};

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

	if(not_first_at)
		if(output_module)
			vzOuputPostRender(output_module);

	// and swap buffers
	glutSwapBuffers();

	// mark flag about first frame
	not_first_at = 1;

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
};

void vz_glut_idle()
{
	// save time of draw stop
	long draw_stop_time = timeGetTime();

	// check if we need to update frames rendering info
	if((global_frame_no - last_title_update) > tv.TV_FRAME_PS)
	{
		char buf[100];
		sprintf
		(
			buf,
			"Frame %ld, drawed in %ld miliseconds, dropped %ld frames, distance in frames %ld",
			global_frame_no,
			draw_stop_time - draw_start_time,
			dropped_frames,
			global_frame_no - rendered_frames
		);
		glutSetWindowTitle(buf);
		last_title_update = global_frame_no;
	};

	// lets sleep for next frames redraw
	unsigned long dur = draw_stop_time - draw_start_time;

	// wait for frame happens
	WaitForSingleObject(global_frame_event,INFINITE);
	ResetEvent(global_frame_event);

	// reset flag of forcing redraw
	skip_draw = 0;

	// notify about redisplay
	glutPostRedisplay();
};


void vz_glut_reshape(int w, int h)
{
	w = tv.TV_FRAME_WIDTH, h = tv.TV_FRAME_HEIGHT;
	glViewport (0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D (0.0, (GLdouble) w, 0.0, (GLdouble) h);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
};

void vz_glut_mouse(int button, int state, int x, int y)
{
};

void vz_glut_keyboard(unsigned char key, int x, int y)
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
	};
};


void vz_glut_start(int argc, char** argv)
{
	printf("Initialization GLUT... ");

	glutInit(&argc, argv);
	glutInitDisplayMode (GLUT_RGBA | GLUT_DOUBLE | GLUT_STENCIL | GLUT_ALPHA);

	glutInitWindowSize (tv.TV_FRAME_WIDTH, tv.TV_FRAME_HEIGHT); 
	glutInitWindowPosition (10, 10);
	glutCreateWindow (argv[0]);

	// request extensions
	char* gl_extensions = (char*)glGetString(GL_EXTENSIONS);
	printf("( OpenGL Extensions found: \"%s\" ) ...",gl_extensions);

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

	// GLUT prep
	printf("Assigning GLUT functions... ");
	glutDisplayFunc(vz_glut_display); 
	glutReshapeFunc(vz_glut_reshape); 
	glutMouseFunc(vz_glut_mouse);
	glutKeyboardFunc(vz_glut_keyboard);
	glutIdleFunc(vz_glut_idle);
	printf("OK\n");

	// save time of draw start
	draw_start_time = timeGetTime();

	printf("Main Loop starting...\n");

	glutMainLoop();
	printf("Main loop finished");
};




/*
----------------------------------------------------------

  MAIN function

----------------------------------------------------------
*/

int main(int argc, char** argv)
{
#ifdef _DEBUG
	{
		vzTTFont* font = new vzTTFont("m1-h",20);
		delete font;

		unsigned long src[4];
		unsigned short yuv[4];
		unsigned char alpha[4];
		vzImageBGRA2YUAYVA(src, yuv, alpha, 4);
	};
#endif

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
			// set config file name
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
		internal_sync_thread = CreateThread
		(
			NULL,
			1024,
			internal_sync_generator,
			frames_counter, // params
			0,
			NULL
		);
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


	// start glut loop
	argc++;
	argv--;*argv = application;
	vz_glut_start(argc,argv);

	// is it ever finished?
	// need to read about this

	return 0;
};