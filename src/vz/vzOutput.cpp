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
		*Hard sync scheme.
		*OpenGL extension scheme load changes.

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

#define FB_CHUNKS 4
#define DEBUG_HARD_SYNC_DROPS

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

VZOUTPUT_API void vzOuputPostRender(void* obj)
{
	((vzOutput*)obj)->post_render();
};

VZOUTPUT_API void vzOuputPreRender(void* obj)
{
	((vzOutput*)obj)->pre_render();
};



/// --------------------------------------------------
#ifdef DEBUG_HARD_SYNC_DROPS
static int prev_p = 0;
static int dropped_frames_count = 0;
#endif /* DEBUG_HARD_SYNC_DROPS */

void* vzOutput::get_output_buf_ptr(void)
{
	int j = _output_framebuffer_pos;

	/* output buffer is lefter */
	int p = (j + VZOUTPUT_OUT_BUFS - 1) % VZOUTPUT_OUT_BUFS;

#ifdef DEBUG_HARD_SYNC
	printf("[%-10d] get_output_buf_ptr:\t%d\n", timeGetTime(), j);
#endif /* DEBUG_HARD_SYNC */

#ifdef DEBUG_HARD_SYNC_DROPS
	if
	(
		((prev_p + 1 + VZOUTPUT_OUT_BUFS) % VZOUTPUT_OUT_BUFS) 
		!= 
		p
	)
	{
#ifdef DEBUG_HARD_SYNC
		printf("[%-10d] get_output_buf_ptr: DROPPPED!!!\n", timeGetTime());
#endif /* DEBUG_HARD_SYNC */
		dropped_frames_count++;
	}
	else
	{
		if(dropped_frames_count)
			printf("vzOutput: dropped %d frames\n", dropped_frames_count);
		dropped_frames_count = 0;
	};
	prev_p = p;
#endif /* DEBUG_HARD_SYNC_DROPS */

	/* return pointer to mapped buffer */
	return _output_framebuffer_data[p];
};

void vzOutput::post_render()
{
	// two method for copying data
	if(_use_offscreen_buffer)
	{
		/* wait async transfer */
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, _output_framebuffer_nums[_output_framebuffer_pos]);
		_output_framebuffer_data[_output_framebuffer_pos] = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
	};

	/* shift forward */
	_output_framebuffer_pos = (_output_framebuffer_pos + VZOUTPUT_OUT_BUFS + 1) % VZOUTPUT_OUT_BUFS;

#ifdef DEBUG_HARD_SYNC
	printf("[%-10d] post_render: %d\n", timeGetTime(), _output_framebuffer_pos);
#endif /* DEBUG_HARD_SYNC */
};

void vzOutput::pre_render()
{
	// two method for copying data
	if(_use_offscreen_buffer)
	{
		/* bind to buffer */
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, _output_framebuffer_nums[_output_framebuffer_pos]);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
		_output_framebuffer_data[_output_framebuffer_pos] = NULL;

		// start asynchroniosly  read from buffer
#ifdef FB_CHUNKS
		for(int j=0; j < FB_CHUNKS ; j++ )
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
			_output_framebuffer_data[_output_framebuffer_pos]
		);
	};

	// restore read buffer
//	glReadBuffer(read_buffer);
//	glDrawBuffer(draw_buffer);
};


vzOutput::vzOutput(void* config, char* name, void* tv)
{
	int b;

	// tv params
	_tv = (vzTVSpec*)tv;

	// assign config
	_config = config;

	// load option
	_use_offscreen_buffer = vzConfigParam(config,"vzOutput","use_offscreen_buffer")?1:0;

	/* check if appropriate exts is loaded */
	if(_use_offscreen_buffer)
		if(!(glGenBuffers)) 
			_use_offscreen_buffer = 0;

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

			/* init OUTPUT buffers here */
			{
				// create framebuffers
				if(_use_offscreen_buffer)
				{
					// generate VZOUTPUT_OUT_BUFS offscreen buffers
					glGenBuffers(VZOUTPUT_OUT_BUFS, _output_framebuffer_nums);

					// init and request mem addrs of buffers
					for(b=0; b<VZOUTPUT_OUT_BUFS; b++)
					{
						// INIT BUFFER SIZE
						glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB,_output_framebuffer_nums[b]);
						glBufferData(GL_PIXEL_PACK_BUFFER_ARB,
							4*_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT,NULL,GL_STREAM_READ);

						// REQUEST BUFFER PTR - MAP AND SAVE PTR
						_output_framebuffer_data[b] = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB,GL_READ_ONLY);

						// UNMAP
						glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);

						// unbind ?
						glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
					};
				}
				else
				{
					/* allocate mem */
					_output_framebuffer_data[0] = malloc(VZOUTPUT_OUT_BUFS*4*_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT);

					/* remap */
					for(b=1; b<VZOUTPUT_OUT_BUFS; b++)
						_output_framebuffer_data[b] = 
						(
							((unsigned char*)_output_framebuffer_data[b - 1]) 
							+ 
							4*_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT
						);
				};

				/* setup output buffer id */
				_output_framebuffer_pos = 0;
			};

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

		// free framebuffer
		if(_use_offscreen_buffer)
		{
			glDeleteBuffers(VZOUTPUT_OUT_BUFS, _output_framebuffer_nums);
		}
		else
		{
			if (_output_framebuffer_data[0])
				free(_output_framebuffer_data[0]);
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

