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
*/
#define _CRT_SECURE_NO_WARNINGS
//#define DEBUG_TIMINGS

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <math.h>
#include <errno.h>

#include "vzOutput.h"
#include "vzOutputContext.h"
#include "vzGlExt.h"
#include "vzLogger.h"

static char* find_ch(char* tokens, long ch)
{
    for(; *tokens; tokens++)
        if(*tokens == ch) return tokens;
    return 0;
};

static int split_str(char* str, char* splitter, char*** s)
{
    int c = 0;
    char *t1, *t2;

    /* check if string defined */
    if(!str || !splitter)
    {
        *s = NULL;
        return 0;
    };

    /* allocate mem for struct */
    *s = (char**)malloc(0);

    for(t2 = str; 1 ; )
    {
        /* find non terminal character - skip */
        for(t1 = t2; (*t1)&&(NULL != find_ch(splitter, *t1)) ; t1++);

        /* check if and found */
        if(0 == (*t1)) break;

        /* find next ternimals */
        for(t2 = t1; (*t2)&&(NULL == find_ch(splitter, *t2)) ; t2++);

        /* check if token found */
        if(t1 != t2)
        {
            /* reallocate space for array */
            *s = (char**)realloc(*s, sizeof(char**) * (c + 1));

            /* allocate space for string part */
            (*s)[c] = (char*)malloc(1 + (t2 - t1));

            /* copy string */
            memcpy((*s)[c], t1, (t2 - t1));
            (*s)[c][t2 - t1] = 0;

            /* increment counter */
            c++;

            /* shift */
            if(*t2) t2++;
        };
    };

    /* setup last node in chain */
    *s = (char**)realloc(*s, sizeof(char**) * (c + 1));
    (*s)[c] = NULL;

    return c;
};

static void split_str_free(char*** s)
{
    char** strs = *s;
    if(strs)
    {
        for(; *strs; strs++) free(*strs);
        free(*s);
    };
    *s = NULL;
};


/** initialize output engine and load/attach listed modules */
VZOUTPUT_API int vzOutputNew(void** obj, void* config, char* names, vzTVSpec* tv)
{
    int c, i;
    char** names_list;

    if(!obj) return -EINVAL;
    *obj = NULL;

    /* allocate runtime */
    vzOutputContext_t* ctx = (vzOutputContext_t*)malloc(sizeof(vzOutputContext_t));
    if(!ctx) return -ENOMEM;

    /* basic setup */
    memset(ctx, 0, sizeof(vzOutputContext_t));
    ctx->config = config;
    ctx->tv = tv;

    /* prepare modules name list */
    c = split_str(names, ";,", &names_list);
    if(c)
    {
        for(i = 0; i < c && ctx->count < MAX_MODULES; i++)
        {
            HMODULE lib;
            vzOutputModule_t* module;
            char filename[1024];

            sprintf(filename,"outputs/%s.dll", names_list[i]);
            logger_printf(0, "vzOutput: probing module '%s' from file '%s'",
                names_list[i], filename);

            /* try to load module */
            lib = LoadLibrary(filename);
            if(!lib)
            {
                logger_printf(1, "vzOutput: LoadLibrary(%s) failed", filename);
                continue;
            };

            /* find procs */
            module = (vzOutputModule_t*)GetProcAddress(lib, "vzOutputModule");
            if(!module)
            {
                FreeLibrary(lib);
                logger_printf(1, "vzOutput: GetProcAddress(%s, 'vzOutputModule') failed", filename);
                continue;
            };

            /* setup */
            ctx->modules[ctx->count].lib = lib;
            ctx->modules[ctx->count].run = module;
            ctx->count++;
        };
    };

    /* free modules names */
    if(names_list)
        split_str_free(&names_list);

    /* setup context */
    *obj = ctx;

    return 0;
};

