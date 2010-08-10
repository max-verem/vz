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
"This plugin perform datasource interface of calculated data "
"controlled by approximation function."
;

static char* _plugin_notes = 
"Some userfull function:\n"
"\n"
"*setup value*:\n"
"\n"
"    SET(<value>)\n"
"\n"
"    example:\n"
"        vzCmdSender.exe localhost SET rect_offset_table s_cmd \"SET(120);SET(120)\"\n"
"\n"
"*linear move to*:\n"
"\n"
"    LINETO(<value>, <speed>, <fields>)\n"
"        if <fields> is zero then <speed> is ignored.\n"
"        <speed> is value per field.\n"
"\n"
"    example:\n"
"        vzCmdSender.exe localhost SET rect_offset_table s_cmd \"LINETO(180, 1.1, 0);LINETO(121, 1.2, 0)\"\n"
"        vzCmdSender.exe localhost SET rect_offset_table s_cmd \"LINETO(180, 0, 150);LINETO(121, 0, 150)\"\n"
"\n"
"*parabolla move to*:\n"
"\n"
"    PARABOLATO(<value>, <fields>, <Tc>, <Vc>)\n"
"        where:\n"
"        <Tc> - center of parabola\n"
"        <Vc> - value in center of parabola\n"
"\n"
"    example:\n"
"        # speed rized\n"
"        vzCmdSender.exe localhost SET rect_offset_table s_cmd \"PARABOLATO(180,150,1);PARABOLATO(180,150,1)\"\n"
"        # speed slowed down\n"
"        vzCmdSender.exe localhost SET rect_offset_table s_cmd \"PARABOLATO(-200,150,3);PARABOLATO(0,150,3)\"\n"
"        # horizontal slowdown, vertial speedup\n"
"        vzCmdSender.exe localhost SET rect_offset_table s_cmd \"PARABOLATO(0,150,3);PARABOLATO(0,150,1)\"\n"
;

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"

#include "trajectory.h"
#include "trajectory_live_func.h"

// declare name and version of plugin
DEFINE_PLUGIN_INFO("trajectory_live");

#define MAX_VALUE_BUF_SIZE 1024

// internal structure of plugin
typedef struct
{
    /* public params */
    char* s_params_list;
    char* s_cmd;

    /* internal state params */
    char** _params_list;
    int _params_count;
    float* _params_values;
    struct trajectory_live_func** _params_funcs;
    void** _params_ctx;

    char* _value_buf;

    HANDLE _lock_update;
} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
    /* public params */
    NULL,                   // char* s_params_list;
    NULL,                   // char* s_cmd;

    /* internal state params */
    NULL,                   // char** _params_list;
    0,                      // int _params_count;
    NULL,                   // float* _params_values;
    NULL,                   // struct trajectory_live_func** _params_funcs;
    NULL,                   // void** _params_ctx;
    NULL,                   // char* _value_buf;

    INVALID_HANDLE_VALUE    // HANDLE _lock_update;
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
    {"s_params_list", "Parameters list to be analized", PLUGIN_PARAMETER_OFFSET(default_value, s_params_list)},
    {"s_cmd", "Command body", PLUGIN_PARAMETER_OFFSET(default_value, s_cmd)},
    {NULL,NULL,0}
};

PLUGIN_EXPORT void* constructor(void* scene, void* parent_container)
{
    // init memmory for structure
    vzPluginData* ctx = (vzPluginData*)malloc(sizeof(vzPluginData));

    // copy default value
    *ctx = default_value;

    // create mutexes
    ctx->_lock_update = CreateMutex(NULL,FALSE,NULL);

    /* allocate buffer */
    ctx->_value_buf = (char*)malloc(MAX_VALUE_BUF_SIZE);
    memset(ctx->_value_buf, 0, MAX_VALUE_BUF_SIZE);

    // return pointer
    return ctx;
};

PLUGIN_EXPORT void destructor(void* data)
{
    vzPluginData* ctx = (vzPluginData*)data;

    /* free values map */
    if(ctx->_params_values) free(ctx->_params_values);

    /* free value buffer */
    if(ctx->_value_buf) free(ctx->_value_buf);

    /* free list */
    if(ctx->_params_list)
    {
        for(int i = 0; ctx->_params_list[i]; i++)
        {
            free(ctx->_params_list[i]);
            if(ctx->_params_funcs[i] && ctx->_params_ctx[i])
                ctx->_params_funcs[i]->destroy(&ctx->_params_ctx[i]);
        };
        free(ctx->_params_list);
        free(ctx->_params_funcs);
        free(ctx->_params_ctx);

    };

    /* delete locks */
    CloseHandle(ctx->_lock_update);

    /* free struct description */
    free(ctx);
};

