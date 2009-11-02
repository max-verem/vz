/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2007 Maksym Veremeyenko.
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
    2008-09-24:
        *logger use for message outputs

	2007-11-17
		*Module start.

*/
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

	/*
		CLSID_DecklinkVideoRenderFilter
	*/
	NOTIFY_RESULT("CLSID_DecklinkVideoRenderFilter");
	hr = CoCreateInstance(CLSID_DecklinkVideoRenderFilter, NULL, 
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
