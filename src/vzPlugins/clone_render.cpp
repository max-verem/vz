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
#include "../vz/vzMain.h"

// declare name and version of plugin
DEFINE_PLUGIN_INFO("clone_render");

// internal structure of plugin
typedef struct
{
    char* s_master;
    void* _scene;
} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
    NULL, NULL
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
    {"s_master", "id of master plugin", PLUGIN_PARAMETER_OFFSET(default_value, s_master)},

    {NULL,NULL,0}
};


PLUGIN_EXPORT void* constructor(void* scene, void* parent_container)
{
    // init memmory for structure
    vzPluginData* ctx = (vzPluginData*)malloc(sizeof(vzPluginData));

    // copy default value
    *ctx = default_value;

    /* save scene pointer */
    ctx->_scene = scene;

    // return pointer
    return ctx;
};

PLUGIN_EXPORT void destructor(void* data)
{
    // free data
    free(data);
};

PLUGIN_EXPORT void prerender(void* data, vzRenderSession* session)
{
};

PLUGIN_EXPORT void postrender(void* data, vzRenderSession* session)
{
};

PLUGIN_EXPORT void render(void* data, vzRenderSession* session)
{
    vzPluginData* ctx;
    ctx = (vzPluginData*)data;

    if(!ctx || !ctx->s_master)
        return;

    vzContainerFunctionRender(ctx->_scene, ctx->s_master, session);
};

PLUGIN_EXPORT void notify(void* data, char* param_name)
{

};
