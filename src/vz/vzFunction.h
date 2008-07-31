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
	2006-09-28:
		*datasource interface extension

    2005-06-08: Code cleanup

*/

#ifndef VZFUNCTION_H
#define VZFUNCTION_H

#include "../templates/hash.hpp"
#include "plugin.h"

#include <windows.h>

class vzContainerFunction;

typedef void* (*plugin_proc_constructor)(void*,void*);
typedef void (*plugin_proc_destructor)(void*);
typedef void (*plugin_proc_postrender)(void* data,vzRenderSession* render_session);
typedef void (*plugin_proc_prerender)(void* data,vzRenderSession* render_session);
typedef void (*plugin_proc_render)(void* data,vzRenderSession* render_session);
typedef void (*plugin_proc_notify)(void* data, char* param_name);
typedef void (*plugin_proc_configure)(void* config);
typedef long (*plugin_proc_datasource)(void* data,vzRenderSession* render_session, long index, char** name, char** value);

class vzFunction: public vzHash<vzPluginParameter*>
{
	// library handle
	HINSTANCE _lib;

	// plugin description
	vzPluginInfo* _info;

	// func of pointers
	plugin_proc_constructor	proc_constructor;
	plugin_proc_destructor	proc_destructor;
	plugin_proc_render		proc_render;
	plugin_proc_postrender	proc_postrender;
	plugin_proc_prerender	proc_prerender;
	plugin_proc_notify		proc_notify;
	plugin_proc_configure	proc_configure;
	plugin_proc_datasource	proc_datasource;

	// config
	void* _config;
	
public:
	vzFunction();
	char* load(void* config, char* filename);
	virtual ~vzFunction();


#ifndef _DEBUG
	inline void* constructor(void* scene,void* parent_container){return (proc_constructor)?proc_constructor(scene, parent_container):NULL;};

	inline void destructor(void* data){ if (proc_destructor) proc_destructor(data);};

	inline void render(void* data,vzRenderSession* render_session)
		{ if (proc_render) proc_render(data,render_session);};

	inline void prerender(void* data,vzRenderSession* render_session)
		{ if (proc_prerender) proc_prerender(data,render_session);};

	inline void postrender(void* data,vzRenderSession* render_session)
		{ if (proc_postrender) proc_postrender(data,render_session);};

	inline void notify(void* data, char* param_name){ if (proc_notify) proc_notify(data, param_name);};

	inline long datasource(void* data,vzRenderSession* render_session, long index, char** name, char** value)
		{ if (proc_datasource) return proc_datasource(data,render_session, index, name, value); else return 0;};

	inline vzPluginInfo* info(void){return _info;};
#else
	void* constructor(void* scene,void* parent_container);
	void destructor(void* data);
	void render(void* data,vzRenderSession* render_session);
	void prerender(void* data,vzRenderSession* render_session);
	void postrender(void* data,vzRenderSession* render_session);
	void notify(void* data, char* param_name);
	long datasource(void* data,vzRenderSession* render_session, long index, char** name, char** value);
	vzPluginInfo* info(void);
#endif
	
	
	unsigned long offset(char* parameter);
};

//void* get_data_param_ptr(char* param_name, void* data,vzFunction* function);
inline void* get_data_param_ptr(char* param_name, void* data,vzFunction* function)
{
	unsigned long offset = function->offset(param_name);
	return (offset != 0xFFFFFFFF)?((char*)data + offset):NULL;
};

int set_data_param_fromtext(char* param_name, char* param_value, void* data ,vzFunction* function,vzContainerFunction* container_function = NULL);



#endif