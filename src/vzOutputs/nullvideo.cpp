/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2011 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1stereo.tv, 2011.

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
*/
#include <windows.h>
#include <stdio.h>

#include "../vz/vzOutput.h"
#include "../vz/vzOutputModule.h"
#include "../vz/vzMain.h"
#include "../vz/vzLogger.h"

#define THIS_MODULE "nullvideo"
#define THIS_MODULE_PREF "nullvideo: "

#include "hr_timer.h"

/* test patters */
#define TP_COUNT 3
#include "nullvideo.tp_bars.h"
#include "nullvideo.tp_grid.h"
#include "nullvideo.tp_lines.h"
static void *tps[TP_COUNT] = { tp_bars_surface, tp_grid_surface, tp_lines_surface};

#define MAX_INPUTS 4

typedef struct nullvideo_input_context_desc
{
    int index;
    int pattern;
    void* parent;
    vzImage img[3];
} nullvideo_input_context_t;

typedef struct nullvideo_runtime_context_desc
{
    int f_exit;
    vzTVSpec* tv;
    void* config;
    void* output_context;
    HANDLE th[2 + MAX_INPUTS];
    struct
    {
        HANDLE src, dst;
        unsigned long* cnt;
    } sync;
    struct hr_sleep_data timer_data;

    nullvideo_input_context_t inputs[MAX_INPUTS];
} nullvideo_runtime_context_t;

static nullvideo_runtime_context_t ctx;

static int nullvideo_init(void* obj, void* config, vzTVSpec* tv)
{
    /* clear context */
    memset(&ctx, 0, sizeof(nullvideo_runtime_context_t));

    /* setup basic components */
    ctx.config = config;
    ctx.tv = tv;
    ctx.output_context = obj;
    ctx.sync.src = CreateEvent(NULL, TRUE, FALSE, NULL);
    hr_sleep_init(&ctx.timer_data);

    return 0;
};

static int nullvideo_release(void* obj, void* config, vzTVSpec* tv)
{
    hr_sleep_destroy(&ctx.timer_data);
    CloseHandle(ctx.sync.src);
    return 0;
};

static int nullvideo_setup(HANDLE* sync_event, unsigned long** sync_cnt)
{
    if(vzConfigParam(ctx.config, THIS_MODULE, "ENABLE_OUTPUT") && sync_event)
    {
        logger_printf(0, THIS_MODULE_PREF "output enabled");

        ctx.sync.dst = *sync_event;
        *sync_event = NULL;

        ctx.sync.cnt = *sync_cnt;
        *sync_cnt = NULL;
    };

    return 0;
};

unsigned long WINAPI nullvideo_thread_syncer(void* obj)
{
    nullvideo_runtime_context_t* ctx = (nullvideo_runtime_context_t*)obj;

    while(!ctx->f_exit)
    {
        hr_sleep(&ctx->timer_data, 1000 * ctx->tv->TV_FRAME_PS_DEN / ctx->tv->TV_FRAME_PS_NOM);
        PulseEvent(ctx->sync.src);
    };
    PulseEvent(ctx->sync.src);

    return 0;
};

unsigned long WINAPI nullvideo_thread_output(void* obj)
{
    vzImage img;
    nullvideo_runtime_context_t* ctx = (nullvideo_runtime_context_t*)obj;

    while(!ctx->f_exit)
    {
        /* wait for internal sync */
        WaitForSingleObject(ctx->sync.src, INFINITE);

        /* request frame */
        vzOutputOutGet(ctx->output_context, &img);

        /* emulate copy or transform */

        /* release frame back */
        vzOutputOutRel(ctx->output_context, &img);

        /* notify main */
        *ctx->sync.cnt = *ctx->sync.cnt + 1;
        PulseEvent(ctx->sync.dst);
    };

    return 0;
};

