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

#ifndef VZMOTIONDIRECTOR_H
#define VZMOTIONDIRECTOR_H

class vzMotionParameter;

#include "vzXMLAttributes.h"
#include "vzMotionParameter.h"
#include "vzScene.h"

#include "../templates/hash.hpp"

#include "vzMotionControl.h"

class vzMotionDirector
{
	// loaded attributes
	vzXMLAttributes* _attributes;

	// ready flag signes compleeted attrs list
	int _ready;

	// is this first run
	int _first_run;

	// value of last global frame no calced
	long _last_frame;

	// parameter name
	char* _id;

	// run-time attributes
	long 
		_run,	// state of director - running or stopped
		_dur,	// director duration
		_loop,	// loop flags - "-1" mean endless, "10" mean 10 time and stop
		_pos;	// current timeline position

	// timelines
	vzHash<vzMotionParameter*> _parameters;

	// control keys
	vzMotionControl* _control;

	// locks
	HANDLE _lock;

public:
	
	vzMotionDirector(DOMNodeX* node,vzScene* scene);
	~vzMotionDirector();

	inline char* id(void) { return _id;};
	void assign(long frame,int field);


	// control method
	void start(char* arg);
	void stop(char* arg);
	void cont(char* arg);
	void reset(char* arg);
};

#endif
