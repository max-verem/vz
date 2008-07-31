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
		*Code start


*/
static char* _plugin_description = 
"Control container visiblitiy, usefull for "
"turning off container from timeline operations."
;

static char* _plugin_notes = 
""
;

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"
#include "../vz/vzMain.h"

#define VALUE_VISIBLE		FOURCC_TO_LONG('_','V','I','S')	/* '_VIS' */
#define VALUE_INVISIBLE		FOURCC_TO_LONG('_','I','N','V') /* '_INV' */

// declare name and version of plugin
DEFINE_PLUGIN_INFO("ctrl_container");

// internal structure of plugin
typedef struct
{
	long L_value;
	void* _parent;
} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
	VALUE_VISIBLE,
	NULL
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"L_value",				"Parent container visiblity control parameter. "
							"Set value '_VIS' to enable container or "
							"set value '_INV' to disable container", 
							PLUGIN_PARAMETER_OFFSET(default_value,L_value)},
	{NULL,NULL,0}
};


PLUGIN_EXPORT void* constructor(void* scene, void* parent_container)
{
	// init memmory for structure
	vzPluginData* data = (vzPluginData*)malloc(sizeof(vzPluginData));

	// copy default value
	*data = default_value;

	/* setup parent container */
	_DATA->_parent = parent_container;

	// return pointer
	return data;
};

PLUGIN_EXPORT void destructor(void* data)
{
	// free data
	free(data);
};

PLUGIN_EXPORT void prerender(void* data,vzRenderSession* session)
{
};

PLUGIN_EXPORT void postrender(void* data,vzRenderSession* session)
{
};

PLUGIN_EXPORT void render(void* data,vzRenderSession* session)
{
};

PLUGIN_EXPORT void notify(void* data, char* param_name)
{
	/* set value for container data */
	if
	(
		(VALUE_VISIBLE == _DATA->L_value)
		&&
		(NULL != _DATA->_parent)
	)
		vzContainerVisible(_DATA->_parent, 1);
	
	if
	(
		(VALUE_INVISIBLE == _DATA->L_value)
		&&
		(NULL != _DATA->_parent)
	)
		vzContainerVisible(_DATA->_parent, 0);
};
