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

    2006-05-01: 
        *memory leak fixed on '_attributes' object

    2005-06-08: Code cleanup

*/

#include "vzMotionDirector.h"
#include "vzLogger.h"
#include <stdio.h>

static const unsigned short tag_param[] = {'p', 'a', 'r', 'a', 'm',0};
static const unsigned short tag_control[] = {'c', 'o', 'n', 't', 'r', 'o', 'l',0};

vzMotionDirector::vzMotionDirector(DOMNodeX* node,vzScene* scene)
{
	// create lock
	_lock = CreateMutex(NULL,FALSE,NULL);

	// auto id generation for timelines hash
	int params_counter = 0;
	char param_name[1024];
	
	_control = NULL;

	// defaults
	_ready = 0;
	_id = NULL;
	_first_run = 1;

	// load attributes
	_attributes = new vzXMLAttributes(node);

	// check if all attributes present
	if(_attributes->find("dur")) _dur = atol(_attributes->find("dur")); else return;
	if(_attributes->find("loop")) _loop = atol(_attributes->find("loop")); else return;
	if(_attributes->find("pos")) _pos = atol(_attributes->find("pos")); else return;
	if(_attributes->find("run")) _run = atol(_attributes->find("run")); else return;
	
	if(!(_id = _attributes->find("id"))) return;

	// load parameters
	// request list of child nodes
	DOMNodeListX* children = node->getChildNodes();

	// enumerate
	for(unsigned int i=0;i<children->getLength();i++)
	{
		// getting child
		DOMNodeX* child = children->item(i);

		// checking type
		if(child->getNodeType() !=  DOMNodeX::ELEMENT_NODE)
			continue;

		// check node name for 'function'
		if (XMLStringX::compareIString(child->getNodeName(),tag_param) == 0)
		{
			// init function Instance object
			vzMotionParameter* parameter = new vzMotionParameter(child,scene);

			// push it into functions hash
			sprintf(param_name,"#%04d",++params_counter); // create internal id
			_parameters.push(param_name,parameter);

			continue;
		};

		// check node name for 'function'
		if (XMLStringX::compareIString(child->getNodeName(),tag_control) == 0)
		{
			// Motion control
			if (!(_control))
				_control = new vzMotionControl(child,scene);

			continue;
		};

	};

	// register itself in tree
	if(_id)
		scene->register_director(_id,this);


	_ready = 1;
};

vzMotionDirector::~vzMotionDirector()
{
	TRACE_POINT();
	for(unsigned int i=0;i<_parameters.count();i++)
	{
		TRACE_POINT();
		delete _parameters.value(i);
		TRACE_POINT();
	};
	TRACE_POINT();
	if (_control)
	{
		TRACE_POINT();
		delete _control;
		TRACE_POINT();
	};
	TRACE_POINT();
	if(_attributes)
	{
		TRACE_POINT();
		delete _attributes;
		TRACE_POINT();
	};
	TRACE_POINT();
	CloseHandle(_lock);
	TRACE_POINT();
};

void vzMotionDirector::assign(long frame, int field)
{
	// if not ready return
	if(!(_ready)) return;

	// lock
	WaitForSingleObject(_lock,INFINITE);

	// check state
	if(_run)
	{
		// running
		
		// now we should check if it first run on not
		if (_first_run)
		{
			_first_run = NULL;
			_last_frame = frame;
		};

		// stop tag
		long _pos_prev = _pos;
		long _pos_new = _pos + frame - _last_frame;
		long _pos_stop;
		if((_control) && ((_pos_stop = _control->find_stop(_pos_prev,_pos_new)) != (-1)))
		{
			// stop tag found
			_pos = _pos_stop;
			_run = 0;
			_first_run = 1;
		}
		else
		{
			// stop tag not found

			// move position of timeline
			_pos += frame - _last_frame;
		};
		
		// check if position is outside of timeline
		if(_pos >= _dur)
		{
			// position is bigger then lenght

			// in a case of single run
			if(_loop)
			{
				// in a case of loop move circly
				_pos = _pos % _dur;
			}
			else
			{
				// if not loop - we reached the end of timeline
				
				// set position into the end
				_pos = _dur - 1;

				// set state int stopped
				_run = 0;
			};
		};

		_last_frame = frame;
	}
	else
	{
		// stopped
	};

	// in stopped state there are not fields
	if (!(_run))
		field = 0;

	// ... and now, for all of childre .....
	for(unsigned i =0 ;i<_parameters.count();i++)
		_parameters.value(i)->assign(_pos,field);

	ReleaseMutex(_lock);

	// this is a "vector operator"
	// [_pos,_field,_last_frame,_first_run] = F x [frame,field,_pos,_first_run,_last_frame,_ready)
};

void vzMotionDirector::start(char* arg)
{
	if(!(_ready)) return;
	WaitForSingleObject(_lock,INFINITE);

	_first_run = 1;
	_run = 1;
	_pos = atol(arg);
	
	ReleaseMutex(_lock);
};

void vzMotionDirector::stop(char* arg)
{
	if(!(_ready)) return;
	WaitForSingleObject(_lock,INFINITE);

	_run = 0;

	ReleaseMutex(_lock);
};

void vzMotionDirector::cont(char* arg)
{
	if(!(_ready)) return;
	WaitForSingleObject(_lock,INFINITE);

	_first_run = 1;
	_run = 1;
	_pos += 1;

	ReleaseMutex(_lock);
};

void vzMotionDirector::reset(char* arg)
{
	if(!(_ready)) return;
	WaitForSingleObject(_lock,INFINITE);

	if(*arg)
		/* goto given frame and stop */
		_pos = atol(arg);
	else
		_pos = 0;

	_run = 0;

	ReleaseMutex(_lock);
};

