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
        vzImage bufs[4];
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
    if(!ctx->inputs[idx].buf)
        ctx->inputs[idx].buf = &ctx->inputs[idx].bufs[ctx->inputs[idx].pos++];

    pArrivedFrame->AddRef();

    ctx->inputs[idx].buf->width = ctx->inputs[idx].buf->base_width =
        pArrivedFrame->GetWidth();
    ctx->inputs[idx].buf->height = ctx->inputs[idx].buf->base_height =
        pArrivedFrame->GetHeight();
    ctx->inputs[idx].buf->bpp = 4;
    ctx->inputs[idx].buf->pix_fmt = VZIMAGE_PIXFMT_BGRA;
    pArrivedFrame->GetBytes(&ctx->inputs[idx].buf->surface);
    ctx->inputs[idx].buf->line_size = pArrivedFrame->GetRowBytes();
    ctx->inputs[idx].buf->priv = pArrivedFrame;

    vzOutputInputPush(ctx->output_context, ctx->inputs[idx].idx,
        (void**)&ctx->inputs[idx].buf);

    if(ctx->inputs[idx].buf)
        ((IDeckLinkVideoInputFrame*)ctx->inputs[idx].buf->priv)->Release();

    return 0;
};

static int decklink_ScheduleNextFrame(decklink_runtime_context_t* ctx, int is_preroll)
{
    vzImage img;
    IDeckLinkMutableVideoFrame* frame;

    /* request frame */
    vzOutputOutGet(ctx->output_context, &img);

    /* create frame */
    if(S_OK != ctx->output.io->CreateVideoFrame(
        img.width, img.height, img.line_size,
        bmdFormat8BitARGB, bmdFrameFlagDefault, &frame))
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
        logger_printf(1, THIS_MODULE_PREF "ScheduledFrameCompleted: bmdOutputFrameDisplayedLate");
//        ctx->output.cnt++;
    };

    if(bmdOutputFrameDropped == result)
    {
        logger_printf(1, THIS_MODULE_PREF "ScheduledFrameCompleted: bmdOutputFrameDropped");
        ctx->output.cnt++;
    };

    // schedule next frame
    decklink_ScheduleNextFrame(ctx, false);

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

static decklink_runtime_context_t ctx;

