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
	"rotate",
	1.0,
	"test-rc0"
};

// internal structure of plugin
typedef struct
{
	float f_angle;
	float f_x;
	float f_y;
	float f_z;
} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
	0.0f,
	0.0f,
	0.0f,
	0.0f
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"f_angle", "The angle of rotation, in degrees", PLUGIN_PARAMETER_OFFSET(default_value,f_angle)},
	{"f_x", "The x coordinates of a vector.",PLUGIN_PARAMETER_OFFSET(default_value,f_x)},
	{"f_y", "The y coordinates of a vector.",PLUGIN_PARAMETER_OFFSET(default_value,f_y)},
	{"f_z", "The z coordinates of a vector.",PLUGIN_PARAMETER_OFFSET(default_value,f_z)},
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
	free(data);
};

PLUGIN_EXPORT void prerender(void* data,vzRenderSession* session)
{
	glPushMatrix();
	glRotatef
	(
		((vzPluginData*)data)->f_angle,
		((vzPluginData*)data)->f_x,
		((vzPluginData*)data)->f_y,
		((vzPluginData*)data)->f_z
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
