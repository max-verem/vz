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
            ctx->libs[ctx->count] = lib;
            ctx->modules[ctx->count] = module;
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
        FreeLibrary(ctx->libs[i]);

    free(ctx);

    return 0;
};

static unsigned long WINAPI internal_sync_generator(void* obj)
{
    vzOutputContext_t* ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    while(!ctx->f_exit)
    {
        Sleep((1000 * ctx->tv->TV_FRAME_PS_DEN) / ctx->tv->TV_FRAME_PS_NOM);

        // reset event
        ResetEvent(ctx->syncer.ev);

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
        ctx->modules[i]->init(ctx, ctx->config, ctx->tv);

    /* initialize buffers */
    ctx->output.gold = ctx->tv->TV_FRAME_WIDTH * ctx->tv->TV_FRAME_HEIGHT * 4;
    ctx->output.size = ctx->output.gold;

    ctx->output.lock = CreateMutex(NULL, FALSE, NULL);
    for(b = 0; b < VZOUTPUT_MAX_BUFS; b++)
    {
        /* create a buffer lock */
        ctx->output.buffers[b].lock = CreateMutex(NULL, FALSE, NULL);

        /* generate offscreen buffer */
        glErrorLog(glGenBuffers(1, &ctx->output.buffers[b].num));

        /* INIT BUFFER SIZE */
        glErrorLog(glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, ctx->output.buffers[b].num));
        glErrorLog(glBufferData(GL_PIXEL_PACK_BUFFER_ARB, ctx->output.gold, NULL, GL_STREAM_READ));

        /* REQUEST BUFFER PTR - MAP AND SAVE PTR */
        glErrorLog(ctx->output.buffers[b].data = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB,GL_READ_ONLY));

        // UNMAP
//        glErrorLog(glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB));

        // unbind ?
        glErrorLog(glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0));
    };

    /* setup output buffer ids */
    ctx->output.cnt_render = 0;
    ctx->output.pos_driver = 0;
    ctx->output.pos_render = 1;

    /* setup modules */
    for(i = 0; i < ctx->count; i++)
        ctx->modules[i]->setup(&sync_event, &sync_cnt);

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
        ctx->modules[i]->release(ctx, ctx->config, ctx->tv);

    /* free buffers */
    CloseHandle(ctx->output.lock);
    for(b = 0; b < VZOUTPUT_MAX_BUFS; b++)
    {
        CloseHandle(ctx->output.buffers[b].lock);

        // free framebuffer
        glErrorLog(glDeleteBuffers(1, &ctx->output.buffers[b].num));
    };

    /* free input queue locks */
    for(i = 0; i < ctx->inputs_count; i++)
        CloseHandle(ctx->inputs_data[i].lock);

    return 0;
};

/** prepare OpenGL buffers download */
VZOUTPUT_API int vzOutputPostRender(void* obj)
{
    vzOutputContext_t* ctx;

    ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    /* wait async transfer */
    glErrorLogD(glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB,
        ctx->output.buffers[ctx->output.pos_render].num));
    ctx->output.buffers[ctx->output.pos_render].data =
        glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
    glErrorLogD(glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0));

    /* unlock buffer */
    ReleaseMutex(ctx->output.buffers[ctx->output.pos_render].lock);

    /* lock buffers head */
    WaitForSingleObject(ctx->output.lock, INFINITE);
    /* set jump flag */
    ctx->output.pos_render_jump = 1;
    /* unlock buffers head */
    ReleaseMutex(ctx->output.lock);

    return 0;
};

/** finish OpenGL buffers download */
VZOUTPUT_API int vzOutputPreRender(void* obj)
{
    vzOutputContext_t* ctx;

    ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    /* lock buffers head */
    WaitForSingleObject(ctx->output.lock, INFINITE);

    /* check if we can use next buffer */
    if(ctx->output.pos_render_jump)
    {
        /* check if next buffer is loaded */
        if(((ctx->output.pos_render + 1) % VZOUTPUT_MAX_BUFS) != ctx->output.pos_driver)
        {
            /* reset buffer frame number */
            ctx->output.buffers[ctx->output.pos_driver].id = 1;

            /* increment position */
            ctx->output.pos_render = (ctx->output.pos_render + 1) % VZOUTPUT_MAX_BUFS;

            /* check drop count */
            if(ctx->output.pos_render_dropped)
            {
                logger_printf(1, "vzOutput: dropped %d frames[render]",
                    ctx->output.pos_render_dropped);
                ctx->output.pos_render_dropped = 0;
            };
        }
        else
        {
            /* increment dropped framescounter */
            ctx->output.pos_render_dropped++;
        };

        /* clear flag - no more chances */
        ctx->output.pos_render_jump = 0;
    };

    /* set buffer frame number */
    ctx->output.buffers[ctx->output.pos_render].id = 1;

    /* lock buffer */
    WaitForSingleObject(ctx->output.buffers[ctx->output.pos_render].lock, INFINITE);

    /* unlock buffers head */
    ReleaseMutex(ctx->output.lock);

    /* bind to buffer */
    glErrorLogD(glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB,
        ctx->output.buffers[ctx->output.pos_render].num));
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
        ctx->modules[i]->run();

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
        ctx->modules[i]->stop();

    return 0;
};