/** release output engine */
VZOUTPUT_API int vzOutputFree(void** obj)
{
    int i;

    if(!obj) return -EINVAL;

    vzOutputContext_t* ctx = (vzOutputContext_t*)(*obj);
    *obj = NULL;

    if(!ctx) return -EINVAL;

    for(i = 0; i < ctx->count; i++)
        FreeLibrary(ctx->modules[i].lib);

    free(ctx);

    return 0;
};

static unsigned long WINAPI internal_sync_generator(void* obj)
{
    unsigned int d;
    vzImage img;
    vzOutputContext_t* ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    d = 1000 * ctx->tv->TV_FRAME_PS_DEN / ctx->tv->TV_FRAME_PS_NOM;

    while(!ctx->f_exit)
    {
        Sleep(d);

        /* request frame */
        vzOutputOutGet(ctx, &img);

        /* release frame back */
        vzOutputOutRel(ctx, &img);

        // increment frame counter
        *ctx->syncer.cnt = 1 + *ctx->syncer.cnt;

        // rise evennt
        PulseEvent(ctx->syncer.ev);
    };

    return 0;
};


/**  */
VZOUTPUT_API int vzOutputInit(void* obj, HANDLE sync_event, unsigned long* sync_cnt)
{
    int i, b;
    vzOutputContext_t* ctx;

    ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    /* initialize modules */
    for(i = 0; i < ctx->count; i++)
        ctx->modules[i].run->init(&ctx->modules[i].ctx, ctx, ctx->config, ctx->tv);

    /* initialize buffers */
    ctx->output.gold = ctx->tv->TV_FRAME_WIDTH * ctx->tv->TV_FRAME_HEIGHT * 4;
    ctx->output.size = ctx->output.gold;

    ctx->output.lock = CreateMutex(NULL, FALSE, NULL);
    for(b = 0; b < VZOUTPUT_MAX_BUFS; b++)
    {
        /* generate offscreen buffer */
        glErrorLog(glGenBuffers(1, &ctx->output.buffers[b].num));

        /* INIT BUFFER SIZE */
        glErrorLog(glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, ctx->output.buffers[b].num));
        glErrorLog(glBufferData(GL_PIXEL_PACK_BUFFER_ARB, ctx->output.gold, NULL, GL_DYNAMIC_READ));

        /* REQUEST BUFFER PTR - MAP AND SAVE PTR */
        glErrorLog(ctx->output.buffers[b].data = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB,GL_READ_ONLY));

        // UNMAP
//        glErrorLog(glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB));

        // unbind ?
        glErrorLog(glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0));
    };

    /* setup output buffer ids */
    ctx->output.cnt_render = 0;
    ctx->output.pos_driver = VZOUTPUT_MAX_BUFS - 1;
    ctx->output.pos_render = 0;

    /* setup modules */
    for(i = 0; i < ctx->count; i++)
        ctx->modules[i].run->setup(ctx->modules[i].ctx, &sync_event, &sync_cnt);

    /* check if we need to drive own sync source */
    if(sync_cnt)
    {
        logger_printf(0, "vzOutput: none of modules provides sync generator. running internal.");
        ctx->syncer.cnt = sync_cnt;
        ctx->syncer.ev = sync_event;
        ctx->syncer.th = CreateThread(NULL, 1024, internal_sync_generator, ctx, 0, NULL);
        SetThreadPriority(ctx->syncer.th, THREAD_PRIORITY_HIGHEST);
    };

    return 0;
};