static int decklink_init(void* obj, void* config, vzTVSpec* tv)
{
    char* o;
    int c = 0, i, j;
    HRESULT hr;
    IDeckLink* deckLink;
    IDeckLink* deckLinks[MAX_BOARDS];
    IDeckLinkIterator *deckLinkIterator = NULL;
    BMDDisplayMode displayMode = bmdModePAL;
    BMDDisplayModeSupport displayModeSupport;

    /* clear context */
    memset(&ctx, 0, sizeof(decklink_runtime_context_t));

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
    ctx.config = config;
    ctx.tv = tv;
    ctx.output_context = obj;

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
    o = (char*)vzConfigParam(ctx.config, THIS_MODULE, "ENABLE_OUTPUT");
    if(o)
    {
        i = atol(o);
        if(i < 0 || i >= c)
            logger_printf(1, THIS_MODULE_PREF "board[%d]: out of range, no output defined", i);
        else
        {
            // Obtain the video output interface
            hr = deckLinks[i]->QueryInterface(IID_IDeckLinkOutput, (void**)&ctx.output.io);
            if(hr != S_OK || !ctx.output.io)
                logger_printf(1, THIS_MODULE_PREF "board[%d]: failed to obtain IID_IDeckLinkOutput", i);
            else
            {
                /* check if mode supported */
                if(!_stricmp("576i", ctx.tv->NAME)) displayMode = bmdModePAL;
                else if(!_stricmp("576p", ctx.tv->NAME)) displayMode = bmdModePAL;
                else if(!_stricmp("480i", ctx.tv->NAME)) displayMode = bmdModeNTSC;
                else if(!_stricmp("480p", ctx.tv->NAME)) displayMode = bmdModeNTSC;
                else if(!_stricmp("720p50", ctx.tv->NAME)) displayMode = bmdModeHD720p50;
                else if(!_stricmp("1080i25", ctx.tv->NAME)) displayMode = bmdModeHD1080p25;
                else if(!_stricmp("1080p25", ctx.tv->NAME)) displayMode = bmdModeHD1080p25;
                else if(!_stricmp("1080p50", ctx.tv->NAME)) displayMode = bmdModeHD1080p50;
                ctx.output.io->DoesSupportVideoMode(displayMode, bmdFormat8BitYUV,
                    bmdVideoOutputFlagDefault, &displayModeSupport, NULL);
                if(bmdDisplayModeSupported != displayModeSupport)
                {
                    logger_printf(1, THIS_MODULE_PREF "board[%d]: do not support [%s] mode",
                        i, ctx.tv->NAME);
                    SAFE_RELEASE(ctx.output.io);
                }
                else
                {
                    /* setup output datas */
                    ctx.output.mode = displayMode;
                    ctx.output.board = deckLinks[i];
                    deckLinks[i] = NULL;

                    /* request keyer */
                    ctx.output.board->QueryInterface(IID_IDeckLinkKeyer,
                        (void**)&ctx.output.keyer);

                };
            };
        };
    };

    /* check inputs */
    for(j = 0; j < MAX_INPUTS; j++)
    {
        char name[32], m[32];

        /* compose parameter name */
        sprintf(name, "ENABLE_INPUT_%d", j + 1);

        /* request param */
        o = (char*)vzConfigParam(ctx.config, THIS_MODULE, name);
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
            (void**)&ctx.inputs[ctx.inputs_count].io);
        if(S_OK != hr)
        {
            logger_printf(1, THIS_MODULE_PREF "board[%d]: failed to obtain IID_IDeckLinkInput", i);
            continue;
        };

        // find mode
        for(j = 0; bmd_modes_name[j]; j++)
            if(!_stricmp(bmd_modes_name[j], name))
                break;
        if(!bmd_modes_name[j])
        {
            logger_printf(1, THIS_MODULE_PREF "board[%d]: video mode [%s] not recognized",
                i, name);
            SAFE_RELEASE(ctx.inputs[ctx.inputs_count].io);
            continue;
        };

        /* check if mode supported */
        ctx.inputs[ctx.inputs_count].io->DoesSupportVideoMode(
            (BMDDisplayMode)bmd_modes_id[j], bmdFormat8BitYUV,
            bmdVideoInputFlagDefault, &displayModeSupport, NULL);
        if(bmdDisplayModeSupported != displayModeSupport)
        {
            logger_printf(1, THIS_MODULE_PREF "board[%d]: do not support [%s] input mode",
                i, bmd_modes_name[j]);
            SAFE_RELEASE(ctx.inputs[ctx.inputs_count].io);
            continue;
        }

        /* set params */
        ctx.inputs[ctx.inputs_count].mode = (BMDDisplayMode)bmd_modes_id[j];
        ctx.inputs[ctx.inputs_count].board = deckLinks[i];
        deckLinks[i] = NULL;
        ctx.inputs_count++;
    };

    /* release unused boards intstances */
    for(i = 0; i < c; i++)
        SAFE_RELEASE(deckLinks[i]);

    SAFE_RELEASE(deckLinkIterator);

    return 0;
};

static int decklink_release(void* obj, void* config, vzTVSpec* tv)
{
    int i;

    SAFE_RELEASE(ctx.output.io);
    SAFE_RELEASE(ctx.output.board);
    SAFE_RELEASE(ctx.output.keyer);
    if(ctx.output.cb) delete ctx.output.cb;

    for(i = 0; i < ctx.inputs_count; i++)
    {
        SAFE_RELEASE(ctx.inputs[i].io);
        SAFE_RELEASE(ctx.inputs[i].board);
        if(ctx.inputs[i].cb) delete ctx.inputs[i].cb;
    };

    return 0;
};