/** retrieve output image */
VZOUTPUT_API int vzOutputOutGet(void* obj, vzImage* img)
{
    vzOutputContext_t* ctx;

    ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

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
        }
        else
        {
            /* increment dropped framescounter */
            ctx->output.pos_driver_dropped++;
        };

        /* clear flag - no more chances */
        ctx->output.pos_driver_jump = 0;
    };

    /* lock buffer */
    WaitForSingleObject(ctx->output.buffers[ctx->output.pos_driver].lock, INFINITE);

    /* unlock buffers head */
    ReleaseMutex(ctx->output.lock);

    /* setup image */
    img->bpp = 4;
    img->pix_fmt = VZIMAGE_PIXFMT_BGRA;
    img->base_x = img->base_y = 0;
    img->base_width = img->width = ctx->tv->TV_FRAME_WIDTH;
    img->base_height = img->height = ctx->tv->TV_FRAME_HEIGHT;
    img->surface = ctx->output.buffers[ctx->output.pos_driver].data;
    img->line_size = img->width * img->bpp;

    return 0;
};

/** release ouput image */
VZOUTPUT_API int vzOutputOutRel(void* obj, vzImage* img)
{
    vzOutputContext_t* ctx;

    ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    /* unlock buffer */
    ReleaseMutex(ctx->output.buffers[ctx->output.pos_driver].lock);

    /* lock buffers head */
    WaitForSingleObject(ctx->output.lock, INFINITE);

    /* set jump flag */
    ctx->output.pos_driver_jump = 1;

    /* unlock buffers head */
    ReleaseMutex(ctx->output.lock);

    return 0;
};

VZOUTPUT_API int vzOutputRenderSlots(void* obj)
{
    int r = 0;
    vzOutputContext_t* ctx;

    ctx = (vzOutputContext_t*)obj;
    if(!ctx) return -EINVAL;

    /* lock buffers head */
    WaitForSingleObject(ctx->output.lock, INFINITE);

    /* check current and next buffers */
    if
    (
        !ctx->output.buffers[ctx->output.pos_render].id
        ||
        !ctx->output.buffers[(ctx->output.pos_render + 1) % VZOUTPUT_MAX_BUFS].id
    )
        r = 1;

    /* unlock buffers head */
    ReleaseMutex(ctx->output.lock);

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

/*
    could be usefull for further reading:

    http://www.nvidia.com/dev_content/nvopenglspecs/GL_EXT_pixel_buffer_object.txt
    http://www.gpgpu.org/forums/viewtopic.php?t=2001&sid=4cb981dff7d9aa406cb721f7bd1072b6
    http://www.elitesecurity.org/t140671-pixel-buffer-object-PBO-extension
    http://www.gamedev.net/community/forums/topic.asp?topic_id=329957&whichpage=1&#2143668
    http://www.gamedev.net/community/forums/topic.asp?topic_id=360729
*/


#if 0


#define _CRT_SECURE_NO_WARNINGS

#ifdef _DEBUG
//#define DUMP_DRV_IO_LOCKS
#endif /* _DEBUG */

////#define FAKE_MIX
////#define FAKE_TONE

#ifdef DUMP_DRV_IO_LOCKS
unsigned _buffers_pos_render = 0;
unsigned _buffers_pos_driver = 0;
#pragma comment(lib, "winmm")
#endif /* DUMP_DRV_IO_LOCKS */


#include "vzGlExt.h"
#include "vzOutput.h"
#include "vzOutputInternal.h"
#include "vzMain.h"
#include "vzLogger.h"

static struct vzOutputBuffers* _global_buffers_info = NULL;
static double Ak = logl(10) / 20.0;


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

void vzOutput::lock_io_bufs(int* buf_idx)
{
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
				logger_printf(1, "vzOutput: dropped %d frames[driver]", _buffers.pos_driver_dropped);
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
    *buf_idx = _buffers.pos_driver;
};


void vzOutput::unlock_io_bufs()
{
    /* unlock buffer */
    ReleaseMutex(_buffers.locks[ _buffers.pos_driver ]);

    /* lock buffers head */
    WaitForSingleObject(_buffers.lock, INFINITE);

    /* set jump flag */
    _buffers.pos_driver_jump = 1;

    /* unlock buffers head */
    ReleaseMutex(_buffers.lock);
};

#ifdef FAKE_TONE
	static unsigned short *tone_buffer = NULL;
#endif /* FAKE_TONE */

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
				logger_printf(1, "vzOutput: dropped %d frames[render]", _buffers.pos_render_dropped);
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

        /* bind to buffer */
        glErrorLogD(glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB,
            _buffers.output.nums[_buffers.pos_render]));
        glErrorLogD(glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB));
        _buffers.output.data[_buffers.pos_render] = NULL;

        // start asynchroniosly read from buffer
        glErrorLogD(glReadPixels
        (
            0,
            0,
            _tv->TV_FRAME_WIDTH,
            _tv->TV_FRAME_HEIGHT,
            GL_BGRA_EXT,
            GL_UNSIGNED_BYTE,
            BUFFER_OFFSET(_buffers.output.offset)
        ));

        glErrorLogD(glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0));
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
		/* wait async transfer */
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, _buffers.output.nums[_buffers.pos_render]);
		_buffers.output.data[_buffers.pos_render] = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);

		/* deal with indexes */
		post_render_aux();
};


