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
#include <comutil.h>
#include <stdio.h>
#include <errno.h>

#include "../vz/vzOutput.h"
#include "../vz/vzOutputModule.h"
#include "../vz/vzMain.h"
#include "../vz/vzLogger.h"

#define THIS_MODULE "decklink"
#define THIS_MODULE_PREF "decklink: "

#include "DeckLinkAPI.h"
#include "DeckLinkAPI_i.c"

#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "winmm")

#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

static const int bmd_modes_id[] =
{
    bmdModeNTSC,
    bmdModeNTSC2398,
    bmdModePAL,
    bmdModeHD1080p2398,
    bmdModeHD1080p24,
    bmdModeHD1080p25,
    bmdModeHD1080p2997,
    bmdModeHD1080p30,
    bmdModeHD1080i50,
    bmdModeHD1080i5994,
    bmdModeHD1080i6000,
    bmdModeHD1080p50,
    bmdModeHD1080p5994,
    bmdModeHD1080p6000,
    bmdModeHD720p50,
    bmdModeHD720p5994,
    bmdModeHD720p60,
    bmdMode2k2398,
    bmdMode2k24,
    bmdMode2k25,
    0
};

static const char* bmd_modes_name[] =
{
    "NTSC",
    "NTSC2398",
    "PAL",
    "HD1080p2398",
    "HD1080p24",
    "HD1080p25",
    "HD1080p2997",
    "HD1080p30",
    "HD1080i50",
    "HD1080i5994",
    "HD1080i6000",
    "HD1080p50",
    "HD1080p5994",
    "HD1080p6000",
    "HD720p50",
    "HD720p5994",
    "HD720p60",
    "2k2398",
    "2k24",
    "2k25",
    0
};

#define MAX_INPUTS  4
#define MAX_BOARDS  8
#define PREROLL     3
#define MAX_INPUT_BUFS 4

class decklink_input_class;
class decklink_output_class;

typedef struct decklink_runtime_context_desc
{
    int f_exit;
    vzTVSpec* tv;
    void* config;
    void* output_context;

    struct
    {
        long long cnt;
        HANDLE sync_event;
        unsigned long* sync_cnt;
        IDeckLink* board;
        IDeckLinkOutput* io;
        IDeckLinkKeyer* keyer;
        decklink_output_class* cb;
        BMDDisplayMode mode;
        BMDTimeValue dur;
        BMDTimeScale ts;
        IDeckLinkMutableVideoFrame* blank_frame;
    } output;

    struct
    {
        int idx;
        IDeckLink* board;
        IDeckLinkInput* io;
        decklink_input_class* cb;
        BMDDisplayMode mode;
        int pos;
        vzImage *buf;
        vzImage *bufs[MAX_INPUT_BUFS];
        int interlaced;
        long long cnt;
    } inputs[MAX_INPUTS];
    int inputs_count;

} decklink_runtime_context_t;

static int decklink_VideoInputFormatChanged
(
    decklink_runtime_context_t* ctx, int idx,
    BMDVideoInputFormatChangedEvents notificationEvents,
    IDeckLinkDisplayMode* newDisplayMode,
    BMDDetectedVideoInputFormatFlags detectedSignalFlags
)
{
    switch(notificationEvents)
    {
        case bmdVideoInputDisplayModeChanged:
            logger_printf(0, THIS_MODULE_PREF "input[%d]: InputDisplayModeChanged", idx);
            break;
        case bmdVideoInputFieldDominanceChanged:
            logger_printf(0, THIS_MODULE_PREF "input[%d]: FieldDominanceChanged", idx);
            break;
        case bmdVideoInputColorspaceChanged:
            logger_printf(0, THIS_MODULE_PREF "input[%d]: ColorspaceChange", idx);
            break;
        default:
            logger_printf(0, THIS_MODULE_PREF "input[%d]: <format changed>", idx);
            break;
    };

    switch(detectedSignalFlags)
    {
        case bmdDetectedVideoInputYCbCr422:
            logger_printf(0, THIS_MODULE_PREF "input[%d]: VideoInputYCbCr422", idx);
            break;
        case bmdDetectedVideoInputRGB444:
            logger_printf(0, THIS_MODULE_PREF "input[%d]: VideoInputRGB444", idx);
            break;
        default:
            logger_printf(0, THIS_MODULE_PREF "input[%d]: <signal changed>", idx);
            break;
    };

    return 0;
};

