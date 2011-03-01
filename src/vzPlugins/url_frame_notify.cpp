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
"URL timer. "
"Download url for specific frames interval"
;

static char* _plugin_notes = 
""
;

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"
#include "../vz/vzMain.h"

#include <stdio.h>

// declare name and version of plugin
DEFINE_PLUGIN_INFO("url_frame_notify");

// internal structure of plugin
typedef struct
{
    char* s_url;       /* url to download */
    long l_delay;

    long _last_url;
    long _last_frame;
    HANDLE _th;
    char* _s_url;
} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
    NULL,

    12,
    0,
    0,
    INVALID_HANDLE_VALUE,
    NULL,
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
    {"s_url",           "URL to download",
                        PLUGIN_PARAMETER_OFFSET(default_value, s_url)},

    {"l_delay",         "frames delay",
                        PLUGIN_PARAMETER_OFFSET(default_value, l_delay)},

    {NULL,NULL,0}
};

//#define CURL_STATICLIB
#include <curl/curl.h>

#ifdef CURL_STATICLIB

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Wldap32.lib")

#ifdef _DEBUG
#pragma comment(lib, "libcurl_DEBUG_STATIC.lib")
#else
#pragma comment(lib, "libcurl_RELEASE_STATIC.lib")
#endif

#else

#ifdef _DEBUG
#pragma comment(lib, "libcurl_DEBUG_DLL.lib")
#else
#pragma comment(lib, "libcurl_RELEASE_DLL.lib")
#endif

#endif /* CURL_STATICLIB */

#define MAX_URL_LEN 2048

PLUGIN_EXPORT void* constructor(void* scene, void* parent_container)
{
    // init memmory for structure
    vzPluginData* data = (vzPluginData*)malloc(sizeof(vzPluginData));

    // copy default value
    *data = default_value;

    /* allocate space for url body */
    data->_s_url = (char*)malloc(MAX_URL_LEN);

    // return pointer
    return data;
};

PLUGIN_EXPORT void destructor(void* data)
{
    vzPluginData* ctx = (vzPluginData*)data;

    /* wait for thread finish */
    if(INVALID_HANDLE_VALUE != ctx->_th)
    {
        WaitForSingleObject(ctx->_th, INFINITE);
        CloseHandle(ctx->_th);
    };

    /* free url body */
    free(ctx->_s_url);

    // free data
    free(data);
};

PLUGIN_EXPORT void prerender(void* data,vzRenderSession* session)
{
};

PLUGIN_EXPORT void postrender(void* data,vzRenderSession* session)
{
};

static size_t get_url_proc_cb( void *ptr, size_t size, size_t nmemb, void *stream)
{
    return size * nmemb;
};

static unsigned long WINAPI get_url_proc(void* data)
{
    int r;
    CURL *curl_handle;

    vzPluginData* ctx = (vzPluginData*)data;

    /* init the curl session */ 
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, ctx->_s_url);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL , 1);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, get_url_proc_cb);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, NULL);
    curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1); 

    /* we do not care about result */
    curl_easy_perform(curl_handle);

    /* always cleanup */ 
    curl_easy_cleanup(curl_handle);

    // and thread
    ExitThread(0);
    return 0;
};

PLUGIN_EXPORT void render(void* data, vzRenderSession* session)
{
    vzPluginData* ctx = (vzPluginData*)data;

    /* check if url template is live */
    if(!ctx->s_url || !ctx->s_url[0])
        return;

    /* increment field counter */
    ctx->_last_frame++;

    /* check if we need get URL */
    if(ctx->_last_frame > ctx->_last_url)
    {
        int r;

        if(INVALID_HANDLE_VALUE == ctx->_th)
            r = WAIT_OBJECT_0;
        else
            r = WaitForSingleObject(ctx->_th, 0);

        if(WAIT_OBJECT_0 == r)
        {
            /* cleanup handler */
            if(INVALID_HANDLE_VALUE != ctx->_th)
                CloseHandle(ctx->_th);

            /* prepare url */
            _snprintf(ctx->_s_url, MAX_URL_LEN, ctx->s_url, ctx->_last_frame);

            /* create new thread */
            _DATA->_th = CreateThread(0, 0, get_url_proc, data, 0, NULL);

            /* update url counter */
            ctx->_last_url = ctx->_last_frame + ctx->l_delay;
        };
    };
};

PLUGIN_EXPORT void notify(void* data, char* param_name)
{
};