PLUGIN_EXPORT void prerender(void* data,vzRenderSession* session)
{
};

PLUGIN_EXPORT void postrender(void* data,vzRenderSession* session)
{
};

PLUGIN_EXPORT void render(void* data,vzRenderSession* session)
{
    vzPluginData* ctx = (vzPluginData*)data;

    // try to lock struct
    WaitForSingleObject(ctx->_lock_update,INFINITE);

    for(int i = 0; i < ctx->_params_count; i++)
        if(ctx->_params_funcs[i] && ctx->_params_ctx[i])
            ctx->_params_funcs[i]->calc(ctx->_params_ctx[i], &ctx->_params_values[i]);

    // unlock
    ReleaseMutex(ctx->_lock_update);
};


PLUGIN_EXPORT void notify(void* data, char* param_name)
{
    vzPluginData* ctx = (vzPluginData*)data;

    // try to lock struct
    WaitForSingleObject(ctx->_lock_update,INFINITE);

    /* process parameters list */
    if(!param_name)
    {
        /* split parameters list */
        ctx->_params_count = split_str(ctx->s_params_list, ",", &ctx->_params_list);

        /* check if it parsed */
        if(!ctx->_params_count)
            logger_printf(1, "trajectory_live: FAILED TO PARSE s_params_list=[%s]", ctx->s_params_list);
        else
        {
            /* allocate and setup initial list data */
            ctx->_params_values = (float*)malloc(sizeof(float) * ctx->_params_count);
            for(int i = 0; i < ctx->_params_count; i++)
                ctx->_params_values[i] = 0.0f;

            ctx->_params_funcs = (struct trajectory_live_func**)malloc(sizeof(struct trajectory_live_func*) * ctx->_params_count);
            memset(ctx->_params_funcs, 0, sizeof(struct trajectory_live_func*) * ctx->_params_count);
            ctx->_params_ctx = (void**)malloc(sizeof(void*) * ctx->_params_count);
            memset(ctx->_params_ctx, 0 , sizeof(void*) * ctx->_params_count);
        };
    };

    /* process command */
    if(!param_name || !strcmp(param_name, "s_cmd"))
    {
        char** func_list = NULL;
        int func_cnt;

        /* split params */
        if(func_cnt = split_str(ctx->s_cmd, ";", &func_list))
        {
            for(int i = 0; i < func_cnt && i < ctx->_params_count; i++)
            {
                int l = strlen(func_list[i]);

                /* check of closed */
                if(l < 2) continue;
                if(')' != func_list[i][l - 1]) continue;
                func_list[i][l - 1] = 0; l--;

                /* lookup for function context */
                int l2;
                struct trajectory_live_func* _params_func =
                    trajectory_live_func_lookup(func_list[i], &l2);

                if(_params_func)
                {
                    /* check if current context is available */
                    if(ctx->_params_funcs[i] && ctx->_params_ctx[i])
                        ctx->_params_funcs[i]->destroy(&ctx->_params_ctx[i]);

                    /* setup new function */
                    ctx->_params_funcs[i] = _params_func;
                    ctx->_params_funcs[i]->init(&ctx->_params_ctx[i],
                        func_list[i] + l2 + 1, &ctx->_params_values[i]);
                };
            };
        };

        if(func_list)
            split_str_free(&func_list);
    };

    // unlock
    ReleaseMutex(ctx->_lock_update);
};

PLUGIN_EXPORT long datasource(void* data,vzRenderSession* render_session, long index, char** name, char** value)
{
    int r = 0;
    vzPluginData* ctx = (vzPluginData*)data;

    WaitForSingleObject(ctx->_lock_update,INFINITE);

    if(index >= 0 && index < ctx->_params_count)
    {
        /* setup name of parameter to return */
        *name = ctx->_params_list[index];

        /* setup value of parameter to return */
        *value = ctx->_value_buf;

        /* build value */
        _snprintf(ctx->_value_buf, MAX_VALUE_BUF_SIZE, "%f", ctx->_params_values[index]);

        /* OK */
        r = 1;
    };

    ReleaseMutex(ctx->_lock_update);

    return r;
};
