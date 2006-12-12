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
	2006-12-12:
		*ring buffer jumping should not be so smart :-((( (in future 'QQ'
		blocks should be deleted.

	2006-11-28:
		*Output buffers paramters aquired from output driver.

	2006-11-27:
		*Memory allocation changes.

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

#ifdef _DEBUG
//#define DUMP_DRV_IO_LOCKS
#endif /* _DEBUG */

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <GL/glut.h>
#include "vzGlExt.h"

#include "vzOutput.h"
#include "vzOutputInternal.h"
#include "vzMain.h"

static struct vzOutputBuffers* _global_buffers_info = NULL;

//vzOutput public DLL method
VZOUTPUT_API void* vzOutputNew(void* config, char* name, void* tv)
{
	return new vzOutput(config,name,tv);
};

VZOUTPUT_API struct vzOutputBuffers* vzOutputIOBuffers(void)
{
	return _global_buffers_info;
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

VZOUTPUT_API int vzOuputRenderSlots(void* obj)
{
	return ((vzOutput*)obj)->render_slots();
};


/// --------------------------------------------------

int vzOutput::render_slots()
{
	int r = 0;

	/* lock buffers head */
	WaitForSingleObject(_buffers.lock, INFINITE);

	/* check current and next buffers */
	if
	(
		(0 == _buffers.id[ _buffers.pos_render ])
		||
		(0 == _buffers.id[ (_buffers.pos_render + 1) % VZOUTPUT_MAX_BUFS])
	)
		r = 1;

	/* unlock buffers head */
	ReleaseMutex(_buffers.lock);

	return r;
};

void vzOutput::lock_io_bufs(void** output, void*** input)
{
	int i;

	/* lock buffers head */
	WaitForSingleObject(_buffers.lock, INFINITE);

	/* check if we can use next buffer */
	if(_buffers.pos_driver_jump)
	{
		/* check if next buffer is loaded */
		if(((_buffers.pos_driver + 1) % VZOUTPUT_MAX_BUFS) != _buffers.pos_render)
		{
			/* reset buffer frame number */
			_buffers.id[ _buffers.pos_driver ] = 0;

			/* increment position */
			_buffers.pos_driver = (_buffers.pos_driver + 1) % VZOUTPUT_MAX_BUFS;

			/* check drop count */
			if(_buffers.pos_driver_dropped)
			{
				printf("vzOutput: dropped %d frames[driver]\n", _buffers.pos_driver_dropped);
				_buffers.pos_driver_dropped = 0;
			};
		}
		else
		{
			/* increment dropped framescounter */
			_buffers.pos_driver_dropped++;
		};

		/* clear flag - no more chances */
		_buffers.pos_driver_jump = 0;
	};

	/* lock buffer */
	WaitForSingleObject(_buffers.locks[ _buffers.pos_driver ], INFINITE);

	/* unlock buffers head */
	ReleaseMutex(_buffers.lock);


	/* setup pointer to mapped buffer */
	*output = _buffers.output.data[ _buffers.pos_driver ];
	*input = _buffers.input.data[ _buffers.pos_driver ];

//printf("lock_io_bufs: id=%d\n",_buffers.id[ _buffers.pos_driver ]);

	/* check if we need to map buffers */
	if(_use_offscreen_buffer)
	{
		/* map buffers */
		for(i = 0; i<(_buffers.input.channels*( (_buffers.input.field_mode)?2:1 )); i++)
		{
			int b = _buffers.input.nums[ _buffers.pos_driver ][i];

			glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, b );
			_buffers.input.data[ _buffers.pos_driver ][i] = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
		};

		/* unbind ? */
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	};

#ifdef DUMP_DRV_IO_LOCKS
	printf("lock_io_bufs: %-10d r=%d d=%d [", timeGetTime(), _buffers.pos_render, _buffers.pos_driver);
	for(i = 0; i<VZOUTPUT_MAX_BUFS ; i++)
		printf("%-4d", _buffers.id[i]);
	printf("]\n");
#endif /* DUMP_DRV_IO_LOCKS */

};


void vzOutput::unlock_io_bufs(void** output, void*** input)
{
	int i, b;

	/* check if we need to unmap buffers */
	if(_use_offscreen_buffer)
	{
		/* map buffers */
		for(i = 0; i<(_buffers.input.channels*( (_buffers.input.field_mode)?2:1 )); i++)
		{
			b = _buffers.input.nums[ _buffers.pos_driver ][i];
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, b );
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
		};
		/* unbind ? */
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	};

	/* unlock buffer */
	ReleaseMutex(_buffers.locks[ _buffers.pos_driver ]);

	/* lock buffers head */
	WaitForSingleObject(_buffers.lock, INFINITE);

	/* try to jump */
#ifdef QQ
	/* check if next buffer is loaded */
	if(((_buffers.pos_driver + 1) % VZOUTPUT_MAX_BUFS) != _buffers.pos_render)
	{
		/* reset buffer frame number */
		_buffers.id[ _buffers.pos_driver ] = 0;

		/* increment position */
		_buffers.pos_driver = (_buffers.pos_driver + 1) % VZOUTPUT_MAX_BUFS;

		/* reset jump flag */
		_buffers.pos_driver_jump = 0;

		/* check drop count */
		if(_buffers.pos_driver_dropped)
		{
			printf("vzOutput: dropped %d frames[driver]\n", _buffers.pos_driver_dropped);
			_buffers.pos_driver_dropped = 0;
		};
	}
	else
#endif /* QQ */
		/* set jump flag */
		_buffers.pos_driver_jump = 1;

	/* unlock buffers head */
	ReleaseMutex(_buffers.lock);
};


void vzOutput::pre_render()
{
	/* lock buffers head */
	WaitForSingleObject(_buffers.lock, INFINITE);

	/* check if we can use next buffer */
	if(_buffers.pos_render_jump)
	{
		/* check if next buffer is loaded */
		if(((_buffers.pos_render + 1) % VZOUTPUT_MAX_BUFS) != _buffers.pos_driver)
		{
			/* increment position */
			_buffers.pos_render = (_buffers.pos_render + 1) % VZOUTPUT_MAX_BUFS;

			/* check drop count */
			if(_buffers.pos_render_dropped)
			{
				printf("vzOutput: dropped %d frames[render]\n", _buffers.pos_render_dropped);
				_buffers.pos_render_dropped = 0;
			};
		}
		else
		{
			/* increment dropped framescounter */
			_buffers.pos_render_dropped++;
		};

		/* clear flag - no more chances */
		_buffers.pos_render_jump = 0;
	};

	/* set buffer frame number */
	_buffers.id[ _buffers.pos_render ] = ++_buffers.cnt_render;

	/* lock buffer */
	WaitForSingleObject(_buffers.locks[ _buffers.pos_render ], INFINITE);

	/* unlock buffers head */
	ReleaseMutex(_buffers.lock);


	// two method for copying data
	if(_use_offscreen_buffer)
	{
		/* bind to buffer */
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, _buffers.output.nums[_buffers.pos_render]);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
		_buffers.output.data[_buffers.pos_render] = NULL;

		// start asynchroniosly  read from buffer
		glReadPixels
		(
			0,
			0,
			_tv->TV_FRAME_WIDTH,
			_tv->TV_FRAME_HEIGHT,
			GL_BGRA_EXT,
			GL_UNSIGNED_BYTE,
			BUFFER_OFFSET( _buffers.output.offset )
		);

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
			((unsigned char*)_buffers.output.data[_buffers.pos_render])
			+
			_buffers.output.offset
		);

		/* deal with indexes */
		post_render_aux();
	};

	// restore read buffer
//	glReadBuffer(read_buffer);
//	glDrawBuffer(draw_buffer);
};


void vzOutput::post_render_aux()
{
	/* unlock buffer */
	ReleaseMutex(_buffers.locks[ _buffers.pos_render ]);

	/* lock buffers head */
	WaitForSingleObject(_buffers.lock, INFINITE);
#ifdef QQ
	/* check if next buffer is loaded */
	if(((_buffers.pos_render + 1) % VZOUTPUT_MAX_BUFS) != _buffers.pos_driver)
	{
		/* increment position */
		_buffers.pos_render = (_buffers.pos_render + 1) % VZOUTPUT_MAX_BUFS;

		/* reset jump flag */
		_buffers.pos_render_jump = 0;

		/* check drop count */
		if(_buffers.pos_render_dropped)
		{
			printf("vzOutput: dropped %d frames[render]\n", _buffers.pos_render_dropped);
			_buffers.pos_render_dropped = 0;
		};
	}
	else
#endif /* QQ */
		/* set jump flag */
		_buffers.pos_render_jump = 1;

	/* unlock buffers head */
	ReleaseMutex(_buffers.lock);
}

void vzOutput::post_render()
{
	// two method for copying data
	if(_use_offscreen_buffer)
	{
		/* wait async transfer */
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, _buffers.output.nums[_buffers.pos_render]);
		_buffers.output.data[_buffers.pos_render] = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);

		/* deal with indexes */
		post_render_aux();
	};
};