static int decklink_setup(HANDLE* sync_event, unsigned long** sync_cnt)
{
    int i;

    if(ctx.output.io && sync_event)
    {
        ctx.output.sync_event = *sync_event;
        *sync_event = NULL;

        ctx.output.sync_cnt = *sync_cnt;
        *sync_cnt = NULL;

        if(!ctx.output.keyer)
            logger_printf(1, THIS_MODULE_PREF "output keyer not supported");
        else
        {
            if(vzConfigParam(ctx.config, THIS_MODULE, "ONBOARD_KEYER"))
            {
                if(S_OK != ctx.output.keyer->Enable(0))
                    logger_printf(1, THIS_MODULE_PREF "failed to enable INTERNAL keyer");
            }
            else
            {
                if(S_OK != ctx.output.keyer->Enable(1))
                    logger_printf(1, THIS_MODULE_PREF "failed to enable EXTERNAL keyer");
            };
            ctx.output.keyer->SetLevel(255);
        };
    }
    else
        logger_printf(1, THIS_MODULE_PREF "output disable: either not activated or activated another module");

    return 0;
};

static int decklink_run()
{
    int i;

    /* run output */
    if(ctx.output.sync_event)
    {
        IDeckLinkDisplayMode* mode;
        IDeckLinkDisplayModeIterator* iter;

        /* assign callback */
        ctx.output.cb = new decklink_output_class(&ctx);
        ctx.output.io->SetScheduledFrameCompletionCallback(ctx.output.cb);

        /* find mode timings */
        ctx.output.io->GetDisplayModeIterator(&iter);
        while(iter->Next(&mode) == S_OK)
        {
            if(mode->GetDisplayMode() != ctx.output.mode)
                continue;

            mode->GetFrameRate(&ctx.output.dur, &ctx.output.ts);

            break;
        };
        SAFE_RELEASE(iter);
        if(!ctx.output.dur)
        {
            logger_printf(1, THIS_MODULE_PREF "failed to find output mode timings");
            return -EINVAL;
        };

        /* enable video output */
        if(S_OK != ctx.output.io->EnableVideoOutput(ctx.output.mode, bmdVideoOutputFlagDefault))
        {
            logger_printf(1, THIS_MODULE_PREF "EnableVideoOutput FAILED");
            return -EINVAL;
        };

        /* send a preroll frames */
        for(i = 0; i < PREROLL; i++)
            decklink_ScheduleNextFrame(&ctx, 1);

        /* start sheduled playback */
        ctx.output.io->StartScheduledPlayback( 0, ctx.output.ts, 1.0 );
    };

    /* setup inputs */
    for(i = 0; i < ctx.inputs_count; i++)
    {
        /* register input */
        ctx.inputs[i].idx = vzOutputInputAdd(ctx.output_context);
        if(ctx.inputs[i].idx < 0)
        {
            logger_printf(1, THIS_MODULE_PREF "vzOutputInputAdd failed with %d",
                ctx.inputs[i].idx);
            continue;
        };

        /* setup callbacks */
        ctx.inputs[i].cb = new decklink_input_class(&ctx, i);
        ctx.inputs[i].io->SetCallback(ctx.inputs[i].cb);
        ctx.inputs[i].io->EnableVideoInput(ctx.inputs[i].mode,
            bmdFormat8BitBGRA, bmdVideoInputEnableFormatDetection /*bmdVideoInputFlagDefault*/);

        ctx.inputs[i].io->StartStreams();

        logger_printf(0, THIS_MODULE_PREF "ENABLE_INPUT_%d will be mapped to liveinput %d",
            i, ctx.inputs[i].idx);
    };

    return 0;
};

