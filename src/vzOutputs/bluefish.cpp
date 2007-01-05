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
	2007-01-05:
		*'blue_emb_audio_group1_enable' introduced in 5.4.18betta drivers.
		*decomposing fields into 2 buffers moved into parallel thread.
		*embedded audio from HANC buffer moved to another async thread.
		WARNING!:
		1. audio delayed in 1 frame if embedded into SDI.
		2. video delayed in 1 frame if fields processing used.

	2006-12-17:
		*input buffers cleanup.
		*HANC buffer size decrease.

	2006-12-16:
		*audio support added.

	2006-12-12:
		*Attemp move board init to DLL attach block.
		*SWAP_INPUT_CONNECTORS added, seem to be usefull when we want
		to use input and anboard keyer.
		*O_SWAP_INPUT_CONNECTORS will swap channels.
		*OVERRUN_RECOVERY feature is usefull then I/O operations longer
		frame duration in 0..7 miliseconds - we will start capture/play 
		frame without wait of H sync.
		.

	2006-12-10:
		*WE STARTED THIS DRIVERS AFTER 1.5 YEAR!!! AT LAST!!!!!

*/

#include <windows.h>

#include "../vz/vzOutput.h"
#include "../vz/vzOutput-devel.h"
#include "../vz/vzImage.h"
#include "../vz/vzOutputInternal.h"
#include "../vz/vzMain.h"
#include "../vz/vzTVSpec.h"

#include <stdio.h>

#include <BlueVelvet4.h>

#include "arch_copy.h"

#define DUPL_LINES_ARCH
#define DIRECT_FRAME_COPY

#define OVERRUN_RECOVERY 7			/* 7 miliseconds is upper limit */

#define dump_line {};
#ifndef dump_line
#define dump_line fprintf(stderr, __FILE__ ":%d\n", __LINE__); fflush(stderr);
#endif

/* ------------------------------------------------------------------

	Module definitions 

------------------------------------------------------------------ */
#define MODULE_PREFIX "bluefish: "
#define CONFIG_O(VAR) vzConfigParam(_config, "bluefish", VAR)

#define O_KEY_INVERT			"KEY_INVERT"
#define O_KEY_WHITE				"KEY_WHITE"
#define O_SINGLE_INPUT			"SINGLE_INPUT"
#define O_DUAL_INPUT			"DUAL_INPUT"
#define O_VIDEO_MODE			"VIDEO_MODE"
#define O_PAL					"PAL"
#define O_ONBOARD_KEYER			"ONBOARD_KEYER"
#define O_H_PHASE_OFFSET		"H_PHASE_OFFSET"
#define O_V_PHASE_OFFSET		"V_PHAZE_OFFSET"
#define O_VERTICAL_FLIP			"VERTICAL_FLIP"
#define O_SCALED_RGB			"SCALED_RGB"
#define O_SOFT_FIELD_MODE		"SOFT_FIELD_MODE"
#define O_SOFT_TWICE_FIELDS		"SOFT_TWICE_FIELDS"
#define O_SWAP_INPUT_CONNECTORS	"SWAP_INPUT_CONNECTORS"

#define O_AUDIO_OUTPUT_EMBED	"AUDIO_OUTPUT_EMBED"
#define O_AUDIO_OUTPUT_ENABLE	"AUDIO_OUTPUT_ENABLE"
#define O_AUDIO_INPUT_ENABLE	"AUDIO_INPUT_ENABLE"
#define O_AUDIO_INPUT_EMBED		"AUDIO_INPUT_EMBED"

#define MAX_HANC_BUFFER_SIZE (64*1024)
//#define MAX_HANC_BUFFER_SIZE (256*1024)
#define MAX_INPUTS 2

/* ------------------------------------------------------------------

	Module variables 

------------------------------------------------------------------ */
static int flag_exit = 0;
static CBlueVelvet4* bluefish[3] = {NULL, NULL, NULL};
static void* bluefish_obj = NULL;
static unsigned char* input_mapped_buffers[MAX_INPUTS][2];
static unsigned int flip_buffers_index = 0;
static HANDLE main_io_loop_thread = INVALID_HANDLE_VALUE ;

static void* _config = NULL;
static vzTVSpec* _tv = NULL;
static frames_counter_proc _fc = NULL;
static struct vzOutputBuffers* _buffers_info = NULL;

/* ------------------------------------------------------------------

	Operations parameters 

------------------------------------------------------------------ */
static unsigned long 
	device_id = 1;

static int 
	boards_count, 
	inputs_count = 0;

static int
	audio_output = 0,
	audio_output_embed = 0,
	audio_input = 0,
	audio_input_embed = 0;

static void* input_hanc_buffers[MAX_INPUTS][2];

