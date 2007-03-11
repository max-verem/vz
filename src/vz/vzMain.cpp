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

#include <stdio.h>

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			// Init xml processor
		    try
			{
				XMLPlatformUtils::Initialize();
			}
			catch (...)
			{
				XMLPlatformUtils::Terminate();
				return FALSE;
		    }
			printf("Loading vzMain-%s\n", VZ_VERSION);
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
				XMLPlatformUtils::Terminate();
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


static vzTVSpec _default_vzTVSpec =		// Default is PAL
{
	40,		///	long TV_FRAME_DUR_MS;	// frame length (time period) (ms)
	576,	/// long TV_FRAME_HEIGHT;	// frame height (px)
	720,	/// long TV_FRAME_WIDTH;	// frame width (px)
	0,		/// long TV_FRAME_1ST;		// first field of frame (interlaced)
	25,		/// long TV_FRAME_PS;		// frames per seconds
};

VZMAIN_API void vzConfigTVSpec(void* config, char* module, void* spec)
{
	vzTVSpec* temp = (vzTVSpec*) spec;
	
	// first set default values to struct
	*temp = _default_vzTVSpec;

	// if config not defined - return
	if(!(config))
		return;

	// load parameters
	char* v;

	if (v = vzConfigParam(config, module, "TV_FRAME_DUR_MS") )
		temp->TV_FRAME_DUR_MS = atol(v);

	if (v = vzConfigParam(config, module, "TV_FRAME_HEIGHT") )
		temp->TV_FRAME_HEIGHT = atol(v);

	if (v = vzConfigParam(config, module, "TV_FRAME_WIDTH") )
		temp->TV_FRAME_WIDTH = atol(v);

	if (v = vzConfigParam(config, module, "TV_FRAME_1ST") )
		temp->TV_FRAME_1ST = atol(v);

	if (v = vzConfigParam(config, module, "TV_FRAME_PS") )
		temp->TV_FRAME_PS = atol(v);

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

