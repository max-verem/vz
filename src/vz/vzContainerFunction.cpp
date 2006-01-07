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


#include "vzContainerFunction.h"

#include "unicode.h"

vzContainerFunction::vzContainerFunction(DOMNode* parent_node,vzFunctions* functions_list,vzScene* scene)
{
	// reset 
	_data = NULL;
	_function = NULL;

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
			char* temp;
			if(temp = _attributes->find("id"))
				scene->register_function(temp,this);

			// create data object
			_data = _function->constructor();
		
			// try to set parameters values
			for(unsigned int i=0;i<_params->count();i++)
				// detect type be param name prefix
				::set_data_param_fromtext(_params->key(i),_params->value(i),_data,_function);

			// notify about params change
			_function->notify(_data);

		};
	};
};

vzContainerFunction::~vzContainerFunction()
{
	delete _attributes;
	delete _params;
	if((_function) && (_data))
		_function->destructor(_data);
};