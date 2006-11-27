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
		*

    2005-06-25:
		*Fake output module developed ....
*/

/*----------------------------------------------------------------------------
  
	HIGH PRECISION TIMERS 

----------------------------------------------------------------------------*/
#include <windows.h>

#pragma comment(lib, "winmm")

struct hr_sleep_data
{
	HANDLE event;
	MMRESULT timer;
};

static void hr_sleep_init(struct hr_sleep_data* sleep_data)
{
	timeBeginPeriod(1);
	sleep_data->event = CreateEvent(NULL, FALSE,FALSE,NULL);
	sleep_data->timer = timeSetEvent
	(
		1,
		0,
		(LPTIMECALLBACK)sleep_data->event,
		NULL,
		TIME_PERIODIC | TIME_CALLBACK_EVENT_PULSE
	);
};

static void hr_sleep_destroy(struct hr_sleep_data* sleep_data)
{
	timeEndPeriod(1);
	timeKillEvent(sleep_data->timer);
	CloseHandle(sleep_data->event);
};

static void hr_sleep(struct hr_sleep_data* sleep_data, unsigned long delay_miliseconds)
{
	long a = timeGetTime();

	while( (timeGetTime() - a) < delay_miliseconds )
		WaitForSingleObject(sleep_data->event, INFINITE);
};

/*---------------------------------------------------------------------------- */


#include "../vz/vzOutput-devel.h"
#include "../vz/vzImage.h"
#include "../vz/vzOutputInternal.h"
#include "../vz/vzMain.h"
#include "../vz/vzTVSpec.h"
#include <stdio.h>

static void* _config = NULL;
static vzTVSpec* _tv = NULL;
struct hr_sleep_data timer_data;
static frames_counter_proc _fc = NULL;

// threads cotrols
HANDLE loop_thread;
static int notify_to_stop_loop = 0;

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

	/* init timer */
	hr_sleep_init(&timer_data);

	return 1;
};


unsigned long WINAPI output_loop(void* obj)
{
	void* buffer_address;

	// cast pointer to vzOutput
	vzOutput* tbc = (vzOutput*)obj;

	// init buffers
	void* alpha_buffer = memset(malloc(_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT*ALPHA_PIXEL_SIZE),0,_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT*ALPHA_PIXEL_SIZE);
	void* yuv_buffer = memset(malloc(_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT*YUV_PIXEL_SIZE),0,_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT*YUV_PIXEL_SIZE);
	void* fake_buffer = memset(malloc(_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT*4),0,_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT*4);

	/* endless loop */
	while(1)
	{
		// mark start time
		long A = timeGetTime();

		// check if all finished !!!
		if(notify_to_stop_loop)
		{
			// event to stop reached !!!
			free(yuv_buffer);
			free(alpha_buffer);
			free(fake_buffer);
			ExitThread(0);
		};

		// render new frame
		tbc->notify_frame_start();
		
		// 1. wait for signal about field #1 starts
		
		// 2. start transfering transcoded buffer

		// 3. request pointer to new buffer
		buffer_address = tbc->get_output_buf_ptr();


		if(_fc)
			_fc();


		// 4. transcode buffer
		if(vzConfigParam(_config,"nullvideo","YUV_CONVERT"))
			if (buffer_address)
				vzImageBGRA2YUAYVA
				(
					buffer_address,
					yuv_buffer,
					alpha_buffer,
					_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT
				);
		/* transfer buffer */
		if(vzConfigParam(_config,"nullvideo","OUTPUT_BUF_TRANSFER"))
			if (buffer_address)
				memcpy
				(
					fake_buffer,
					buffer_address,
					_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT*4
				);

		tbc->notify_frame_stop();

		// mark stop time
		long B = timeGetTime();

		// mark time to sleep
		long C = B - A;
		if((C>=0) && (C<40))
			hr_sleep(&timer_data, 40 - C);

#ifdef DEBUG_HARD_SYNC
		printf("A=%d, B=%d, C=%d\n", A, B, C);
#endif /* DEBUG_HARD_SYNC */
	};
};

VZOUTPUTS_EXPORT void vzOutput_StartOutputLoop(void* tbc)
{
	// create events for thread control
	notify_to_stop_loop = 0;

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
	notify_to_stop_loop = 1;

	// close everithing
	CloseHandle(loop_thread);
};

VZOUTPUTS_EXPORT long vzOutput_SetSync(frames_counter_proc fc)
{
	return (long)(_fc =  fc);
};




