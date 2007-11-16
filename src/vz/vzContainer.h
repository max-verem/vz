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


#ifndef VZCONTAINER_H
#define VZCONTAINER_H

#include "vzXMLAttributes.h"
#include "vzContainerFunction.h"
#include "vzFunctions.h"

#include "xerces.h"
#include "../templates/hash.hpp"

#include "vzScene.h"

class vzContainer
{
	// function in container
	vzHash<vzContainerFunction*> _functions;
	
	// child containers
	vzHash<vzContainer*> _containers;

	// 'visible' attribute
	long _visible;

	// attributes
	vzXMLAttributes* _attributes;

	// nodes counter
	unsigned long nodes_counter;

public:
	vzContainer(DOMNodeX* parent_node,vzFunctions* functions_list,vzScene* scene);
	void draw(vzRenderSession* render_session);
	virtual ~vzContainer();
	void visible(char* value);
	void visible(long value);
	long visible();
};


#endif

