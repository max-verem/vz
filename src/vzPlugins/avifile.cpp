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
	2006-12-13:
		*Force avi deinit in mem_preload mode.

	2006-12-03:
		*Fixed bug with frame size to transfer.

	2006-11-19:
		*free transform feature added

	2006-11-18:
		*memory preloading clips
		*texture flipping flags

	2006-11-16:
		*Compleatly rewriting code with asyncronous reader.
		*'CoInitializeEx' usage to avoid COM uninitilizes answer from AVI* 
		functions (CO_E_NOTINITIALIZED returns) in multithreaded inviroment.

	2005-06-30:
		*Draft Version

*/
static char* _plugin_description = 
"Play AVI file as texture."
;

static char* _plugin_notes = 
"Accept 24 and 32 bpp files. Uses system decompressor."
;

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"
#include "../vz/plugin-procs.h"
#include "../vz/vzImage.h"
#include "../vz/vzGlExt.h"

#include <stdio.h>
#include <windows.h>
#include <io.h>
#include <vfw.h>

#include "free_transform.h"

#define RING_BUFFER_LENGTH 10
#define MAX_AVI_LOADERS 5

#define MAX_CONCUR_LOAD 1

#define AVI_OP_LOCK
//#define VERBOSE

/*
#define _WIN32_DCOM
#define _WIN32_WINNT 0x0400
#include <objbase.h>
*/
#define COINIT_MULTITHREADED 0
WINOLEAPI  CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);
#pragma comment(lib, "ole32.LIB")


#pragma comment(lib, "VFW32.LIB")
#pragma comment(lib, "winmm.lib")

#ifdef AVI_OP_LOCK
static HANDLE _avi_op_lock;
#endif /* AVI_OP_LOCK */

#ifdef MAX_CONCUR_LOAD
static HANDLE _avi_concur_lock;
static int _avi_concur_pending = 0;
static int _avi_concur_working = 0;
#endif /* MAX_CONCUR_LOAD */

#define PBO_SLICES  4
#define _pbo_empty_size (2048 * 2048 * 4) /* 1920x1080 should fit */
static unsigned int _pbo_empty_buf;

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
			break;
    }
    return TRUE;
}


// declare name and version of plugin
DEFINE_PLUGIN_INFO("avifile");

#define CMD_PLAY		FOURCC_TO_LONG('_','P','L','Y')
#define CMD_STOP		FOURCC_TO_LONG('_','S','T','P')
#define CMD_PAUSE		FOURCC_TO_LONG('_','P','S','E')
#define CMD_CONTINUE	FOURCC_TO_LONG('_','C','N','T')

/* ------------------------------------------------------------------------- */

#define GL_BGR                            0x80E0
struct aviloader_desc
{
	int flag_ready;
	int flag_exit;
	int flag_gone;
	int flag_mem_preload;

	char filename[128];					/* file name */

	long frames_count;
	long width;
	long height;
	long bpp;

	HANDLE lock;						/* struct update lock */
	HANDLE task;						/* thread id */
	HANDLE wakeup;						/* wakeup signal */

	void** buf_data;					/* buffers */
	int* buf_frame;
	int* buf_clear;
	int* buf_fill;
	int* buf_filled;
    int buf_cnt;                        /* buffers count allocated */
	long cursor;
};

static char* avi_err(long err)
{
	switch(err)
	{
		case AVIERR_BADFORMAT:		return "The file couldn't be read, indicating a corrupt file or an unrecognized format.";
		case AVIERR_MEMORY:			return "The file could not be opened because of insufficient memory.";
		case AVIERR_FILEREAD:		return "A disk error occurred while reading the file.";
		case AVIERR_FILEOPEN:		return "A disk error occurred while opening the file.";
		case REGDB_E_CLASSNOTREG:	return "According to the registry, the type of file specified in AVIFileOpen does not have a handler to process it.";
		case AVIERR_NODATA:			return "The file does not contain a stream corresponding to the values of fccType and lParam";
		case CO_E_NOTINITIALIZED:	return "CO_E_NOTINITIALIZED";
		default:					return "not known error";
	};
};

static void imgseqloader_full(struct aviloader_desc* desc,
    char** list_data, int list_len,
    vzImage* ref, int frame_size)
{
    int i, j, r;
    vzImage* image;

    for(i = 0, j = 0; i < desc->buf_cnt && !desc->flag_exit && !j; i++)
    {
        /* load image */
        r = vzImageLoad(&image, list_data[i]);

        /* check if image loaded */
        if(!r)
        {
            /* check if it equal to reference */
            if(ref->width == image->width && ref->height == image->height &&
                ref->bpp == image->bpp && ref->pix_fmt == image->pix_fmt)
            {
                /* copy data */
                memcpy
                (
                    desc->buf_data[i],
                    image->surface,
                    frame_size
                );

                /* setup flags */
                desc->buf_fill[i] = 0;
                desc->buf_clear[i] = 0;
                desc->buf_filled[i] = i + 1;
            }
            else
                r = 1;

            /* free image */
            vzImageRelease(&image);
        }
        else
            r = 2;

        /* check if errors occured */
        if(r)
        {
            /* inc error counter */
            j++;

            switch(r)
            {
                case 1:
                    logger_printf(1, "avifile: imgseqloader_full image ('%s') differs from reference",
                        list_data[i]);
                    break;

                case 2:
                    logger_printf(1, "avifile: imgseqloader_full image ('%s') failed to load",
                        list_data[i]);
                    break;
            };
        };
    };

    /* rise ready flag if all frames loaded */
    if(!j)
    {
        desc->flag_ready = 1;
        logger_printf(0, "avifile: imgseqloader_full loaded %d frames from %s",
            i, desc->filename);
    };

};

static void aviloader_full(struct aviloader_desc* desc, PGETFRAME pgf,
    LPBITMAPINFOHEADER frame_info, int frame_size)
{
    int i, j;
    LPBITMAPINFOHEADER frame;

    for(i = 0, j = 0; i < desc->buf_cnt && !desc->flag_exit && !j; i++)
    {
        /* load frame */
        frame = (LPBITMAPINFOHEADER)AVIStreamGetFrame(pgf, i);

        /* check if frame loaded */
        if(frame)
        {
            /* copy data */
            memcpy
            (
                desc->buf_data[i],
                ((unsigned char*)frame) + frame_info->biSize,
                frame_size
            );

            /* setup flags */
            desc->buf_fill[i] = 0;
            desc->buf_clear[i] = 0;
            desc->buf_filled[i] = i + 1;
        }
        else
        {
            /* inc error counter */
            j++;

            logger_printf(1, "avifile: aviloader_full AVIStreamGetFrame('%s', %d) FAILED",
                desc->filename, i);
        };
    };

    /* rise ready flag if all frames loaded */
    if(!j)
    {
        desc->flag_ready = 1;
        logger_printf(0, "avifile: aviloader_full loaded %d frames from %s",
            i, desc->filename);
    };
};

