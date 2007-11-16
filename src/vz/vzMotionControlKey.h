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

#ifndef VZMOTIONCONTROLKEY
#define VZMOTIONCONTROLKEY

#include "xerces.h"

#include "vzXMLAttributes.h"

class vzScene;

class vzMotionControlKey
{
	// ready flag signes compleeted attrs list
	int _ready;

	// parameter name
	char* _id;

	long _action;
	long _time;
	long _enable;
	char* _value;

	int copystrval(char* &variable,char* val);

public:

	vzMotionControlKey(DOMNodeX* node,vzScene* scene);
	~vzMotionControlKey();


	inline char* id(void) { return _id;};
	inline long action(void) {return _action;}
	inline long time(void) { return _time;};
	inline char* value(void) {return _value;}
	inline long enable(void) { return _enable;};
	inline int ready(void) { return _ready;};


	inline long action(long i) {return (_action = i);}
	inline long action(char* i) {return (_action = (long)atoi(i));}

	inline long time(long i) { return (_time = i);};
	inline long time(char* i) { return (_time = (long)atoi(i));};

	inline long enable(long i) { return (_enable = i);};
	inline long enable(char* i) { return (_enable = (long)atoi(i));};

	inline char* value(char* i) {return copystrval(_value,i)?_value:NULL;}
};

#endif // VZMOTIONCONTROLKEY