static int decklink_VideoInputFrameArrived
(
    decklink_runtime_context_t* ctx, int idx,
    IDeckLinkVideoInputFrame* pArrivedFrame,
    IDeckLinkAudioInputPacket* pAudio
)
{
    int i;
    vzImage src;

    /* init images on first call */
    if(!ctx->inputs[idx].bufs[0])
        for(i = 0; i < MAX_INPUT_BUFS; i++)
            vzImageCreate(&ctx->inputs[idx].bufs[i],
                pArrivedFrame->GetWidth(), pArrivedFrame->GetHeight());

    if(!ctx->inputs[idx].buf)
        ctx->inputs[idx].buf = ctx->inputs[idx].bufs[ctx->inputs[idx].pos++];

    /* setup basic parameters */
    src.width = src.base_width = pArrivedFrame->GetWidth();
    src.height = src.base_height = pArrivedFrame->GetHeight();
    src.line_size = pArrivedFrame->GetRowBytes();
    pArrivedFrame->GetBytes(&src.surface);
    src.bpp = 2;
    src.pix_fmt = VZIMAGE_PIXFMT_UYVY;
    src.interlaced = ctx->inputs[idx].interlaced;

    i = vzImageConv_UYVY_to_BGRA(&src, ctx->inputs[idx].buf);

    ctx->inputs[idx].buf->sys_id = ctx->inputs[idx].cnt++;

    vzOutputInputPush(ctx->output_context, ctx->inputs[idx].idx,
        (void**)&ctx->inputs[idx].buf);

    return 0;
};

static int decklink_ScheduleNextFrame2(decklink_runtime_context_t* ctx)
{
    /* send frame */
    ctx->output.io->ScheduleVideoFrame
    (
        ctx->output.blank_frame,
        ctx->output.cnt * ctx->output.dur,
        ctx->output.dur, ctx->output.ts
    );

    ctx->output.cnt++;

    return 0;
};

static int decklink_ScheduleNextFrame(decklink_runtime_context_t* ctx)
{
    vzImage img;
    IDeckLinkMutableVideoFrame* frame;

    /* request frame */
    vzOutputOutGet(ctx->output_context, &img);

    /* create frame */
    if(S_OK != ctx->output.io->CreateVideoFrame(
        img.width, img.height, img.line_size,
        bmdFormat8BitBGRA, bmdFrameFlagFlipVertical, &frame))
    {
        logger_printf(1, THIS_MODULE_PREF "CreateVideoFrame(%d, %d, %d) failed",
            img.width, img.height, img.line_size);
    }
    else
    {
        int i;
        unsigned char* dst;

        if(S_OK == frame->GetBytes((void**)&dst) && dst)
        {
            /* copy data */
            memcpy(dst, img.surface, img.height * img.line_size);

            /* send frame */
            ctx->output.io->ScheduleVideoFrame
            (
                frame,
                ctx->output.cnt * ctx->output.dur,
                ctx->output.dur, ctx->output.ts
            );
        }
        else
            logger_printf(1, THIS_MODULE_PREF "frame->GetBytes failed");

        frame->Release();
    };

    /* release frame back */
    vzOutputOutRel(ctx->output_context, &img);

    ctx->output.cnt++;

    /* notify main */
    *ctx->output.sync_cnt = 1 + *ctx->output.sync_cnt;
    PulseEvent(ctx->output.sync_event);

    return 0;
};