static int decklink_stop()
{
    int i;

    for(i = 0; i < ctx.inputs_count; i++)
    {
        ctx.inputs[i].io->StopStreams();
        ctx.inputs[i].io->DisableVideoInput();
    };

    if(ctx.output.sync_event)
    {
        ctx.output.io->StopScheduledPlayback(0, 0, 0);
        ctx.output.io->DisableVideoOutput();
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

/////////////////////////////////////////////////////////////////////////////////////

#if 0
#define _WIN32_DCOM

#pragma comment(lib, "Ole32.Lib")
#pragma comment(lib, "OleAut32.Lib")
#pragma comment(lib, "User32.Lib")
#pragma comment(lib, "AdvAPI32.Lib")

#include <windows.h>

#include "../vz/vzOutput.h"
#include "../vz/vzOutput-devel.h"
#include "../vz/vzImage.h"
#include "../vz/vzOutputInternal.h"
#include "../vz/vzMain.h"
#include "../vz/vzTVSpec.h"
#include "../vz/vzLogger.h"

/*
    prepare:

    midl DecklinkInterface.idl /env win32 /h DecklinkInterface.h -I "C:\Program Files\Microsoft SDKs\Windows\v6.0A\Include"
    midl DecklinkInterface.idl /env x64 /h DecklinkInterface.h -I "C:\Program Files\Microsoft SDKs\Windows\v6.0A\Include"

*/
#include "DecklinkInterface.h"
#include "DecklinkInterface_i.c"

#include <stdio.h>

/* ------------------------------------------------------------------

	Module definitions 

------------------------------------------------------------------ */
#define MODULE_PREFIX "decklink: "
#define CONFIG_O(VAR) vzConfigParam(_config, "decklink", VAR)

#define O_ONBOARD_KEYER			"ONBOARD_KEYER"
#define O_ANALOGUE_OUTPUT		"ANALOGUE_OUTPUT"
#define O_VIDEO_INPUT			"VIDEO_INPUT"
#define O_IN_COMPONENT_LEVEL_SMPTE	\
								"IN_COMPONENT_LEVEL_SMPTE"
#define O_IN_SETUP_IS_75		"IN_SETUP_IS_75"
#define O_OUT_COMPONENT_LEVEL_SMPTE \
								"OUT_COMPONENT_LEVEL_SMPTE"
#define O_OUT_SETUP_IS_75		"OUT_SETUP_IS_75"
#define O_TIMING_OFFSET			"TIMING_OFFSET"
#define O_KEYER_ALPHA			"KEYER_ALPHA"
#define O_BOARD_INDEX           "BOARD_INDEX"

/* ------------------------------------------------------------------

	Module variables 

------------------------------------------------------------------ */
static void* _config = NULL;
static void* _tbc = NULL;
static vzTVSpec* _tv = NULL;
static frames_counter_proc _fc = NULL;
static struct vzOutputBuffers* _buffers_info = NULL;
static int _boards_count = 0;

#include "vzdssrc.h"

/* ------------------------------------------------------------------

	Operations parameters 

------------------------------------------------------------------ */
static IGraphBuilder *pGraph = NULL;
#ifdef _DEBUG
unsigned long graph_rot_id;
#endif
static IMediaControl *pMediaControl = NULL;
static IBaseFilter *vz_output = NULL;
static IBaseFilter *decklink_renderer = NULL;
static IDecklinkIOControl* decklink_ioctl = NULL;
static unsigned long decklink_ioctl_state = 0;
static IDecklinkKeyer* decklink_keyer = NULL;
static IDecklinkStatus* decklink_status = NULL;
static int decklink_video_status = 0, decklink_genlock_status = 0;
static char
	*decklink_video_status_text = "unknown", 
	*decklink_genlock_status_text = "unknown";

/* ------------------------------------------------------------------

	Decklink board init methods

------------------------------------------------------------------ */

#define NOTIFY_RESULT(MSG) logger_printf(1, MODULE_PREFIX MSG "... ");
#define CHECK_RESULT						\
	if (FAILED(hr))							\
	{										\
		logger_printf(1, MODULE_PREFIX "+- failed"); \
		return 0;							\
	}										\
	logger_printf(0, MODULE_PREFIX "+- OK");

static void decklink_configure(void)
{
	HRESULT hr;

	/* check if possible to configure */
	if
	(
		(NULL == decklink_keyer)
		||
		(NULL == decklink_ioctl)
	)
		return;

	/* setup keyer */
	{
        unsigned int l;

		logger_printf
		(
			0, MODULE_PREFIX "Setting %s keyer... ", 
			(NULL != CONFIG_O(O_ONBOARD_KEYER))?"INTERNAL":"EXTERNAL"
		);

		hr = decklink_keyer->set_AlphaBlendModeOn((NULL == CONFIG_O(O_ONBOARD_KEYER))?1:0);

		if(S_OK == hr) logger_printf(0, MODULE_PREFIX " +- OK");
		else if(E_PROP_ID_UNSUPPORTED == hr) logger_printf(1, MODULE_PREFIX " +- UNSUPPORTED");
		else logger_printf(1, MODULE_PREFIX " +- FAILED");

		/* setup alpha level to default */
        if(CONFIG_O(O_KEYER_ALPHA))
            l = atol(CONFIG_O(O_KEYER_ALPHA));
        else
            l = 255;
        logger_printf(0, MODULE_PREFIX "Setting keyer alpha to %d... ", l);

        hr = decklink_keyer->set_AlphaLevel(l);

        if(S_OK == hr) logger_printf(0, MODULE_PREFIX " +- OK");
		else logger_printf(1, MODULE_PREFIX " +- FAILED");
    };

	/* set video input O_VIDEO_INPUT */
	if(NULL != CONFIG_O(O_VIDEO_INPUT))
	{
		unsigned long video_output = DECKLINK_VIDEOOUTPUT_COMPOSITE;
		char* video_output_name = "COMPOSITE";

		switch(atol(CONFIG_O(O_VIDEO_INPUT)))
		{
			case 0:
				video_output_name = "COMPOSITE";
				video_output = DECKLINK_VIDEOOUTPUT_COMPOSITE;
				break;
			case 1:
				video_output_name = "SVIDEO";
				video_output = DECKLINK_VIDEOOUTPUT_SVIDEO;
				break;
			case 2:
				video_output_name = "COMPONENT";
				video_output = DECKLINK_VIDEOOUTPUT_COMPONENT;
				break;
			case 3:
				video_output_name = "SDI";
				video_output = DECKLINK_VIDEOINPUT_SDI;
				break;
		};
		logger_printf(0, MODULE_PREFIX "Setting video input to %s ... ", video_output_name);
		
		hr = decklink_ioctl->SetVideoInput2
		(
			video_output,
			(NULL == CONFIG_O(O_IN_SETUP_IS_75))?1:0,
			(NULL == CONFIG_O(O_IN_COMPONENT_LEVEL_SMPTE))?1:0
		);

		if(S_OK == hr) logger_printf(0, MODULE_PREFIX " +- OK");
		else if(E_INVALIDARG == hr) logger_printf(1, MODULE_PREFIX " +- INVALIDARG");
		else logger_printf(1, MODULE_PREFIX " +- FAILED");
	};

	/* setup analoge output */
	if(NULL != CONFIG_O(O_ANALOGUE_OUTPUT))
	{
		unsigned long video_output = DECKLINK_VIDEOOUTPUT_COMPOSITE;
		char* video_output_name = "COMPOSITE";

		switch(atol(CONFIG_O(O_ANALOGUE_OUTPUT)))
		{
			case 0:
				video_output_name = "COMPOSITE";
				video_output = DECKLINK_VIDEOOUTPUT_COMPOSITE;
				break;
			case 1:
				video_output_name = "SVIDEO";
				video_output = DECKLINK_VIDEOOUTPUT_SVIDEO;
				break;
			case 2:
				video_output_name = "COMPONENT";
				video_output = DECKLINK_VIDEOOUTPUT_COMPONENT;
				break;
		};
		logger_printf(0, MODULE_PREFIX "Setting analogue video output to %s ... ", video_output_name);
		
		hr = decklink_ioctl->SetAnalogueOutput2
		(
			video_output,
			(NULL == CONFIG_O(O_OUT_SETUP_IS_75))?1:0,
			(NULL == CONFIG_O(O_OUT_COMPONENT_LEVEL_SMPTE))?1:0
		);

		if(S_OK == hr) logger_printf(0, MODULE_PREFIX " +- OK");
		else if(E_INVALIDARG == hr) logger_printf(1, MODULE_PREFIX " +- INVALIDARG");
		else logger_printf(1, MODULE_PREFIX " +- FAILED");
	};

	/* setup timing of the genlock input  */
	if(NULL != CONFIG_O(O_TIMING_OFFSET))
	{
		int offset = atol(CONFIG_O(O_TIMING_OFFSET));
		logger_printf(0, MODULE_PREFIX "Setting timing of the genlock input to %d ... ", offset);
		hr = decklink_ioctl->SetGenlockTiming(offset);

		if(S_OK == hr) logger_printf(0, MODULE_PREFIX " +- OK");
		else if(E_INVALIDARG == hr) logger_printf(1, MODULE_PREFIX " +- INVALIDARG");
		else logger_printf(1, MODULE_PREFIX " +- FAILED");
	};
};

static void decklink_destroy(void)
{
#ifdef _DEBUG
	RemoveFromRot(graph_rot_id);
#endif

#define SAFE_REL(OBJ) if(NULL != OBJ) { OBJ->Release(); OBJ = NULL; };
	SAFE_REL(decklink_status);
	SAFE_REL(decklink_ioctl);
	SAFE_REL(decklink_keyer);
	SAFE_REL(decklink_renderer);
	SAFE_REL(vz_output);
	SAFE_REL(pMediaControl);
	SAFE_REL(pGraph);
};

static int decklink_init()
{
	HRESULT hr;
    int decklink_index = 0;

    static char* decklink_clsid_name_array[] =
    {
        "CLSID_DecklinkVideoRenderFilter",
        "CLSID_DecklinkVideoRenderFilter2",
        "CLSID_DecklinkVideoRenderFilter3",
        "CLSID_DecklinkVideoRenderFilter4",
        "CLSID_DecklinkVideoRenderFilter5",
        "CLSID_DecklinkVideoRenderFilter6",
        "CLSID_DecklinkVideoRenderFilter7",
        "CLSID_DecklinkVideoRenderFilter8"
    };

    static CLSID decklink_clsid_array[] =
    {
        CLSID_DecklinkVideoRenderFilter,
        CLSID_DecklinkVideoRenderFilter2,
        CLSID_DecklinkVideoRenderFilter3,
        CLSID_DecklinkVideoRenderFilter4,
        CLSID_DecklinkVideoRenderFilter5,
        CLSID_DecklinkVideoRenderFilter6,
        CLSID_DecklinkVideoRenderFilter7,
        CLSID_DecklinkVideoRenderFilter8
    };

	logger_printf(0, MODULE_PREFIX "init...");
	/*
		Create graph 
	*/
	NOTIFY_RESULT("CLSID_FilterGraph");
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, 
		CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&pGraph);
	CHECK_RESULT;

#ifdef _DEBUG
	hr = AddToRot(pGraph, &graph_rot_id);
#endif

    /* setup proper board index */
    if(CONFIG_O(O_BOARD_INDEX))
    {
        int idx = atol(CONFIG_O(O_BOARD_INDEX));
        if(idx >= 0 && idx <= 7)
            decklink_index = idx;
        else
            logger_printf(1, MODULE_PREFIX "BOARD_INDEX is out of range, allowed [0...7]");
    };
    logger_printf(0, MODULE_PREFIX "BOARD_INDEX=%d, will choose clsid=%s",
        decklink_index, decklink_clsid_name_array[decklink_index]);

    /*
        CLSID_DecklinkVideoRenderFilter
    */
    NOTIFY_RESULT("CLSID_DecklinkVideoRenderFilterX");
    hr = CoCreateInstance(decklink_clsid_array[decklink_index], NULL, 
        CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void **)&decklink_renderer);
    CHECK_RESULT;

	/*
		IDecklinkIOControl
	*/
	NOTIFY_RESULT("IID_IDecklinkIOControl");
	hr = decklink_renderer->QueryInterface(IID_IDecklinkIOControl, (void **)&decklink_ioctl);
	CHECK_RESULT;

	hr = decklink_ioctl->GetIOFeatures(&decklink_ioctl_state);
	logger_printf
	(
		0, 
		MODULE_PREFIX "Decklink IO options available:"
	);
#define DECKLINK_IO_OPTION_OUTPUT(F, M) \
	if(decklink_ioctl_state & F)		\
		logger_printf(0, MODULE_PREFIX " +- " M /* " [" #F "]" */);
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_SUPPORTSRGB10BITCAPTURE, "DeckLink card supports 10-bit RGB capture. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_SUPPORTSRGB10BITPLAYBACK, "DeckLink card supports 10-bit RGB playback. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_SUPPORTSINTERNALKEY, "DeckLink card supports internal keying. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_SUPPORTSEXTERNALKEY, "DeckLink card supports external keying. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_HASCOMPONENTVIDEOOUTPUT, "DeckLink card has component video output. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_HASCOMPOSITEVIDEOOUTPUT, "DeckLink card has composite video output. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_HASDIGITALVIDEOOUTPUT, "DeckLink card has digital video output. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_HASDVIVIDEOOUTPUT, "DeckLink card has DVI video output. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_HASCOMPONENTVIDEOINPUT, "DeckLink card has component video input. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_HASCOMPOSITEVIDEOINPUT, "DeckLink card has composite video input. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_HASDIGITALVIDEOINPUT, "DeckLink card has digital video input. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_HASDUALLINKOUTPUT, "DeckLink card supports dual link output. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_HASDUALLINKINPUT, "DeckLink card supports dual link input. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_SUPPORTSHD, "DeckLink card supports HD. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_SUPPORTS2KOUTPUT, "DeckLink card supports 2K output. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_SUPPORTSHDDOWNCONVERSION, "DeckLink card supports HD downconversion. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_HASAESAUDIOINPUT, "DeckLink card has S/PDIF audio input. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_HASANALOGUEAUDIOINPUT, "DeckLink card has analogue audio input. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_HASSVIDEOINPUT, "DeckLink card has S-video input. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_HASSVIDEOOUTPUT, "DeckLink card has S-video output. ");
	DECKLINK_IO_OPTION_OUTPUT(DECKLINK_IOFEATURES_SUPPORTSMULTICAMERAINPUT, "DeckLink card supports multicamera video input. ");

	/*
		IDecklinkKeyer
	*/
	NOTIFY_RESULT("IID_IDecklinkKeyer");
	hr = decklink_renderer->QueryInterface(IID_IDecklinkKeyer, (void **)&decklink_keyer);
	CHECK_RESULT;
	NOTIFY_RESULT("get_DeviceSupportsKeying");
	if (FAILED(decklink_keyer->get_DeviceSupportsKeying()))
		logger_printf(1, MODULE_PREFIX "does NOT support alpha keying!");
	else
		logger_printf(0, MODULE_PREFIX "DOES support alpha keying!");
	if (FAILED(decklink_keyer->get_DeviceSupportsExternalKeying()))
		logger_printf(1, MODULE_PREFIX "does NOT support external keying!");
	else
		logger_printf(0, MODULE_PREFIX "DOES support external keying!");

	/*
		IDecklinkStatus
	*/
	NOTIFY_RESULT("IID_IDecklinkStatus");
	hr = decklink_renderer->QueryInterface(IID_IDecklinkStatus, (void **)&decklink_status);
	CHECK_RESULT;

	hr = decklink_status->GetVideoInputStatus(&decklink_video_status, &decklink_genlock_status);
	switch(decklink_video_status)
	{
		case DECKLINK_INPUT_NONE: decklink_video_status_text = "Input is not present."; break;
		case DECKLINK_INPUT_PRESENT: decklink_video_status_text = "Input is present."; break;
	};
	switch(decklink_genlock_status)
	{
		case DECKLINK_GENLOCK_NOTSUPPORTED: decklink_genlock_status_text="Genlock is not available on the DeckLink card."; break;
		case DECKLINK_GENLOCK_NOTCONNECTED: decklink_genlock_status_text="Genlock is available but no valid signal is connected."; break;
		case DECKLINK_GENLOCK_LOCKED: decklink_genlock_status_text="Genlock is available and is locked to the input."; break;
		case DECKLINK_GENLOCK_NOTLOCKED: decklink_genlock_status_text="Genlock is available but is not locked to the input."; break;
	};
	logger_printf
	(
		0, 
		MODULE_PREFIX 
		"Decklink video status: %s"
		" /"
		"Decklink Genlock status: %s", 
		decklink_video_status_text, decklink_genlock_status_text
	);

	/* return success */
	return (_boards_count = 1);
};

