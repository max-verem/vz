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
		OpenGL extension scheme load changes.

	2005-07-07:
		*Removed unused blocks of code

	2005-07-06:
		*Added syncing to video hardware: If videooutput hardware supports
		recieving events about fields/frames - USE IT!

	2005-06-28:
		*TBC operation modifications.

	2005-06-25:
		*Internal TBC remastered.

    2005-06-08:
		*Code cleanup
		*Modified 'vzOutputNew' to accept additional parameter 'tv'.
		*Added 'tv' parameter to contructor
		*'InitBoard(void* _tv)' now accept tv system param

*/

/*
http://www.nvidia.com/dev_content/nvopenglspecs/GL_EXT_pixel_buffer_object.txt
http://www.gpgpu.org/forums/viewtopic.php?t=2001&sid=4cb981dff7d9aa406cb721f7bd1072b6
http://www.elitesecurity.org/t140671-pixel-buffer-object-PBO-extension
http://www.gamedev.net/community/forums/topic.asp?topic_id=329957&whichpage=1&#2143668
http://www.gamedev.net/community/forums/topic.asp?topic_id=360729

*/

#include "vzOutputInternal.h"
#include "vzOutput.h"
#include "vzMain.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <GL/glut.h>
#include "vzGlExt.h"

//vzOutput public DLL method
VZOUTPUT_API void* vzOutputNew(void* config, char* name, void* tv)
{
	return new vzOutput(config,name,tv);
};

VZOUTPUT_API void vzOutputFree(void* &obj)
{
	if (obj)
	{
		delete (vzOutput*)obj;
		obj = NULL;
	};
};

VZOUTPUT_API int vzOutputReady(void* obj)
{
	return ((vzOutput*)obj)->ready();
};

VZOUTPUT_API int vzOutputSync(void* obj,void* fc)
{
	return ((vzOutput*)obj)->set_sync_proc(fc);
};

VZOUTPUT_API void vzOutputInitBuffers(void* obj)
{
	((vzOutput*)obj)->init_buffers();
};

VZOUTPUT_API void vzOuputPostRender(void* obj)
{
	((vzOutput*)obj)->post_render();
};

VZOUTPUT_API void vzOuputPreRender(void* obj)
{
	((vzOutput*)obj)->pre_render();
};



/// --------------------------------------------------

#define _max_framebuffer_nums \
	( (_framebuffer_nums[0]>_framebuffer_nums[1]) && (_framebuffer_nums[0]>_framebuffer_nums[2]) ) \
	? \
	0 \
	: \
	( \
		( (_framebuffer_nums[1]>_framebuffer_nums[0]) && (_framebuffer_nums[1]>_framebuffer_nums[2]) ) \
		? \
		1 \
		: \
		2 \
	) 

#define _min_framebuffer_nums \
	( (_framebuffer_nums[0]<_framebuffer_nums[1]) && (_framebuffer_nums[0]<_framebuffer_nums[2]) ) \
	? \
	0 \
	: \
	( \
		( (_framebuffer_nums[1]<_framebuffer_nums[0]) && (_framebuffer_nums[1]<_framebuffer_nums[2]) ) \
		? \
		1 \
		: \
		2 \
	) 


void vzOutput::post_render()
{
};