static void aviloader_live(struct aviloader_desc* desc, PGETFRAME pgf,
    LPBITMAPINFOHEADER frame_info, int frame_size)
{
    int i;
    LPBITMAPINFOHEADER frame;

    /* setup start plan */
    for(i = 0; i < desc->buf_cnt && i < desc->frames_count && !desc->flag_exit; i++)
    {
        desc->buf_frame[i] = i;
        desc->buf_fill[i] = 1;
        desc->buf_clear[i] = 1;
    };

    /* "endless" loop */
    while(!desc->flag_exit)
    {
        /* check if frames should be loaded */
        for(i = 0; i < desc->buf_cnt && i < desc->frames_count && !desc->flag_exit; i++)
        {
            if
            (!(
                (1 == desc->buf_clear[i])       /* frame is clear */
                &&                              /* and */
                (1 == desc->buf_fill[i])        /* should be loaded */
            ))
                continue;

            int f = desc->buf_frame[i];
            if(f < desc->frames_count) /* frame in range */
            {
                frame = (LPBITMAPINFOHEADER)AVIStreamGetFrame(pgf, f);
                if(frame)
                {
                    /* copy frame data */
                    memcpy
                    (
                        desc->buf_data[i],
                        ((unsigned char*)frame) + frame_info->biSize,
                        frame_size
                    );
                    /* setup flags */
                    desc->buf_fill[i] = 0;
                    desc->buf_clear[i] = 0;
                    desc->buf_filled[i] = f + 1;
#ifdef VERBOSE
                    logger_printf(0, "avifile: aviloader_live loaded frame %d into slot %d", f, i);
#endif /* VERBOSE */
                } else {
                    /* setup flags */
                    desc->buf_fill[i] = 0;
                    logger_printf(1, "avifile: aviloader_live AVIStreamGetFrame('%d') == NULL",
                        desc->buf_frame[i]);
                };

                Sleep(0);   /* allow context switch */
            } else {
                /* setup flags */
                desc->buf_fill[i] = 0;
                logger_printf(1, "avifile: aviloader_live desc->buf_frame[%d] >= %d",
                    desc->buf_frame[i], i, desc->frames_count);
            };
        };

        /* rise flag about ready */
        desc->flag_ready = 1;

        /* wait for signal if no jobs done */
        WaitForSingleObject(desc->wakeup, 10);
        ResetEvent(desc->wakeup);
    };
};

static unsigned long WINAPI aviloader_proc(void* p)
{
    HRESULT hr;
    struct aviloader_desc* desc;
    PAVIFILE avi_file = NULL;
    PAVISTREAM avi_stream = NULL;
    AVISTREAMINFO *strhdr = NULL;   /* AVI stream definition structs */
    LPBITMAPINFOHEADER frame_info;
    PGETFRAME pgf = NULL;
    int i, j;

    /* cast struct */
    desc = (struct aviloader_desc*)p;

    logger_printf(0, "avifile: aviloader_proc started('%s')", desc->filename);

    /* init AVI for this thread */
#ifdef AVI_OP_LOCK
    WaitForSingleObject(_avi_op_lock,INFINITE);
#endif /* AVI_OP_LOCK */
    CoInitializeEx( NULL, COINIT_MULTITHREADED );
    AVIFileInit();
#ifdef AVI_OP_LOCK
    ReleaseMutex(_avi_op_lock);
#endif /* AVI_OP_LOCK */

    hr = AVIFileOpen
    (
        &avi_file,              /* PAVIFILE *ppfile,    */
        desc->filename,         /* LPCTSTR szFile,      */
        OF_READ,                /* UINT mode,           */
        NULL                    /* CLSID pclsidHandler  */
    );
    if(hr)
    {
        logger_printf(1, "avifile: aviloader_proc AVIFileOpen('%s') FAILED: %s",
            desc->filename, avi_err(hr));
        goto ex1;
    };

    /* open avi file stream */
    hr = AVIFileGetStream
    (
        avi_file,               /* PAVIFILE pfile,      */
        &avi_stream,            /* PAVISTREAM  *ppavi,  */
        streamtypeVIDEO,        /* DWORD fccType,       */
        0                       /* LONG lParam          */
    );
    if(hr)
    {
        logger_printf(1, "avifile: aviloader_proc AVIFileGetStream('%s') FAILED: %s",
            desc->filename, avi_err(hr));
        goto ex1;
    };

    /* request frames count */
    desc->frames_count = AVIStreamLength(avi_stream);
    if(desc->frames_count < 0)
    {
        logger_printf(1, "avifile: aviloader_proc AVIStreamLength('%s') FAILED",
            desc->filename);
        goto ex1;
    };

    /* prepare struct for stream header */
    strhdr = (AVISTREAMINFO*)malloc(sizeof(AVISTREAMINFO));
    memset(strhdr, 0, sizeof(AVISTREAMINFO));

    /* request stream info */
    hr = AVIStreamInfo
    (
        avi_stream,             /* PAVISTREAM pavi      */
        strhdr,                 /* AVISTREAMINFO * psi, */
        sizeof(AVISTREAMINFO)   /* LONG lSize           */
    );
    if(hr)
    {
        logger_printf(1, "avifile: aviloader_proc AVIStreamInfo('%s') FAILED",
            desc->filename);
        goto ex1;
    };

    /* prepares to decompress video frames */
    pgf = AVIStreamGetFrameOpen
    (
        avi_stream,         /* PAVISTREAM pavi,                 */
        NULL                /* LPBITMAPINFOHEADER lpbiWanted    */
    );
    if(!pgf)
    {
        logger_printf(1, "avifile: aviloader_proc AVIStreamGetFrameOpen('%s') FAILED",
            desc->filename);
        goto ex1;
    };

    /* test load  */
    frame_info = (LPBITMAPINFOHEADER)AVIStreamGetFrame(pgf, 0);
    if(!frame_info)
    {
        logger_printf(1, "avifile: aviloader_proc AVIStreamGetFrame('%s', 0) FAILED",
            desc->filename);
        goto ex1;
    };

    /* setup width, height, buffer type */
    desc->width = frame_info->biWidth;
    desc->height = frame_info->biHeight;
    if(32 == frame_info->biBitCount)
        desc->bpp = GL_BGRA_EXT;
    else if (24 == frame_info->biBitCount)
        desc->bpp = GL_BGR;
    else
    {
        logger_printf(1, "avifile: aviloader_proc biBitCount=%d NOT SUPPORTED",
            frame_info->biBitCount);
        goto ex1;
    };

    /* init flags buffers */
    desc->buf_cnt = (desc->flag_mem_preload)?desc->frames_count:RING_BUFFER_LENGTH;
    desc->buf_frame     = (int*)malloc(sizeof(int) * desc->buf_cnt);
    desc->buf_clear     = (int*)malloc(sizeof(int) * desc->buf_cnt);
    desc->buf_fill      = (int*)malloc(sizeof(int) * desc->buf_cnt);
    desc->buf_filled    = (int*)malloc(sizeof(int) * desc->buf_cnt);
    if(!desc->buf_frame || !desc->buf_clear || !desc->buf_fill || !desc->buf_filled)
    {
        logger_printf(1, "avifile: aviloader_proc ENOMEM1, file=%s", desc->filename);
        goto ex1;
    };

    /* init data buffer */
    desc->buf_data = (void**)malloc(sizeof(void*) * desc->buf_cnt);
    if(!desc->buf_data)
    {
        logger_printf(1, "avifile: aviloader_proc ENOMEM2, file=%s", desc->filename);
        goto ex1;
    };
    memset(desc->buf_data, 0, sizeof(void*) * desc->buf_cnt);
    int frame_size = frame_info->biWidth * frame_info->biHeight *
        (frame_info->biBitCount / 8);
    for(i = 0, j = 0; i < desc->buf_cnt && !j; i++)
    {
        desc->buf_data[i] = malloc(frame_size);
        if(!desc->buf_data[i]) j++;
        else memset(desc->buf_data[i], 0, frame_size);
    };
    if(j)
    {
        logger_printf(1, "avifile: aviloader_proc ENOMEM3, file=%s", desc->filename);
        goto ex1;
    };

    /* check method */
    if(desc->flag_mem_preload)
    {
#ifdef MAX_CONCUR_LOAD
        /* increment waiters counter */
        WaitForSingleObject(_avi_concur_lock, INFINITE);
        _avi_concur_pending++;
        logger_printf(0, "avifile: aviloader_proc pending %d, working %d: PENDING[%s]",
            _avi_concur_pending, _avi_concur_working, desc->filename);
        ReleaseMutex(_avi_concur_lock);

        /* wait */
        for(int c = 1; c ; Sleep(80))
        {
            WaitForSingleObject(_avi_concur_lock, INFINITE);
            if(_avi_concur_working < MAX_CONCUR_LOAD)
            {
                _avi_concur_working++;
                c = 0;
                logger_printf(0, "avifile: aviloader_proc pending %d, working %d: START[%s]",
                    _avi_concur_pending, _avi_concur_working, desc->filename);
            };
            ReleaseMutex(_avi_concur_lock);
        };
#endif /* MAX_CONCUR_LOAD */

        /* call mem preloader */
        if(!desc->flag_exit)
            aviloader_full(desc, pgf, frame_info, frame_size);

#ifdef MAX_CONCUR_LOAD
        WaitForSingleObject(_avi_concur_lock, INFINITE);
        _avi_concur_pending--;
        _avi_concur_working--;
        logger_printf(0, "avifile: aviloader_proc pending %d, working %d: FINISHED[%s]",
            _avi_concur_pending, _avi_concur_working, desc->filename);
        ReleaseMutex(_avi_concur_lock);
#endif /* MAX_CONCUR_LOAD */

        /* wait for exit flag */
        while(desc->flag_ready && !desc->flag_exit)
        {
            /* wait for signal if no jobs done */
            WaitForSingleObject(desc->wakeup, 10);
            ResetEvent(desc->wakeup);
        };
    }
    else
    {
        /* call live preloader */
        if(!desc->flag_exit)
            aviloader_live(desc, pgf, frame_info, frame_size);
    };

ex1:
    /* free buffers */
    if(desc->buf_data)
    {
        for(i = 0; i < desc->buf_cnt; i++) if(desc->buf_data[i]) free(desc->buf_data[i]);
        free(desc->buf_data);
        desc->buf_data = NULL;
    };
    if(desc->buf_frame)     free(desc->buf_frame);  desc->buf_frame = NULL;
    if(desc->buf_clear)     free(desc->buf_clear);  desc->buf_clear = NULL;
    if(desc->buf_fill)      free(desc->buf_fill);   desc->buf_fill = NULL;
    if(desc->buf_filled)    free(desc->buf_filled); desc->buf_filled = NULL;

    /* setup flag of exiting */
    desc->flag_gone = 1;

    /* free stream header */
    if(strhdr) free(strhdr);

    /* close frame pointer */
    if(pgf)
        AVIStreamGetFrameClose(pgf);

    /* release avi file */
    if(avi_stream)
        AVIStreamRelease(avi_stream);

    if(avi_file)
        AVIFileRelease(avi_file);

    /* init AVI for this thread */
#ifdef AVI_OP_LOCK
    WaitForSingleObject(_avi_op_lock,INFINITE);
#endif /* AVI_OP_LOCK */
    AVIFileExit();
    ::CoUninitialize();
#ifdef AVI_OP_LOCK
    ReleaseMutex(_avi_op_lock);
#endif /* AVI_OP_LOCK */

    logger_printf(0, "avifile: aviloader_proc exiting('%s')", desc->filename);

    /* exit thread */
    return 0;
};

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

