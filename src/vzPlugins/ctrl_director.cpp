/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2010 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1stereo.tv, 2010.

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
"Control director behaviour, usefull for "
"director operation from trajectory."
;

static char* _plugin_notes = 
""
;

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"
#include "../vz/vzMain.h"

#include <stdio.h>

// declare name and version of plugin
DEFINE_PLUGIN_INFO("ctrl_director");

// internal structure of plugin
typedef struct
{
    char* s_director;       /* director name */

    long l_trig_start;
    long l_trig_stop;
    long l_trig_continue;
    long l_trig_reset;

    void* _scene;

    long _trig_start;
    long _trig_stop;
    long _trig_continue;
    long _trig_reset;

} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
    NULL,

    -1,
    -1,
    -1,
    -1,

    NULL,

    -1,
    -1,
    -1,
    -1,
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
    {"s_director",      "director name top operate with",
                        PLUGIN_PARAMETER_OFFSET(default_value, s_director)},

    {"l_trig_start",    "trigger director start",
                        PLUGIN_PARAMETER_OFFSET(default_value, l_trig_start)},

    {"l_trig_stop",     "trigger director stop",
                        PLUGIN_PARAMETER_OFFSET(default_value, l_trig_stop)},

    {"l_trig_continue", "trigger director continue",
                        PLUGIN_PARAMETER_OFFSET(default_value, l_trig_continue)},

    {"l_trig_reset",    "trigger director reset",
                        PLUGIN_PARAMETER_OFFSET(default_value, l_trig_reset)},

    {NULL,NULL,0}
};


PLUGIN_EXPORT void* constructor(void* scene, void* parent_container)
{
    // init memmory for structure
    vzPluginData* data = (vzPluginData*)malloc(sizeof(vzPluginData));

    // copy default value
    *data = default_value;

    /* setup parent container */
    _DATA->_scene = scene;

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

#define MAX_CMD_SIZE 1024

PLUGIN_EXPORT void notify(void* data, char* param_name)
{
    vzPluginData* ctx = (vzPluginData*)data;
    char* error_log;
    static const char* tmpl = "tree.motion.director";
    char buf[MAX_CMD_SIZE] = "";

    if(!param_name) return;

    if(!ctx->s_director) return;

    if(!strcmp(param_name, "l_trig_start"))
    {
        if(ctx->l_trig_start && ctx->_trig_start != ctx->l_trig_start)
            _snprintf(buf, MAX_CMD_SIZE, "%s.%s.start(%d)",
                tmpl, ctx->s_director, ctx->l_trig_start - 1);

        ctx->_trig_start = ctx->l_trig_start;
    }
    else if(!strcmp(param_name, "l_trig_stop"))
    {
        if(ctx->l_trig_stop && ctx->_trig_stop != ctx->l_trig_stop)
            _snprintf(buf, MAX_CMD_SIZE, "%s.%s.stop()",
                tmpl, ctx->s_director);

        ctx->_trig_stop = ctx->l_trig_stop;
    }
    else if(!strcmp(param_name, "l_trig_continue"))
    {
        if(ctx->l_trig_continue && ctx->_trig_continue != ctx->l_trig_continue)
            _snprintf(buf, MAX_CMD_SIZE, "%s.%s.continue()",
                tmpl, ctx->s_director);

        ctx->_trig_continue = ctx->l_trig_continue;
    }
    else if(!strcmp(param_name, "l_trig_reset"))
    {
        if(ctx->l_trig_reset && ctx->_trig_reset != ctx->l_trig_reset)
            _snprintf(buf, MAX_CMD_SIZE, "%s.%s.reset(%d)",
                tmpl, ctx->s_director, ctx->l_trig_reset - 1);

        ctx->_trig_reset = ctx->l_trig_reset;
    }

    if(buf[0])
        vzMainSceneCommand(ctx->_scene, buf, &error_log);
};