unsigned long WINAPI nullvideo_thread_input(void* obj)
{
    int i;
    vzImage* img;
    nullvideo_input_context_t* inp = (nullvideo_input_context_t*)obj;
    nullvideo_runtime_context_t* ctx = (nullvideo_runtime_context_t*)inp->parent;

    /* setup framebuffers */
    for(i = 0; i < 3; i++)
    {
        inp->img[i].priv = (void*)i;
        inp->img[i].bpp = 4;
        inp->img[i].pix_fmt = VZIMAGE_PIXFMT_BGRA;
        inp->img[i].surface = tps[inp->pattern];
        inp->img[i].width = inp->img[i].base_width = tp_bars_width;
        inp->img[i].height = inp->img[i].base_height = tp_bars_height;
        inp->img[i].line_size = inp->img[i].width * inp->img[i].bpp;
    };

    /* push first frames */
    img = &inp->img[0];
    vzOutputInputPush(ctx->output_context, inp->index, (void**)&img);
    img = &inp->img[1];
    vzOutputInputPush(ctx->output_context, inp->index, (void**)&img);
    img = &inp->img[2];

    while(!ctx->f_exit)
    {
        /* wait for internal sync */
        WaitForSingleObject(ctx->sync.src, INFINITE);

        /* just push a frame */
        vzOutputInputPush(ctx->output_context, inp->index, (void**)&img);
    };

    return 0;
};


static int nullvideo_run()
{
    int i;

    /* reset exit flag */
    ctx.f_exit = 0;

    /* run syncer thread */
    ctx.th[0] = CreateThread
    (
        NULL,
        1024,
        nullvideo_thread_syncer,
        &ctx,
        0,
        NULL
    );

    /* set thread priority */
    SetThreadPriority(ctx.th[0] , THREAD_PRIORITY_HIGHEST);

    /* run output thread */
    if(ctx.sync.dst)
    {
        ctx.th[1] = CreateThread
        (
            NULL,
            1024,
            nullvideo_thread_output,
            &ctx,
            0,
            NULL
        );
    };

    for(i = 0; i < MAX_INPUTS; i++)
    {
        int j;
        void* c;
        char name[32];

        /* check if parameter specified */
        sprintf(name, "ENABLE_INPUT_%d", i + 1);
        c = vzConfigParam(ctx.config, THIS_MODULE, name);
        if(!c) continue;

        /* check if index in range */
        j = atol((char*)c);
        if(j < 0 || j >= TP_COUNT)
        {
            logger_printf(1, THIS_MODULE_PREF "pattern value %d of parameter %s out of range",
                j, name);
            continue;
        };

        ctx.inputs[i].pattern = j;

        j = vzOutputInputAdd(ctx.output_context);
        if(j < 0)
        {
            logger_printf(1, THIS_MODULE_PREF "vzOutputInputAdd failed with %d", j);
            continue;
        };

        logger_printf(0, THIS_MODULE_PREF "ENABLE_INPUT_%d will be mapped to liveinput %d", i, j);

        ctx.inputs[i].parent = &ctx;
        ctx.inputs[i].index = j;

        ctx.th[2 + i] = CreateThread
        (
            NULL,
            1024,
            nullvideo_thread_input,
            &ctx.inputs[i],
            0,
            NULL
        );
    };

    return 0;
};

static int nullvideo_stop()
{
    int i;

    /* reset exit flag */
    ctx.f_exit = 1;

    /* wait for threads finished */
    for(i = 0; i < (2 + MAX_INPUTS); i++)
        if(ctx.th[i])
        {
            logger_printf(0, THIS_MODULE_PREF "waiting for thread %d to finish...", i);
            WaitForSingleObject(ctx.th[i], INFINITE);
            CloseHandle(ctx.th[i]);
            ctx.th[i] = NULL;
            logger_printf(0, THIS_MODULE_PREF "ok");
        };

    return 0;
};

extern "C" __declspec(dllexport) vzOutputModule_t vzOutputModule =
{
    nullvideo_init,
    nullvideo_release,
    nullvideo_setup,
    nullvideo_run,
    nullvideo_stop
};

#if 0

// threads cotrols


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
	logger_printf(0, "nullvideo");
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

