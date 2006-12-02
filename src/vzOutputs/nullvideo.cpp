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
	2006-11-29:
		*Output/Input buffers described by the same data block

	2006-11-28:
		*Output buffers paramters aquired from output driver.

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


#include "../vz/vzOutput.h"
#include "../vz/vzOutput-devel.h"
#include "../vz/vzImage.h"
#include "../vz/vzOutputInternal.h"
#include "../vz/vzMain.h"
#include "../vz/vzTVSpec.h"
#include <stdio.h>

static void* _config = NULL;
static vzTVSpec* _tv = NULL;
static struct vzOutputBuffers* _buffers_info = NULL;
static struct hr_sleep_data timer_data;
static frames_counter_proc _fc = NULL;
static unsigned long test_patter_line[720];
static unsigned long bars_colour_values[8] = { 0xFFFFFFFF, 0xFFBFBE01, 0xFF01BFBF, 0xFF00BF00, 0xFFBF00BF, 0xFFC00000, 0xFF0100C0, 0xFF000000};


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

	/* init test pattern line */
	for(int i=0;i<720; i++)
		test_patter_line[i] = bars_colour_values[i / 90];

	return 1;
};


unsigned long WINAPI output_loop(void* obj)
{
	int j, b;
	void *output_buffer, **input_buffers;

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
		tbc->lock_io_bufs(&output_buffer, &input_buffers);

		/* send sync */
		if(_fc)
			_fc();

		// 4. transcode buffer
		if(vzConfigParam(_config,"nullvideo","YUV_CONVERT"))
			if (output_buffer)
				vzImageBGRA2YUAYVA
				(
					output_buffer,
					yuv_buffer,
					alpha_buffer,
					_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT
				);
		/* transfer buffer */
		if(vzConfigParam(_config,"nullvideo","OUTPUT_BUF_TRANSFER"))
			if (output_buffer)
				memcpy
				(
					fake_buffer,
					output_buffer,
					_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT*4
				);

		/* transfer input */
		if(_buffers_info->input.channels)
		{
			for
			(
				b = 0
				; 
				b
				<
				(
					(
						_buffers_info->input.channels 
						* 
						(
							(_buffers_info->input.field_mode)
							?
							2
							:
							1
						)
					)
				)
				;
				b++
			)
			{
				unsigned char *p = (unsigned char*)input_buffers[b];
				for
				(
					j = 0
					;
					j
					<
					(
						(_buffers_info->input.field_mode)
						?
						(_tv->TV_FRAME_HEIGHT/2)
						:
						_tv->TV_FRAME_HEIGHT
					)
					;
					j++, p += 4*_tv->TV_FRAME_WIDTH
				)
					memcpy(p, test_patter_line, 4*_tv->TV_FRAME_WIDTH);
			};
		};
				

		tbc->notify_frame_stop();

		// mark stop time
		long B = timeGetTime();

		// 3. request pointer to new buffer
		tbc->unlock_io_bufs(&output_buffer, &input_buffers);

		// mark time to sleep
		long C = B - A;
		if((C>=0) && (C<40))
			hr_sleep(&timer_data, 40 - C);
		else
			printf("nullvideo: %d miliseconds overrun\n", C - 40);

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

	/* set thread priority */
	SetThreadPriority(loop_thread , THREAD_PRIORITY_HIGHEST);
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

VZOUTPUTS_EXPORT void vzOutput_GetBuffersInfo(struct vzOutputBuffers* b)
{
	b->output.offset = 0;
	b->output.size = 4*_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT;
	b->output.gold = ((b->output.size + DMA_PAGE_SIZE)/DMA_PAGE_SIZE)*DMA_PAGE_SIZE;

	if(vzConfigParam(_config,"nullvideo","INPUTS_COUNT"))
	{
		b->input.channels = atol(vzConfigParam(_config,"nullvideo","INPUTS_COUNT"));
		b->input.field_mode = vzConfigParam(_config,"nullvideo","FIELD_MODE")?1:0;

		b->input.offset = 0;
		b->input.size = 4*_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT / (b->input.field_mode?2:1) ;
		b->input.gold = ((b->input.size + DMA_PAGE_SIZE)/DMA_PAGE_SIZE)*DMA_PAGE_SIZE;

		b->input.width = _tv->TV_FRAME_WIDTH;
		b->input.height = _tv->TV_FRAME_HEIGHT / (b->input.field_mode?2:1);
	};

	_buffers_info = b;
};

VZOUTPUTS_EXPORT void vzOutput_AssignBuffers(struct vzOutputBuffers* b)
{

};