static int tmplcmp(char* a, char* b)
{
    for(;*a && *b; a++, b++)
    {
        if('#' == *a || '#' == *b) continue;
        if(*a == *b) continue;
        if(*a > *b) return -1;
        else return 1;
    };

    if(*a) return -1;
    if(*b) return 1;

    return 0;
};

static unsigned long WINAPI imgseqloader_proc(void* p)
{
    struct aviloader_desc* desc = (struct aviloader_desc*)p;
    int i, j;

    long hfile;
    struct _finddata_t fd;

    char filename[MAX_PATH];
    char path[MAX_PATH];
    char *t, *tmpl;

    char** list_data = NULL;
    int list_len = 0;

    int r;
    vzImage* image = NULL;

    logger_printf(0, "avifile: imgseqloader_proc('%s') started", desc->filename);

    /* split directory name from file */
    tmpl = desc->filename;
    t = MAX(strrchr(tmpl, '\\'), strrchr(tmpl, '/'));

    /* found anything */
    if(!t)
    {
        logger_printf(1, "avifile: imgseqloader_proc('%s') FAILED: cant parse filename",
            desc->filename);
        goto ex1;
    };

    /* copy filename and path */
    strncpy(filename, t + 1, MAX_PATH);
    memcpy(path, tmpl, t - tmpl + 1); path[t - tmpl + 1] = 0;

    /* append search template */
    strcat(path, "*");

    /* start lookup */
    hfile = _findfirst(path, &fd);
    if(-1 == hfile)
    {
        logger_printf(1, "avifile: imgseqloader_proc('%s') FAILED: path ('%s') not found", path);
        goto ex1;
    };

    /* init list */
    list_len  = 0;
    list_data = (char**)malloc((list_len + 1) * sizeof(char*));
    list_data[list_len] = NULL;

    /* find a files */
    do
    {
        /* check if directory */
        if(fd.attrib & _A_SUBDIR) continue;

        /* check name */
        if(tmplcmp(fd.name, filename)) continue;

        /* add filename to list */
        list_data[list_len] = (char*)malloc(strlen(fd.name) + 2 + strlen(path));
        memcpy(list_data[list_len], tmpl, t - tmpl + 1);
        strcpy(list_data[list_len] + (t - tmpl) + 1, fd.name);

        list_len++;
        list_data = (char**)realloc(list_data, (list_len + 1) * sizeof(char*));
        list_data[list_len] = NULL;
    }
    while (!_findnext(hfile, &fd));

    /* release search */
    _findclose(hfile);

    /* sort list */
    if(list_len)
    {
        for(int i = 0; i < list_len; i++)
        {
            int k, j;

            /* find larges */
            for(k = i, j = i + 1; j < list_len; j++)
                if(_stricmp(list_data[j], list_data[k]) < 0)
                    k = j;
            /* swap */
            if(k != i)
            {
                char* tmp = list_data[k];
                list_data[k] = list_data[i];
                list_data[i] = tmp;
            };
        };
    };

    /* request frames count */
    desc->frames_count = list_len;
    if(!desc->frames_count)
    {
        logger_printf(1, "avifile: imgseqloader_proc('%s') FAILED: no files found",
            desc->filename);
        goto ex1;
    };

    /* probe first file for format definition */
    r = vzImageLoad(&image, list_data[0]);
    if(r)
    {
        logger_printf(1, "avifile: imgseqloader_proc('%s') FAILED: first image ('%s') not recognized",
            desc->filename, list_data[0]);
        goto ex1;
    };

    /* setup width, height, buffer type */
    desc->width     = image->width;
    desc->height    = image->height;
    desc->bpp       = vzImagePixFmt2OGL(image->pix_fmt);

    /* init flags buffers */
    desc->buf_cnt = desc->frames_count;
    desc->buf_frame     = (int*)malloc(sizeof(int) * desc->buf_cnt);
    desc->buf_clear     = (int*)malloc(sizeof(int) * desc->buf_cnt);
    desc->buf_fill      = (int*)malloc(sizeof(int) * desc->buf_cnt);
    desc->buf_filled    = (int*)malloc(sizeof(int) * desc->buf_cnt);
    if(!desc->buf_frame || !desc->buf_clear || !desc->buf_fill || !desc->buf_filled)
    {
        logger_printf(1, "avifile: imgseqloader_proc('%s') ENOMEM1", desc->filename);
        goto ex1;
    };

    /* init data buffer */
    desc->buf_data = (void**)malloc(sizeof(void*) * desc->buf_cnt);
    if(!desc->buf_data)
    {
        logger_printf(1, "avifile: imgseqloader_proc('%s') ENOMEM1", desc->filename);
        goto ex1;
    };
    memset(desc->buf_data, 0, sizeof(void*) * desc->buf_cnt);
    int frame_size = image->line_size * image->height;
    for(i = 0, j = 0; i < desc->buf_cnt && !j; i++)
    {
        desc->buf_data[i] = malloc(frame_size);
        if(!desc->buf_data[i]) j++;
        else memset(desc->buf_data[i], 0, frame_size);
    };
    if(j)
    {
        logger_printf(1, "avifile: imgseqloader_proc('%s') ENOMEM1", desc->filename);
        goto ex1;
    };

#ifdef MAX_CONCUR_LOAD
    /* increment waiters counter */
    WaitForSingleObject(_avi_concur_lock, INFINITE);
    _avi_concur_pending++;
    logger_printf(0, "avifile: imgseqloader_proc pending %d, working %d: PENDING[%s]",
        _avi_concur_pending, _avi_concur_working, desc->filename);
    ReleaseMutex(_avi_concur_lock);

    /* wait */
    for(int c = 1; c ; Sleep(80))
    {
        WaitForSingleObject(_avi_concur_lock, INFINITE);
        if(_avi_concur_working < MAX_CONCUR_LOAD)
        {
            _avi_concur_working++;
            c = 0;
            logger_printf(0, "avifile: imgseqloader_proc pending %d, working %d: START[%s]",
                _avi_concur_pending, _avi_concur_working, desc->filename);
        };
        ReleaseMutex(_avi_concur_lock);
    };
#endif /* MAX_CONCUR_LOAD */

    /* call mem preloader */
    if(!desc->flag_exit)
        imgseqloader_full(desc, list_data, list_len, image, frame_size);

#ifdef MAX_CONCUR_LOAD
    WaitForSingleObject(_avi_concur_lock, INFINITE);
    _avi_concur_pending--;
    _avi_concur_working--;
    logger_printf(0, "avifile: imgseqloader_proc pending %d, working %d: FINISHED[%s]",
        _avi_concur_pending, _avi_concur_working, desc->filename);
    ReleaseMutex(_avi_concur_lock);
#endif /* MAX_CONCUR_LOAD */

    /* wait for exit flag */
    while(desc->flag_ready && !desc->flag_exit)
    {
        /* wait for signal if no jobs done */
        WaitForSingleObject(desc->wakeup, 10);
        ResetEvent(desc->wakeup);
    };

ex1:
    /* free buffers */
    if(desc->buf_data)
    {
        for(i = 0; i < desc->buf_cnt; i++) if(desc->buf_data[i]) free(desc->buf_data[i]);
        free(desc->buf_data);
        desc->buf_data = NULL;
    };
    if(desc->buf_frame)     free(desc->buf_frame);  desc->buf_frame = NULL;
    if(desc->buf_clear)     free(desc->buf_clear);  desc->buf_clear = NULL;
    if(desc->buf_fill)      free(desc->buf_fill);   desc->buf_fill = NULL;
    if(desc->buf_filled)    free(desc->buf_filled); desc->buf_filled = NULL;

    /* setup flag of exiting */
    desc->flag_gone = 1;

    /* free stream header */
    if(image) vzImageRelease(&image);

    /* free list */
    if(list_len || list_data)
    {
        for(int i = 0; list_data[i]; i++)
            free(list_data[i]);
        free(list_data);
    };

    logger_printf(0, "avifile: imgseqloader_proc exiting('%s')", desc->filename);

    /* exit thread */
    return 0;
};

