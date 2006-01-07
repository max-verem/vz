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

	// reset libriary pointer
	_lib = NULL;
};

vzFunction::~vzFunction()
{
	// unload lib
	if(_lib)
	{
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
	proc_configure		= (plugin_proc_configure)	GetProcAddress(_lib,"configure");

	// configure module
	if(proc_configure)
		proc_configure(_config);

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

int set_data_param_fromtext(char* name, char* value, void* data ,vzFunction* function,vzContainerFunction* container_function)
{
	void* param = get_data_param_ptr(name,data,function);
	char* temp_char;

	if(!(param))
		return 0;

	switch(name[0])
	{
		// SCALAR TYPES

		// float
		case 'f':
			*((float*)param) = (float)atof(value);
			break;

		// long
		case 'l':
			*((long*)param) = (long)atol(value);
			break;

		// long hex
		case 'h':
			*((long*)param) = (long)strtol(value,NULL,0);
			break;

		// four characters
		case 'L':
			*((long*)param) = *((long*)value);
			break;

		// one characters
		case 'c':
			*((char*)param) = value[0];
			break;

		// text
		case 's':
			// free used string if it's is not null
			temp_char = strcpy( (char*)malloc(strlen(value)+1) , value);
			if(*((char**)param))
				free((*((char**)param)));
			*((char**)param) = temp_char;
			break;
		default:
			return 0;
	};
	
	// should we notify or not
	if(container_function)
		container_function->notify();

	return 1;
};


#ifdef _DEBUG
void* vzFunction::constructor()
{
	return (proc_constructor)?proc_constructor():NULL;
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

void vzFunction::notify(void* data)
{
	if (proc_notify) proc_notify(data);
};

vzPluginInfo* vzFunction::info(void)
{
	return _info;
};

#endif