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
"This plugin perform sequenced text source"
;

static char* _plugin_notes = 
"\n"
;

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"

#include "trajectory.h"

// declare name and version of plugin
DEFINE_PLUGIN_INFO("text_seq");

struct text_seq_item
{
    char* body;
    int offset;
};

// internal structure of plugin
typedef struct
{
    /* public params */
    char* s_param;
    char* s_seq;
    long l_trig_start;

    /* internal state params */
    struct text_seq_item* _seq_list;
    int _seq_len;
    int _seq_idx;
    int _seq_offset;

    HANDLE _lock_update;
} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
    /* public params */
    NULL,                   // char* s_param;
    NULL,                   // char* s_seq;
    0,                      // long l_trig_start;

    /* internal state params */
    NULL,                   // struct text_seq_item* _seq_list;
    0,                      // int _seq_len;
    0,                      // int _seq_idx;
    0,                      // int _seq_offset;

    INVALID_HANDLE_VALUE    // HANDLE _lock_update;
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
    {"s_param", "Target plugin's field name to update", PLUGIN_PARAMETER_OFFSET(default_value, s_param)},
    {"s_seq", "Sequential", PLUGIN_PARAMETER_OFFSET(default_value, s_seq)},
    {"l_trig_start", "", PLUGIN_PARAMETER_OFFSET(default_value, l_trig_start)},
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

    // return pointer
    return ctx;
};

inline void free_seq_list(vzPluginData* ctx)
{
    /* free sequential buffer */
    if(ctx->_seq_list)
    {
        for(int i = 0; i < ctx->_seq_len; i++)
            if(ctx->_seq_list[i].body)
                free(ctx->_seq_list[i].body);

        free(ctx->_seq_list);

        ctx->_seq_list = NULL;
        ctx->_seq_len = NULL;
    };
};

PLUGIN_EXPORT void destructor(void* data)
{
    vzPluginData* ctx = (vzPluginData*)data;

    free_seq_list(ctx);

    /* delete locks */
    CloseHandle(ctx->_lock_update);

    /* free struct description */
    free(ctx);
};

PLUGIN_EXPORT void prerender(void* data, vzRenderSession* session)
{
};

PLUGIN_EXPORT void postrender(void* data, vzRenderSession* session)
{
};

PLUGIN_EXPORT void render(void* data, vzRenderSession* session)
{
};


PLUGIN_EXPORT void notify(void* data, char* param_name)
{
    vzPluginData* ctx = (vzPluginData*)data;

    // try to lock struct
    WaitForSingleObject(ctx->_lock_update,INFINITE);

    /* process s_seq */
    if(!param_name || !strcmp(param_name, "s_seq"))
    {
        /* reset trigger */
        ctx->l_trig_start = 0;
        ctx->_seq_offset = 0;

        /* free prev seq */
        free_seq_list(ctx);

        /* build new */
        char** parts;
        int len = split_str(ctx->s_seq, "^", &parts);
        if(len)
        {
            int l, i;
            char* body;

            for(i = 0; i < len; i++)
            {
                struct text_seq_item item;

                /* get the length */
                l = strlen(parts[i]);

                /* alloc temp body buffer */
                item.body = (char*)malloc(l + 1);

                /* try to parse */
                body = find_ch(parts[i], ':');

                /* check */
                if(!body)
                    free(item.body);
                else
                {
                    /* scan offset */
                    sscanf(parts[i], "%d", &item.offset);
                    /* copy body */
                    strncpy(item.body, body + 1, l);

                    /* insert into the list */
                    if(!ctx->_seq_list)
                    {
                        ctx->_seq_list = (struct text_seq_item*)malloc(sizeof(struct text_seq_item));
                        ctx->_seq_list[0] = item;
                        ctx->_seq_len = 1;
                    }
                    else
                    {
                        ctx->_seq_list = (struct text_seq_item*)realloc(ctx->_seq_list,
                            sizeof(struct text_seq_item) * (ctx->_seq_len + 1));
                        ctx->_seq_list[ctx->_seq_len] = item;
                        ctx->_seq_len++;
                    };

                    /* recalc offset */
                    if(ctx->_seq_len > 1)
                        ctx->_seq_list[ctx->_seq_len - 1].offset += ctx->_seq_list[ctx->_seq_len - 2].offset;
                };
            };
        };
        /* free used parts */
        if(parts) split_str_free(&parts);

        /* mark cmd as dirty */
        if(ctx->s_seq)
            ctx->s_seq[0] = 0;
    };

    /* process l_trig_start */
    if(param_name && !strcmp(param_name, "l_trig_start"))
    {
        if(!ctx->_seq_len) ctx->l_trig_start = 0;

        ctx->_seq_offset = 0;
    };

    // unlock
    ReleaseMutex(ctx->_lock_update);
};

PLUGIN_EXPORT long datasource(void* data,vzRenderSession* render_session, long index, char** name, char** value)
{
    int r = 0;
    vzPluginData* ctx = (vzPluginData*)data;

    WaitForSingleObject(ctx->_lock_update,INFINITE);

    if(ctx->_seq_len && !index && ctx->l_trig_start)
    {
        int i;

        /* find the text value to send */
        for(i = 0; i < ctx->_seq_len && !r; i++)
        {
            if(ctx->_seq_list[i].offset == ctx->_seq_offset)
            {
                r = 1;
                *name = ctx->s_param;
                *value = ctx->_seq_list[i].body;
            };
        };

        /* inc counter */
        ctx->_seq_offset++;

        /* trigger off on the end */
        if(ctx->_seq_list[ctx->_seq_len - 1].offset < ctx->_seq_offset)
            ctx->l_trig_start = 0;
    };

    ReleaseMutex(ctx->_lock_update);

    return r;
};