void vzOutput::pre_render()
{
	// what buffer will be used to grab picture
	// lock it for update
	int buffer_num;
	void* buffer_address;
	HANDLE lock = lock_write(&buffer_address,&buffer_num);
	int b = buffer_num;

	// two method for copying data
	if(_use_offscreen_buffer)
	{
		// start asynchroniosly  read from buffer
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB,_offscreen_buffers[b]);

#define BUFFER_OFFSET(i) ((char *)NULL + (i))
		// read pixels into offscreen area

#define FB_CHUNKS 4

#ifdef FB_CHUNKS
		for(int j=0,b=0; j < FB_CHUNKS ; j++ )
			glReadPixels
			(
				0,
				j * (_tv->TV_FRAME_HEIGHT / FB_CHUNKS),
				_tv->TV_FRAME_WIDTH,
				_tv->TV_FRAME_HEIGHT / FB_CHUNKS,
				GL_BGRA_EXT,
				GL_UNSIGNED_BYTE,
				BUFFER_OFFSET( j * (_tv->TV_FRAME_HEIGHT / FB_CHUNKS)*4*_tv->TV_FRAME_WIDTH   )
			);
#else /* !FB_CHUNKS */
		glReadPixels
		(
			0,
			0,
			_tv->TV_FRAME_WIDTH,
			_tv->TV_FRAME_HEIGHT,
			GL_BGRA_EXT,
			GL_UNSIGNED_BYTE,
			BUFFER_OFFSET(0)
		);
#endif /* FB_CHUNKS */

		// map buffer to wait for finish read
//		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB,_offscreen_buffers[b]);
//		glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB,GL_READ_ONLY);
//		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB,_offscreen_buffers[b]);
//		glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);

		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
	}
	else
	{
		// classic method
	
		// read pixels into initialized buffer
		glReadPixels
		(
			0,
			0,
			_tv->TV_FRAME_WIDTH,
			_tv->TV_FRAME_HEIGHT,
			GL_BGRA_EXT,
			GL_UNSIGNED_BYTE,
			_framebuffer[b]
		);
	};

	// update earliest buffer index
	long i_max = _max_framebuffer_nums;
	long i_min = _min_framebuffer_nums;
	_framebuffer_nums[i_min] = 1 + _framebuffer_nums[i_max];

	// unlock buffer
	ReleaseMutex(lock);	

	// restore read buffer
//	glReadBuffer(read_buffer);
//	glDrawBuffer(draw_buffer);

	// chage last write buffer and unlock
	
};


long buffers_inited = 0;

HANDLE vzOutput::lock_read(void** mem,int* num)
{
	// lock buffers pointers modoficator
	WaitForSingleObject(_framebuffer_lock,INFINITE);

	// determinate max and min index framebuffers queueu
	long i_max = _max_framebuffer_nums;
	long i_min = _min_framebuffer_nums;

	// determinate mutex latest fb
	HANDLE lock = _framebuffer_locks[i_max];

	// try to lock that buffer - wait!
	WaitForSingleObject(lock,INFINITE);

	*mem = _framebuffer[i_max];
	*num = i_max;

	ReleaseMutex(_framebuffer_lock);

	return lock;
};

HANDLE vzOutput::lock_write(void** mem,int* num)
{
	// lock buffers pointers modoficator
	WaitForSingleObject(_framebuffer_lock,INFINITE);

	// determinate id of max fbuffer
	long i_max = _max_framebuffer_nums;
	long i_min = _min_framebuffer_nums;

	// determinate mutex earliest fb
	HANDLE lock = _framebuffer_locks[i_min];

	// try to lock that buffer - wait!
	WaitForSingleObject(lock,INFINITE);

	*mem = _framebuffer[i_min];
	*num = i_min;
	
	ReleaseMutex(_framebuffer_lock);

	return lock;
};


void vzOutput::init_buffers()
{
	/* check if appropriate exts is loaded */
	if(_use_offscreen_buffer)
		if(!(glGenBuffers)) 
			_use_offscreen_buffer = 0;
	
	// create framebuffers
	if(_use_offscreen_buffer)
	{
		// generate 3 offscreen buffers
		glGenBuffers(3, _offscreen_buffers);

		// init and request mem addrs of buffers
		for(int b=0;b<3;b++)
		{
			// INIT BUFFER SIZE
			glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB,_offscreen_buffers[b]);
			glBufferData(GL_PIXEL_PACK_BUFFER_ARB,4*_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT,NULL,GL_STREAM_READ);

			// REQUEST BUFFER PTR - MAP AND SAVE PTR
			glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB,_offscreen_buffers[b]);
			_framebuffer[b] = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB,GL_READ_ONLY);

			// UNMAP
			glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB,_offscreen_buffers[b]);
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
		};

		buffers_inited = 1;
	}
	else
	{
		_framebuffer[0] = malloc(3*4*_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT);
		_framebuffer[1] = ((unsigned char*)_framebuffer[0]) + 4*_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT;
		_framebuffer[2] = ((unsigned char*)_framebuffer[0]) + 4*_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT;
	};
};

