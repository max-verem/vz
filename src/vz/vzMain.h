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

	2005-06-13:
		*Modified 'vzMainSceneNew' arguments list - added parameter 'tv';

	2005-06-10
		*Added parameter config to vzMainSceneNew(void* functions,void* config);

    2005-06-08: Code cleanup

*/


#ifndef VZMAIN_H
#define VZMAIN_H

#ifdef VZMAIN_EXPORTS
#define VZMAIN_API __declspec(dllexport)
#else
#define VZMAIN_API __declspec(dllimport)
#pragma comment(lib, "vzMain.lib") 
#endif

/*
	vzMain module
	Main task of this module is to isolate user's application
	of vz-classes ....
*/

// function interface
VZMAIN_API void* vzMainNewFunctionsList(void* config);
VZMAIN_API void vzMainFreeFunctionsList(void* &list);

// scene interface
VZMAIN_API void* vzMainSceneNew(void* functions,void* config,void* tv);
VZMAIN_API void vzMainSceneFree(void* &scene);
VZMAIN_API int vzMainSceneLoad(void* scene, char* filename);
VZMAIN_API int vzMainSceneCommand(void* scene, char* cmd, char** error_log);
VZMAIN_API int vzMainSceneCommand(void* scene, int cmd, int index, void* buf);
VZMAIN_API void vzMainSceneDisplay(void* scene, long frame, long renders_count = 0, void** renderers_list = 0);
VZMAIN_API void vzMainSceneInit(void* scene);
VZMAIN_API void vzMainSceneRelease(void* scene);
VZMAIN_API void vzContainerFunctionRender(void* scene, char* function, void* session);


// config interface
VZMAIN_API void* vzConfigOpen(char* filename);
VZMAIN_API void vzConfigClose(void* &config);
VZMAIN_API char* vzConfigParam(void* config, char* module, char* name);
VZMAIN_API char* vzConfigAttr(void* config, char* module, char* name);


// tv spec loads from config or set default
VZMAIN_API void vzConfigTVSpec(void* config, char* module, void* spec);

/* vzContainer wrapper */
VZMAIN_API void vzContainerVisible(void* container, int visible);
VZMAIN_API void vzContainerDraw(void* container, void* session);

/* Xerces init */
VZMAIN_API int vzMainXMLInit();
VZMAIN_API void vzMainXMLRelease();

#endif
