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
    2008-09-24:
        *logger use for message outputs

	2005-06-08: Code cleanup


*/

// plugin development functions

#ifndef PLUGIN_DEVEL_H
#define PLUGIN_DEVEL_H

#include "memleakcheck.h"

// main includes
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "vzGlExt.h"
#include "vzVersion.h"
#include "vzLogger.h"

// dll exporst deals
#define PLUGIN_EXPORT extern "C" __declspec(dllexport)
//#pragma comment(linker, "/LIBPATH:E:\\sdk\\xerces\\lib\\")

// offset calculating
#define PLUGIN_PARAMETER_OFFSET(struct_begin,member) ((unsigned char*)(&struct_begin.member)-(unsigned char*)(&struct_begin))

// some debug output
#define DEBUG_LINE_ARG "%s:%d: "
#define DEBUG_LINE_PARAM __FILE__,__LINE__

#define _DATA ((vzPluginData*)data)
/*
#ifdef _DEBUG
	#pragma comment(lib, "xerces-c_2D.lib") 
#else
	#pragma comment(lib, "xerces-c_2.lib") 
#endif
*/

/* plugins additional threads priorities */
#define VZPLUGINS_AUX_THREAD_PRIO THREAD_PRIORITY_LOWEST

/* versioning info */
#define DEFINE_PLUGIN_INFO(NAME)											\
PLUGIN_EXPORT vzPluginInfo info =											\
{																			\
	NAME,																	\
	VZ_VERSION,																\
	_plugin_description,													\
	_plugin_notes															\
}

#define DEFINE_PLUGIN_INFO_EXT(NAME, VERSION)								\
PLUGIN_EXPORT vzPluginInfo info =											\
{																			\
	NAME,																	\
	VZ_VERSION "-" VERSION,													\
	_plugin_description,													\
	_plugin_notes															\
}

#ifndef VZ_MIN
#define VZ_MIN(a,b) (((a)>(b))?(b):(a))
#endif

#ifndef VZ_MAX
#define VZ_MAX(a,b) (((a)>(b))?(a):(b))
#endif

#endif
