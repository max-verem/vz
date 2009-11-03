/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2008 Maksym Veremeyenko.
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
        *intergrate logger code

	2008-09-23:
		*vzTVSpec rework

	2007-11-16: 
		*Visual Studio 2005 migration.

	2006-12-13:
		*vzContainer wrapper added. one more hook.

	2006-04-23:
		*XML initialization moved to DLL_PROCESS_ATTACH section

	2005-06-13:
		*Modified 'vzMainSceneNew' argument list - added 'tv' parameter

	2005-06-10:
		*Added parameter config to vzMainSceneNew(void* functions,void* config)
		and modified scene class initialization

    2005-06-08:
		*Code cleanup.
		*vzTVSpec auxilarity function added

*/

#include "vzVersion.h"
#include "vzMain.h"
#include "vzScene.h"
#include "vzConfig.h"
#include "vzTVSpec.h"
#include "vzLogger.h"

#include <stdio.h>

BOOL APIENTRY DllMain( HANDLE hModule, 
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


// function interface
VZMAIN_API void* vzMainNewFunctionsList(void* config)
{
	vzFunctions* functions = new vzFunctions(config);
	functions->load();
	return functions;
};

VZMAIN_API void vzMainFreeFunctionsList(void* &list)
{
	delete (vzFunctions*)list;
	list = NULL;
};

// scene interface
VZMAIN_API void* vzMainSceneNew(void* functions,void* config,void* tv)
{
	vzScene* scene = new vzScene((vzFunctions*)functions,config,(vzTVSpec*)tv);
	return scene;
};

VZMAIN_API void vzMainSceneFree(void* &scene)
{
	delete (vzScene*)scene;
	scene = NULL;
};

VZMAIN_API int vzMainSceneLoad(void* scene, char* file_name)
{
	return ((vzScene*)scene)->load(file_name);
};

VZMAIN_API int vzMainSceneCommand(void* scene, char* cmd,char** error_log)
{
	return ((vzScene*)scene)->command(cmd,error_log);
};

VZMAIN_API int vzMainSceneCommand(void* scene, int cmd, int index, void* buf)
{
	return ((vzScene*)scene)->command(cmd, index, buf);
};

VZMAIN_API void vzMainSceneDisplay(void* scene, long frame)
{
	((vzScene*)scene)->display(frame);
};


/* --------------------------------------------------------------------------

	config procs

-------------------------------------------------------------------------- */
VZMAIN_API void* vzConfigOpen(char* filename)
{
	return new vzConfig(filename);
};
VZMAIN_API void vzConfigClose(void* &config)
{
	delete (vzConfig*)config;
	config = NULL;
};
VZMAIN_API char* vzConfigParam(void* config, char* module, char* name)
{
	return (config)?((vzConfig*)config)->param(module,name):NULL;
};
VZMAIN_API char* vzConfigAttr(void* config, char* module, char* name)
{
	return (config)?((vzConfig*)config)->attr(module,name):NULL;
};


/*
--------------------------------------------------------------------------

  tv system struct init

--------------------------------------------------------------------------
*/

static struct _vzTVSpecSA _safe_PAL_4_3 =
{
	{33, 71},
	{28, 57}
};

static struct _vzTVSpecSA _safe_NTSC_4_3 =
{
	{33, 71},
	{28, 57}
};

static struct _vzTVSpec _known_tv_modes[] = 
{
	/* PAL interlaced frame */
	{
		"576i",		/** NAME					*/
		576,		/** TV_FRAME_HEIGHT			*/
		720,		/** TV_FRAME_WIDTH			*/
		1,			/** TV_FRAME_INTERLACED		*/
		0,			/** TV_FRAME_1ST			*/
		25,			/** TV_FRAME_PS_NOM			*/
		1,			/** TV_FRAME_PS_DEN			*/
		4,			/** TV_FRAME_DAR_H			*/
		3,			/** TV_FRAME_DAR_V			*/
		1,			/** TV_FRAME_PAR_H			*/
		1,			/** TV_FRAME_PAR_V			*/
		&_safe_PAL_4_3 /** safe area markers */
	},

	/* PAL progressive frame */
	{
		"576p",		/** NAME					*/
		576,		/** TV_FRAME_HEIGHT			*/
		720,		/** TV_FRAME_WIDTH			*/
		0,			/** TV_FRAME_INTERLACED		*/
		0,			/** TV_FRAME_1ST			*/
		25,			/** TV_FRAME_PS_NOM			*/
		1,			/** TV_FRAME_PS_DEN			*/
		4,			/** TV_FRAME_DAR_H			*/
		3,			/** TV_FRAME_DAR_V			*/
		1,			/** TV_FRAME_PAR_H			*/
		1,			/** TV_FRAME_PAR_V			*/
		&_safe_PAL_4_3 /** safe area markers */
	},

	/* NTSC interlaced frame */
	{
		"480i",		/** NAME					*/
		480,		/** TV_FRAME_HEIGHT			*/
		720,		/** TV_FRAME_WIDTH			*/
		1,			/** TV_FRAME_INTERLACED		*/
		1,			/** TV_FRAME_1ST			*/
		30000,		/** TV_FRAME_PS_NOM			*/
		1001,		/** TV_FRAME_PS_DEN			*/
		4,			/** TV_FRAME_DAR_H			*/
		3,			/** TV_FRAME_DAR_V			*/
		1,			/** TV_FRAME_PAR_H			*/
		1,			/** TV_FRAME_PAR_V			*/
		&_safe_NTSC_4_3 /** safe area markers */
	},

	/* NTSC progressive frame */
	{
		"480p",		/** NAME					*/
		480,		/** TV_FRAME_HEIGHT			*/
		720,		/** TV_FRAME_WIDTH			*/
		0,			/** TV_FRAME_INTERLACED		*/
		0,			/** TV_FRAME_1ST			*/
		30000,		/** TV_FRAME_PS_NOM			*/
		1001,		/** TV_FRAME_PS_DEN			*/
		4,			/** TV_FRAME_DAR_H			*/
		3,			/** TV_FRAME_DAR_V			*/
		1,			/** TV_FRAME_PAR_H			*/
		1,			/** TV_FRAME_PAR_V			*/
		&_safe_NTSC_4_3 /** safe area markers */
	},

	/* HD mode:  System 1 (S1) with 1280 horizontal 
	samples and 720 active lines in progressive scan 
	with a frame rate of 50Hz, 16 x 9 aspect ratio. */
	{
		"720p50",	/** NAME					*/
		720,		/** TV_FRAME_HEIGHT			*/
		1280,		/** TV_FRAME_WIDTH			*/
		0,			/** TV_FRAME_INTERLACED		*/
		0,			/** TV_FRAME_1ST			*/
		50,			/** TV_FRAME_PS_NOM			*/
		1,			/** TV_FRAME_PS_DEN			*/
		16,			/** TV_FRAME_DAR_H			*/
		9,			/** TV_FRAME_DAR_V			*/
		1,			/** TV_FRAME_PAR_H			*/
		1,			/** TV_FRAME_PAR_V			*/
		NULL		/** safe area markers */
	},

	/* HD mode:  System 2 (S2) with 1920 horizontal samples
	and 1080 active lines in interlaced scan with a frame rate
	of 25Hz, 16 x 9 aspect ratio. */
	{
		"1080i25",	/** NAME					*/
		1080,		/** TV_FRAME_HEIGHT			*/
		1920,		/** TV_FRAME_WIDTH			*/
		1,			/** TV_FRAME_INTERLACED		*/
		0,			/** TV_FRAME_1ST			*/
		25,			/** TV_FRAME_PS_NOM			*/
		1,			/** TV_FRAME_PS_DEN			*/
		16,			/** TV_FRAME_DAR_H			*/
		9,			/** TV_FRAME_DAR_V			*/
		1,			/** TV_FRAME_PAR_H			*/
		1,			/** TV_FRAME_PAR_V			*/
		NULL		/** safe area markers */
	},

	/* HD mode:  System 3 (S3) with 1920 horizontal samples
	and 1080 active lines in progressive scan and a frame rate
	of 25Hz, 16 x 9 aspect ratio. */
	{
		"1080p25",	/** NAME					*/
		1080,		/** TV_FRAME_HEIGHT			*/
		1920,		/** TV_FRAME_WIDTH			*/
		0,			/** TV_FRAME_INTERLACED		*/
		0,			/** TV_FRAME_1ST			*/
		25,			/** TV_FRAME_PS_NOM			*/
		1,			/** TV_FRAME_PS_DEN			*/
		16,			/** TV_FRAME_DAR_H			*/
		9,			/** TV_FRAME_DAR_V			*/
		1,			/** TV_FRAME_PAR_H			*/
		1,			/** TV_FRAME_PAR_V			*/
		NULL		/** safe area markers */
	},

	/* HD mode:  System 4 (S4) with 1920 horizontal samples
	and 1080 active lines in progressive scan at a frame rate
	of 50Hz, 16 x 9 aspect ratio. */
	{
		"1080p50",	/** NAME					*/
		1080,		/** TV_FRAME_HEIGHT			*/
		1920,		/** TV_FRAME_WIDTH			*/
		0,			/** TV_FRAME_INTERLACED		*/
		0,			/** TV_FRAME_1ST			*/
		50,			/** TV_FRAME_PS_NOM			*/
		1,			/** TV_FRAME_PS_DEN			*/
		16,			/** TV_FRAME_DAR_H			*/
		9,			/** TV_FRAME_DAR_V			*/
		1,			/** TV_FRAME_PAR_H			*/
		1,			/** TV_FRAME_PAR_V			*/
		NULL		/** safe area markers */
	},

	/* list terminator */
	{
		NULL
	}
};

static struct _vzTVSpec* lookup_tv_mode(char* name)
{
	int i;

	if(NULL != name)
	{
		for(i = 0; NULL != _known_tv_modes[i].NAME; i++)
			if(0 == _stricmp(name, _known_tv_modes[i].NAME))
				return &_known_tv_modes[i];
	}
	else
		return &_known_tv_modes[0];

	/* default mode */
	return NULL;
};

VZMAIN_API void vzConfigTVSpec(void* config, char* module, void* spec)
{
	char* v;
	vzTVSpec* temp;
	
	/* set default */
	memcpy(spec, lookup_tv_mode(NULL), sizeof(struct _vzTVSpec));

	// if config not defined - return
	if(NULL == config) return;

	/* try to override mode */
	if(NULL != (v = vzConfigParam(config, module, "TV_MODE")))
		if(NULL != (temp = lookup_tv_mode(v)))
			memcpy(spec, temp, sizeof(struct _vzTVSpec));

	/* override parameters */
	temp = (vzTVSpec*) spec;

#define MAP_P(VAR) \
	if(NULL != (v = vzConfigParam(config, module, #VAR))) \
			temp->VAR = atol(v);

	MAP_P(TV_FRAME_HEIGHT);
	MAP_P(TV_FRAME_WIDTH);
	MAP_P(TV_FRAME_INTERLACED);
	MAP_P(TV_FRAME_1ST);
	MAP_P(TV_FRAME_PS_NOM);
	MAP_P(TV_FRAME_PS_DEN);
	MAP_P(TV_FRAME_DAR_H);
	MAP_P(TV_FRAME_DAR_V);
	MAP_P(TV_FRAME_PAR_NOM);
	MAP_P(TV_FRAME_PAR_DEN);
	MAP_P(VGA_SCALE);

	/* configure VGA screen parameters */

	/* set scale */
	temp->vga_height = temp->TV_FRAME_HEIGHT >> temp->VGA_SCALE;
	temp->vga_width = temp->TV_FRAME_WIDTH >> temp->VGA_SCALE;

	/* check for anamorphic flag */
	if(NULL != (v = vzConfigParam(config, module, "ANAMNORPHIC")))
		temp->anamorphic = 1;

	/* check for wide */
	if(NULL != (v = vzConfigParam(config, module, "WIDE_PAL")))
	{
		temp->TV_FRAME_PAR_NOM = 512;
		temp->TV_FRAME_PAR_DEN = 351;
	};
	temp->vga_width = (temp->vga_width * temp->TV_FRAME_PAR_NOM) / temp->TV_FRAME_PAR_DEN;
};

/* --------------------------------------------------------------------------

	vzContainer procs

-------------------------------------------------------------------------- */
VZMAIN_API void vzContainerVisible(void* container, int visible)
{
	if (container)
		((vzContainer*)container)->visible(visible);
};

VZMAIN_API void vzContainerDraw(void* container, void* session)
{
	if (container)
		((vzContainer*)container)->draw((vzRenderSession*)session);
};

VZMAIN_API int vzMainXMLInit()
{
    // Init xml processor
    try
    {
        XMLPlatformUtilsX::Initialize();
    }
    catch (...)
    {
        XMLPlatformUtilsX::Terminate();
        return -1;
    };

    return 0;
};

VZMAIN_API void vzMainXMLRelease()
{
    XMLPlatformUtilsX::Terminate();
};
