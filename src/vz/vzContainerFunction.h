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
		*datasource interface added.

    2005-06-08: Code cleanup

*/


#ifndef VZCONTAINERFUNCTION_H
#define VZCONTAINERFUNCTION_H

class vzScene;

#include "xerces.h"
#include "vzFunctions.h"
#include "vzXMLAttributes.h"
#include "vzXMLParams.h"
#include "vzScene.h"


class vzContainerFunction
{
	// attributes
	vzXMLAttributes* _attributes;

	// parameters
	vzXMLParams* _params;

	// data stored 
	void* _data;

	// function
	vzFunction* _function;

public:
	vzContainerFunction(DOMNode* parent_node,vzFunctions* functions_list,vzScene* scene, vzContainer* parent_container);
	~vzContainerFunction();

	inline void render(vzRenderSession* render_session)
	{ if (_function) _function->render(_data,render_session);};

	inline void prerender(vzRenderSession* render_session)
	{ if (_function) _function->prerender(_data,render_session);};

	inline void postrender(vzRenderSession* render_session)
	{ if (_function) _function->postrender(_data,render_session);};

	inline void notify(void)
	{ if (_function) _function->notify(_data);};

	inline void* get_data_param_ptr(char* param_name)
	{ return (_function)?::get_data_param_ptr(param_name, _data, _function):NULL; };

	inline int set_data_param_fromtext(char* param_name, char* param_value)
	{ return (_function)?::set_data_param_fromtext(param_name,param_value, _data , _function, this):0; };
		
	inline long datasource(vzRenderSession* render_session, long index, char** name, char** value)
	{ if(_function) return _function->datasource(_data,render_session, index, name, value) ; else return 0;};
};

#endif