static unsigned long 
	video_format, 
	memory_format = MEM_FMT_ARGB_PC, 
	update_format = UPD_FMT_FRAME, 
	video_resolution = RES_FMT_NORMAL,
	video_engine = VIDEO_ENGINE_PLAYBACK,
	scaled_rgb = 0,
	buffers_count = 0, 
	buffers_length = 0,
	buffers_actual = 0, 
	buffers_golden = 0;

static unsigned int
	h_phase, v_phase, h_phase_max, v_phase_max;

static int
	key_on = 1,
	key_v4444 = 0,
	key_invert = 0,
	key_white = 0,
	video_on = 1,
	onboard_keyer = 0,
	vertical_flip = 1;

/* ------------------------------------------------------------------

	VIDEO IO

------------------------------------------------------------------ */
struct io_in_desc
{
	int id;
	void** buffers;
	void* audio;
};

struct io_out_desc
{
	void* buffer;
	void* audio;
};

static unsigned long WINAPI hanc_decode(void* p)
{
	struct io_in_desc* desc = (struct io_in_desc*)p;

	/* audio deals */
	if(audio_input_embed)
	{
		/* clear buffer */
		memset(desc->audio, 0, 2 * 2 * VZOUTPUT_AUDIO_SAMPLES);

		/* decode embedded audio */
		int c = emb_audio_decoder
		(
			(unsigned int *)input_hanc_buffers[desc->id - 1][1 - flip_buffers_index],
			desc->audio,
			VZOUTPUT_AUDIO_SAMPLES,					/* BLUE_UINT32 req_audio_sample_count, */
			STEREO_PAIR_1,							/* BLUE_UINT32 required_audio_channels, */
			AUDIO_CHANNEL_LITTLEENDIAN | 
			AUDIO_CHANNEL_16BIT						/* BLUE_UINT32 sample_type */
		);
	};
	
	return 0;
};


static unsigned long WINAPI demux_fields(void* p)
{
	int i;
	struct io_in_desc* desc = (struct io_in_desc*)p;

	/* defferent methods for fields and frame mode */
	if(_buffers_info->input.field_mode)
	{
		dump_line;

		/* field mode */
		long l = _tv->TV_FRAME_WIDTH*4;
		unsigned char
			*src = (unsigned char*)input_mapped_buffers[desc->id - 1][1 - flip_buffers_index],
			*dstA = (unsigned char*)desc->buffers[0],
			*dstB = (unsigned char*)desc->buffers[1];
		dump_line;
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
		dump_line;
	};

	return 0;
};

static unsigned long WINAPI io_in_a(void* p)
{
#define MAX_DROPPED_CLEAN 5
	unsigned long **buffers = (unsigned long **)p;
	unsigned long *composite_buffer = (unsigned long *)malloc(MAX_DROPPED_CLEAN*VZOUTPUT_MAX_CHANNELS * 2 * 2 * VZOUTPUT_AUDIO_SAMPLES);
	int 
		r = 0,
		n = 0,
		channel_map = (2 == inputs_count)?(STEREO_PAIR_1 | STEREO_PAIR_2):STEREO_PAIR_1
		;

	/* clean buffer */
	memset(composite_buffer, 0, 2 * 2 * VZOUTPUT_AUDIO_SAMPLES);

	/* read first buffer */
	n = bluefish[0]->ReadAudioSample
	(
		channel_map,
		composite_buffer,
		VZOUTPUT_AUDIO_SAMPLES * MAX_DROPPED_CLEAN,
		AUDIO_CHANNEL_LITTLEENDIAN | AUDIO_CHANNEL_16BIT,
		0
	);

	/* demultiplex buffers */
	if(2 == inputs_count)
	{
		for(int i=0, j=0 ; j<VZOUTPUT_AUDIO_SAMPLES; j++)
		{
			buffers[0][j] = composite_buffer[i]; i++;
			buffers[1][j] = composite_buffer[i]; i++;
		};
	}
	else
		memcpy(buffers[0], composite_buffer, 4 * VZOUTPUT_AUDIO_SAMPLES);

	/* free buffers */
	free(composite_buffer);

	return 0;
};

static unsigned long WINAPI io_out(void* p)
{
	unsigned long address, buffer_id, underrun, next_buf_id;
	struct io_out_desc* desc = (struct io_out_desc*)p;
	void* buffer = desc->buffer;

	if(BLUE_OK(bluefish[0]->video_playback_allocate((void **)&address,buffer_id,underrun)))
	{
		if (buffer_id  != -1)
		{
			bluefish[0]->system_buffer_write_async
			(
				(unsigned char*)buffer,	/* buffer id */
				buffers_golden,					/* buffer size */
				NULL,
				buffer_id,						/* host buffer id */ 
				0								/* offset */
			);
			bluefish[0]->video_playback_present
			(
				next_buf_id,
				buffer_id,
				1,								/* count */
				0,								/* keep */
				0								/* odd */
			);

			/* write audio */
			if(audio_output)
			{
				int c = bluefish[0]->WriteAudioSample
				(
					STEREO_PAIR_1,					/* int AudioChannelMap, */
					desc->audio,					/* void * pBuffer, */
					VZOUTPUT_AUDIO_SAMPLES,			/* long nSampleCount, */
					AUDIO_CHANNEL_LITTLEENDIAN |	/* int bFlag, */
					AUDIO_CHANNEL_16BIT,
					0
				);
//fprintf(stderr, "out: c=%d\n", c);
			};
		}
		else
		{
			fprintf(stderr, MODULE_PREFIX "'buffer_id' failed for 'video_playback_allocate'\n");
		};
	}
	else
	{
		fprintf(stderr, MODULE_PREFIX "'video_playback_allocate' failed\n");
	};

	return 0;
};