vzOutput::vzOutput(void* config, char* name, void* tv)
{
	int b, i, n;

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
		&&
		(GetBuffersInfo = (output_proc_vzOutput_GetBuffersInfo)GetProcAddress(_lib,"vzOutput_GetBuffersInfo"))
		&&
		(AssignBuffers = (output_proc_vzOutput_AssignBuffers)GetProcAddress(_lib,"vzOutput_AssignBuffers"))
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

			/* init INPUT/OUTPUT buffers here */
			{
				/* ask for output buffer sizes */
				memset(&_buffers, 0, sizeof(struct vzOutputBuffers));
				_buffers.lock = CreateMutex(NULL,FALSE,NULL);
				for(b = 0; b < VZOUTPUT_MAX_BUFS; b++)
					_buffers.locks[b] = CreateMutex(NULL,FALSE,NULL);
				GetBuffersInfo(&_buffers);

				// create framebuffers
				if(_use_offscreen_buffer)
				{
					// generate VZOUTPUT_MAX_BUFS offscreen buffers
					glGenBuffers(VZOUTPUT_MAX_BUFS, _buffers.output.nums);

					// init and request mem addrs of buffers
					for(b=0; b<VZOUTPUT_MAX_BUFS; b++)
					{
						// INIT BUFFER SIZE
						glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, _buffers.output.nums[b]);
						glBufferData(GL_PIXEL_PACK_BUFFER_ARB, _buffers.output.gold, NULL, GL_STREAM_READ);

						// REQUEST BUFFER PTR - MAP AND SAVE PTR
						_buffers.output.data[b] = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB,GL_READ_ONLY);

						// UNMAP
						glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);

						/* deal with input buffers */
						n = (_buffers.input.channels*( (_buffers.input.field_mode)?2:1));
						glGenBuffers(n, _buffers.input.nums[b]);
						for(i = 0; i<n; i++)
						{
							glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, _buffers.input.nums[b][i]);
							glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, _buffers.input.gold, NULL, GL_STREAM_DRAW);
						};

						// unbind ?
						glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
					};

				}
				else
				{

					/* allocate mem */
					for(b=0; b<VZOUTPUT_MAX_BUFS; b++)
					{
						_buffers.output.data[b] = VirtualAlloc
						(
							NULL, 
							_buffers.output.gold,
							MEM_COMMIT, 
							PAGE_READWRITE
						);
						VirtualLock(_buffers.output.data[b], _buffers.output.gold);

						/* deal with input buffers */
						n = (_buffers.input.channels*( (_buffers.input.field_mode)?2:1));
						for(i = 0; i<n; i++)
						{
							_buffers.input.data[b][i] = VirtualAlloc
							(
								NULL, 
								_buffers.input.gold,
								MEM_COMMIT, 
								PAGE_READWRITE
							);
							VirtualLock(_buffers.input.data[b][i], _buffers.input.gold);
						};
					};
				};

				/* Man/Remap or other ops */
				AssignBuffers(&_buffers);

				/* setup output buffer ids */
				_buffers.cnt_render = 0;
				_buffers.pos_driver = 0;
				_buffers.pos_render = 1;
