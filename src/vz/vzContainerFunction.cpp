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
	2007-11-16: 
		*Visual Studio 2005 migration.

    2005-06-08: Code cleanup

*/
#define _CRT_SECURE_NO_WARNINGS

#include "vzContainerFunction.h"
#include "vzLogger.h"

#include "unicode.h"

vzContainerFunction::vzContainerFunction(DOMNodeX* parent_node,vzFunctions* functions_list,vzScene* scene, vzContainer* parent_container)
{
	// reset 
	_data = NULL;
	_function = NULL;
	_id = NULL;
	_datatarget = NULL;

	// load attributes
	_attributes = new vzXMLAttributes(parent_node);

	// load parameters
	_params = new vzXMLParams(parent_node);

	// request name of function
	char* function_name = _attributes->find("name");

	// check if function name given and if given - find the function object
	if(function_name)
	{
		_function = functions_list->find(function_name);

		// check if function found create object
		if(_function)
		{
			// register function
			if(_id = _attributes->find("id"))
			{
				scene->register_function(_id,this);

				// register datasource
				if(_datatarget = _attributes->find("datatarget"))
					scene->register_datasource(_id,this);
			};

			// create data object
			_data = _function->constructor(scene, parent_container);
		
			// try to set parameters values
			for(unsigned int i=0;i<_params->count();i++)
				// detect type be param name prefix
				::set_data_param_fromtext(_params->key(i),_params->value(i),_data,_function, NULL);

			/* notify about all params loaded */
			_function->notify(_data, NULL);
		};
	};
};

vzContainerFunction::~vzContainerFunction()
{
	TRACE_POINT();
	delete _attributes;
	TRACE_POINT();
	delete _params;
	TRACE_POINT();
	if(_function)
	{
		// free text params
		TRACE_POINT();
		for(unsigned int i=0;i<_function->count();i++)
		{
			TRACE_POINT();
			vzPluginParameter* p = _function->value(i);
			if(p->name[0] == 's')
			{
				TRACE_POINT();
				void* param = get_data_param_ptr(p->name);
				if(*((char**)param))
					free((*((char**)param)));
				*((char**)param) = NULL;
				TRACE_POINT();
			};
			TRACE_POINT();
		};

		// destruct instance
		TRACE_POINT();
		if(_data)
		{
			TRACE_POINT();
			_function->destructor(_data);
			TRACE_POINT();
		};
		TRACE_POINT();
	};
};