static unsigned long WINAPI io_in(void* p)
{
	struct io_in_desc* desc = (struct io_in_desc*)p;
	void* buf;
	int i;
	
	unsigned long address, buffer_id, dropped_count, remain_count;
	dump_line;
	if
	(
		BLUE_OK
		(
			bluefish[desc->id]->video_capture_harvest
			(
				(void **)&address, 
				buffer_id, 
				dropped_count, 
				remain_count
			)
		)
	)
	{
		dump_line;

		if (buffer_id  != -1)
		{
			if(!(_buffers_info->input.field_mode))
				/* if no transformation required - directly */
				buf = desc->buffers[0];
			else
				/* read from host mem to mapped memmory */
				buf = input_mapped_buffers[desc->id - 1][flip_buffers_index];

			/* read video mem */
			bluefish[desc->id]->system_buffer_read_async
			(
				(unsigned char *)buf,
				buffers_golden,
				NULL,
				buffer_id
			);
			dump_line;

			/* audio deals */
			if(audio_input_embed)
			{
				/* read HANC data */
				bluefish[desc->id]->system_buffer_read_async
				(
					(unsigned char *)input_hanc_buffers[desc->id - 1][flip_buffers_index],
					MAX_HANC_BUFFER_SIZE,
					NULL,
					BlueImage_VBI_HANC_DMABuffer
					(
						buffer_id,
						BLUE_DATA_HANC
					)
				);
			};

			/* check dropped frames */
/*			if(dropped_count)
			{
				fprintf(stderr, MODULE_PREFIX "dropped %d frames at [%d]\n", dropped_count, desc->id);
			}; */

		}
		else
		{
			fprintf(stderr, MODULE_PREFIX "'buffer_id' failed for 'video_capture_harvest[%d]'\n", desc->id);
		};
	}
	else
	{
		fprintf(stderr, MODULE_PREFIX "failed 'video_capture_harvest[%d]'\n", desc->id);
	};

	dump_line;

	return 0;
};