/* ------------------------------------------------------------------

	Module method

------------------------------------------------------------------ */

VZOUTPUTS_EXPORT long vzOutput_FindBoard(char** error_log = NULL)
{
	HRESULT hr;

	/* 
		Start by calling CoInitialize to initialize the COM library
	*/
	logger_printf(0, MODULE_PREFIX "CoInitializeEx...");
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (FAILED(hr))
	{
		logger_printf(1, MODULE_PREFIX " +- Error");
		return 0;
	};
	logger_printf(0, MODULE_PREFIX " +- OK");

	timeBeginPeriod(1);

	/* init board */
	if(0 == decklink_init())
		decklink_destroy();

	return _boards_count;
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
	HRESULT hr;

	// assign tv value 
	_tv = (vzTVSpec*)tv;

	/* 
		Create a VZ output filter instance 
	*/
	{
		/*
			"Step 6. Add Support for COM"
			http://msdn2.microsoft.com/en-us/library/ms787698.aspx
		*/

		CVZPushSource* src = (CVZPushSource*)CVZPushSource::CreateInstance(NULL, &hr);
		NOTIFY_RESULT("IID_IBaseFilter(CVZPushSource)");
		hr = src->QueryInterface(IID_IBaseFilter, (void**)&vz_output);
		CHECK_RESULT;
	};

	/*
		Compose graph
	*/
	NOTIFY_RESULT("AddFilter(VZ)");
	hr = pGraph->AddFilter(vz_output, L"VZ");
	CHECK_RESULT;

	NOTIFY_RESULT("AddFilter(DECKLINK)");
	hr = pGraph->AddFilter(decklink_renderer, L"RENDERER");
	CHECK_RESULT;

	NOTIFY_RESULT("ConnectFilters(VZ, DECKLINK)");
	hr = ConnectFilters(pGraph, vz_output, decklink_renderer);
	CHECK_RESULT;

	/* configure */
	decklink_configure();

	return 1;
};

