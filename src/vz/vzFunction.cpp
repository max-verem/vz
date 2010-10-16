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
    2005-06-08: Code cleanup

*/

#include "memleakcheck.h"

#include "vzFunction.h"

#include "vzContainerFunction.h"

vzFunction::vzFunction() : vzHash<vzPluginParameter*>() 
{
	// set functions pointer to null
	proc_constructor = NULL;
	proc_destructor = NULL;
	proc_render = NULL;
	proc_postrender = NULL;
	proc_prerender = NULL;
    proc_load = NULL;
    proc_unload = NULL;
    proc_init = NULL;
    proc_release = NULL;

	// reset libriary pointer
	_lib = NULL;
};

vzFunction::~vzFunction()
{
	// unload lib
	if(_lib)
	{
        if(proc_unload)
            proc_unload(_config);
		FreeLibrary(_lib);
		_lib = NULL;
	};
};


char* vzFunction::load(void* config, char* filename)
{
	_config = config;

	// try load
	_lib = LoadLibrary(filename);

	// check if lib is loaded
	if (!_lib)
		return NULL;

	// lib info
	_info = (vzPluginInfo*)GetProcAddress(_lib,"info");
	if(!_info)
		return NULL;


	// parameters description
	vzPluginParameter* parameters = (vzPluginParameter*)GetProcAddress(_lib,"parameters");
	for(int i=0;(parameters[i].name);i++)
		push(parameters[i].name,&parameters[i]);

	// function mapping
	proc_constructor	= (plugin_proc_constructor)	GetProcAddress(_lib,"constructor");
	proc_destructor		= (plugin_proc_destructor)	GetProcAddress(_lib,"destructor");
	proc_render			= (plugin_proc_render)		GetProcAddress(_lib,"render");
	proc_postrender		= (plugin_proc_postrender)	GetProcAddress(_lib,"postrender");
	proc_prerender		= (plugin_proc_prerender)	GetProcAddress(_lib,"prerender");
	proc_notify			= (plugin_proc_notify)		GetProcAddress(_lib,"notify");
	proc_datasource		= (plugin_proc_datasource)	GetProcAddress(_lib,"datasource");
    proc_load           = (plugin_proc_load)        GetProcAddress(_lib, "load");
    proc_unload         = (plugin_proc_load)        GetProcAddress(_lib, "unload");
    proc_init           = (plugin_proc_init)        GetProcAddress(_lib, "init");
    proc_release        = (plugin_proc_release)     GetProcAddress(_lib, "release");

    // "load" module
    if(proc_load)
        proc_load(_config);

	return _info->name;
};

unsigned long vzFunction::offset(char* parameter_name)
{
	vzPluginParameter* parameter = find(parameter_name);
	if (parameter)
		return parameter->offset;
	else
		return 0xFFFFFFFF;
};

/*
void* get_data_param_ptr(char* param_name, void* data,vzFunction* function)
{
	return (char*)data + function->offset(param_name);
}
*/
#include <stdio.h>
int set_data_param_fromtext(char* name, char* value, void* data ,vzFunction* function,vzContainerFunction* container_function)
{
	void* param = get_data_param_ptr(name,data,function);
	int is_dirty = 0;
	float s_f; long s_l; unsigned long s_ul; char s_c; char* s_ch;

	if(!(param))
		return 0;

	switch(name[0])
	{
		// SCALAR TYPES

		// float
		case 'f':
			s_f = (float)atof(value);
			if(*((float*)param) != s_f)
			{
				*((float*)param) = s_f;
				is_dirty = 1;
			};
			break;

		// long
		case 'l':
			s_l = (long)atol(value);
			if(*((long*)param) != s_l)
			{
				*((long*)param) = (long)atol(value);
				is_dirty = 1;
			};
			break;

		// long hex
		case 'h':
			s_ul = (unsigned long)strtoul(value,NULL,16);
			if(*((unsigned long*)param) != s_ul)
			{
				*((unsigned long*)param) = s_ul;
				is_dirty = 1;
			};
			break;

		// four characters
		case 'L':
			s_l = *((long*)value);
			if(*((long*)param) != s_l)
			{
				*((long*)param) = s_l;
				is_dirty = 1;
			};
			break;

		// one characters
		case 'c':
			s_c = value[0];
			if(*((char*)param) != s_c)
			{
				*((char*)param) = s_c;
				is_dirty = 1;
			};
			break;

		// text
		case 's':
			s_ch = *((char**)param);

			/* compare if it not null */
			if(NULL != s_ch)
			{
				if(0 != strcmp(s_ch, value))
				{
					free(s_ch);
					s_ch = NULL;
				};
			};

			/* update content of string if it new or different */
			if(NULL == s_ch)
			{
				/* alloc and copy string */
				*((char**)param) = 
					strcpy( (char*)malloc(strlen(value)+1) , value);

				is_dirty = 1;
			};
			break;

		default:
			return 0;
	};

//printf("set_data_param_fromtext: container_function=%.8X, name='%s', value='%s', is_dirty=%d\n", 
//	   container_function, name, value, is_dirty);

	// should we notify or not
	if((container_function)&&(0 != is_dirty))
		container_function->notify(name);

	return 1;
};


#ifdef _DEBUG
void* vzFunction::constructor(void* scene,void* parent_container)
{
	return (proc_constructor)?proc_constructor(scene,parent_container):NULL;
};

void vzFunction::destructor(void* data)
{
	if (proc_destructor) 
		proc_destructor(data);
};

void vzFunction::render(void* data,vzRenderSession* render_session)
{
	if (proc_render) 
		proc_render(data,render_session);
};

void vzFunction::prerender(void* data,vzRenderSession* render_session)
{
	if (proc_prerender)
		proc_prerender(data,render_session);
};

void vzFunction::postrender(void* data,vzRenderSession* render_session)
{
	if (proc_postrender) proc_postrender(data,render_session);
};

void vzFunction::notify(void* data, char* param_name)
{
	if (proc_notify) proc_notify(data, param_name);
};

long vzFunction::datasource(void* data,vzRenderSession* render_session, long index, char** name, char** value)
{ 
	if (proc_datasource) 
		return proc_datasource(data,render_session, index, name, value); 
		else return 0;
};

vzPluginInfo* vzFunction::info(void)
{
	return _info;
};


#endif