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

#include "memleakcheck.h"

#include "vzMotionControl.h"
#include "vzScene.h"
#include <stdio.h>


static const XMLCh tag_key[] = {'k', 'e', 'y',0};

vzMotionControl::vzMotionControl(DOMNodeX* node,vzScene* scene) : vzHash<vzMotionControlKey*>()
{
	// auto id generation for timelines hash
	int keys_counter = 0;
	char key_name[1024];
	
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
		if (XMLStringX::compareIString(child->getNodeName(),tag_key) == 0)
		{
			// init function Instance object
			vzMotionControlKey* key = new vzMotionControlKey(child,scene);

			// push it into functions hash
			sprintf(key_name,"#%04d",++keys_counter); // create internal id
			push(key_name,key);

			continue;
		};
	};

};

vzMotionControl::~vzMotionControl()
{
	for(unsigned int i=0;i<count();i++)
		delete value(i);
};

vzMotionControlKey* vzMotionControl::find(long from, long to, enum vzMotionControlTypes type)
{
    for(unsigned int i=0;i<count();i++)
    {
        vzMotionControlKey* key = value(i);

        if(key->time() > to) continue;
        if(key->time() < from) continue;
        if(!key->enable()) continue;
        if(key->action() != type) continue;
        return key;
    };
    return NULL;
};