static struct aviloader_desc* aviloader_init(char* filename, int mem_preload)
{
	unsigned long thread_id;

	/* allocate and clear struct */
	struct aviloader_desc* desc = (struct aviloader_desc*)malloc(sizeof(struct aviloader_desc));
	memset(desc, 0, sizeof(struct aviloader_desc));

	/* copy argument */
	strcpy(desc->filename, filename);
	desc->flag_mem_preload = mem_preload;

	/* init events */
	desc->wakeup = CreateEvent(NULL, TRUE, FALSE, NULL);
	ResetEvent(desc->wakeup);

	/* init mutexes */
	desc->lock = CreateMutex(NULL,FALSE,NULL);

	/* run thread */
    if(strchr(desc->filename, '#'))
        desc->task = CreateThread(0, 0, imgseqloader_proc, desc, 0, &thread_id);
    else
        desc->task = CreateThread(0, 0, aviloader_proc, desc, 0, &thread_id);
    SetThreadPriority(desc->task , VZPLUGINS_AUX_THREAD_PRIO);

	return desc;
};


static void aviloader_destroy(struct aviloader_desc* desc)
{
	/* check if struct is not dead */
	if(NULL == desc) return;

	/* rise flag */
	desc->flag_exit = 1;

	/* wait for thread finish */
	WaitForSingleObject(desc->task, INFINITE);

	/* close all */
	CloseHandle(desc->task);
	CloseHandle(desc->lock);
	CloseHandle(desc->wakeup);

	/* free mem */
	free(desc);
};


/* ------------------------------------------------------------------------- */


