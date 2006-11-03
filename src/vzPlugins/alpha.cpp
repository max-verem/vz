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

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"


// declare name and version of plugin
PLUGIN_EXPORT vzPluginInfo info =
{
	"alpha",
	1.0,
	"rc8"
};

// internal structure of plugin
typedef struct
{
	float f_value;
	float f_prev_value;
} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
	0.0f,
	0.0f
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"f_value", "Alpha value", PLUGIN_PARAMETER_OFFSET(default_value,f_value)},
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
	// free data
	free(data);
};

PLUGIN_EXPORT void prerender(void* data,vzRenderSession* session)
{
	// save value of alpha channel
	_DATA->f_prev_value = session->f_alpha;
	session->f_alpha *= _DATA->f_value;
};

PLUGIN_EXPORT void postrender(void* data,vzRenderSession* session)
{
	// restore value of alpha channel
	session->f_alpha = _DATA->f_prev_value;
};

PLUGIN_EXPORT void render(void* data,vzRenderSession* session)
{

};

PLUGIN_EXPORT void notify(void* data)
{

};