VZOUTPUT_API int vzOutputRelease(void* obj)
{
    int i, b;
    vzOutputContext_t* ctx;

    ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    /* rise exit flag */
    ctx->f_exit = 1;

    /* stop thread */
    if(ctx->syncer.th)
    {
        WaitForSingleObject(ctx->syncer.th, INFINITE);
        CloseHandle(ctx->syncer.th);
    };

    /* release modules modules */
    for(i = 0; i < ctx->count; i++)
        ctx->modules[i].run->release(&ctx->modules[i].ctx, ctx, ctx->config, ctx->tv);

    /* free buffers */
    CloseHandle(ctx->output.lock);
    for(b = 0; b < VZOUTPUT_MAX_BUFS; b++)
        // free framebuffer
        glErrorLog(glDeleteBuffers(1, &ctx->output.buffers[b].num));

    /* free input queue locks */
    for(i = 0; i < ctx->inputs_count; i++)
        CloseHandle(ctx->inputs_data[i].lock);

    return 0;
};


static int probe_pos_driver_jump(vzOutputContext_t* ctx, int force)
{
    /* lock buffers head */
    WaitForSingleObject(ctx->output.lock, INFINITE);

    /* check if we can use next buffer */
    if(ctx->output.pos_driver_jump)
    {
        /* check if next buffer is loaded */
        if(((ctx->output.pos_driver + 1) % VZOUTPUT_MAX_BUFS) != ctx->output.pos_render)
        {
            /* increment position */
            ctx->output.pos_driver = (ctx->output.pos_driver + 1) % VZOUTPUT_MAX_BUFS;

            /* check drop count */
            if(ctx->output.pos_driver_dropped)
            {
                logger_printf(1, "vzOutput: dropped %d frames[driver]",
                    ctx->output.pos_driver_dropped);
                ctx->output.pos_driver_dropped = 0;
            };

            /* no need to jump */
            ctx->output.pos_driver_jump = 0;
        }
        else
        {
            /* increment dropped framescounter */
            if(force)
                ctx->output.pos_driver_dropped++;
        };

        /* clear flag - no more chances */
        if(force)
            ctx->output.pos_driver_jump = 0;
    };

    /* unlock buffers head */
    ReleaseMutex(ctx->output.lock);

    return 0;
};

static int probe_pos_render_jump(vzOutputContext_t* ctx, int force)
{
    /* lock buffers head */
    WaitForSingleObject(ctx->output.lock, INFINITE);

    /* check if we can use next buffer */
    if(ctx->output.pos_render_jump)
    {
        /* check if next buffer is loaded */
        if(((ctx->output.pos_render + 1) % VZOUTPUT_MAX_BUFS) != ctx->output.pos_driver)
        {
            /* increment position */
            ctx->output.pos_render = (ctx->output.pos_render + 1) % VZOUTPUT_MAX_BUFS;

            /* check drop count */
            if(ctx->output.pos_render_dropped)
            {
                logger_printf(1, "vzOutput: dropped %d frames[render]",
                    ctx->output.pos_render_dropped);
                ctx->output.pos_render_dropped = 0;
            };

            /* no need to jump */
            ctx->output.pos_render_jump = 0;
        }
        else
        {
            /* increment dropped framescounter */
            if(force)
                ctx->output.pos_render_dropped++;
        };

        /* clear flag - no more chances */
        if(force)
            ctx->output.pos_render_jump = 0;
    };

    /* unlock buffers head */
    ReleaseMutex(ctx->output.lock);

    return 0;
};

/** prepare OpenGL buffers download */
VZOUTPUT_API int vzOutputPostRender(void* obj)
{
    vzOutputContext_t* ctx;

#ifdef DEBUG_TIMINGS
    logger_printf(1, "vzOutput: vzOutputPostRender enter");
#endif

    ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    /* wait async transfer */
    glErrorLogD(glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB,
        ctx->output.buffers[ctx->output.pos_render].num));

    /* wait async transfer */
    ctx->output.buffers[ctx->output.pos_render].data =
        glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);

    /* set jump flag */
    ctx->output.pos_render_jump = 1;

    /* probe jump to next buf */
    probe_pos_render_jump(ctx, 0);

#ifdef DEBUG_TIMINGS
    logger_printf(1, "vzOutput: vzOutputPostRender exit");
#endif

    return 0;
};

