/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2011 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1stereo.tv, 2011.

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
*/
static char* _plugin_description = 
""
;

static char* _plugin_notes = 
""
;

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"
#include "../vz/plugin-procs.h"

#include <math.h>

#ifndef PI
#define PI 3.1415926535897931
#endif

// declare name and version of plugin
DEFINE_PLUGIN_INFO("fillarc");

// internal structure of plugin
typedef struct
{
    float f_a;
    float f_r;
    float f_angle_from;
    float f_angle_to;
    float f_colour_R;
    float f_colour_G;
    float f_colour_B;
    float f_colour_A;
} vzPluginData;

// default value of structure
vzPluginData default_value = 
{
    1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
    {"f_a", "Angle triag step, defualt is 1 degree", PLUGIN_PARAMETER_OFFSET(default_value, f_a)},
    {"f_r", "Radius", PLUGIN_PARAMETER_OFFSET(default_value, f_r)},
    {"f_angle_from", "Start angle (degrees: 0..360)", PLUGIN_PARAMETER_OFFSET(default_value, f_angle_from)},
    {"f_angle_to", "Start angle (degrees: 0..360)", PLUGIN_PARAMETER_OFFSET(default_value, f_angle_to)},
    {"f_colour_R", "Red colour component", PLUGIN_PARAMETER_OFFSET(default_value, f_colour_R)},
    {"f_colour_G", "Green colour component", PLUGIN_PARAMETER_OFFSET(default_value, f_colour_G)},
    {"f_colour_B", "Blue colour component", PLUGIN_PARAMETER_OFFSET(default_value, f_colour_B)},
    {"f_colour_A", "Alpha colour component", PLUGIN_PARAMETER_OFFSET(default_value, f_colour_A)},
    {NULL,NULL,0}
};


PLUGIN_EXPORT void* constructor(void* scene, void* parent_container)
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
    float t, A, B, S;
    vzPluginData* ctx = (vzPluginData*)data;

    /* recalc angles to radians */
    A = PI * ctx->f_angle_from / 180.0f;
    B = PI * ctx->f_angle_to / 180.0f;
    S = PI * ctx->f_a / 180.0f;

    glBegin(GL_TRIANGLE_FAN);

    // set colour
    glColor4f
    (
        ctx->f_colour_R, ctx->f_colour_G, ctx->f_colour_B,
        ctx->f_colour_A * session->f_alpha
    );

    glVertex3f(0, 0, 0);

    /* draw rectangles */
    for(t = A; t <= B; t += S)
        glVertex3f(ctx->f_r * cosf(t), ctx->f_r * sinf(t), 0);

    glVertex3f(0, 0, 0);

    glEnd();

};

PLUGIN_EXPORT void notify(void* data, char* param_name)
{

};