static unsigned long WINAPI main_io_loop(void* p)
{
	/* cast pointer to vzOutput */
	vzOutput* tbc = (vzOutput*)p;

	void *output_buffer, **input_buffers, *output_a_buffer, **input_a_buffers;
	struct io_in_desc in1;
	struct io_in_desc in2;
	struct io_out_desc out_main;

	unsigned long f1;
	HANDLE io_ops[8];
	unsigned long io_ops_id[8];
	int r, i;

	/* clear thread handles values */
	for(i = 0; i<8 ; i++)
		io_ops[i] = INVALID_HANDLE_VALUE;

#ifdef OVERRUN_RECOVERY
	int skip_wait_sync = 0;
#endif /* OVERRUN_RECOVERY */


	/* map buffers for input */
	if(inputs_count)
	{
		for(i = 0; i<2; i++)
		{
			input_mapped_buffers[0][i] = (unsigned char*)VirtualAlloc
			(
				NULL, 
				buffers_golden,
				MEM_COMMIT, 
				PAGE_READWRITE
			);
			VirtualLock(input_mapped_buffers[0][i], buffers_golden);
		};

		if(inputs_count > 1)
		{
			for(i = 0; i<2; i++)
			{
				input_mapped_buffers[1][i] = (unsigned char*)VirtualAlloc
				(
					NULL, 
					buffers_golden,
					MEM_COMMIT, 
					PAGE_READWRITE
				);
				VirtualLock(input_mapped_buffers[1][i], buffers_golden);
			};
		};
	};
	fprintf(stdout, MODULE_PREFIX "input buffers: 0x%.8X/0x%.8X, 0x%.8X/0x%.8X\n", 
		(unsigned long)input_mapped_buffers[0][0], (unsigned long)input_mapped_buffers[0][1],
		(unsigned long)input_mapped_buffers[1][0], (unsigned long)input_mapped_buffers[1][1]);
	
	/* check if mapped buffer is OK */
	if
	(
		(
			(inputs_count)
			&&
			(
				(NULL == input_mapped_buffers[0][0])
				||
				(NULL == input_mapped_buffers[0][1])
			)
		)
		||
		(
			(inputs_count > 1)
			&&
			(
				(NULL == input_mapped_buffers[1][0])
				||
				(NULL == input_mapped_buffers[1][1])
			)
		)
	)
	{
		fprintf(stderr, MODULE_PREFIX "ERROR!! CRITICAL!!! RESTART PC - Buffers are NULL\n");
		exit(-1);
	};


	/* start playback/capture */
    r = bluefish[0]->video_playback_start(false, false);
	if(inputs_count)
		r = bluefish[1]->video_capture_start();
	if(inputs_count > 1)
		r = bluefish[2]->video_capture_start();

	/* start audio capture / playout */
	if(audio_output)
	{
		r = bluefish[0]->InitAudioPlaybackMode();
		r = bluefish[0]->StartAudioPlayback(0);
		if
		(
			(audio_input)
			&&
			(!(audio_input_embed))
		)
		{
			bluefish[0]->InitAudioCaptureMode();
			{
				VARIANT v;
				v.ulVal= 0; /* 0 - AES, 1 - Analouge, 2 - SDI A, 3 - SDI B */
				v.vt = VT_UI4;
				bluefish[0]->SetCardProperty(AUDIO_INPUT_PROP,v);
			}
			bluefish[0]->StartAudioCapture(0);		
		};
	};


	/* notify */
	fprintf(stderr, MODULE_PREFIX "'main_io_loop' started\n");

	/* skip 2 cyrcles */
	bluefish[0]->wait_output_video_synch(update_format, f1);
	bluefish[0]->wait_output_video_synch(update_format, f1);

	long A,B, C1 = 0, C2 = 0;

	/* endless loop */
	while(!(flag_exit))
	{
		dump_line;

#ifdef OVERRUN_RECOVERY
		if(!(skip_wait_sync))
#endif /* OVERRUN_RECOVERY */
		/* wait output vertical sync */
		bluefish[0]->wait_output_video_synch(update_format, f1);

		/* flip buffers index */
		flip_buffers_index = 1 - flip_buffers_index;
		
		/* fix time */
		C1 = (A = timeGetTime());

		if(C2)
		{
			if((C1 - C2) > 50)
				fprintf(stderr, MODULE_PREFIX "%d out of sync \n",  (C1 - C2) - 40);
		};
		C2 = C1;
		

		dump_line;

		/* send sync */
		if(_fc)
			_fc();

		/* request pointers to buffers */
		tbc->lock_io_bufs(&output_buffer, &input_buffers, &output_a_buffer, &input_a_buffers);

		dump_line;

		/* start output thread */
		out_main.audio = output_a_buffer;
		out_main.buffer = output_buffer;
		io_ops[0] = CreateThread(0, 0, io_out,  &out_main , 0, &io_ops_id[0]);

		/* start audio output */
		if
		(
			(inputs_count)
			&&
			(audio_input)
			&&
			(!(audio_input_embed))
		)
			io_ops[3] = CreateThread(0, 0, io_in_a,  input_a_buffers , 0, &io_ops_id[3]);
		else
			io_ops[3] = INVALID_HANDLE_VALUE;


		/* start inputs */
		io_ops[1] = INVALID_HANDLE_VALUE;
		io_ops[2] = INVALID_HANDLE_VALUE;
		io_ops[4] = INVALID_HANDLE_VALUE;
		io_ops[5] = INVALID_HANDLE_VALUE;
		io_ops[6] = INVALID_HANDLE_VALUE;
		io_ops[7] = INVALID_HANDLE_VALUE;
		if(inputs_count)
		{
			in1.id = 1;
			in1.buffers = &input_buffers[0];
			in1.audio = input_a_buffers[0];
			io_ops[1] = CreateThread(0, 0, io_in,  &in1 , 0, &io_ops_id[1]);
			io_ops[4] = CreateThread(0, 0, demux_fields,  &in1 , 0, &io_ops_id[4]);
			io_ops[6] = CreateThread(0, 0, hanc_decode,  &in1 , 0, &io_ops_id[6]);

			if(inputs_count > 1)
			{
				in2.id = 2;
				in2.buffers = &input_buffers[ (_buffers_info->input.field_mode)?2:1 ];
				in2.audio = input_a_buffers[1];
				io_ops[2] = CreateThread(0, 0, io_in,  &in2 , 0, &io_ops_id[2]);
				io_ops[5] = CreateThread(0, 0, demux_fields,  &in2 , 0, &io_ops_id[5]);
				io_ops[7] = CreateThread(0, 0, hanc_decode,  &in2 , 0, &io_ops_id[7]);
			}
		};

		/* wait for thread ends */
		for(i = 0; i<8; i++)
			if(INVALID_HANDLE_VALUE != io_ops[i])
			{
				WaitForSingleObject(io_ops[i], INFINITE);
				CloseHandle(io_ops[i]);
				io_ops[i] = INVALID_HANDLE_VALUE;
			};
		dump_line;

		/* unlock buffers */
		tbc->unlock_io_bufs(&output_buffer, &input_buffers, &output_a_buffer, &input_a_buffers);
		dump_line;

		B = timeGetTime();

		if ( (B - A) >= 40 )
		{
			fprintf(stderr, MODULE_PREFIX "%d miliseconds overrun",  (B - A) - 40);
#ifdef OVERRUN_RECOVERY
			if
			(
				( (B - A - 40) < OVERRUN_RECOVERY)	/* overrun in our range */
				&&									/* and */
				(0 == skip_wait_sync)				/* previous not skipped */
			)
			{
				skip_wait_sync = 1;
				fprintf(stderr, " [recovering]");
			}
			else
				skip_wait_sync = 0;
#endif /* OVERRUN_RECOVERY */
			fprintf(stderr, "\n");
		}
#ifdef OVERRUN_RECOVERY
		else 
		{
			skip_wait_sync = 0;
		}
#endif /* OVERRUN_RECOVERY */
		;
	};

	/* stop playback/capture */
    bluefish[0]->video_playback_stop(false, false);
	if(inputs_count)
	{
		bluefish[1]->video_capture_stop();
		if(inputs_count > 1)
			bluefish[2]->video_capture_stop();
	};

	/* stop audio capture / playout */
	if(audio_output)
	{
		r = bluefish[0]->StopAudioPlayback();
		r = bluefish[0]->EndAudioPlaybackMode();
		if
		(
			(audio_input)
			&&
			(!(audio_input_embed))
		)
		{
			r = bluefish[0]->StopAudioCapture();
			r = bluefish[0]->EndAudioCaptureMode(); 
		};
	};

	/* unmap drivers buffer */
	if(inputs_count)
	{
		for(i = 0; i<2 ;i++)
		{
			VirtualUnlock(input_mapped_buffers[0][i], buffers_golden);
			VirtualFree(input_mapped_buffers[0][i], buffers_golden, MEM_RELEASE);
		};

		if(inputs_count > 1)
		{
			for(i = 0; i<2 ;i++)
			{
				VirtualUnlock(input_mapped_buffers[1][i], buffers_golden);
				VirtualFree(input_mapped_buffers[1][i], buffers_golden, MEM_RELEASE);
			};
		};

	};

	/* notify */
	fprintf(stderr, MODULE_PREFIX "'main_io_loop' finished\n");

	return 0;
};

