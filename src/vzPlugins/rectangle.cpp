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
	2005-06-08: Code Cleanup. Modification centering algoritms

*/

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"
#include "../vz/plugin-procs.h"


// declare name and version of plugin
PLUGIN_EXPORT vzPluginInfo info =
{
	"rectangle",
	1.0,
	"rc7"
};

// internal structure of plugin
typedef struct
{
	float f_width;
	float f_height;
	float f_colour_R;
	float f_colour_G;
	float f_colour_B;
	float f_colour_A;
	long L_center;
} vzPluginData;

// default value of structure
vzPluginData default_value = 
{
	0.0f,
	0.0f,

	0.0f,
	0.0f,
	0.0f,
	0.0f,
	GEOM_CENTER_CM
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"f_width", "Width", PLUGIN_PARAMETER_OFFSET(default_value,f_width)},
	{"f_height", "Height", PLUGIN_PARAMETER_OFFSET(default_value,f_height)},

	{"f_colour_R", "Red colour component", PLUGIN_PARAMETER_OFFSET(default_value,f_colour_R)},
	{"f_colour_G", "Green colour component", PLUGIN_PARAMETER_OFFSET(default_value,f_colour_G)},
	{"f_colour_B", "Blue colour component", PLUGIN_PARAMETER_OFFSET(default_value,f_colour_B)},
	{"f_colour_A", "Alpha colour component", PLUGIN_PARAMETER_OFFSET(default_value,f_colour_A)},

	{"L_center", "Center of rectanle", PLUGIN_PARAMETER_OFFSET(default_value,L_center)},

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
};

PLUGIN_EXPORT void postrender(void* data,vzRenderSession* session)
{
};

PLUGIN_EXPORT void render(void* data,vzRenderSession* session)
{
	// determine center offset 
	float co_X = 0.0f, co_Y = 0.0f, co_Z = 0.0f;

	// translate coordinates accoring to base image
	center_vector(_DATA->L_center,_DATA->f_width,_DATA->f_height,co_X,co_Y);

	glBegin(GL_QUADS);

	// set colour
	glColor4f
	(
		((vzPluginData*)data)->f_colour_R,
		((vzPluginData*)data)->f_colour_G,
		((vzPluginData*)data)->f_colour_B,
		((vzPluginData*)data)->f_colour_A * session->f_alpha
	);

	glVertex3f(co_X + 0.0f, co_Y + 0.0f, co_Z + 0.0f);
	glVertex3f(co_X + 0.0f, co_Y + _DATA->f_height, co_Z + 0.0f);
	glVertex3f(co_X + _DATA->f_width, co_Y + _DATA->f_height, co_Z + 0.0f);
	glVertex3f(co_X + _DATA->f_width, co_Y + 0.0f, co_Z + 0.0f);

	glEnd(); // Stop drawing QUADS

};

PLUGIN_EXPORT void notify(void* data)
{

};