VZOUTPUTS_EXPORT void vzOutput_StartOutputLoop(void* tbc)
{
	HRESULT hr;

	/* check if graph built */
	if(NULL == pGraph) 
	{
		logger_printf(1, MODULE_PREFIX "ERROR: graph not initialized");
		return;
	};

	/* setup tbc */
	_tbc = tbc;

	/* Query control interface */
	NOTIFY_RESULT("IID_IMediaControl");
	hr = pGraph->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
	if (FAILED(hr))
	{
		logger_printf(1, MODULE_PREFIX " +- failed");
		return;
	}
	else
		logger_printf(0, MODULE_PREFIX " +- OK");

	/* Start graph */
	NOTIFY_RESULT("IID_IMediaControl::Run");
	hr = pMediaControl->Run();
	if (FAILED(hr))
	{
		logger_printf(1, MODULE_PREFIX " +- failed");
		return;
	}
	else
		logger_printf(0, MODULE_PREFIX " +- OK");
};

VZOUTPUTS_EXPORT void vzOutput_StopOutputLoop()
{
	/* check if graph built */
	if(NULL == pGraph) return;

	/* check if control interface is available */
	if(NULL == pMediaControl) return;

	/* Stop graph */
	pMediaControl->Stop();

	/* destroy */
	decklink_destroy();
};