/* ------------------------------------------------------------------

	Bluefish operations

------------------------------------------------------------------ */


static void bluefish_destroy(void)
{
	int i;

	/* finish */
	for(i = 0; i< 3; i++)
		if(bluefish[i])
		{
			bluefish[i]->device_detach();
			delete bluefish[i];
			bluefish[i] = NULL;
		};

	if(bluefish_obj)
		blue_detach_from_device(&bluefish_obj);
};

static int bluefish_init(int board_id)
{
	int r;

	/* init object */
	bluefish[0] = BlueVelvetFactory4();
	bluefish[1] = BlueVelvetFactory4();
	bluefish[2] = BlueVelvetFactory4();
	printf(MODULE_PREFIX "%s\n", BlueVelvetVersion());

	/* find boards */
	r = bluefish[0]->device_enumerate(boards_count);
	printf(MODULE_PREFIX "Found %d boards\n", boards_count);

	/* check for boards present */
	if (0 == boards_count)
	{
		bluefish_destroy();
		return 0;
	}

	/* select device */
	r = bluefish[0]->device_attach(board_id,0); /* 1 - device number, 0 - no audio */
	r = bluefish[1]->device_attach(board_id,0); /* 1 - device number, 0 - no audio */
	r = bluefish[2]->device_attach(board_id,0); /* 1 - device number, 0 - no audio */
	bluefish_obj = blue_attach_to_device(board_id);

	return 1;
};

