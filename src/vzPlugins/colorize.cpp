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

// declare name and version of plugin
DEFINE_PLUGIN_INFO("pixeltransfer");

// internal structure of plugin
typedef struct
{
    float f_red_scale;
    float f_green_scale;
    float f_blue_scale;

    float f_red_bias;
    float f_green_bias;
    float f_blue_bias;

    float _red_scale;
    float _green_scale;
    float _blue_scale;

    float _red_bias;
    float _green_bias;
    float _blue_bias;
} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
    1.0f, 1.0f, 1.0f,
    0.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 1.0f,
    0.0f, 0.0f, 0.0f
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
    {"f_red_scale", "the red component is multiplied by", PLUGIN_PARAMETER_OFFSET(default_value, f_red_scale)},
    {"f_green_scale", "the green component is multiplied by", PLUGIN_PARAMETER_OFFSET(default_value, f_green_scale)},
    {"f_blue_scale", "the blue component is multiplied by", PLUGIN_PARAMETER_OFFSET(default_value, f_blue_scale)},

    {"f_red_bias", "the red component bias", PLUGIN_PARAMETER_OFFSET(default_value, f_red_bias)},
    {"f_green_bias", "the green component bias", PLUGIN_PARAMETER_OFFSET(default_value, f_green_bias)},
    {"f_blue_bias", "the blue component bias", PLUGIN_PARAMETER_OFFSET(default_value, f_blue_bias)},

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

PLUGIN_EXPORT void prerender(void* data, vzRenderSession* session)
{
    vzPluginData* ctx = (vzPluginData*)data;

    /* save prev values */
    glGetFloatv(GL_RED_SCALE, &ctx->_red_scale);
    glGetFloatv(GL_GREEN_SCALE, &ctx->_green_scale);
    glGetFloatv(GL_BLUE_SCALE, &ctx->_blue_scale);
    glGetFloatv(GL_RED_BIAS, &ctx->_red_bias);
    glGetFloatv(GL_GREEN_BIAS, &ctx->_green_bias);
    glGetFloatv(GL_BLUE_BIAS, &ctx->_blue_bias);

    /* setup new values */
    glPixelTransferf(GL_RED_SCALE, ctx->f_red_scale);
    glPixelTransferf(GL_GREEN_SCALE, ctx->f_green_scale);
    glPixelTransferf(GL_BLUE_SCALE, ctx->f_blue_scale);
    glPixelTransferf(GL_RED_BIAS, ctx->f_red_bias);
    glPixelTransferf(GL_GREEN_BIAS, ctx->f_green_bias);
    glPixelTransferf(GL_BLUE_BIAS, ctx->f_blue_bias);
};

PLUGIN_EXPORT void postrender(void* data, vzRenderSession* session)
{
    vzPluginData* ctx = (vzPluginData*)data;

    /* setup new values */
    glPixelTransferf(GL_RED_SCALE, ctx->_red_scale);
    glPixelTransferf(GL_GREEN_SCALE, ctx->_green_scale);
    glPixelTransferf(GL_BLUE_SCALE, ctx->_blue_scale);
    glPixelTransferf(GL_RED_BIAS, ctx->_red_bias);
    glPixelTransferf(GL_GREEN_BIAS, ctx->_green_bias);
    glPixelTransferf(GL_BLUE_BIAS, ctx->_blue_bias);
};

PLUGIN_EXPORT void render(void* data, vzRenderSession* session)
{

};

PLUGIN_EXPORT void notify(void* data, char* param_name)
{

};