vzOutput::vzOutput(void* config, char* name, void* tv)
{
    int b, i, j;

	// tv params
	_tv = (vzTVSpec*)tv;

	// assign config
	_config = config;

    /* clear buffers into struct */
    memset(&_buffers, 0, sizeof(struct vzOutputBuffers));

	// prepare filename
	char filename[1024];
	sprintf(filename,"outputs/%s.dll",name);

	logger_printf(0, "vzOuput: Loading '%s' ... ",filename);

	// try load
	_lib = LoadLibrary(filename);

	// check if lib is loaded
	if (!_lib)
	{
		logger_printf(1, "vzOuput: Failed to load '%s'",filename);
		return;
	};
	logger_printf(0, "vzOuput: Loaded '%s'",filename);

	logger_printf(0, "vzOuput: Looking for procs ... ");
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
		// all loaded
		logger_printf(0, "vzOuput: All procs loaded");

		// set config
		logger_printf(0, "vzOutput: submitting config");
		SetConfig(_config);
		logger_printf(0, "vzOutput: config submitted");

		// initializ
		logger_printf(0, "vzOuput: found output board '%s'",name);
		int c = FindBoard(NULL);
		if(c)
		{
			int board_id = 0;
			logger_printf(0, "vzOuput: found %d output boards '%s'", c, name);

			// selecting board
			logger_printf(0, "vzOutput: Selecting board '%s:%d' ... ", name, board_id);
			SelectBoard(0,NULL);
			logger_printf(0, "vzOutput: Selected board '%s:%d'", name, board_id);

			// init board
			logger_printf(0, "vzOutput: Init board '%s:%d' ... ", name, board_id);
			InitBoard(_tv);
			logger_printf(0, "vzOutput: Initizalized board '%s:%d'", name, board_id);

			/* init INPUT/OUTPUT buffers here */
			{
				/* ask for output buffer sizes */
				memset(&_buffers, 0, sizeof(struct vzOutputBuffers));
				_buffers.lock = CreateMutex(NULL,FALSE,NULL);
				for(b = 0; b < VZOUTPUT_MAX_BUFS; b++)
					_buffers.locks[b] = CreateMutex(NULL,FALSE,NULL);
				GetBuffersInfo(&_buffers);

				// create framebuffers

                    // generate VZOUTPUT_MAX_BUFS offscreen buffers
                    glErrorLog(glGenBuffers(VZOUTPUT_MAX_BUFS, _buffers.output.nums));

					// init and request mem addrs of buffers
					for(b=0; b<VZOUTPUT_MAX_BUFS; b++)
					{
						// INIT BUFFER SIZE
						glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, _buffers.output.nums[b]);
						glBufferData(GL_PIXEL_PACK_BUFFER_ARB, _buffers.output.gold, NULL, GL_STREAM_READ);

						// REQUEST BUFFER PTR - MAP AND SAVE PTR
						_buffers.output.data[b] = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB,GL_READ_ONLY);

//						// UNMAP
//						glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);

//						// unbind ?
						glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);

                        /* deal with input buffers */
                        for(i = 0; i < _buffers.input_channels; i++)
                        {
                            for(j = 0; j <= _buffers.input[i].field_mode; j++)
                            {
                                _buffers.input[i].data[b][j] = VirtualAlloc
                                (
                                    NULL,
                                    _buffers.input[i].gold,
                                    MEM_COMMIT,
                                    PAGE_READWRITE
                                );
                                VirtualLock(_buffers.input[i].data[b][j],
                                    _buffers.input[i].gold);
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
			logger_printf(0, "vzOutput: Start Ouput Thread... ");
			StartOutputLoop(this);
			logger_printf(0, "vzOutput: Started Ouput Thread");
			return;
		};
	};

	logger_printf(1, "vzOuput: Failed procs lookup");

	// unload library
	FreeLibrary(_lib);
	_lib = NULL;
};

vzOutput::~vzOutput()
{
    int b, i, j;

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
			glDeleteBuffers(VZOUTPUT_MAX_BUFS, _buffers.output.nums);

			/* release mem */
			for(b=0; b<VZOUTPUT_MAX_BUFS; b++)
			{

                /* deal with input buffers */
                for(i = 0; i < _buffers.input_channels; i++)
                    for(j = 0; j <= _buffers.input[i].field_mode; j++)
                        if(_buffers.input[i].data[b][j])
                        {
                            VirtualUnlock(_buffers.input[i].data[b][j],
                                _buffers.input[i].gold);
                            VirtualFree(_buffers.input[i].data[b][j],
                                _buffers.input[i].gold, MEM_RELEASE);
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

#endif