#ifndef _M_X64
static void vzImageBGRA2YUAYVA(void* src, void* dst_yuv, void* dst_alpha, long count)
{
/*	short _Y[] = {29,150,76,0};
	short _U[] = {127,-85,-43,128};
	short _V[] = {-21,-106,127,128};
*/
// test !!!!
//	*((unsigned long*)src) = 0x01020304;
//	*(((unsigned long*)src) + 1) = 0x05060708;

	__asm
	{
//		EMMS
		jmp		begin
ALIGN 16
mask1:
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0xFF
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0xFF
_Y:
// = 
	__asm _emit 0x1D
	__asm _emit 0x00
	__asm _emit 0x96
	__asm _emit 0x00
	__asm _emit 0x4C
	__asm _emit 0x00
	__asm _emit 0x00
	__asm _emit 0x00
_V:
// = 
	__asm _emit 0xEB 
	__asm _emit 0xFF 
	__asm _emit 0x96 
	__asm _emit 0xFF 
	__asm _emit 0x7F 
	__asm _emit 0x00 
	__asm _emit 0x80 
	__asm _emit 0x00
_U:
// = 
	__asm _emit 0x7F 
	__asm _emit 0x00 
	__asm _emit 0xAB 
	__asm _emit 0xFF 
	__asm _emit 0xD5 
	__asm _emit 0xFF 
	__asm _emit 0x80 
	__asm _emit 0x00

begin:
		// loading source surface pointer
		mov		esi, dword ptr[src];

		// loading dst surface pointer
		mov		edi, dword ptr[dst_yuv];

		// loading dst alpha surface pointer
		mov		ebx, dword ptr[dst_alpha];

		// counter
		mov		ecx, dword ptr[count];
		sar		ecx,1	// devide on 2


pixel:
		// prefetch to cache
//		PREFETCHNTA [esi];
//		PREFETCHNTA [edi];

		// load 2 pixels
		movq		MM0, qword ptr[esi]

		// extract alpha
		movq		MM3, MM0
		psrad		MM3, 24
		packsswb	MM3, MM0
		packsswb	MM3, MM0
		movd		eax,MM3
		mov			word ptr [ebx], ax
		add			ebx,2

		// unpacking
		por			MM0, qword ptr [mask1]	// hide alphas
		movq		MM1,MM0		//	move to register for PIXEL 1
		movq		MM2,MM0		//  move to register for PIXEL 2
		pxor		MM0,MM0		//	clear register
		punpcklbw	MM1,MM0		//  unpacking
		punpckhbw	MM2,MM0		//  unpacking

		// loading vectors
		movq		MM3,qword ptr [_Y]
		movq		MM5,qword ptr [_U]
		movq		MM4,MM3
		movq		MM6,qword ptr [_V]

		// multiply
		pmullw		MM3,MM1
		pmullw		MM4,MM2
		pmullw		MM5,MM1
		pmullw		MM6,MM2

// PIXEL #1 (MM0 <- MM3, MM7 <- MM5) -> eax
		movq		MM0,MM3 // duplicate (1)
		movq		MM7,MM5
		psllq		MM0,32	// shift duplicated (1)
		psllq		MM7,32
		paddw		MM3,MM0 // adding (1)
		paddw		MM5,MM7
		movq		MM0,MM3 // duplicate (2)
		movq		MM7,MM5
		psrlq		MM0,16	// shift duplicated (2)
		psrlq		MM7,16
		paddw		MM3,MM0 // adding (2)
		paddw		MM5,MM7
// pack
		psrlw		MM3,8	// shift duplicated (2)
		psrlw		MM5,8	
		PUNPCKHWD	MM5,MM3
		PACKUSWB	MM5,MM5
// save to AX (Y1U1 -> AX)
		movd		eax,MM5
		and			eax,0xFFFF

// PIXEL #2 (MM0 <- MM4, MM7 <- MM6) -> edx
		movq		MM0,MM4 // duplicate (1)
		movq		MM7,MM6
		psllq		MM0,32	// shift duplicated (1)
		psllq		MM7,32
		paddw		MM4,MM0 // adding (1)
		paddw		MM6,MM7
		movq		MM0,MM4 // duplicate (2)
		movq		MM7,MM6
		psrlq		MM0,16	// shift duplicated (2)
		psrlq		MM7,16
		paddw		MM4,MM0 // adding (2)
		paddw		MM6,MM7
// pack
		psrlw		MM4,8	// shift duplicated (2)
		psrlw		MM6,8	
		PUNPCKHWD	MM6,MM4
		PACKUSWB	MM6,MM6
// save to AX (Y1U1 -> DX)
		movd		edx,MM6

// save to memmory
		sal			edx,16
		or			edx,eax
		mov			dword ptr [edi],edx

// increment
		add			esi,8
		add			edi,4
		dec			ecx

		jnz			pixel
		emms
	}

	return;
};
#endif /*  !_M_X64 */

