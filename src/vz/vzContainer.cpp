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
	2006-12-13:
		*Attempt to submit container object to plugin.

	2005-06-24:
		*Fixed 'draw' method to use '_visible' parameter.

	2005-06-11:
		*To avoid scene rebuilding added parameter to 'RenderSession' struct,
		containers draw order depends on '.order' value.

    2005-06-08:
		*Code cleanup

*/


//vzContainer

#include "vzContainer.h"
#include "unicode.h"

#include <string.h>
#include <stdio.h>


static const unsigned short tag_container[] = {'c','o','n','t','a','i','n','e','r',0};
static const unsigned short tag_function[] = {'f','u','n','c','t','i','o','n',0};

vzContainer::vzContainer(DOMNodeX* parent_node,vzFunctions* functions_list,vzScene* scene)
{
	nodes_counter = 0;
	char node_name[1024];

	// load attributes of this container
	_attributes = new vzXMLAttributes(parent_node);

	// assign visible attribute
	_visible = 1;
	char* temp;
	if(temp = _attributes->find("visible"))
		_visible = atol(temp);

	// check if id registred?
	if(temp = _attributes->find("id"))
		scene->register_container(temp,this);

/*
	// init function "list"
	_functions = new vzHash<vzContainerFunction*>();

	// init containers "list"
	_containers = new vzHash<vzContainer*>();
*/

	// request list of child nodes
	DOMNodeListX* children = parent_node->getChildNodes();

	// enumerate
	for(unsigned int i=0;i<children->getLength();i++)
	{
		// getting child
		DOMNodeX* child = children->item(i);

		// checking type
		if(child->getNodeType() !=  DOMNodeX::ELEMENT_NODE)
			continue;

		// check node name for 'function'
		if (XMLStringX::compareIString(child->getNodeName(),tag_function) == 0)
		{
			// init function Instance object
			vzContainerFunction* function = new vzContainerFunction(child,functions_list,scene,this);

			// push it into functions hash
			sprintf(node_name,"#%04d",++nodes_counter); // create internal id
			_functions.push(node_name,function);

			continue;
		};

		// check node name for 'container'
		if (XMLStringX::compareIString(child->getNodeName(),tag_container) == 0)
		{
			// init function Instance object
			vzContainer* container = new vzContainer(child,functions_list,scene);

			// N.B. !!!!!!
			// if function have ID we should register in
			// scene functions enumerator

			// push it into containers hash
			sprintf(node_name,"#%04d",++nodes_counter); // create internal id
			_containers.push(node_name,container);

			continue;
		};

	};

};

vzContainer::~vzContainer()
{
	unsigned int i;
	
	// delete functions
	for(i=0;i<_functions.count();i++)
		delete (vzContainerFunction*)_functions.value(i);

	// delete containers
	for(i=0;i<_containers.count();i++)
		delete (vzContainer*)_containers.value(i);

	delete _attributes;

};

void vzContainer::draw(vzRenderSession* render_session)
{
	// check if it visible
	if(!(_visible)) return;

	unsigned int i;

	/* save origin parent */
	void* parent = render_session->container;

	/* setup new parent - this */
	render_session->container = this;

	// 1. prerender of all function
	for(i=0;i<_functions.count();i++)
		_functions.value(i)->prerender(render_session);

	// 2. render them
	for(i=0;i<_functions.count();i++)
		_functions.value(i)->render(render_session);

	// 3. child containers
	if(render_session->order == 0)
	{
		// direct (ussualy with dst
		for(i=0;i<_containers.count();i++)
			_containers.value(i)->draw(render_session);
	}
	else
	{
		// reverse (ussualy with src alpha)
		for(i=_containers.count();i>0;i--)
			_containers.value(i - 1)->draw(render_session);
	};

	/* setup new parent - this */
	render_session->container = this;

	// 4. post-render functions
	for(i=_functions.count();i>0;i--)
		_functions.value(i - 1)->postrender(render_session);

	/* restore orignal container */
	render_session->container = parent;
};

void vzContainer::visible(char* value)
{
	_visible = atol(value);
};

void vzContainer::visible(long value)
{
	_visible = value;
};

long vzContainer::visible()
{
	return _visible;
};

