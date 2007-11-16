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


#include "vzMotion.h"
#include <stdio.h>

static const unsigned short tag_director[] = {'d', 'i', 'r', 'e', 'c', 't', 'o', 'r',0};

vzMotion::vzMotion(DOMNodeX* node,vzScene* scene) : vzHash<vzMotionDirector*>()
{
	// auto id generation for Directors (if not present)
	int directors_counter = 0;
	char director_name[1024];

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
		if (XMLStringX::compareIString(child->getNodeName(),tag_director) == 0)
		{
			// init function Instance object
			vzMotionDirector* director = new vzMotionDirector(child,scene);

			// if id of director given - use it
			char* key = (director->id())?director->id():director_name;

			// create internal id
			sprintf(director_name,"#%04d",++directors_counter); 

			push(key,director);

			continue;
		};
	};
};

vzMotion::~vzMotion()
{
	for(unsigned int i=0;i<count();i++)
		delete value(i);
};

void vzMotion::assign(long frame, int field)
{
	for(unsigned int i=0;i<count();i++)
		value(i)->assign(frame, field);
};

