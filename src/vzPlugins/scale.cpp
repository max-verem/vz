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
	"scale",
	1.0,
	"rc6"
};

// internal structure of plugin
typedef struct
{
	float f_scale_x;
	float f_scale_y;
	float f_scale_z;
} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
	1.0f,
	1.0f,
	1.0f
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"f_scale_x", "Specify scale factors along the x axes.",PLUGIN_PARAMETER_OFFSET(default_value,f_scale_x)},
	{"f_scale_y", "Specify scale factors along the y axes.",PLUGIN_PARAMETER_OFFSET(default_value,f_scale_y)},
	{"f_scale_z", "Specify scale factors along the z axes.",PLUGIN_PARAMETER_OFFSET(default_value,f_scale_z)},
	{NULL,NULL,0}
};


PLUGIN_EXPORT void* constructor(void)
{
//	DEBUG_PROC_ENTRY("contructor","enter")
	// init memmory for structure
	vzPluginData* data = (vzPluginData*)malloc(sizeof(vzPluginData));

	// copy default value
	*data = default_value;

	// return pointer
//	DEBUG_PROC_ENTRY("contructor","exit")
	return data;
};

PLUGIN_EXPORT void destructor(void* data)
{
//	DEBUG_PROC_ENTRY("destructor","enter")
	free(data);
//	DEBUG_PROC_ENTRY("destructor","exit")
};

PLUGIN_EXPORT void prerender(void* data,vzRenderSession* session)
{
	glPushMatrix();
	glScalef
	(
		((vzPluginData*)data)->f_scale_x,
		((vzPluginData*)data)->f_scale_y,
		((vzPluginData*)data)->f_scale_z
	);
};

PLUGIN_EXPORT void postrender(void* data,vzRenderSession* session)
{
	glPopMatrix();
};

PLUGIN_EXPORT void render(void* data,vzRenderSession* session)
{

};

PLUGIN_EXPORT void notify(void* data)
{

};