/** finish OpenGL buffers download */
VZOUTPUT_API int vzOutputPreRender(void* obj)
{
    vzOutputContext_t* ctx;

#ifdef DEBUG_TIMINGS
    logger_printf(1, "vzOutput: vzOutputPreRender enter");
#endif

    ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    /* choose next render buffer in force mode */
    probe_pos_render_jump(ctx, 1);

    /* bind to buffer */
    glErrorLogD(glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB,
        ctx->output.buffers[ctx->output.pos_render].num));

    /* unmap it */
    glErrorLogD(glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB));
    ctx->output.buffers[ctx->output.pos_render].data = NULL;

    // start asynchroniosly read from buffer
    glErrorLogD(glReadPixels
    (
        0,
        0,
        ctx->tv->TV_FRAME_WIDTH,
        ctx->tv->TV_FRAME_HEIGHT,
        GL_BGRA_EXT,
        GL_UNSIGNED_BYTE,
        BUFFER_OFFSET(ctx->output.offset)
    ));

    glErrorLogD(glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0));


#ifdef DEBUG_TIMINGS
    logger_printf(1, "vzOutput: vzOutputPreRender exit %d",
        ctx->output.pos_render);
#endif

    return 0;
};

/** run output */
VZOUTPUT_API int vzOutputRun(void* obj)
{
    int i;
    vzOutputContext_t* ctx;

    ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    for(i = 0; i < ctx->count; i++)
        ctx->modules[i].run->run(ctx->modules[i].ctx);

    return 0;
};

/** release output */
VZOUTPUT_API int vzOutputStop(void* obj)
{
    int i;
    vzOutputContext_t* ctx;

    ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    for(i = 0; i < ctx->count; i++)
        ctx->modules[i].run->stop(ctx->modules[i].ctx);

    return 0;
};

/** retrieve output image */
VZOUTPUT_API int vzOutputOutGet(void* obj, vzImage* img)
{
    vzOutputContext_t* ctx;

#ifdef DEBUG_TIMINGS
    logger_printf(1, "vzOutput: vzOutputOutGet enter");
#endif

    ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    probe_pos_driver_jump(ctx, 1);

    /* setup image */
    img->bpp = 4;
    img->pix_fmt = VZIMAGE_PIXFMT_BGRA;
    img->base_x = img->base_y = 0;
    img->base_width = img->width = ctx->tv->TV_FRAME_WIDTH;
    img->base_height = img->height = ctx->tv->TV_FRAME_HEIGHT;
    img->surface = ctx->output.buffers[ctx->output.pos_driver].data;
    img->line_size = img->width * img->bpp;

#ifdef DEBUG_TIMINGS
    logger_printf(1, "vzOutput: vzOutputOutGet exit %d",
        ctx->output.pos_driver);
#endif

    return 0;
};

/** release ouput image */
VZOUTPUT_API int vzOutputOutRel(void* obj, vzImage* img)
{
    vzOutputContext_t* ctx;

#ifdef DEBUG_TIMINGS
    logger_printf(1, "vzOutput: vzOutputOutRel enter");
#endif

    ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    /* set jump flag */
    ctx->output.pos_driver_jump = 1;

    probe_pos_driver_jump(ctx, 0);

#ifdef DEBUG_TIMINGS
    logger_printf(1, "vzOutput: vzOutputOutRel exit");
#endif

    return 0;
};

VZOUTPUT_API int vzOutputRenderSlots(void* obj)
{
    int r = 0;
    vzOutputContext_t* ctx;

#ifdef DEBUG_TIMINGS
    logger_printf(1, "vzOutput: vzOutputRenderSlots enter");
#endif

    ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    /* lock buffers head */
    WaitForSingleObject(ctx->output.lock, INFINITE);

    /* check next buffers */
    if(((ctx->output.pos_render + ctx->output.pos_render_jump)
        % VZOUTPUT_MAX_BUFS) != ctx->output.pos_driver)
        r = 1;

    /* unlock buffers head */
    ReleaseMutex(ctx->output.lock);

#ifdef DEBUG_TIMINGS
    logger_printf(1, "vzOutput: vzOutputRenderSlots exit %d", r);
#endif

    return r;
};