// internal structure of plugin
typedef struct
{
// public data
	char* s_filename;		// avi file name
	long L_center;			// centering of image
	long l_loop;			// flag indicated that loop playing is active
	long l_field_mode;		// field incrementator
	long l_start_frame;		// start frame
	long l_auto_play;		// indicate autoplay state
	long l_flip_v;			/* flip vertical flag */
	long l_flip_h;			/* flip vertical flag */
	long l_mem_preload;		/* preload whole clip in memmory */
// free transform coords
	float f_x1;				/* left bottom coner */
	float f_y1;
	float f_z1;
	float f_x2;				/* left upper coner */
	float f_y2;
	float f_z2;
	float f_x3;				/* right upper coner */
	float f_y3;
	float f_z3;
	float f_x4;				/* right bottom coner */
	float f_y4;
	float f_z4;
	long  l_tr_lod;			/* number of breaks */

// trigger events for online control
	long l_trig_play;		/* play from beginning */
	long l_trig_cont;		/* continie play from current position */
	long l_trig_stop;		/* stop/pause playing */
	long l_trig_jump;		/* jump to desired position */

// internal datas
	char* _filename;		// mirror pointer to filename
	HANDLE _lock_update;	// update struct mutex
	long _playing;			/* flags indicate PLAY state */
	long _current_frame;	// pointer on AVI frame
	long _cursor_loaded;	/* previous cursor - detect to upload texture */
	struct aviloader_desc** _loaders;
	unsigned int _texture;
	unsigned int _texture_initialized;
	long _width;
	long _height;

	float* _ft_vertices;
	float* _ft_texels;

    unsigned int _pbo[PBO_SLICES];
    unsigned int _pbo_size;

} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
// public data
	NULL,					// char* s_filename;		// avi file name
	GEOM_CENTER_CM,			// long L_center;			// centering of image
	0,						// long l_loop;				// flag indicated that loop playing is active
	0,						// long l_field_mode;		// field incrementator
	0,						// long l_start_frame;		// start frame
	0,						// long l_auto_play;		// indicate autoplay state
	0,						// long l_flip_v;			/* flip vertical flag */
	0,						// long l_flip_h;			/* flip vertical flag */
	0,						// long l_mem_preload;		/* preload whole clip in memmory */
// free transform coords
	0.0,					// float f_x1;				/* left bottom coner */
	0.0,					// float f_y1;
	0.0,					// float f_z1;
	0.0,					// float f_x2;				/* left upper coner */
	0.0,					// float f_y2;
	0.0,					// float f_z2;
	0.0,					// float f_x3;				/* right upper coner */
	0.0,					// float f_y3;
	0.0,					// float f_z3;
	0.0,					// float f_x4;				/* right bottom coner */
	0.0,					// float f_y4;
	0.0,					// float f_z4;
	30,						// long  l_tr_lod;			/* number of breaks */


// trigger events for online control
	0,						// long l_trig_play;		/* play from beginning */
	0,						// long l_trig_cont;		/* continie play from current position */
	0,						// long l_trig_stop;		/* stop/pause playing */
	0xFFFFFFF,				// long l_trig_jump;		/* jump to desired position */

