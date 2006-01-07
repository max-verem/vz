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
    2005-06-25:
		*Fake output module developed ....
*/

#pragma comment(lib, "winmm.lib") 

#include "../vz/vzOutput-devel.h"
#include "../vz/vzImage.h"
#include "../vz/vzOutputInternal.h"
#include "../vz/vzMain.h"
#include "../vz/vzTVSpec.h"
#include <stdio.h>

static void* _config = NULL;
static vzTVSpec* _tv = NULL;

// threads cotrols
HANDLE notify_to_stop_loop, notify_loop_stopped, loop_thread;

#define ALPHA_PIXEL_SIZE 1
#define YUV_PIXEL_SIZE 2
#define SRC_PIXEL_SIZE 4

// init
VZOUTPUTS_EXPORT long vzOutput_FindBoard(char** error_log = NULL)
{
	return 1;
};

VZOUTPUTS_EXPORT void vzOutput_SetConfig(void* config)
{
	_config = config;
};


VZOUTPUTS_EXPORT long vzOutput_SelectBoard(unsigned long id,char** error_log = NULL)
{
	printf("nullvideo");
	return id;
};

VZOUTPUTS_EXPORT long vzOutput_InitBoard(void* tv)
{
	// assign tv value 
	_tv = (vzTVSpec*)tv;
	return 1;
};


unsigned long WINAPI output_loop(void* obj)
{
	// cast pointer to vzOutput
	vzOutput* tbc = (vzOutput*)obj;

	// init buffers
	void* alpha_buffer = memset(malloc(_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT*ALPHA_PIXEL_SIZE),0,_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT*ALPHA_PIXEL_SIZE);
	void* yuv_buffer = memset(malloc(_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT*YUV_PIXEL_SIZE),0,_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT*YUV_PIXEL_SIZE);

	while(1)
	{
		// check if all finished !!!
		if(WaitForSingleObject(notify_to_stop_loop,0) == WAIT_OBJECT_0)
		{
			// event to stop reached !!!
			free(yuv_buffer);
			free(alpha_buffer);
			ExitThread(0);
		};

		// mark start time
		long A = timeGetTime();

		// 1. wait for signal about field #1 starts
		
		// 2. start transfering transcoded buffer

		// 3. request pointer to new buffer
		int buffer_num;
		void* buffer_address;
		HANDLE lock = tbc->lock_read(&buffer_address,&buffer_num);

		// 4. transcode buffer
		if (buffer_address)
			vzImageBGRA2YUAYVA
			(
				buffer_address,
				yuv_buffer,
				alpha_buffer,
				_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT
			);



		// 5. unlock read buffer
		ReleaseMutex(lock);

		// mark stop time
		long B = timeGetTime();

		// mark time to sleep
		long C = B - A;
		if(C>0)
			Sleep(C);

		// frame ends
		tbc->notify_frame_stop();

	};
};

VZOUTPUTS_EXPORT void vzOutput_StartOutputLoop(void* tbc)
{
	// create events for thread control
	notify_to_stop_loop = CreateEvent(NULL, TRUE, FALSE, NULL);

	// start thread
	loop_thread = CreateThread
	(
		NULL,
		1024,
		output_loop,
		tbc, //&_thread_data, // params
		0,
		NULL
	);
};


VZOUTPUTS_EXPORT void vzOutput_StopOutputLoop()
{
	// notify to stop thread
	PulseEvent(notify_to_stop_loop);

	// sleep 2 frame
	Sleep(80);

	// close everithing
	CloseHandle(notify_to_stop_loop);
	CloseHandle(loop_thread);
};





