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

	2006-05-01: 
        *memory leak fixed on '_attributes' object

*/
#define _CRT_SECURE_NO_WARNINGS

#include "memleakcheck.h"

#include "vzMotionParameter.h"
#include <stdio.h>


static const XMLCh tag_timeline[] = {'t', 'i', 'm', 'e', 'l', 'i', 'n', 'e',0};


vzMotionParameter::vzMotionParameter(DOMNodeX* node,vzScene* scene) : vzHash<vzMotionTimeline*>()
{
	// auto id generation for timelines hash
	int timelines_counter = 0;
	char timeline_name[1024];
	
	// defaults
	_ready = 0;

	// load attributes
	_attributes = new vzXMLAttributes(node);

	// check if name is defined
	if(!(_name = _attributes->find("name")))
		return;

	// try to find function
	vzContainerFunction* function;
	if(!( function = (vzContainerFunction*)scene->get_ided_object(_name,&_name_local)))
		return;

	// load timelines !!!!

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
		if (XMLStringX::compareIString(child->getNodeName(),tag_timeline) == 0)
		{
			// init function Instance object
			vzMotionTimeline* timeline = new vzMotionTimeline(child,scene,_name_local,function);

			// push it into functions hash
			sprintf(timeline_name,"#%04d",++timelines_counter); // create internal id
			push(timeline_name,timeline);

			continue;
		};
	};

	// all attributes present !!!
	_ready = 1;
};


vzMotionParameter::~vzMotionParameter()
{
	for(unsigned int i=0;i<count();i++)
		delete value(i);
	if(_attributes)
		delete _attributes;
};

void vzMotionParameter::assign(long frame, int field)
{
	// ... and now, for all of childre .....
	for(unsigned i =0 ;i<count();i++)
		value(i)->assign(frame,field);
};