unsigned long WINAPI output_loop(void* obj)
{
	int j, b, i, c, buf_idx;
//	void *output_buffer, **input_buffers, *output_a_buffer, **input_a_buffers;

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
        tbc->lock_io_bufs(&buf_idx);

		/* send sync */
		if(_fc)
			_fc();

		// 4. transcode buffer
#ifndef _M_X64
		if(vzConfigParam(_config,"nullvideo","YUV_CONVERT"))
			if (_buffers_info->output.data)
				vzImageBGRA2YUAYVA
				(
					_buffers_info->output.data,
					yuv_buffer,
					alpha_buffer,
					_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT
				);
#endif /* !_M_X64 */
        /* transfer buffer */
        if(vzConfigParam(_config, "nullvideo", "OUTPUT_BUF_TRANSFER"))
            if (_buffers_info->output.data)
                memcpy
                (
                    fake_buffer,
                    _buffers_info->output.data,
                    _tv->TV_FRAME_WIDTH * _tv->TV_FRAME_HEIGHT*4
                );

        /* transfer input(s) */
        for(c = 0; c < _buffers_info->input_channels; c++)
        {
            /* different methods for fields and frame mode */
            if(_buffers_info->input[c].field_mode)
			{
				/* field mode */
				long l = _tv->TV_FRAME_WIDTH*4;
				unsigned char
					*src = (unsigned char*)tps[channels_tp[c]],
					*dstA = (unsigned char*)_buffers_info->input[c].data[buf_idx][0],
					*dstB = (unsigned char*)_buffers_info->input[c].data[buf_idx][1];

				for(i = 0; i < _tv->TV_FRAME_HEIGHT; i++)
				{
					/* calc case number */
					int d = 
						((i&1)?2:0)
						|
						((_buffers_info->input[c].twice_fields)?1:0);

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
                    _buffers_info->input[c].data[buf_idx][0],
                    tps[channels_tp[c]],
                    _buffers_info->input[c].size
                );
            };
		};


		tbc->notify_frame_stop();

		// mark stop time
		long B = timeGetTime();

		// 3. request pointer to new buffer
		tbc->unlock_io_bufs();

		// mark time to sleep
		long C = B - A;
		if((C>=0) && (C<40))
			hr_sleep(&timer_data, 40 - C);
		else
		{
			logger_printf(0, "nullvideo: %d miliseconds overrun", C - 40);
			Sleep(0);
		};

#ifdef DEBUG_HARD_SYNC
		logger_printf(2, "A=%d, B=%d, C=%d", A, B, C);
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
        b->input_channels = atol(vzConfigParam(_config,"nullvideo","INPUTS_COUNT"));
        for(i = 0; i < b->input_channels; i++)
        {
            b->input[i].field_mode = vzConfigParam(_config,"nullvideo","FIELD_MODE")?1:0;
            if(b->input[i].field_mode)
                b->input[i].twice_fields = vzConfigParam(_config,"nullvideo","TWICE_FIELDS")?1:0;
            else
                b->input[i].twice_fields = 0;

            int k = ((b->input[i].field_mode) && (!(b->input[i].twice_fields)))?2:1;

            b->input[i].offset = 0;
            b->input[i].size = 4*_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT / k;
            b->input[i].gold = ((b->input[i].size + DMA_PAGE_SIZE)/DMA_PAGE_SIZE)*DMA_PAGE_SIZE;

            b->input[i].width = _tv->TV_FRAME_WIDTH;
            b->input[i].height = _tv->TV_FRAME_HEIGHT / k;
        };
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
	logger_printf(0, "nullvideo: %s detected", (sse_supported)?"SSE2":"NO SEE2");
#endif /* DUPL_LINES_ARCH */
};

VZOUTPUTS_EXPORT void vzOutput_AssignBuffers(struct vzOutputBuffers* b)
{

};

#endif
