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
#define TP_COUNT 6
#include "nullvideo.tp_bars.h"
#include "nullvideo.tp_grid.h"
#include "nullvideo.tp_lines.h"
#include "nullvideo.tp_0249.h"
#include "nullvideo.tp_PM5544.h"
#include "nullvideo.tp_UEIT.h"

#define MAX_INPUTS 4

typedef struct nullvideo_input_context_desc
{
    int index;
    int pattern;
    void* parent;
    vzImage* img[3];
    long long cnt;
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
    vzImage* tp[TP_COUNT];
} nullvideo_runtime_context_t;

static nullvideo_runtime_context_t ctx;

static int nullvideo_init(void* obj, void* config, vzTVSpec* tv)
{
    int i;

    /* clear context */
    memset(&ctx, 0, sizeof(nullvideo_runtime_context_t));

    /* setup basic components */
    ctx.config = config;
    ctx.tv = tv;
    ctx.output_context = obj;
    ctx.sync.src = CreateEvent(NULL, TRUE, FALSE, NULL);
    hr_sleep_init(&ctx.timer_data);

    /* setup test patterns */
    for(i = 0; i < TP_COUNT; i++)
    {
        vzImage img;

        switch(i)
        {
            case 0: _load_img_tp_UEIT(&img); break;
            case 1: _load_img_tp_PM5544(&img); break;
            case 2: _load_img_tp_0249(&img); break;
            case 3: _load_img_tp_bars(&img); break;
            case 4: _load_img_tp_grid(&img); break;
            case 5: _load_img_tp_lines(&img); break;
        };

        vzImageCreate(&ctx.tp[i], img.width, img.height, img.pix_fmt);
        memcpy(ctx.tp[i]->surface, img.surface, img.height * img.line_size);
    };

    return 0;
};

static int nullvideo_release(void* obj, void* config, vzTVSpec* tv)
{
    int i;

    hr_sleep_destroy(&ctx.timer_data);
    CloseHandle(ctx.sync.src);

    for(i = 0; i < TP_COUNT; i++)
        vzImageRelease(&ctx.tp[i]);

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
    int i = 0, c = 0;
    vzImage* img = NULL;
    nullvideo_input_context_t* inp = (nullvideo_input_context_t*)obj;
    nullvideo_runtime_context_t* ctx = (nullvideo_runtime_context_t*)inp->parent;

    /* setup framebuffers */
    for(i = 0; i < 3; i++)
        inp->img[i] = ctx->tp[inp->pattern];

    /* endless loop */
    while(!ctx->f_exit)
    {
        /* wait for internal sync */
        WaitForSingleObject(ctx->sync.src, INFINITE);

        /* fetch new image */
        if(!img)
            img = inp->img[c++];

        /* inc counter */
        img->sys_id = inp->cnt++;

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
        if(j < 0)
        {
            logger_printf(1, THIS_MODULE_PREF "pattern value %d of parameter %s out of range",
                j, name);
            continue;
        };
        j = j % TP_COUNT;

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