VZOUTPUTS_EXPORT void vzOutput_AssignBuffers(struct vzOutputBuffers* b)
{
	_buffers_info = b;
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

	/* calc buffer from given frame parameters */
	b->output.size = 4*_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT;

	/* try to make count in 4096 pages */
	b->output.gold = ( ( b->output.size / 4096 ) + 1) * 4096;

	/* no audio output buffer */
	b->output.audio_buf_size = 0;
};

/* ------------------------------------------------------------------

	DLL Initialization 

------------------------------------------------------------------ */

#pragma comment(lib, "winmm")

#ifdef _DEBUG
#pragma comment(linker,"/NODEFAULTLIB:MSVCRT.lib")
#pragma comment(linker,"/NODEFAULTLIB:LIBCMTD.lib")
#else
#pragma comment(linker,"/NODEFAULTLIB:LIBCMT.lib")
#endif

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

			timeEndPeriod(1);
			
			/* wait all finished */
			logger_printf(0, MODULE_PREFIX "trying to stop graphs");
			if(NULL != pMediaControl)
				pMediaControl->Stop();

			/* deinit */
			logger_printf(0, MODULE_PREFIX "force decklink deinit");
			decklink_destroy();

			CoUninitialize();
			logger_printf(0, MODULE_PREFIX "decklink.dll finished");

			break;
    }
    return TRUE;
}
#endif
