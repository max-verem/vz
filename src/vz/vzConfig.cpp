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

#include "vzConfig.h"
#include "xerces.h"

#include "vzXMLAttributes.h"

static const unsigned short tag_module[] = {'m', 'o', 'd', 'u', 'l', 'e',0};

vzConfig::vzConfig(char* config_file)
{
	//init parser
	XercesDOMParserX *parser = new XercesDOMParserX;

    try
    {
        parser->parse(config_file);
    }
    catch (...)
    {
		delete parser;
		return;
    }


	DOMDocumentX* doc = parser->getDocument();
	if(!doc)
	{
		return;
	};

	DOMElementX* config = doc->getDocumentElement();

	DOMNodeListX* modules_params = config->getChildNodes();

	for(unsigned int i=0;i<modules_params->getLength();i++)
	{
		// getting child 
		DOMNodeX* module = modules_params->item(i);

		// checking type
		if(module->getNodeType() !=  DOMNodeX::ELEMENT_NODE)
			continue;

		// check node name
		if (XMLStringX::compareIString(module->getNodeName(),tag_module) == 0)
		{
			// load attributes
			vzXMLAttributes* attrs = new vzXMLAttributes(module);

			char* name;

			if (name = attrs->find("name"))
			{
				_attributes.push(name, attrs);
				_parameters.push(name, new vzXMLParams(module));
			}
			else
			{
				delete attrs;
			};
		};
	};

	delete parser;
};

vzConfig::~vzConfig()
{
	unsigned int i;

	// delete attrs
	for(i = 0;i<_attributes.count();i++)
		delete _attributes.value(i);

	// delete attrs
	for(i = 0;i<_parameters.count();i++)
		delete _parameters.value(i);

};

char* vzConfig::param(char* module,char* name)
{
	// try to find pointer to hash with attributes
	vzXMLParams* target_module = _parameters.find(module);

	// return found value
	return (target_module)?target_module->find(name):NULL;
};

char* vzConfig::attr(char* module,char* name)
{
	// try to find pointer to hash with attributes
	vzXMLAttributes* target_module = _attributes.find(module);

	// return found value
	return (target_module)?target_module->find(name):NULL;
};