static void bluefish_configure(void)
{
	int r = 0, i=0;
	char* temp;


	/* detect PAL/NTSC */
	if(NULL == (temp = CONFIG_O(O_VIDEO_MODE)))
		video_format = VID_FMT_PAL;
	else
	{
		if(0 == stricmp(temp, O_PAL))
			video_format = VID_FMT_PAL;
		else
			video_format = VID_FMT_NTSC;
	};

	/* setup default channels */
	{
		VARIANT v;
		v.vt = VT_UI4;
	
		/* outputs */
		v.ulVal = BLUE_VIDEO_OUTPUT_CHANNEL_A;
		r = bluefish[0]->SetCardProperty(DEFAULT_VIDEO_OUTPUT_CHANNEL, v);

		v.ulVal = BLUE_VIDEO_OUTPUT_CHANNEL_A;
		r = bluefish[1]->SetCardProperty(DEFAULT_VIDEO_OUTPUT_CHANNEL, v);

		v.ulVal = BLUE_VIDEO_OUTPUT_CHANNEL_B;
		r = bluefish[2]->SetCardProperty(DEFAULT_VIDEO_OUTPUT_CHANNEL, v);


		/* inputs */
//		v.ulVal = BLUE_VIDEO_INPUT_CHANNEL_A;
		v.ulVal = (!CONFIG_O(O_SWAP_INPUT_CONNECTORS))?BLUE_VIDEO_INPUT_CHANNEL_A:BLUE_VIDEO_INPUT_CHANNEL_B;
		r = bluefish[0]->SetCardProperty(DEFAULT_VIDEO_INPUT_CHANNEL, v);

//		v.ulVal = BLUE_VIDEO_INPUT_CHANNEL_A;
		v.ulVal = (!CONFIG_O(O_SWAP_INPUT_CONNECTORS))?BLUE_VIDEO_INPUT_CHANNEL_A:BLUE_VIDEO_INPUT_CHANNEL_B;
		r = bluefish[1]->SetCardProperty(DEFAULT_VIDEO_INPUT_CHANNEL, v);

//		v.ulVal = BLUE_VIDEO_INPUT_CHANNEL_B;
		v.ulVal = (!CONFIG_O(O_SWAP_INPUT_CONNECTORS))?BLUE_VIDEO_INPUT_CHANNEL_B:BLUE_VIDEO_INPUT_CHANNEL_A;
		r = bluefish[2]->SetCardProperty(DEFAULT_VIDEO_INPUT_CHANNEL, v);

	};

	/* check if use input */
	if
	(
		(NULL != CONFIG_O(O_SINGLE_INPUT))
		||
		(NULL != CONFIG_O(O_DUAL_INPUT))
	)
	{
		inputs_count = 1;
		video_engine = VIDEO_ENGINE_DUPLEX;

		/* check if we should use 2 video channels */
		if(NULL != CONFIG_O(O_DUAL_INPUT))
			inputs_count = 2;
	};

	/* detach unused devices */
	if(inputs_count < 2)
	{
		/* free and detach */
		bluefish[2]->device_detach();
		delete bluefish[2];
		bluefish[2] = NULL;

		if(inputs_count < 1)
		{
			/* free and detach */
			bluefish[1]->device_detach();
			delete bluefish[1];
			bluefish[1] = NULL;
		};
	};


	/* configure matrix */
	{
		EBlueConnectorPropertySetting routes[2];
		memset(routes, 0, sizeof(routes));


		/* configure dual output */
		routes[0].channel = BLUE_VIDEO_OUTPUT_CHANNEL_A;	
		routes[0].connector = BLUE_CONNECTOR_DVID_1;
		routes[0].signaldirection = BLUE_CONNECTOR_SIGNAL_OUTPUT;
		routes[0].propType = BLUE_CONNECTOR_PROP_DUALLINK_LINK_1;

		routes[1].channel = BLUE_VIDEO_OUTPUT_CHANNEL_A;	
		routes[1].connector = BLUE_CONNECTOR_DVID_2;
		routes[1].signaldirection = BLUE_CONNECTOR_SIGNAL_OUTPUT;
		routes[1].propType = BLUE_CONNECTOR_PROP_DUALLINK_LINK_2;

		r = blue_set_connector_property(bluefish_obj, 2, routes);

		if(inputs_count)
		{
			/* first input for channel A */
			routes[0].channel = BLUE_VIDEO_INPUT_CHANNEL_A;
			routes[0].connector = BLUE_CONNECTOR_DVID_3;
			routes[0].signaldirection = BLUE_CONNECTOR_SIGNAL_INPUT;
			routes[0].propType = BLUE_CONNECTOR_PROP_SINGLE_LINK;
			r = blue_set_connector_property(bluefish_obj, 1, routes);

			if(inputs_count > 1)
			{
				/* second input for channel B */
				routes[0].channel = BLUE_VIDEO_INPUT_CHANNEL_B;
				routes[0].connector = BLUE_CONNECTOR_DVID_4;
				routes[0].signaldirection = BLUE_CONNECTOR_SIGNAL_INPUT;
				routes[0].propType = BLUE_CONNECTOR_PROP_SINGLE_LINK;
				r = blue_set_connector_property(bluefish_obj, 1, routes);
			};
		
		};
	};

	/* setup video */
	for(i=0 ; (i<3) && (bluefish[i]) ; i++)
	r = bluefish[i]->set_video_framestore_style(video_format, memory_format, update_format, video_resolution);

	/* setup engine */
	for(i=0 ; (i<3) && (bluefish[i]) ; i++)
	r = bluefish[i]->set_video_engine(video_engine);

	/* deal with key */
	key_white = (NULL != CONFIG_O(O_KEY_WHITE))?1:0;
	key_invert = (NULL != CONFIG_O(O_KEY_INVERT))?1:0;
	r = bluefish[0]->set_output_key(key_on, key_v4444, key_invert, key_white);

	/* enable video output */
	r = bluefish[0]->set_output_video(video_on);

	/* enable onboard keyer */
	onboard_keyer = (NULL != CONFIG_O(O_ONBOARD_KEYER))?1:0;
	r = bluefish[0]->set_onboard_keyer (onboard_keyer);

	/* vertical flip */
	vertical_flip = (NULL != CONFIG_O(O_VERTICAL_FLIP))?0:1;
	for(i=0 ; (i<3) && (bluefish[i]) ; i++)
	r = bluefish[i]->set_vertical_flip(vertical_flip);

	/* rgb scaling */
	scaled_rgb = (NULL != CONFIG_O(O_SCALED_RGB))?1:0;
	for(i=0 ; (i<3) && (bluefish[i]) ; i++)
	r = bluefish[i]->set_scaled_rgb(scaled_rgb);

	/* genlock source */
	//SetCardProperty


	/* timing */
	r = bluefish[0]->get_timing_adjust(h_phase, v_phase, h_phase_max, v_phase_max);
	printf(MODULE_PREFIX "current h_phase=%ld, v_phase=%ld, h_phase_max=%ld, v_phase_max=%ld\n", h_phase, v_phase, h_phase_max, v_phase_max);
	/*warn about GENLOCK */
	if
	(
		(0 == h_phase_max)
		||
		(0 == v_phase_max)
	)
		printf(MODULE_PREFIX "WARNING! GENLOCK failed!\n");
	if(NULL != CONFIG_O(O_H_PHASE_OFFSET))
		h_phase += atol(CONFIG_O(O_H_PHASE_OFFSET));
	if(NULL != CONFIG_O(O_V_PHASE_OFFSET))
		v_phase += atol(CONFIG_O(O_V_PHASE_OFFSET));
	if
	(
		/* time change required */
		(
			(NULL != CONFIG_O(O_V_PHASE_OFFSET))
			||
			(NULL != CONFIG_O(O_H_PHASE_OFFSET))
		)
		&&
		/* and in check range */
		(
			(h_phase < h_phase_max)
			&&
			(v_phase < v_phase_max)
		)
	)
		r = bluefish[0]->set_timing_adjust(h_phase, v_phase);

	/* check if audio output is enables */
	if(audio_output = CONFIG_O(O_AUDIO_OUTPUT_ENABLE)?1:0)
	{
		/* embed audio output */
		audio_output_embed = CONFIG_O(O_AUDIO_OUTPUT_EMBED)?1:0;

		/* check input options */
		if(audio_input = CONFIG_O(O_AUDIO_INPUT_ENABLE)?1:0)
			audio_input_embed = CONFIG_O(O_AUDIO_INPUT_EMBED)?1:0;

		/* enable embedded output */
		{
			blue_card_property prop;
			prop.video_channel = BLUE_VIDEO_OUTPUT_CHANNEL_A;
			prop.prop = EMBEDEDDED_AUDIO_OUTPUT;
			prop.value.vt = VT_UI4;
			prop.value.ulVal = 
				(audio_output_embed)
				?
				(blue_auto_aes_to_emb_audio_encoder | blue_emb_audio_enable | blue_emb_audio_group1_enable)
				:
				0;
			printf
			(
				MODULE_PREFIX "Enabling SDI embedded output (err=%d)\n",
				r = blue_set_video_property(bluefish_obj, 1, &prop)
			);
		};
	};

	/* detect buffers */
	r = bluefish[0]->render_buffer_sizeof(buffers_count, buffers_length, buffers_actual, buffers_golden);
};