// internal datas
	NULL,					// char* _filename;		// mirror pointer to filename
	INVALID_HANDLE_VALUE,	// HANDLE _lock_update;	// update struct mutex
	0,						// long _playing;			/* flags indicate PLAY state */
	0,						// long _current_frame;	// pointer on AVI frame
	0,						// long _cursor_loaded;	/* previous cursor - detect to upload texture */
	NULL,					// struct aviloader_desc** _loaders;
	0,						// unsigned int _texture;
	0,						// unsigned int _texture_initialized;
	0,						// long _width;
	0,						// long _height;
	NULL,					// float* _ft_vertices;
	NULL,					// float* _ft_texels;
    {0},                    // unsigned int _pbo[PBO_SLICES];
    0                       // unsigned int _pbo_size;
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"s_filename",		"avi file name",
						PLUGIN_PARAMETER_OFFSET(default_value,s_filename)},

	{"L_center",		"centering of image",
						PLUGIN_PARAMETER_OFFSET(default_value,L_center)},

	{"l_loop",			"flag indicated that loop playing is active",
						PLUGIN_PARAMETER_OFFSET(default_value,l_loop)},

	{"l_field_mode",	"field mode frame counter",
						PLUGIN_PARAMETER_OFFSET(default_value,l_field_mode)},

	{"l_start_frame",	"start frame number",
						PLUGIN_PARAMETER_OFFSET(default_value,l_start_frame)},

	{"l_auto_play",		"indicate autoplay state",
						PLUGIN_PARAMETER_OFFSET(default_value,l_auto_play)},

	{"l_trig_play",		"start playing (from beginning) trigger",
						PLUGIN_PARAMETER_OFFSET(default_value,l_trig_play)},

	{"l_trig_cont",		"continue playing from current position",
						PLUGIN_PARAMETER_OFFSET(default_value,l_trig_cont)},

	{"l_trig_stop",		"stop/pause playing",
						PLUGIN_PARAMETER_OFFSET(default_value,l_trig_stop)},

	{"l_trig_jump",		"jump to specified frame position",
						PLUGIN_PARAMETER_OFFSET(default_value,l_trig_jump)},

	{"l_flip_v",		"flag to vertical flip",
						PLUGIN_PARAMETER_OFFSET(default_value,l_flip_v)},

	{"l_flip_h",		"flag to horozontal flip",
						PLUGIN_PARAMETER_OFFSET(default_value,l_flip_h)},

	{"l_mem_preload",	"preload whole clip in memmory",
						PLUGIN_PARAMETER_OFFSET(default_value,l_mem_preload)},

	{"f_x1",			"X of left bottom corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_x1)},
	{"f_y1",			"Y of left bottom corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_y1)},
	{"f_z1",			"Z of left bottom corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_z1)},

	{"f_x2",			"X of left upper corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_x2)},
	{"f_y2",			"Y of left upper corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_y2)},
	{"f_z2",			"Z of left upper corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_z2)},

	{"f_x3",			"X of right upper corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_x3)},
	{"f_y3",			"Y of right upper corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_y3)},
	{"f_z3",			"Z of right upper corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_z3)},

	{"f_x4",			"X of right bottom corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_x4)},
	{"f_y4",			"Y of right bottom corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_y4)},
	{"f_z4",			"Z of right bottom corner (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, f_z4)},

	{"l_tr_lod",		"Level of triangulation (free transorm mode)", 
						PLUGIN_PARAMETER_OFFSET(default_value, l_tr_lod)},

	{NULL,NULL,0}
};

PLUGIN_EXPORT int load(void* config)
{
    int r;
    void* p;

#ifdef AVI_OP_LOCK
    _avi_op_lock = CreateMutex(NULL,FALSE,NULL);
#endif /* AVI_OP_LOCK */
#ifdef MAX_CONCUR_LOAD
    _avi_concur_lock = CreateMutex(NULL,FALSE,NULL);
#endif /* MAX_CONCUR_LOAD */

    /* init empty PBO object */
    glGenBuffers(1, &_pbo_empty_buf);
    r = glGetError();
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, _pbo_empty_buf);
    glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, _pbo_empty_size, 0, GL_STREAM_DRAW);
    p = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
    memset(p, 0, _pbo_empty_size);
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);

    return 0;
};

PLUGIN_EXPORT int unload(void* config)
{
    /* check for global empty pbo */
    glDeleteBuffers(1, &_pbo_empty_buf);

#ifdef AVI_OP_LOCK
    CloseHandle(_avi_op_lock);
#endif /* AVI_OP_LOCK */
#ifdef MAX_CONCUR_LOAD
    CloseHandle(_avi_concur_lock);
#endif /* MAX_CONCUR_LOAD */

    return 0;
};

PLUGIN_EXPORT void* constructor(void)
{
	// init memmory for structure
	vzPluginData* data = (vzPluginData*)malloc(sizeof(vzPluginData));

	// copy default value
	*data = default_value;

	// create mutexes
	_DATA->_lock_update = CreateMutex(NULL,FALSE,NULL);
	ReleaseMutex(_DATA->_lock_update);

	/* create loaders array */
	_DATA->_loaders = (struct aviloader_desc**)malloc(MAX_AVI_LOADERS * sizeof(struct aviloader_desc*));
	memset(_DATA->_loaders, 0, MAX_AVI_LOADERS * sizeof(struct aviloader_desc*));

	// return pointer
	return data;
};

PLUGIN_EXPORT void destructor(void* data)
{
	int i;

    vzPluginData* ctx = (vzPluginData*)data;

	// try to lock struct
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

	// check if texture initialized
	if(_DATA->_texture_initialized)
        glExtDeleteTextures(1, &(_DATA->_texture));

    /* delete buffers from non opengl context */
    if(ctx->_pbo_size)
        glExtDeleteBuffers(PBO_SLICES, ctx->_pbo);

	/* destroy loaders */
	for(i = 0; i<MAX_AVI_LOADERS; i++)
		if(_DATA->_loaders[i])
		{
			aviloader_destroy(_DATA->_loaders[i]);
			_DATA->_loaders[i] = NULL;
		};
	free(_DATA->_loaders);

	/* free arrays coords */
	if(_DATA->_ft_vertices) free(_DATA->_ft_vertices);
	if(_DATA->_ft_texels) free(_DATA->_ft_texels);

	// unlock
	ReleaseMutex(_DATA->_lock_update);

	// close mutexes
	CloseHandle(_DATA->_lock_update);

	// free data
	free(data);
};

inline unsigned int pix_size(int t)
{
    switch(t)
    {
        case GL_BGR:
        case GL_RGB:
            return 3;
        case GL_BGRA_EXT:
        case GL_RGBA:
            return 4;
        case GL_LUMINANCE:
            return 1;
    };

    return 0;
};

PLUGIN_EXPORT void prerender(void* data,vzRenderSession* session)
{
	unsigned long r;
    int b;
    vzPluginData* ctx = (vzPluginData*)data;

	// try to lock struct
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

	/* check if we need to init/reinit texture */
	if
	(
		(_DATA->_loaders[0])									/* current loader is defined */
		&&														/* and */
		(_DATA->_loaders[0]->flag_ready)						/* pipeline theoreticaly ready */
	)
	{
        /* recreare PBO if needs */
        if(ctx->_pbo_size != pix_size(_DATA->_loaders[0]->bpp) *
            _DATA->_loaders[0]->width * _DATA->_loaders[0]->height)
        {
            /* delete buffer if needed */
            if(ctx->_pbo_size)
                glDeleteBuffers(PBO_SLICES, ctx->_pbo);

            /* update frame size */
            ctx->_pbo_size = pix_size(_DATA->_loaders[0]->bpp) *
                _DATA->_loaders[0]->width * _DATA->_loaders[0]->height;

            /* generate new buffers */
            glGenBuffers(PBO_SLICES, ctx->_pbo);

            /* setup buffers size */
            for(b = 0; b < PBO_SLICES; b++)
            {
                glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, ctx->_pbo[b]);
                glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, ctx->_pbo_size / PBO_SLICES, 0, GL_STREAM_DRAW);
            };
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
        };

		/* pipeline ready - check texture */
		if
		(
			(_DATA->_width != POT(_DATA->_loaders[0]->width))
			||
			(_DATA->_height != POT(_DATA->_loaders[0]->height))
		)
		{
			/* texture should be (re)initialized */

			if(_DATA->_texture_initialized)
				glDeleteTextures_D (1, &(_DATA->_texture));

			/* generate new texture */
			glGenTextures(1, &_DATA->_texture);

			/* set flags */
			_DATA->_width = POT(_DATA->_loaders[0]->width);
			_DATA->_height = POT(_DATA->_loaders[0]->height);
			_DATA->_texture_initialized = 1;

			/* create texture (init texture memory) */
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, _pbo_empty_buf);
			glBindTexture(GL_TEXTURE_2D, _DATA->_texture);
			glTexImage2D
			(
				GL_TEXTURE_2D,			// GLenum target,
				0,						// GLint level,
                GL_RGBA8,               // GLint components,
				_DATA->_width,			// GLsizei width, 
				_DATA->_height,			// GLsizei height, 
				0,						// GLint border,
				GL_BGRA_EXT,			// GLenum format,
				GL_UNSIGNED_BYTE,		// GLenum type,
                NULL                    // const GLvoid *pixels
			);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

#ifdef VERBOSE
			logger_printf("avifile: reinitialized texture %dx%d", _DATA->_width, _DATA->_height);
#endif /* VERBOSE */

		};

		/* load new texture , if it ready*/
		if
		(
			(_DATA->_cursor_loaded != _DATA->_loaders[0]->cursor)
			&&
			(_DATA->_loaders[0]->buf_filled[ _DATA->_loaders[0]->cursor ] )
		)
		{
            /* load frame */
            if(!(0.0f == session->f_alpha && ctx->l_loop))  // we ignore loading texture for looped hidden animations
            {
                /* draw buffers */
                for(b = 0; b < PBO_SLICES; b++)
                {
                    void* pbo_ptr;

                    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, ctx->_pbo[b]);
                    pbo_ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
                    if(pbo_ptr)
                    {
                        void* src_slice = 
                            (unsigned char*)_DATA->_loaders[0]->buf_data[ _DATA->_loaders[0]->cursor ] +
                            b * (ctx->_pbo_size / PBO_SLICES);

                        memcpy(pbo_ptr, src_slice, ctx->_pbo_size / PBO_SLICES);

                        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
                    };

                    glBindTexture(GL_TEXTURE_2D, _DATA->_texture);
                    glTexSubImage2D
                    (
                        GL_TEXTURE_2D,                                      // GLenum target,
                        0,                                                  // GLint level,
                        (_DATA->_width - _DATA->_loaders[0]->width)/2,      // GLint xoffset,
                        (_DATA->_height - _DATA->_loaders[0]->height)/2 +
                            b * (_DATA->_loaders[0]->height / PBO_SLICES),
//                        (_DATA->_height - _DATA->_loaders[0]->height)/2,    // GLint yoffset,
                        _DATA->_loaders[0]->width,                          // GLsizei width,
                        _DATA->_loaders[0]->height / PBO_SLICES,
//                        _DATA->_loaders[0]->height,                         // GLsizei height,
                        _DATA->_loaders[0]->bpp,                            // GLenum format,
                        GL_UNSIGNED_BYTE,                                   // GLenum type,
                        NULL                                                // const GLvoid *pixels 
                    );
                };
                glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
            };

			/* sync cursor value */
			_DATA->_cursor_loaded = _DATA->_loaders[0]->cursor;

			/* setup flag is clear for current frame */
			_DATA->_loaders[0]->buf_clear[ _DATA->_loaders[0]->cursor ] = 1;

#ifdef VERBOSE
			logger_printf(0, "avifile: loaded texture frame %d from slot %d", _DATA->_loaders[0]->buf_filled[ _DATA->_loaders[0]->cursor ], _DATA->_loaders[0]->cursor);
#endif /* VERBOSE */
		};

	};

	// release mutex
	ReleaseMutex(_DATA->_lock_update);
};

PLUGIN_EXPORT void postrender(void* data,vzRenderSession* session)
{
	int i, t, c, f;
	// try to lock struct
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

	/* check mode */
	if
	(
		/* we increment number diff in frame and field mode */
		(
			(_DATA->l_field_mode)								/* if we are rendering by field */
			||													/* or */
			(
				(!(_DATA->l_field_mode))						/* if we are rendering by frame */
				&&												/* and */
				(0 == session->field)							/* current field is upper */
			)
		)
		&&														/* and */
		/* async pipeline is ready */
		(
			(_DATA->_loaders[0])								/* current loader is defined */
			&&													/* and */
			(_DATA->_loaders[0]->flag_ready)					/* pipeline theoreticaly ready */
		)
	)
	{

		if(_DATA->l_mem_preload)
		{
			/* we are in memory preload mode */

			/* increment current frame if we are in playing mode */
			if(_DATA->_playing)
				_DATA->_current_frame++;

			/* check range */
			if(_DATA->_current_frame >= _DATA->_loaders[0]->frames_count)
			{
				/* check loop mode */
				if(_DATA->l_loop)
					_DATA->_current_frame = 0;
				else
				{
					/* set to last frame */
					_DATA->_current_frame = _DATA->_loaders[0]->frames_count - 1;

					/* exit play mode */
					_DATA->_playing = 0;
				};
			};

			/* sync cursor to frame number */
			_DATA->_loaders[0]->cursor = _DATA->_current_frame;
		}
		else
		{
			/* we are in async load mode */

			/* plan frames order in pipeline */
			for(i = 0, t = 0; i<((RING_BUFFER_LENGTH*3)/4) ; i++)
			{
				/* calc cursor and frame number */
				c = (_DATA->_loaders[0]->cursor + i) % RING_BUFFER_LENGTH;
				f = (_DATA->_current_frame + i) % _DATA->_loaders[0]->frames_count;

				/* check if position planned */
				if(_DATA->_loaders[0]->buf_frame[c] != f)
				{
					t++;
					_DATA->_loaders[0]->buf_frame[c] = f;
					_DATA->_loaders[0]->buf_fill[c] = 1;
					_DATA->_loaders[0]->buf_filled[c] = 0;
					_DATA->_loaders[0]->buf_clear[c] = 1;
				};
			};
			if(t) PulseEvent(_DATA->_loaders[0]->wakeup);
		
			/* try shift cursor */
			c = (_DATA->_loaders[0]->cursor + 1 + RING_BUFFER_LENGTH) % RING_BUFFER_LENGTH;
			if
			(
				(_DATA->_cursor_loaded == _DATA->_loaders[0]->cursor) /* current frame loaded */
				&&													/* and */
				(_DATA->_playing)									/* we are in playing mode */
				&&													/* and */
				(0 != _DATA->_loaders[0]->buf_filled[c])			/* frame is loaded */
			)
			{
#ifdef VERBOSE
				logger_printf(0, "avifile: trying to shift cursor");
#endif /* VERBOSE */

				/* determinate how shift frame position and cursor */
				if((_DATA->_current_frame + 1)< _DATA->_loaders[0]->frames_count)
				{
					_DATA->_current_frame++;
					_DATA->_loaders[0]->cursor = c;
#ifdef VERBOSE
					logger_printf(0, "avifile: shifted to current_frame=%d, _DATA->_loaders[0]->cursor=%d", _DATA->_current_frame, _DATA->_loaders[0]->cursor);
#endif /* VERBOSE */
				}
				else
				{
					if(_DATA->l_loop)
					{
						/* jump to first frame */
						_DATA->_current_frame = 0;
						_DATA->_loaders[0]->cursor = c;
#ifdef VERBOSE
						logger_printf(0, "avifile: loop condition detected");
#endif /* VERBOSE */

					}
					else
					{
						/* stop playing */
						_DATA->_playing = 0;
#ifdef VERBOSE
						logger_printf(0, "avifile: end of file detected");
#endif /* VERBOSE */
					};
				};
			}
			else
			{
#ifdef VERBOSE2
				logger_printf(0, "avifile: no condition to shift cursor");
#endif /* VERBOSE */

#ifdef VERBOSE2
			if(!(0 != _DATA->_loaders[0]->buf_filled[c]))
				logger_printf(0, "avifile: _DATA->_loaders[0]->buf_filled[%d] == %d", c, _DATA->_loaders[0]->buf_filled[c]);
#endif /* VERBOSE */

			};
		};
	};

	// release mutex
	ReleaseMutex(_DATA->_lock_update);

};

PLUGIN_EXPORT void render(void* data,vzRenderSession* session)
{
	// check if texture initialized
	if
	(
		(_DATA->_texture_initialized)
		&&
		(_DATA->_loaders[0])
		&&
		(_DATA->_loaders[0]->flag_ready)
	)
	{
		if (FOURCC_TO_LONG('_','F','T','_') == _DATA->L_center)
		{
			int i;
			double p;

			/* free transform mode */
			float X1,X2,X3,X4, Y1,Y2,Y3,Y4, Z1,Z2,Z3,Z4, XC,YC,ZC;

			/* reset Z */
			ZC = (Z1 = (Z2 = (Z3 = (Z4 = 0.0f))));

			/* calc coordiantes of image */
			calc_free_transform
			(
				/* dimentsions */
				_DATA->_width, _DATA->_height,
				_DATA->_loaders[0]->width, _DATA->_loaders[0]->height,

				/* source coordinates */
				_DATA->f_x1, _DATA->f_y1,
				_DATA->f_x2, _DATA->f_y2,
				_DATA->f_x3, _DATA->f_y3,
				_DATA->f_x4, _DATA->f_y4,

				/* destination coordinates */
				&X1, &Y1,
				&X2, &Y2,
				&X3, &Y3,
				&X4, &Y4
			);

			/* begin drawing */
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, _DATA->_texture);

			/* setup colour & alpha */
			glColor4f(1.0f,1.0f,1.0f,session->f_alpha);

			/* reinit arrays */
			if(_DATA->_ft_vertices) free(_DATA->_ft_vertices);
			if(_DATA->_ft_texels) free(_DATA->_ft_texels);
			_DATA->_ft_vertices = (float*)malloc(sizeof(float) * 3 * (2*_DATA->l_tr_lod + 2));
			_DATA->_ft_texels = (float*)malloc(sizeof(float) * 2 * (2*_DATA->l_tr_lod + 2));

			/* fill array */
			for(i = 0; i<= (2*_DATA->l_tr_lod + 1); i++)
			{
				p = ((double)(i/2))/_DATA->l_tr_lod;

				if(i & 1)
				{
					/* 'right' line */

					/* setup texture coords */
					_DATA->_ft_texels[i * 2 + 0] = 1.0f;
					_DATA->_ft_texels[i * 2 + 1] = p;

					/* calc vector coord */
					calc_ft_vec_part(X3,Y3, X4,Y4, p, &XC,&YC);
				}
				else
				{
					/* left line */

					/* setup texture coords */
					_DATA->_ft_texels[i * 2 + 0] = 0.0f;
					_DATA->_ft_texels[i * 2 + 1] = p;

					/* calc vector coord */
					calc_ft_vec_part(X2,Y2, X1,Y1, p, &XC,&YC);
				};

				_DATA->_ft_vertices[i * 3 + 0] = XC;
				_DATA->_ft_vertices[i * 3 + 1] = YC;
				_DATA->_ft_vertices[i * 3 + 2] = ZC;
			};

			/* enable vertex and texture coors array */
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);

			/* init vertext and texel array pointers */
			glVertexPointer(3, GL_FLOAT, 0, _DATA->_ft_vertices);
			glTexCoordPointer(2, GL_FLOAT, 0, _DATA->_ft_texels);

			/* draw array */
			glDrawArrays(GL_TRIANGLE_STRIP, 0,  (2*_DATA->l_tr_lod + 2) );

			/* enable vertex and texture coors array */
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);

			glDisable(GL_TEXTURE_2D);
		}
		else
		{
			/* no transform mode */
		
			// determine center offset 
			float co_X = 0.0f, co_Y = 0.0f, co_Z = 0.0f;

			// translate coordinates accoring to base image
			center_vector(_DATA->L_center, _DATA->_loaders[0]->width, _DATA->_loaders[0]->height, co_X, co_Y);

#ifdef VERBOSE2
			logger_printf(0, "avifile: center_vector: co_X = %f, co_Y = %f", co_X, co_Y);
#endif /* VERBOSE */

			// translate coordinate according to real image
			co_Y -= (_DATA->_height - _DATA->_loaders[0]->height)/2;
			co_X -= (_DATA->_width - _DATA->_loaders[0]->width)/2;

#ifdef VERBOSE2
			if(_DATA->_playing)
				logger_printf
				(
					0,
					"avifile: translate: co_X = %f, co_Y = %f "
					"(_DATA->_loaders[0]->width=%d, _DATA->_loaders[0]->height=%d", 
					co_X, co_Y,
					_DATA->_loaders[0]->width, _DATA->_loaders[0]->height
				); 
#endif /* VERBOSE */

			// begin drawing
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, _DATA->_texture);

			// Draw a quad (ie a square)
			glBegin(GL_QUADS);

			glColor4f(1.0f,1.0f,1.0f,session->f_alpha);

			glTexCoord2f
			(
				(_DATA->l_flip_h)?1.0f:0.0f, 
				(_DATA->l_flip_v)?0.0f:1.0f
			);
			glVertex3f(co_X + 0.0f, co_Y + 0.0f, co_Z + 0.0f);

			glTexCoord2f
			(
				(_DATA->l_flip_h)?1.0f:0.0f,
				(_DATA->l_flip_v)?1.0f:0.0f
			);
			glVertex3f(co_X + 0.0f, co_Y + _DATA->_height, co_Z + 0.0f);

			glTexCoord2f
			(
				(_DATA->l_flip_h)?0.0f:1.0f,
				(_DATA->l_flip_v)?1.0f:0.0f
			);
			glVertex3f(co_X + _DATA->_width, co_Y + _DATA->_height, co_Z + 0.0f);

			glTexCoord2f
			(
				(_DATA->l_flip_h)?0.0f:1.0f,
				(_DATA->l_flip_v)?0.0f:1.0f
			);
			glVertex3f(co_X + _DATA->_width, co_Y + 0.0f, co_Z + 0.0f);

			glEnd(); // Stop drawing QUADS

			glDisable(GL_TEXTURE_2D);
		};
	};
};

