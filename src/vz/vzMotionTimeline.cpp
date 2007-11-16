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


#include "vzMotionTimeline.h"


vzMotionTimeline::vzMotionTimeline(DOMNodeX* node, vzScene* scene, char* param_name, vzContainerFunction* function)
{
	// create lock
	_lock = CreateMutex(NULL,FALSE,NULL);

	// defaults
	_ready = 0;

	// assigning parameters
	_param_name = param_name;
	_function = function;

	// load attributes
	_attributes = new vzXMLAttributes(node);

	// id
	_id = _attributes->find("id");

	// string values
	_s_y2 = (_s_y1 = NULL);

	// check if all attributes present
	if(_attributes->find("t1")) _t1 = atol(_attributes->find("t1")); else return;
	if(_attributes->find("t2")) _t2 = atol(_attributes->find("t2")); else return;
	if(_attributes->find("f")) _f = atol(_attributes->find("f")); else return;
	if(!(_attributes->find("y1"))) return;
	if(!(_attributes->find("y2"))) return;

	// assigning function values according to
	// their types
	switch(*_param_name)
	{
		case 'L':
			_L_y1 = *((long*)_attributes->find("y1"));
			_L_y2 = *((long*)_attributes->find("y2"));
			break;

		case 's':
			strcpy(_s_y1 = (char*)malloc(strlen(_attributes->find("y1"))+1),_attributes->find("y1"));
			strcpy(_s_y2 = (char*)malloc(strlen(_attributes->find("y2"))+1),_attributes->find("y2"));
			break;

		case 'f':
			_f_y1 = (float)atof(_attributes->find("y1"));
			_f_y2 = (float)atof(_attributes->find("y2"));
			break;

		case 'l':
			_l_y1 = atol(_attributes->find("y1"));
			_l_y2 = atol(_attributes->find("y2"));
			break;

		default:
			return;
	};

	// map parameter
	if (!(_param_ptr = _function->get_data_param_ptr(_param_name)))
		return;

	// register itself
	if(_id)
		scene->register_timeline(_id,this);

	// all attributes present !!!
	_ready = 1;
};


vzMotionTimeline::~vzMotionTimeline()
{
	delete _attributes;
	if(_s_y2) free(_s_y2);
	if(_s_y1) free(_s_y1);
	CloseHandle(_lock);
};

void vzMotionTimeline::assign(long frame, int field)
{
	// if not ready return
	if(!(_ready)) return;

	// check if frame in ragne
	if(! ( (_t1<=frame) && (frame<=_t2) ) )
		return;

	WaitForSingleObject(_lock,INFINITE);

	// if position in right range position - no fields
	if (frame == _t2)
		field = 0;
	
	// functions !!!
	if (_f == 0)
	{
		// const function

		// no calculation - just set it from text
		_function->set_data_param_fromtext(_param_name,_attributes->find("y1"));
		_function->notify();
		ReleaseMutex(_lock);
		return;
	};

	if (_f == 1)
	{
		// linear approximation function

#define LINE_K(T1,T2,Y1,Y2) ( (T1 == T2) ? 0: ( ((float)Y1 - (float)Y2) / ((float)T1 - (float)T2) ) )

		float k,f;
		long l;

		// parameter type depends function
		switch(*_param_name)
		{
			case 'f':
				k = LINE_K(_t1,_t2,_f_y1,_f_y2);
				f = (float) ( k*( 0.5 * (float)field + (float)frame) + _f_y1 - _t1 * k );
				*((float*)_param_ptr) = f;
				break;

			case 'l':
				k = LINE_K(_t1,_t2,_l_y1,_l_y2);
				l = (long) ( k*( 0.5 * (float) field + (float)frame) + _l_y1 - _t1 * k );
				*((long*)_param_ptr) = l;
				break;

			// if type are incompatibe - return
			default:
				ReleaseMutex(_lock);
				return;
		};
		_function->notify();
		ReleaseMutex(_lock);
		return;
	};

	ReleaseMutex(_lock);
};


void vzMotionTimeline::set_t1(char* value)
{
};

void vzMotionTimeline::set_t2(char* value)
{
};

void vzMotionTimeline::set_y1(char* value)
{
	if(!(_ready)) return;

	WaitForSingleObject(_lock,INFINITE);

	switch(*_param_name)
	{
		case 'L':
			_L_y1 = *((long*)value);
			break;

		case 's':
			if(_s_y1) free(_s_y1);
			strcpy(_s_y1 = (char*)malloc(strlen(value)+1),value);
			break;

		case 'f':
			_f_y1 = (float)atof(value);
			break;

		case 'l':
			_l_y1 = atol(value);
			break;
	};
	
	ReleaseMutex(_lock);
};

void vzMotionTimeline::set_y2(char* value)
{
	if(!(_ready)) return;

	WaitForSingleObject(_lock,INFINITE);

	switch(*_param_name)
	{
		case 'L':
			_L_y2 = *((long*)value);
			break;

		case 's':
			if(_s_y2) free(_s_y2);
			strcpy(_s_y2 = (char*)malloc(strlen(value)+1),value);
			break;

		case 'f':
			_f_y2 = (float)atof(value);
			break;

		case 'l':
			_l_y2 = atol(value);
			break;
	};

	ReleaseMutex(_lock);	
};
