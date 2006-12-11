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
//#define USE_KERNEL_BUFFERS

#define dump_line {};
#ifndef dump_line
#define dump_line fprintf(stderr, __FILE__ ":%d\n", __LINE__); fflush(stderr);
#endif

/* ------------------------------------------------------------------

	Module definitions 

------------------------------------------------------------------ */
#define MODULE_PREFIX "bluefish: "
#define CONFIG_O(VAR) vzConfigParam(_config, "bluefish", VAR)

#define O_KEY_INVERT		"KEY_INVERT"
#define O_KEY_WHITE			"KEY_WHITE"
#define O_SINGLE_INPUT		"SINGLE_INPUT"
#define O_DUAL_INPUT		"DUAL_INPUT"
#define O_VIDEO_MODE		"VIDEO_MODE"
#define O_PAL				"PAL"
#define O_ONBOARD_KEYER		"ONBOARD_KEYER"
#define O_H_PHASE_OFFSET	"H_PHASE_OFFSET"
#define O_V_PHASE_OFFSET	"V_PHAZE_OFFSET"
#define O_VERTICAL_FLIP		"VERTICAL_FLIP"
#define O_SCALED_RGB		"SCALED_RGB"
#define O_SOFT_FIELD_MODE	"SOFT_FIELD_MODE"
#define O_SOFT_TWICE_FIELDS	"SOFT_TWICE_FIELDS"


/* ------------------------------------------------------------------

	Module variables 

------------------------------------------------------------------ */
static int flag_exit = 0;
static CBlueVelvet4* bluefish[3] = {NULL, NULL, NULL};
static void* bluefish_obj = NULL;
static unsigned char* input_mapped_buffers[2] = {NULL, NULL};
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

static unsigned long WINAPI io_out(void* buffer)
{
	unsigned long address, buffer_id, underrun, next_buf_id;


	bluefish[0]->video_playback_allocate((void **)&address,buffer_id,underrun);

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
	};

	return 0;
};

struct io_in_desc
{
	int id;
	void** buffers;
};

