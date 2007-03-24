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
	2006-12-05:
		*Field duplicating in 'FIELD_MODE' adjusted by parameter 'TWICE_FIELDS'
		*DUPL_LINES_ARCH enable use mmx and sse field duplicator.
		*SSE possibility usage detection.

	2006-12-04:
		*Input test pattern added.
		*Pattern selected via INPUT_%d_PATTERN config var.
		*Added MMX and SSE2 copy fields data routines.

	2006-11-29:
		*Output/Input buffers described by the same data block

	2006-11-28:
		*Output buffers paramters aquired from output driver.

    2005-06-25:
		*Fake output module developed ....
*/

#define DUPL_LINES_ARCH


#include <windows.h>
#include "hr_timer.h"
#include "arch_copy.h"

#include "../vz/vzOutput.h"
#include "../vz/vzOutput-devel.h"
#include "../vz/vzImage.h"
#include "../vz/vzOutputInternal.h"
#include "../vz/vzMain.h"
#include "../vz/vzTVSpec.h"
#include <stdio.h>

/* test patters */
#define TP_COUNT 3
#include "nullvideo.tp_bars.h"
#include "nullvideo.tp_grid.h"
#include "nullvideo.tp_lines.h"
static void *tps[TP_COUNT] = { tp_bars_surface, tp_grid_surface, tp_lines_surface};
static int channels_tp[VZOUTPUT_MAX_CHANNELS] = {0, 0, 0, 0};

static void* _config = NULL;
static vzTVSpec* _tv = NULL;
static struct vzOutputBuffers* _buffers_info = NULL;
static struct hr_sleep_data timer_data;
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
	int j, b, i, c;
	void *output_buffer, **input_buffers, *output_a_buffer, **input_a_buffers;

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
		tbc->lock_io_bufs(&output_buffer, &input_buffers, &output_a_buffer, &input_a_buffers);

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

		/* transfer input(s) */
		for(c = 0; c < _buffers_info->input.channels; c++)
		{
			/* defferent methods for fields and frame mode */
			if(_buffers_info->input.field_mode)
			{
				/* field mode */
				long l = _tv->TV_FRAME_WIDTH*4;
				unsigned char
					*src = (unsigned char*)tps[channels_tp[c]],
					*dstA = (unsigned char*)input_buffers[2*c],
					*dstB = (unsigned char*)input_buffers[2*c + 1];

				for(i = 0; i < 576; i++)
				{
					/* calc case number */
					int d = 
						((i&1)?2:0)
						|
						((_buffers_info->input.twice_fields)?1:0);

					switch(d)
					{
						case 3:
							/* odd field */
							/* duplicate fields */
#ifdef DUPL_LINES_ARCH
							if(sse_supported)
								copy_data_twice_sse(dstB, src);
							else
								copy_data_twice_mmx(dstB, src);
							dstB += 2*l;
#else  /* !DUPL_LINES_ARCH */
							memcpy(dstB, src, l); dstB += l;
							memcpy(dstB, src, l); dstB += l;
#endif /* DUPL_LINES_ARCH */
							break;

						case 2:
							/* odd field */
							/* NO duplication */
							memcpy(dstB, src, l); dstB += l;
							break;

						case 1:
							/* even field */
							/* duplicate fields */
#ifdef DUPL_LINES_ARCH
							if(sse_supported)
								copy_data_twice_sse(dstA, src);
							else
								copy_data_twice_mmx(dstA, src);
							dstA += 2*l;
#else  /* !DUPL_LINES_ARCH */
							memcpy(dstA, src, l); dstA += l;
							memcpy(dstA, src, l); dstA += l;
#endif /* DUPL_LINES_ARCH */
							break;

						case 0:
							/* even field */
							/* NO duplication */
							memcpy(dstA, src, l); dstA += l;
							break;
					};

					/* step forward src */
					src += l;
				};
			}
			else
			{
				/* frame mode */
				memcpy
				(
					input_buffers[c],
					tps[channels_tp[c]],
					_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT*4
				);
			};
		};
				

		tbc->notify_frame_stop();

		// mark stop time
		long B = timeGetTime();

		// 3. request pointer to new buffer
		tbc->unlock_io_bufs(&output_buffer, &input_buffers, &output_a_buffer, &input_a_buffers);

		// mark time to sleep
		long C = B - A;
		if((C>=0) && (C<40))
			hr_sleep(&timer_data, 40 - C);
		else
		{
			printf("nullvideo: %d miliseconds overrun\n", C - 40);
			Sleep(0);
		};

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

	/* wait for thread finish */
	WaitForSingleObject(loop_thread, INFINITE);

	/* close thread handle */
	CloseHandle(loop_thread);
};

VZOUTPUTS_EXPORT long vzOutput_SetSync(frames_counter_proc fc)
{
	return (long)(_fc =  fc);
};

VZOUTPUTS_EXPORT void vzOutput_GetBuffersInfo(struct vzOutputBuffers* b)
{
	int i;
	char temp[128];

	b->output.offset = 0;
	b->output.size = 4*_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT;
	b->output.gold = ((b->output.size + DMA_PAGE_SIZE)/DMA_PAGE_SIZE)*DMA_PAGE_SIZE;

	/* inputs count conf */
	if(vzConfigParam(_config,"nullvideo","INPUTS_COUNT"))
	{
		b->input.channels = atol(vzConfigParam(_config,"nullvideo","INPUTS_COUNT"));
		b->input.field_mode = vzConfigParam(_config,"nullvideo","FIELD_MODE")?1:0;
		if(b->input.field_mode)
			b->input.twice_fields = vzConfigParam(_config,"nullvideo","TWICE_FIELDS")?1:0;
		else
			b->input.twice_fields = 0;

		int k = ((b->input.field_mode) && (!(b->input.twice_fields)))?2:1;

		b->input.offset = 0;
		b->input.size = 4*_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT / k;
		b->input.gold = ((b->input.size + DMA_PAGE_SIZE)/DMA_PAGE_SIZE)*DMA_PAGE_SIZE;

		b->input.width = _tv->TV_FRAME_WIDTH;
		b->input.height = _tv->TV_FRAME_HEIGHT / k;
	};

	/* input test pattern */
	for(i = 0; i< VZOUTPUT_MAX_CHANNELS ; i++)
	{
		sprintf(temp, "INPUT_%d_PATTERN", i + 1);
		if(vzConfigParam(_config,"nullvideo", temp))
			if(TP_COUNT <= (channels_tp[i] = atol(vzConfigParam(_config,"nullvideo",temp))))
				channels_tp[i] = 0;
	};
	
	_buffers_info = b;

#ifdef DUPL_LINES_ARCH
	/* detect SSE2 support */
	DUPL_LINES_ARCH_SSE2_DETECT;
	/* report */
	printf("nullvideo: %s detected\n", (sse_supported)?"SSE2":"NO SEE2");
#endif /* DUPL_LINES_ARCH */
};

VZOUTPUTS_EXPORT void vzOutput_AssignBuffers(struct vzOutputBuffers* b)
{

};