static int decklink_ScheduledFrameCompleted
(
    decklink_runtime_context_t* ctx,
    IDeckLinkVideoFrame *completedFrame,
    BMDOutputFrameCompletionResult result
)
{
    // ignore handler if frame was flushed
    if(bmdOutputFrameFlushed == result)
        return 0;

    // step forward frames counter if underrun
    if(bmdOutputFrameDisplayedLate == result)
    {
#ifdef _DEBUG
        logger_printf(1, THIS_MODULE_PREF "ScheduledFrameCompleted: bmdOutputFrameDisplayedLate");
#endif
        ctx->output.cnt++;
    };

    if(bmdOutputFrameDropped == result)
    {
        logger_printf(1, THIS_MODULE_PREF "ScheduledFrameCompleted: bmdOutputFrameDropped");
        ctx->output.cnt++;
    };

    // schedule next frame
    decklink_ScheduleNextFrame(ctx);

    return 0;
};

static int decklink_ScheduledPlaybackHasStopped
(
    decklink_runtime_context_t* ctx
)
{
    return 0;
};


#include "decklink_input_class.h"
#include "decklink_output_class.h"

static int decklink_init(void** pctx, void* obj, void* config, vzTVSpec* tv)
{
    char* o;
    int c = 0, i, j;
    HRESULT hr;
    IDeckLink* deckLink;
    IDeckLink* deckLinks[MAX_BOARDS];
    IDeckLinkIterator *deckLinkIterator = NULL;
    BMDDisplayMode displayMode = bmdModePAL;
    BMDDisplayModeSupport displayModeSupport;
    decklink_runtime_context_t *ctx;

    if(!pctx)
        return -EINVAL;

    /* allocate module context */
    ctx = (decklink_runtime_context_t *)malloc(sizeof(decklink_runtime_context_t));
    if(!ctx)
        return -ENOMEM;

    /* clear context */
    memset(ctx, 0, sizeof(decklink_runtime_context_t));

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if(FAILED(hr))
    {
        logger_printf(1, THIS_MODULE_PREF "CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) failed");
        return -EFAULT;
    };

    timeBeginPeriod(1);

    /* clear boards list */
    memset(deckLinks, 0, sizeof(IDeckLink*) * MAX_BOARDS);

    /* setup basic components */
    ctx->config = config;
    ctx->tv = tv;
    ctx->output_context = obj;

    /* create iterator */
    hr = CoCreateInstance(CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL,
        IID_IDeckLinkIterator, (void**)&deckLinkIterator);
    if(hr != S_OK || !deckLinkIterator)
    {
        logger_printf(1, THIS_MODULE_PREF "CoCreateInstance(CLSID_CDeckLinkIterator) failed");
        return -1;
    };

    // Enumerate all cards in this system
    while(S_OK == deckLinkIterator->Next(&deckLink) && c < MAX_BOARDS)
    {
        char* m;
        BSTR deviceNameBSTR;

        /* get name */
        deckLink->GetModelName(&deviceNameBSTR);
        m = _com_util::ConvertBSTRToString(deviceNameBSTR);
        logger_printf(0, THIS_MODULE_PREF "board[%d]: %s", c, m);
        SysFreeString(deviceNameBSTR);
        delete[] m;

        /* save into list */
        deckLinks[c] = deckLink; c++;
    };

    /* check output */
    o = (char*)vzConfigParam(ctx->config, THIS_MODULE, "ENABLE_OUTPUT");
    if(o)
    {
        i = atol(o);
        if(i < 0 || i >= c)
            logger_printf(1, THIS_MODULE_PREF "board[%d]: out of range, no output defined", i);
        else
        {
            // Obtain the video output interface
            hr = deckLinks[i]->QueryInterface(IID_IDeckLinkOutput, (void**)&ctx->output.io);
            if(hr != S_OK || !ctx->output.io)
                logger_printf(1, THIS_MODULE_PREF "board[%d]: failed to obtain IID_IDeckLinkOutput", i);
            else
            {
                /* check if mode supported */
                if(!_stricmp("576i", ctx->tv->NAME)) displayMode = bmdModePAL;
                else if(!_stricmp("576p", ctx->tv->NAME)) displayMode = bmdModePAL;
                else if(!_stricmp("480i", ctx->tv->NAME)) displayMode = bmdModeNTSC;
                else if(!_stricmp("480p", ctx->tv->NAME)) displayMode = bmdModeNTSC;
                else if(!_stricmp("720p50", ctx->tv->NAME)) displayMode = bmdModeHD720p50;
                else if(!_stricmp("1080i25", ctx->tv->NAME)) displayMode = bmdModeHD1080i50;
                else if(!_stricmp("1080p25", ctx->tv->NAME)) displayMode = bmdModeHD1080p25;
                else if(!_stricmp("1080p50", ctx->tv->NAME)) displayMode = bmdModeHD1080p50;
                ctx->output.io->DoesSupportVideoMode(displayMode, bmdFormat8BitYUV,
                    bmdVideoOutputFlagDefault, &displayModeSupport, NULL);
                if(bmdDisplayModeSupported != displayModeSupport)
                {
                    logger_printf(1, THIS_MODULE_PREF "board[%d]: do not support [%s] mode",
                        i, ctx->tv->NAME);
                    SAFE_RELEASE(ctx->output.io);
                }
                else
                {
                    /* setup output datas */
                    ctx->output.mode = displayMode;
                    ctx->output.board = deckLinks[i];
                    deckLinks[i] = NULL;

                    /* request keyer */
                    ctx->output.board->QueryInterface(IID_IDeckLinkKeyer,
                        (void**)&ctx->output.keyer);

                    /* create blank frame */
                    ctx->output.io->CreateVideoFrame(
                        ctx->tv->TV_FRAME_WIDTH,
                        ctx->tv->TV_FRAME_HEIGHT,
                        ctx->tv->TV_FRAME_WIDTH * 4,
                        bmdFormat8BitBGRA, bmdFrameFlagFlipVertical,
                        &ctx->output.blank_frame);
                };
            };
        };
    };

    /* check inputs */
    for(j = 0; j < MAX_INPUTS; j++)
    {
        int p;
        char name[32], m[32];

        /* compose parameter name */
        sprintf(name, "ENABLE_INPUT_%d", j + 1);

        /* request param */
        o = (char*)vzConfigParam(ctx->config, THIS_MODULE, name);
        if(!o) continue;

        /* parse */
        if(2 != sscanf(o, "%d:%s", &i, m))
        {
            logger_printf(1, THIS_MODULE_PREF "failed to parse ENABLE_INPUT_%d=\"%s\"", j, o);
            continue;
        };
        strncpy(name, m, 32);

        /* index range */
        if(i < 0 || i >= c)
        {
            logger_printf(0, THIS_MODULE_PREF "ENABLE_INPUT_%d specifies out of range index %d", j, i);
            continue;
        };

        /* check if used */
        if(!deckLinks[i])
        {
            logger_printf(0, THIS_MODULE_PREF "ENABLE_INPUT_%d specifies already used index %d", j, i);
            continue;
        };

        // Obtain the audio/video input interface
        hr = deckLinks[i]->QueryInterface(IID_IDeckLinkInput,
            (void**)&ctx->inputs[ctx->inputs_count].io);
        if(S_OK != hr)
        {
            logger_printf(1, THIS_MODULE_PREF "board[%d]: failed to obtain IID_IDeckLinkInput", i);
            continue;
        };

        // find mode
        for(p = 0; bmd_modes_name[p]; p++)
            if(!_stricmp(bmd_modes_name[p], name))
                break;
        if(!bmd_modes_name[p])
        {
            logger_printf(1, THIS_MODULE_PREF "board[%d]: video mode [%s] not recognized",
                i, name);
            SAFE_RELEASE(ctx->inputs[ctx->inputs_count].io);
            continue;
        };

        /* check if mode supported */
        ctx->inputs[ctx->inputs_count].io->DoesSupportVideoMode(
            (BMDDisplayMode)bmd_modes_id[p], bmdFormat8BitYUV,
            bmdVideoInputFlagDefault, &displayModeSupport, NULL);
        if(bmdDisplayModeSupported != displayModeSupport)
        {
            logger_printf(1, THIS_MODULE_PREF "board[%d]: do not support [%s] input mode",
                i, bmd_modes_name[p]);
            SAFE_RELEASE(ctx->inputs[ctx->inputs_count].io);
            continue;
        }

        /* set params */
        ctx->inputs[ctx->inputs_count].mode = (BMDDisplayMode)bmd_modes_id[p];
        ctx->inputs[ctx->inputs_count].board = deckLinks[i];

        /* set interlaced */
        switch(ctx->inputs[ctx->inputs_count].mode)
        {
            case bmdModeHD1080i50:
            case bmdModePAL:
                ctx->inputs[ctx->inputs_count].interlaced = VZIMAGE_INTERLACED_U;
                break;

            case bmdModeHD1080i5994:
            case bmdModeHD1080i6000:
            case bmdModeNTSC:
            case bmdModeNTSC2398:
                ctx->inputs[ctx->inputs_count].interlaced = VZIMAGE_INTERLACED_L;
                break;
            default:
                ctx->inputs[ctx->inputs_count].interlaced = VZIMAGE_INTERLACED_NONE;
        };

        /* save decklink pointer */
        deckLinks[i] = NULL;
        ctx->inputs_count++;
    };

    /* release unused boards intstances */
    for(i = 0; i < c; i++)
        SAFE_RELEASE(deckLinks[i]);

    SAFE_RELEASE(deckLinkIterator);

    /* return module context */
    *pctx = ctx;

    return 0;
};