vzOutput::vzOutput(void* config, char* name, void* tv)
{
	// tv params
	_tv = (vzTVSpec*)tv;

	// assign config
	_config = config;

	// load option
	_use_offscreen_buffer = vzConfigParam(config,"vzOutput","use_offscreen_buffer")?1:0;

	// set framebuffers pointers to NULL
	_framebuffer[2] = (_framebuffer[1] = (_framebuffer[0] = NULL));

	// create events
	_framebuffer_locks[0] = CreateMutex(NULL,FALSE,NULL);
	_framebuffer_locks[1] = CreateMutex(NULL,FALSE,NULL);
	_framebuffer_locks[2] = CreateMutex(NULL,FALSE,NULL);
	_framebuffer_lock = CreateMutex(NULL,FALSE,NULL);
	_framebuffer_nums[0] = 0;
	_framebuffer_nums[1] = 1;
	_framebuffer_nums[2] = 2;

	// prepare filename
	char filename[1024];
	sprintf(filename,"outputs/%s.dll",name);

	printf("vzOuput: Loading '%s' ... ",filename);

	// try load
	_lib = LoadLibrary(filename);

	// check if lib is loaded
	if (!_lib)
	{
		printf("Failed!\n");
		return;
	};
	printf("Loaded!\n");

	printf("vzOuput: Looking for procs ... ");
	// try to load pionters to libs
	if
	(
		(SetConfig = (output_proc_vzOutput_SetConfig)GetProcAddress(_lib,"vzOutput_SetConfig"))
		&&
		(FindBoard = (output_proc_vzOutput_FindBoard)GetProcAddress(_lib,"vzOutput_FindBoard"))
		&&
		(SelectBoard = (output_proc_vzOutput_SelectBoard)GetProcAddress(_lib,"vzOutput_SelectBoard"))
		&&
		(InitBoard = (output_proc_vzOutput_InitBoard)GetProcAddress(_lib,"vzOutput_InitBoard"))
		&&
		(StartOutputLoop = (output_proc_vzOutput_StartOutputLoop)GetProcAddress(_lib,"vzOutput_StartOutputLoop"))
		&&
		(StopOutputLoop = (output_proc_vzOutput_StopOutputLoop)GetProcAddress(_lib,"vzOutput_StopOutputLoop"))
	)
	{
		printf("OK!\n");
		// all loaded

		// set config
		printf("vzOutput: submitting config ... ");
		SetConfig(_config);
		printf("OK\n");

		// initializ
		printf("vzOuput: find output board '%s' ... ",name);
		int c = FindBoard(NULL);
		printf(" Found %d boards\n",c);
		if(c)
		{
			int board_id = 0;

			// selecting board
			printf("vzOutput: Selecting board '%d' ... ",board_id);
			SelectBoard(0,NULL);
			printf("OK\n");

			// init board
			printf("vzOutput: Init board '%d' ... ",board_id);
			InitBoard(_tv);
			printf("OK\n");

			// request sync proc !
			SetSync = (output_proc_vzOutput_SetSync)GetProcAddress(_lib,"vzOutput_SetSync");

			// starting loop
			printf("vzOutput: Start Ouput Thread... ");
			StartOutputLoop(this);
			printf("OK\n");

			return;
		};
	};

	printf("Failed!\n");

	// unload library
	FreeLibrary(_lib);
	_lib = NULL;
};

vzOutput::~vzOutput()
{
	if(_lib)
	{
		// delete all objects
		StopOutputLoop();
		FreeLibrary(_lib);
		_lib = NULL;

		// delete mutexes
		CloseHandle(_framebuffer_locks[1]);
		CloseHandle(_framebuffer_locks[2]);
		CloseHandle(_framebuffer_locks[0]);
		CloseHandle(_framebuffer_lock);

		// free framebuffer

		if(_use_offscreen_buffer)
		{
			for(int i=0;i<3;i++)
			{
				glDeleteBuffers(3, _offscreen_buffers);
			};
		}
		else
		{
			if (*_framebuffer)
				free(*_framebuffer);
		};
	};
};

VZOUTPUT_API long vzOutput::set_sync_proc(void* fc)
{
	if(SetSync)
		return SetSync(fc);
	else
		return 0;
};



VZOUTPUT_API void vzOutput::notify_frame_start()
{
};

VZOUTPUT_API void vzOutput::notify_frame_stop()
{
};

VZOUTPUT_API void vzOutput::notify_field0_start()
{
};

VZOUTPUT_API void vzOutput::notify_field0_stop()
{
};

VZOUTPUT_API void vzOutput::notify_field1_start()
{
};

VZOUTPUT_API void vzOutput::notify_field1_stop()
{
};

