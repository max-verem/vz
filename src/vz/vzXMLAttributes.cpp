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

#include "vzXMLAttributes.h"

#include "unicode.h"

static const unsigned short tag_param[] = {'p','a','r','a','m',0};
static const unsigned short tag_value[] = {'v','a','l','u','e',0};
static const unsigned short tag_name[] = {'n','a','m','e',0};

vzXMLAttributes::vzXMLAttributes(DOMNodeX* node) : vzHash<char*>() 
{
	// processing element's attributes
	DOMNamedNodeMapX* attributes = node->getAttributes();
	
	for(unsigned int i=0;i<attributes->getLength();i++)
	{
		DOMNodeX* attribute = attributes->item(i);

		char* temp_value = uni2utf8((short*)attribute->getNodeValue());
		char* temp_name = uni2utf8((short*)attribute->getNodeName());

		if ((temp_name) && (temp_value))
			push(temp_name,temp_value);

		if (temp_name)
			free(temp_name);
	};


};

vzXMLAttributes::~vzXMLAttributes()
{
	// dispose all characters
	for(unsigned int i=0;i<count();i++)
		free(value(i));
};