PLUGIN_EXPORT void notify(void* data, char* param_name)
{
	//wait for mutext free
	WaitForSingleObject(_DATA->_lock_update,INFINITE);

	/* check if avi filename is given */
	if
	(
		(NULL == param_name)
		||
		(0 == strcmp(param_name, "s_filename"))
	)
	{
		/* change current frame position*/
		_DATA->_current_frame = _DATA->l_start_frame;
		_DATA->_cursor_loaded = 0xFFFFFFFF;
		_DATA->_playing = _DATA->l_auto_play;

		/* create loader */
		struct aviloader_desc* loader = aviloader_init(_DATA->s_filename, _DATA->l_mem_preload);
/*		_DATA->_filename = _DATA->s_filename; */

		/* check second slot */
		if(_DATA->_loaders[1])
		{
			aviloader_destroy(_DATA->_loaders[1]);
			_DATA->_loaders[1] = NULL;
		};

		/* shift loaders */
		_DATA->_loaders[1] = _DATA->_loaders[0];
		_DATA->_loaders[0] = loader;

		/* notify old loader to die */
		if(_DATA->_loaders[1])
			_DATA->_loaders[1]->flag_exit = 1;
	};

	/* control triggers */
	if(_DATA->l_trig_play)
	{
		_DATA->_current_frame = _DATA->l_start_frame;
		_DATA->_cursor_loaded = 0xFFFFFFFF;
		_DATA->_playing = 1;

		if(_DATA->l_mem_preload)
			_DATA->_loaders[0]->cursor = _DATA->_current_frame;

		/* reset trigger */
		_DATA->l_trig_play = 0;
	};
	if(_DATA->l_trig_cont)
	{
		_DATA->_playing = 1;

		/* reset trigger */
		_DATA->l_trig_cont = 0;
	};
	if(_DATA->l_trig_stop)
	{
		_DATA->_playing = 0;

		/* reset trigger */
		_DATA->l_trig_stop = 0;
	};
	if(0xFFFFFFFF != _DATA->l_trig_jump)
	{
		if
		(
			(_DATA->_loaders[0])
			&&
			(_DATA->_loaders[0]->flag_ready)
		)
		{
			/* loader ready */

			/* check range */
			if(_DATA->l_trig_jump < 0)
				_DATA->_current_frame = 0;
			else if (_DATA->l_trig_jump >= _DATA->_loaders[0]->frames_count)
				_DATA->_current_frame = _DATA->_loaders[0]->frames_count - 1;
			else
				_DATA->_current_frame = _DATA->l_trig_jump;

			/* sync cursor */
			if(_DATA->l_mem_preload)
				_DATA->_loaders[0]->cursor = _DATA->_current_frame;
		};
		
		/* reset trigger */
		_DATA->l_trig_jump = 0xFFFFFFFF;
	};

	// release mutex -  let created tread work
	ReleaseMutex(_DATA->_lock_update);

};
