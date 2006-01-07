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

#include "vzMotionControlKey.h"
#include "vzScene.h"


int vzMotionControlKey::copystrval(char* &variable,char* val)
{
	if(val)
	{
		if(variable) free(variable);
		strcpy(variable = (char*)malloc(strlen(val)+1),val);
		return 1;
	}
	else
		return 0;
};

vzMotionControlKey::vzMotionControlKey(DOMNode* node,vzScene* scene)
{
	// start params
	_id = NULL; 
	_value = NULL;
	_action = 0;
	_time = 0;
	_ready = 0;
	_enable = 1;

	// load params
	vzXMLAttributes* _attributes = new vzXMLAttributes(node);

	// time
	if(_attributes->find("time")) _time = atol(_attributes->find("time")); else 
	{
		delete _attributes; return;
	};

	// action
	if(_attributes->find("action")) _action = atol(_attributes->find("action")); else
	{
		delete _attributes; return;
	};

	// enable
	if(_attributes->find("enable")) _enable = atol(_attributes->find("enable")); else
	{
		_enable = 1;
	};

	// value
	value(_attributes->find("action"));

	// id
	if(_attributes->find("id"))
	{
		copystrval(_id,_attributes->find("id"));
		scene->register_controlkey(_id,this);
	};

	delete _attributes;

	_ready = 1;
};

vzMotionControlKey::~vzMotionControlKey()
{
	if(_id) free(_id);
	if(_value) free(_value);
};