static int decklink_release(void** pctx, void* obj, void* config, vzTVSpec* tv)
{
    int i, j;
    decklink_runtime_context_t *ctx;

    if(!pctx)
        return -EINVAL;

    ctx = (decklink_runtime_context_t *)(*pctx);
    *pctx = NULL;

    if(!ctx)
        return -EINVAL;

    SAFE_RELEASE(ctx->output.io);
    SAFE_RELEASE(ctx->output.board);
    SAFE_RELEASE(ctx->output.keyer);
    SAFE_RELEASE(ctx->output.blank_frame);
    if(ctx->output.cb) delete ctx->output.cb;

    for(i = 0; i < ctx->inputs_count; i++)
    {
        SAFE_RELEASE(ctx->inputs[i].io);
        SAFE_RELEASE(ctx->inputs[i].board);
        if(ctx->inputs[i].cb) delete ctx->inputs[i].cb;
        for(j = 0; j < MAX_INPUT_BUFS; j++)
            if(ctx->inputs[i].bufs[j])
                vzImageRelease(&ctx->inputs[i].bufs[j]);
    };

    free(ctx);

    return 0;
};

static int decklink_setup(void* pctx, HANDLE* sync_event, unsigned long** sync_cnt)
{
    int i;
    decklink_runtime_context_t *ctx;

    if(!pctx)
        return -EINVAL;

    ctx = (decklink_runtime_context_t *)pctx;

    if(!ctx)
        return -EINVAL;

    if(ctx->output.io && sync_event)
    {
        ctx->output.sync_event = *sync_event;
        *sync_event = NULL;

        ctx->output.sync_cnt = *sync_cnt;
        *sync_cnt = NULL;

        if(!ctx->output.keyer)
            logger_printf(1, THIS_MODULE_PREF "output keyer not supported");
        else
        {
            if(vzConfigParam(ctx->config, THIS_MODULE, "ONBOARD_KEYER"))
            {
                if(S_OK != ctx->output.keyer->Enable(0))
                    logger_printf(1, THIS_MODULE_PREF "failed to enable INTERNAL keyer");
            }
            else
            {
                if(S_OK != ctx->output.keyer->Enable(1))
                    logger_printf(1, THIS_MODULE_PREF "failed to enable EXTERNAL keyer");
            };
            ctx->output.keyer->SetLevel(255);
        };
    }
    else
        logger_printf(1, THIS_MODULE_PREF "output disable: either not activated or activated another module");

    return 0;
};

