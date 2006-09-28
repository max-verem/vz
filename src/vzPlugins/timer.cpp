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

ChangleLog:
	2005-06-08: Code Cleanup. 

*/

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"


// declare name and version of plugin
PLUGIN_EXPORT vzPluginInfo info =
{
	"timer",
	1.0,
	"rc6"
};

static char* working_param = "s_text";

// internal structure of plugin
typedef struct
{
	char* s_format;
	char* _value;
	long _c;
} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
	NULL,
	NULL,
	0
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"s_format", "Timer format", PLUGIN_PARAMETER_OFFSET(default_value,s_format)},
	{NULL,NULL,0}
};


PLUGIN_EXPORT void* constructor(void)
{
	// init memmory for structure
	vzPluginData* data = (vzPluginData*)malloc(sizeof(vzPluginData));

	// copy default value
	*data = default_value;

	// return pointer
	return data;
};

PLUGIN_EXPORT void destructor(void* data)
{
	if (_DATA->_value) free(_DATA->_value);
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

PLUGIN_EXPORT void notify(void* data)
{

};

PLUGIN_EXPORT long datasource(void* data,vzRenderSession* render_session, long index, char** name, char** value)
{
	if (_DATA->_value == NULL)
	{
		_DATA->_value = (char*)malloc(128);
		strcpy(_DATA->_value, "PriVed");
	};
	sprintf(_DATA->_value, "%.6d", _DATA->_c++);

	*name = working_param;
	*value = _DATA->_value;

	return (index < 1)?1:0;
};
