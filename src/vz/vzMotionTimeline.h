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


#ifndef VZMOTIONTIMELINE_H
#define VZMOTIONTIMELINE_H


#include "vzXMLAttributes.h"
#include "vzContainerFunction.h"
#include "xerces.h"

// N.B.!!! (2005-05-22)
// Only "linear" and "const" approximation function present
// How to code function type? int? YEAR!!!
// 0 - const
// 1 - linear

#include "vzScene.h"

class vzMotionTimeline
{
	// loaded attributes
	vzXMLAttributes* _attributes;

	// ready flag signes compleeted attrs list
	int _ready;

	// parameter name
	char* _param_name;

	// param ptr
	void* _param_ptr;

	// id
	char* _id;

	// function object
	vzContainerFunction* _function;

	// function arguments common parameters
	long _t1,_t2;

	// function id
	long _f;

	//  type-dependent function values
	float _f_y1, _f_y2; 
	long _l_y1, _l_y2;
	long _L_y1, _L_y2; // not approximatable
	char *_s_y1, *_s_y2;// not approximatable

	// lock
	HANDLE _lock;


public:
	vzMotionTimeline(DOMNode* node, vzScene* scene, char* param_name, vzContainerFunction* function);
	~vzMotionTimeline();
	void assign(long frame, int field);

	// control method
	void set_t1(char* value);
	void set_t2(char* value);
	void set_y1(char* value);
	void set_y2(char* value);
};

#endif
