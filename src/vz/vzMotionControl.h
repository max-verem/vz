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


#ifndef VZMOTIONCONTROL_H
#define VZMOTIONCONTROL_H

#define _CRT_SECURE_NO_WARNINGS

#include "vzMotionControlKey.h"
#include "../templates/hash.hpp"

enum vzMotionControlTypes
{
    vzMotionControlTypeSTOP = 1,
    vzMotionControlTypeINLOOPOUT,
};

class vzMotionControl : public vzHash<vzMotionControlKey*>
{
public:
	vzMotionControl(DOMNodeX* node,vzScene* scene);
	~vzMotionControl();
    vzMotionControlKey* find(long from, long to, enum vzMotionControlTypes type);
};


#endif // VZMOTIONCONTROL_H