/* ------------------------------------------------------------------

	Module method

------------------------------------------------------------------ */

VZOUTPUTS_EXPORT long vzOutput_FindBoard(char** error_log = NULL)
{
	return boards_count;
};

VZOUTPUTS_EXPORT void vzOutput_SetConfig(void* config)
{
	_config = config;
};


VZOUTPUTS_EXPORT long vzOutput_SelectBoard(unsigned long id,char** error_log = NULL)
{
	return id;
};

VZOUTPUTS_EXPORT long vzOutput_InitBoard(void* tv)
{
	// assign tv value 
	_tv = (vzTVSpec*)tv;

#ifdef DUPL_LINES_ARCH
	/* detect SSE2 support */
	DUPL_LINES_ARCH_SSE2_DETECT;
	/* report */
	printf(MODULE_PREFIX "%s detected\n", (sse_supported)?"SSE2":"NO SEE2");
#endif /* DUPL_LINES_ARCH */

	/* configure */
	bluefish_configure();

	return 1;
};

VZOUTPUTS_EXPORT void vzOutput_StartOutputLoop(void* tbc)
{
	flag_exit = 0;

	// start thread
	main_io_loop_thread = CreateThread
	(
		NULL,
		1024,
		main_io_loop,
		tbc, //&_thread_data, // params
		0,
		NULL
	);

	/* set thread priority */
	SetThreadPriority(main_io_loop_thread , THREAD_PRIORITY_HIGHEST);
};