static int decklink_run(void* pctx)
{
    int i;
    HRESULT hr;
    decklink_runtime_context_t *ctx;

    if(!pctx)
        return -EINVAL;

    ctx = (decklink_runtime_context_t *)pctx;

    /* run output */
    if(ctx->output.sync_event)
    {
        IDeckLinkDisplayMode* mode;
        IDeckLinkDisplayModeIterator* iter;

        /* assign callback */
        ctx->output.cb = new decklink_output_class(ctx);
        ctx->output.io->SetScheduledFrameCompletionCallback(ctx->output.cb);

        /* find mode timings */
        ctx->output.io->GetDisplayModeIterator(&iter);
        while(iter->Next(&mode) == S_OK)
        {
            if(mode->GetDisplayMode() != ctx->output.mode)
                continue;

            mode->GetFrameRate(&ctx->output.dur, &ctx->output.ts);

            break;
        };
        SAFE_RELEASE(iter);
        if(!ctx->output.dur)
        {
            logger_printf(1, THIS_MODULE_PREF "failed to find output mode timings");
            return -EINVAL;
        };

        /* enable video output */
        if(S_OK != ctx->output.io->EnableVideoOutput(ctx->output.mode, bmdVideoOutputFlagDefault))
        {
            logger_printf(1, THIS_MODULE_PREF "EnableVideoOutput FAILED");
            return -EINVAL;
        };

        /* send a preroll frames */
        for(i = 0; i < PREROLL; i++)
            decklink_ScheduleNextFrame2(ctx);

        /* start sheduled playback */
        ctx->output.io->StartScheduledPlayback( 0, ctx->output.ts, 1.0 );
    };

    /* setup inputs */
    for(i = 0; i < ctx->inputs_count; i++)
    {
        /* register input */
        ctx->inputs[i].idx = vzOutputInputAdd(ctx->output_context);
        if(ctx->inputs[i].idx < 0)
        {
            logger_printf(1, THIS_MODULE_PREF "vzOutputInputAdd failed with %d",
                ctx->inputs[i].idx);
            continue;
        };

        /* setup callbacks */
        ctx->inputs[i].cb = new decklink_input_class(ctx, i);
        hr = ctx->inputs[i].io->SetCallback(ctx->inputs[i].cb);
        if(S_OK != hr)
        {
            logger_printf(1, THIS_MODULE_PREF "SetCallback failed");
            continue;
        };

        hr = ctx->inputs[i].io->EnableVideoInput(ctx->inputs[i].mode,
            bmdFormat8BitYUV, /* bmdVideoInputEnableFormatDetection */ bmdVideoInputFlagDefault);
        if(S_OK != hr)
        {
            logger_printf(1, THIS_MODULE_PREF "EnableVideoInput failed");
            continue;
        };

        hr = ctx->inputs[i].io->StartStreams();
        if(S_OK != hr)
        {
            logger_printf(1, THIS_MODULE_PREF "StartStreams failed");
            continue;
        };

        logger_printf(0, THIS_MODULE_PREF "ENABLE_INPUT_%d will be mapped to liveinput %d",
            i, ctx->inputs[i].idx);
    };

    return 0;
};

static int decklink_stop(void* pctx)
{
    int i;
    decklink_runtime_context_t *ctx;

    if(!pctx)
        return -EINVAL;

    ctx = (decklink_runtime_context_t *)pctx;

    for(i = 0; i < ctx->inputs_count; i++)
    {
        ctx->inputs[i].io->StopStreams();
        ctx->inputs[i].io->DisableVideoInput();
    };

    if(ctx->output.sync_event)
    {
        ctx->output.io->StopScheduledPlayback(0, 0, 0);
        ctx->output.io->DisableVideoOutput();
    };

    return 0;
};

extern "C" __declspec(dllexport) vzOutputModule_t vzOutputModule =
{
    decklink_init,
    decklink_release,
    decklink_setup,
    decklink_run,
    decklink_stop
};