//				for(i = 0; i<VZOUTPUT_MAX_BUFS; i++)
//					_buffers.id[i] = ++_buffers.cnt_render;

				/* assign to module global version */
				_global_buffers_info = &_buffers;
			};

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
	int b, n, i;

	if(_lib)
	{
		// delete all objects
		StopOutputLoop();
		FreeLibrary(_lib);
		_lib = NULL;

		/* close locks */
		CloseHandle(_buffers.lock);
		for(b = 0; b < VZOUTPUT_MAX_BUFS; b++)
			CloseHandle(_buffers.locks[b]);

		// free framebuffer
		if(_use_offscreen_buffer)
		{
			glDeleteBuffers(VZOUTPUT_MAX_BUFS, _buffers.output.nums);

			/* deal with input buffers */
			n = (_buffers.input.channels*( (_buffers.input.field_mode)?2:1));
			for(b=0; b<VZOUTPUT_MAX_BUFS; b++)
				glDeleteBuffers(n, _buffers.input.nums[b]);
		}
		else
		{
			/* release mem */
			for(b=0; b<VZOUTPUT_MAX_BUFS; b++)
			{
				if(_buffers.output.data[b])
				{
					VirtualUnlock(_buffers.output.data[b], _buffers.output.gold);
					VirtualFree(_buffers.output.data[b], _buffers.output.gold, MEM_RELEASE);
				};

				/* deal with input buffers */
				n = (_buffers.input.channels*( (_buffers.input.field_mode)?2:1));
				for(i = 0; i<n; i++)
					if(_buffers.input.data[b][i])
					{
						VirtualUnlock(_buffers.input.data[b][i], _buffers.input.gold);
						VirtualFree(_buffers.input.data[b][i], _buffers.input.gold, MEM_RELEASE);
					};
			};
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