/** create input queue */
VZOUTPUT_API int vzOutputInputAdd(void* obj)
{
    int i;
    vzOutputContext_t* ctx;

    ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    if(ctx->inputs_count == MAX_INPUTS)
        return -ENOMEM;

    ctx->inputs_data[ctx->inputs_count].lock = CreateMutex(NULL, FALSE, NULL);
    i = ctx->inputs_count;
    logger_printf(0, "vzOutput: input #%d added", i);
    ctx->inputs_count++;

    return i;
};

/** push image into queue, older image will be set into img / IMAGE SOURCER */
VZOUTPUT_API int vzOutputInputPush(void* obj, int idx, void** img)
{
    void* tmp;
    vzOutputContext_t* ctx;

    ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    if(idx < 0 || idx >= ctx->inputs_count)
        return -ENOENT;

    WaitForSingleObject(ctx->inputs_data[idx].lock, INFINITE);

    if(ctx->inputs_data[idx].back)
    {
        tmp = ctx->inputs_data[idx].back;
        ctx->inputs_data[idx].back = ctx->inputs_data[idx].front;
        ctx->inputs_data[idx].front = *img;
    }
    else
    {
        tmp = ctx->inputs_data[idx].front;
        ctx->inputs_data[idx].front = *img;
    };
    *img = tmp;

    ReleaseMutex(ctx->inputs_data[idx].lock);

    return 0;
};

/** IMAGE DESTINATOR */
VZOUTPUT_API int vzOutputInputPull(void* obj, int idx, void** img)
{
    vzOutputContext_t* ctx;

    ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    if(idx < 0 || idx >= ctx->inputs_count)
        return -ENOENT;

    WaitForSingleObject(ctx->inputs_data[idx].lock, INFINITE);

    *img = ctx->inputs_data[idx].front;
    ctx->inputs_data[idx].front = ctx->inputs_data[idx].back;
    ctx->inputs_data[idx].back = NULL;

    ReleaseMutex(ctx->inputs_data[idx].lock);

    return 0;
};

VZOUTPUT_API int vzOutputInputPullBack(void* obj, int idx, void** img)
{
    vzOutputContext_t* ctx;

    ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    if(idx < 0 || idx >= ctx->inputs_count)
        return -ENOENT;

    WaitForSingleObject(ctx->inputs_data[idx].lock, INFINITE);

    if(ctx->inputs_data[idx].back)
    {
        logger_printf(1, "vzOutput: vzOutputInputPullBack(%d) OVERRUN!", idx);
    }
    else
    {
        ctx->inputs_data[idx].back = ctx->inputs_data[idx].front;
        ctx->inputs_data[idx].front = *img;
    };

    ReleaseMutex(ctx->inputs_data[idx].lock);

    return 0;
};

static vzOutputContext_t* global_ctx = NULL;

VZOUTPUT_API void* vzOutputGlobalContextGet()
{
    return global_ctx;
};

VZOUTPUT_API void* vzOutputGlobalContextSet(void* obj)
{
    return (global_ctx = (vzOutputContext_t*)obj);
};

/*
    could be usefull for further reading:

    http://www.nvidia.com/dev_content/nvopenglspecs/GL_EXT_pixel_buffer_object.txt
    http://www.gpgpu.org/forums/viewtopic.php?t=2001&sid=4cb981dff7d9aa406cb721f7bd1072b6
    http://www.elitesecurity.org/t140671-pixel-buffer-object-PBO-extension
    http://www.gamedev.net/community/forums/topic.asp?topic_id=329957&whichpage=1&#2143668
    http://www.gamedev.net/community/forums/topic.asp?topic_id=360729
*/