static unsigned long WINAPI io_in(void* p)
{
	struct io_in_desc* desc = (struct io_in_desc*)p;
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
			/* read from host mem to mapped memmory */
			void* buf = input_mapped_buffers[desc->id - 1];

#ifdef DIRECT_FRAME_COPY
			/* if no transoramtion required - directly */
			if(!(_buffers_info->input.field_mode))
				buf = desc->buffers[0];
#endif /* DIRECT_FRAME_COPY */

			/* read mem */
			bluefish[desc->id]->system_buffer_read_async
			(
				(unsigned char *)buf,
				buffers_golden,
				NULL,
				buffer_id
			);
			dump_line;

			/* check dropped frames */
/*			if(dropped_count)
			{
				fprintf(stderr, MODULE_PREFIX "dropped %d frames at [%d]\n", dropped_count, desc->id);
			}; */

//----------------

			/* defferent methods for fields and frame mode */
			if(_buffers_info->input.field_mode)
			{
				dump_line;

				/* field mode */
				long l = _tv->TV_FRAME_WIDTH*4;
				unsigned char
					*src = (unsigned char*)input_mapped_buffers[desc->id - 1],
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
			}
			else
			{
#ifndef DIRECT_FRAME_COPY
				dump_line;
				/* frame mode */
				memcpy
				(
					desc->buffers[0],
					input_mapped_buffers[desc->id - 1],
					_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT*4
				);
				dump_line;
#endif /* !DIRECT_FRAME_COPY */
			};

//----------------
			dump_line;
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

	void *output_buffer, **input_buffers;
	struct io_in_desc in1;
	struct io_in_desc in2;

	unsigned long f1;
	HANDLE io_ops[3] = {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE};
	unsigned long io_ops_id[3];

	/* map buffers for input */
	if(inputs_count)
	{
#ifdef USE_KERNEL_BUFFERS
		bluefish[1]->system_buffer_map((void**)&input_mapped_buffers[0], BUFFER_ID_VIDEO0);
#else /* USE_KERNEL_BUFFERS */
		input_mapped_buffers[0] = (unsigned char*)VirtualAlloc
		(
			NULL, 
			buffers_golden,
			MEM_COMMIT, 
			PAGE_READWRITE
		);
		VirtualLock(input_mapped_buffers[0], buffers_golden);
#endif /* USE_KERNEL_BUFFERS */

		if(inputs_count > 1)
		{
#ifdef USE_KERNEL_BUFFERS
			bluefish[2]->system_buffer_map((void**)&input_mapped_buffers[1], BUFFER_ID_VIDEO1);
#else /* USE_KERNEL_BUFFERS */
			input_mapped_buffers[1] = (unsigned char*)VirtualAlloc
			(
				NULL, 
				buffers_golden,
				MEM_COMMIT, 
				PAGE_READWRITE
			);
			VirtualLock(input_mapped_buffers[1], buffers_golden);
#endif /* USE_KERNEL_BUFFERS */
		};
	};
	fprintf(stdout, MODULE_PREFIX "input buffers: 0x%.8X, 0x%.8X\n", (unsigned long)input_mapped_buffers[0], (unsigned long)input_mapped_buffers[1]);
	
	/* check if mapped buffer is OK */
	if
	(
		(
			(inputs_count)
			&&
			(NULL == input_mapped_buffers[0])
		)
		||
		(
			(inputs_count > 1)
			&&
			(NULL == input_mapped_buffers[1])
		)
	)
	{
		fprintf(stderr, MODULE_PREFIX "ERROR!! CRITICAL!!! RESTART PC - Buffers are NULL\n");
		exit(-1);
	};


	/* start playback/capture */
    bluefish[0]->video_playback_start(false, false);
	if(inputs_count)
		bluefish[1]->video_capture_start();
	if(inputs_count > 1)
		bluefish[2]->video_capture_start();

	/* skip 2 cyrcles */
	bluefish[0]->wait_output_video_synch(update_format, f1);
	bluefish[0]->wait_output_video_synch(update_format, f1);

	/* endless loop */
	while(!(flag_exit))
	{
		dump_line;

		/* wait output vertical sync */
		bluefish[0]->wait_output_video_synch(update_format, f1);
		
		dump_line;

		/* request pointers to buffers */
		tbc->lock_io_bufs(&output_buffer, &input_buffers);

		dump_line;

		/* send sync */
		if(_fc)
			_fc();

		/* start output thread */
		io_ops[0] = CreateThread(0, 0, io_out,  output_buffer , 0, &io_ops_id[0]);

		/* start inputs */
		if(inputs_count)
		{
			in1.id = 1;
			in1.buffers = &input_buffers[0];
			io_ops[1] = CreateThread(0, 0, io_in,  &in1 , 0, &io_ops_id[1]);

			if(inputs_count > 1)
			{
				in2.id = 2;
				in2.buffers = &input_buffers[ (_buffers_info->input.field_mode)?2:1 ];
				io_ops[2] = CreateThread(0, 0, io_in,  &in2 , 0, &io_ops_id[2]);
			};
		};

		/* wait for thread ends */
		WaitForSingleObject(io_ops[0], INFINITE);
		CloseHandle(io_ops[0]);
		if(inputs_count)
		{
			WaitForSingleObject(io_ops[1], INFINITE);
			CloseHandle(io_ops[1]);
			if(inputs_count>1)
			{
				WaitForSingleObject(io_ops[2], INFINITE);
				CloseHandle(io_ops[2]);
			};
		};
		dump_line;

		/* unlock buffers */
		tbc->unlock_io_bufs(&output_buffer, &input_buffers);
		dump_line;
	};

	/* stop playback/capture */
    bluefish[0]->video_playback_stop(false, false);
	if(inputs_count)
	{
		bluefish[1]->video_capture_stop();
		if(inputs_count > 1)
			bluefish[2]->video_capture_stop();
	};


	/* unmap drivers buffer */
	if(inputs_count)
	{
#ifdef USE_KERNEL_BUFFERS
		bluefish[1]->system_buffer_unmap(input_mapped_buffers[0]);
#else /* USE_KERNEL_BUFFERS */
		VirtualUnlock(input_mapped_buffers[0], buffers_golden);
		VirtualFree(input_mapped_buffers[0], buffers_golden, MEM_RELEASE);
#endif /* USE_KERNEL_BUFFERS */

		if(inputs_count > 1)
		{
#ifdef USE_KERNEL_BUFFERS
			bluefish[2]->system_buffer_unmap(input_mapped_buffers[1]);
#else /* USE_KERNEL_BUFFERS */
			VirtualUnlock(input_mapped_buffers[1], buffers_golden);
			VirtualFree(input_mapped_buffers[1], buffers_golden, MEM_RELEASE);
#endif /* USE_KERNEL_BUFFERS */
		};

	};

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
	bluefish_obj = blue_attach_to_device(device_id);

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


		/* inputss */
		v.ulVal = BLUE_VIDEO_INPUT_CHANNEL_A;
		r = bluefish[0]->SetCardProperty(DEFAULT_VIDEO_INPUT_CHANNEL, v);

		v.ulVal = BLUE_VIDEO_INPUT_CHANNEL_A;
		r = bluefish[1]->SetCardProperty(DEFAULT_VIDEO_INPUT_CHANNEL, v);

		v.ulVal = BLUE_VIDEO_INPUT_CHANNEL_B;
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


	/* detect buffers */
	r = bluefish[0]->render_buffer_sizeof(buffers_count, buffers_length, buffers_actual, buffers_golden);
};



/* ------------------------------------------------------------------

	Module method

------------------------------------------------------------------ */

VZOUTPUTS_EXPORT long vzOutput_FindBoard(char** error_log = NULL)
{
	int c, r;
	CBlueVelvet4* b = BlueVelvetFactory4();
	r = b->device_enumerate(c);
	delete b;
	return c;
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

	/* init board */
	if(0 == bluefish_init(device_id))
	{
		bluefish_destroy();
		exit(-1);
	};

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

	/* inputs count conf */
	

	if(vzConfigParam(_config,"nullvideo","INPUTS_COUNT"))
	{

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

*/

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
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
	
			
			/* wait all finished */
			fprintf(stderr, MODULE_PREFIX "waiting loop end\n");
			flag_exit = 1;
			if (INVALID_HANDLE_VALUE != main_io_loop_thread)
			{
				WaitForSingleObject(main_io_loop_thread, INFINITE);
				CloseHandle(main_io_loop_thread);
				main_io_loop_thread = INVALID_HANDLE_VALUE;
			};

			/* deinit */
			fprintf(stderr, MODULE_PREFIX "force bluefish deinit\n");
			bluefish_destroy();

			break;
    }
    return TRUE;
}