VZOUTPUTS_EXPORT void vzOutput_StopOutputLoop()
{
	// notify to stop thread
	flag_exit = 1;

	/* wait thread exits */
	if (INVALID_HANDLE_VALUE != main_io_loop_thread)
	{
		WaitForSingleObject(main_io_loop_thread, INFINITE);
		CloseHandle(main_io_loop_thread);
		main_io_loop_thread = INVALID_HANDLE_VALUE;
	};
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
	b->output.gold = buffers_golden;

	b->output.audio_buf_size = 
		2 /* 16 bits */ * 
		2 /* 2 channels */ * 
		VZOUTPUT_AUDIO_SAMPLES;

	/* inputs count conf */
	

	if(vzConfigParam(_config,"nullvideo","INPUTS_COUNT"))
	{
		b->input.audio_buf_size = 
			2 /* 16 bits */ * 
			2 /* 2 channels */ * 
			VZOUTPUT_AUDIO_SAMPLES;
		b->input.channels = inputs_count;
		b->input.field_mode = CONFIG_O(O_SOFT_FIELD_MODE)?1:0;
		if(b->input.field_mode)
			b->input.twice_fields = CONFIG_O(O_SOFT_TWICE_FIELDS)?1:0;
		else
			b->input.twice_fields = 0;

		int k = ((b->input.field_mode) && (!(b->input.twice_fields)))?2:1;

		b->input.offset = 0;
		b->input.size = 4*_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT / k;
		b->input.gold = buffers_golden;

		b->input.width = _tv->TV_FRAME_WIDTH;
		b->input.height = _tv->TV_FRAME_HEIGHT / k;
	};
};

VZOUTPUTS_EXPORT void vzOutput_AssignBuffers(struct vzOutputBuffers* b)
{
	_buffers_info = b;
};




/* ------------------------------------------------------------------

	DLL Initialization 

------------------------------------------------------------------ */

#pragma comment(lib, "winmm")

#ifdef _DEBUG
#pragma comment(lib, "BlueVelvet3_d.lib") 
#else /* !_DEBUG */
#pragma comment(lib, "BlueVelvet3.lib") 
#endif /* _DEBUG */


BOOL APIENTRY DllMain
(
	HANDLE hModule, 
    DWORD  ul_reason_for_call, 
    LPVOID lpReserved
)
{
	int i, j;

    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:

			timeBeginPeriod(1);

			/* clean vars */
			memset(input_mapped_buffers, 0, sizeof(input_mapped_buffers));
			memset(input_hanc_buffers, 0, sizeof(input_hanc_buffers));

			/* init HANC buffers */
			for(i = 0; i < MAX_INPUTS; i++)
				for(j = 0; j< 2; j++)
				{
					input_hanc_buffers[i][j] = VirtualAlloc
					(
						NULL, 
						MAX_HANC_BUFFER_SIZE,
						MEM_COMMIT, 
						PAGE_READWRITE
					);
					VirtualLock(input_hanc_buffers[i][j], MAX_HANC_BUFFER_SIZE);
				};

			/* init board */
			if(0 == bluefish_init(device_id))
			{
				bluefish_destroy();
				exit(-1);
			};

			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:

			timeEndPeriod(1);
			
			/* wait all finished */
			fprintf(stderr, MODULE_PREFIX "waiting loop end\n");
			flag_exit = 1;
			if (INVALID_HANDLE_VALUE != main_io_loop_thread)
			{
				WaitForSingleObject(main_io_loop_thread, INFINITE);
				CloseHandle(main_io_loop_thread);
				main_io_loop_thread = INVALID_HANDLE_VALUE;
			};

			/* trying to stop all devices */
		    if(bluefish[0])
				bluefish[0]->video_playback_stop(false, false);
			if(bluefish[1])
				bluefish[1]->video_capture_stop();
			if(bluefish[2])
				bluefish[2]->video_capture_stop();

			/* deinit */
			fprintf(stderr, MODULE_PREFIX "force bluefish deinit\n");
			bluefish_destroy();

			/* free HANC buffers */
			for(i = 0; i < MAX_INPUTS; i++)
				for(j = 0; j< 2; j++)
				{
					VirtualUnlock(input_hanc_buffers[i][j], MAX_HANC_BUFFER_SIZE);
					VirtualFree(input_hanc_buffers[i][j], MAX_HANC_BUFFER_SIZE, MEM_RELEASE);
				};


			break;
    }
    return TRUE;
}

