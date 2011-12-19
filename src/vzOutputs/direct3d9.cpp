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
#include <windows.h>
#include <D3D9.h>
#include <stdio.h>
#include <errno.h>

#include "../vz/vzOutput.h"
#include "../vz/vzOutputModule.h"
#include "../vz/vzMain.h"
#include "../vz/vzLogger.h"
#include "../vzPlugins/trajectory.h"

#include "hr_timer.h"

#pragma comment(lib,"D3D9.lib")
#pragma comment(lib,"User32.lib")

#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

static char* UuidToString2(GUID *pguid, char* szGuid)
{
    if(pguid)
        sprintf(szGuid, "%08X-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X",
            pguid->Data1, pguid->Data2, pguid->Data3,
            pguid->Data4[0], pguid->Data4[1], pguid->Data4[2],pguid->Data4[3],
            pguid->Data4[4],pguid->Data4[5],pguid->Data4[6],pguid->Data4[7]);
    else
        strcpy(szGuid, "<NULL>");

    return szGuid;
};

#define MAX_DISPLAYS    16
#define MAX_MAPS        64
#define THIS_MODULE     "direct3d9"
#define THIS_MODULE_PREF THIS_MODULE ": "

//#define DEBUG_TIMINGS

typedef struct d3d9_display_mapping_desc
{
    RECT src, dst;
} d3d9_display_mapping_t;

typedef struct d3d9_display_context_desc
{
    LPDIRECT3DSURFACE9 gpu;
    LPDIRECT3DSURFACE9 cpu;
    D3DADAPTER_IDENTIFIER9 ident;
    D3DPRESENT_PARAMETERS param;
    IDirect3DDevice9* dev;
    IDirect3DSwapChain9 *swap;
    D3DDISPLAYMODE mode;
    D3DCAPS9 caps;
    int adapter_idx;
    int f_requested, f_confirmed;
    HANDLE th;
    int display_idx;
    void* ctx;
    int mapping_count;
    d3d9_display_mapping_t mapping_data[MAX_MAPS];
    int f_alpha;
    vzImage *img;
}
d3d9_display_context_t;

typedef struct d3d9_runtime_context_desc
{
    int f_exit;
    vzTVSpec* tv;
    void* config;
    void* output_context;
    HANDLE th;

    IDirect3D9* d3d;

    struct
    {
        int count;
        HWND hwnd;
        d3d9_display_context_t list[MAX_DISPLAYS];
    } displays;

    struct
    {
        HANDLE src, dst, th;
        unsigned long* cnt;
    } sync;

    struct hr_sleep_data timer_data;

    int vsync;
}
d3d9_runtime_context_t;

const char* D3DERR(HRESULT hr)
{
    switch(hr)
    {
        case D3D_OK:                    return "OK";
        case D3DERR_DEVICELOST:         return "DEVICELOST";
        case D3DERR_INVALIDCALL:        return "INVALIDCALL";
        case D3DERR_NOTAVAILABLE:       return "NOTAVAILABLE";
        case D3DERR_OUTOFVIDEOMEMORY:   return "OUTOFVIDEOMEMORY";
        case D3DERR_WASSTILLDRAWING:    return "WASSTILLDRAWING";
    };

    return "<unknown>";
};

LRESULT CALLBACK WndProcWrap(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CLOSE:
            DestroyWindow(hwnd);
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
        break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

static HWND create_window(d3d9_runtime_context_t* ctx, HINSTANCE hInstance, HWND hParent)
{
    // Register the windows class
    WNDCLASSEX wndClass;
    HWND hWnd;
    static const char* window_name = THIS_MODULE "window";

    if(!GetClassInfoEx(hInstance, window_name, &wndClass))
    {
        wndClass.cbSize = sizeof(wndClass);
        wndClass.style = 0;
        wndClass.lpfnWndProc = WndProcWrap;
        wndClass.cbClsExtra = 0;
        wndClass.cbWndExtra = 0;
        wndClass.hInstance = hInstance;
        wndClass.hIcon = NULL;
        wndClass.hCursor = NULL;
        wndClass.hbrBackground = NULL;
        wndClass.lpszMenuName = NULL;
        wndClass.lpszClassName = window_name;
        wndClass.hIconSm = NULL;

        if(!RegisterClassEx(&wndClass))
        {
            int r = GetLastError();
            logger_printf(1, THIS_MODULE_PREF "RegisterClassEx failed: GetLastError=%d", r);
            return NULL;
        };
    };

    // Create the render window
    hWnd = CreateWindowEx(WS_EX_TOPMOST, window_name,
        window_name, WS_EX_TOPMOST | WS_POPUP | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hParent, NULL, hInstance, NULL);

    if(!hWnd)
    {
        int r = GetLastError();
        logger_printf(1, THIS_MODULE_PREF "CreateWindowEx failed: GetLastError=%d", r);
    };

    return hWnd;
};

static int d3d9_enum(d3d9_runtime_context_t* ctx)
{
    int i, j, c, a;
    char buf[MAX_PATH];

    D3DCAPS9 caps;
    D3DDISPLAYMODE mode;
    D3DADAPTER_IDENTIFIER9 ident;

    c = ctx->d3d->GetAdapterCount();
    logger_printf(0, THIS_MODULE_PREF "Found %d adapters", c);

    for(a = 0, i = 0; i < c; i++)
    {
        /* request info */
        ctx->d3d->GetAdapterIdentifier(i, 0, &ident);
        ctx->d3d->GetAdapterDisplayMode(i, &mode);
        ctx->d3d->GetDeviceCaps(i, D3DDEVTYPE_HAL, &caps);

        logger_printf(0, THIS_MODULE_PREF "VGA %d:", i);
        logger_printf(0, THIS_MODULE_PREF "    DeviceName:  %s",
            ident.DeviceName);
        logger_printf(0, THIS_MODULE_PREF "    Description: %s",
            ident.Description);
        logger_printf(0, THIS_MODULE_PREF "    GUID:        %s",
            UuidToString2(&ident.DeviceIdentifier, buf));
        logger_printf(0, THIS_MODULE_PREF "    mode:        %dx%d@%d [0x%.8X]",
            mode.Width, mode.Height,
            mode.RefreshRate, mode.Format);
        logger_printf(0, THIS_MODULE_PREF "    caps:");
        logger_printf(0, THIS_MODULE_PREF "        NumberOfAdaptersInGroup %d",
            caps.NumberOfAdaptersInGroup);
        logger_printf(0, THIS_MODULE_PREF "        MasterAdapterOrdinal    %d",
            caps.MasterAdapterOrdinal);
        logger_printf(0, THIS_MODULE_PREF "        AdapterOrdinalInGroup   %d",
            caps.AdapterOrdinalInGroup);

        /* check if it requested */
        for(j = 0; j < MAX_DISPLAYS; j++)
        {
            /* setup self ident */
            ctx->displays.list[j].ctx = ctx;
            ctx->displays.list[j].display_idx = j;

            /* check if display is requested */
            if(!ctx->displays.list[j].f_requested)
                continue;

            /* check if our index is here */
            if(!ctx->displays.list[j].adapter_idx != i)
                continue;

            /* already occupied */
            if(ctx->displays.list[j].f_confirmed)
                continue;

            /* save this data */
            ctx->displays.list[j].caps = caps;
            ctx->displays.list[j].mode = mode;
            ctx->displays.list[j].ident = ident;
            ctx->displays.list[j].f_confirmed = 1;

            /* notify */
            logger_printf(0, THIS_MODULE_PREF "display %d use adapter %d", j, i);

            a++;
        };
    };

    return a;
};


static int d3d9_init(void** pctx, void* obj, void* config, vzTVSpec* tv)
{
    int i, c, j;
    char **list, *v;
    d3d9_runtime_context_t *ctx;

    if(!pctx)
        return -EINVAL;
    *pctx = NULL;

    /* allocate module context */
    ctx = (d3d9_runtime_context_t *)malloc(sizeof(d3d9_runtime_context_t));
    if(!ctx)
        return -ENOMEM;
    *pctx = ctx;

    /* clear context */
    memset(ctx, 0, sizeof(d3d9_runtime_context_t));

    /* setup basic components */
    ctx->config = config;
    ctx->tv = tv;
    ctx->output_context = obj;
    ctx->sync.src = CreateEvent(NULL, TRUE, FALSE, NULL);
    hr_sleep_init(&ctx->timer_data);

    /* check if vsync honor */
    v = (char*)vzConfigParam(ctx->config, THIS_MODULE, "VSYNC");
    if(v)
        ctx->vsync = atol(v);

    /* display options */
    for(i = 0; i < MAX_DISPLAYS; i++)
    {
        int idx;
        char name[32];
        d3d9_display_context_t *display = &ctx->displays.list[i];

        /* compose parameter name */
        sprintf(name, "DISPLAY_%d_ADAPTER", i);

        /* request param */
        v = (char*)vzConfigParam(ctx->config, THIS_MODULE, name);
        if(!v) continue;
        idx = atol(v);

        /* check for default monitor */
        if(!idx)
        {
            logger_printf(1, THIS_MODULE_PREF "default adapter 0 not allowed");
            continue;
        };

        /* compose parameter name */
        sprintf(name, "DISPLAY_%d_MAPPING", i);

        /* request param */
        v = (char*)vzConfigParam(ctx->config, THIS_MODULE, name);
        if(!v) continue;

        /* split mappings */
        c = split_str(v, ";", &list);
        for(j = 0; j < c; j++)
        {
            int r;
            d3d9_display_mapping_t m;

            r = sscanf(list[j], "%d,%d,%d,%d:%d,%d,%d,%d",
                &m.src.left, &m.src.top, &m.src.right, &m.src.bottom,
                &m.dst.left, &m.dst.top, &m.dst.right, &m.dst.bottom);

            if(8 != r)
                logger_printf(1, THIS_MODULE_PREF "failed to parse mapping value [%s]\n", list[j]);
            else
                display->mapping_data[display->mapping_count++] = m;
        };
        split_str_free(&list);

        /* setup displays requests */
        display->f_requested = 1;
        display->adapter_idx = idx;

        /* compose parameter name */
        sprintf(name, "DISPLAY_%d_ALPHA", i);

        /* request param */
        v = (char*)vzConfigParam(ctx->config, THIS_MODULE, name);
        if(v)
            display->f_alpha = 1;
    };

    ctx->d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if(!ctx->d3d)
    {
        logger_printf(1, THIS_MODULE_PREF "Direct3DCreate9 failed");
        return -EFAULT;
    };

    /* assiciate displays with adapters */
    if(!d3d9_enum(ctx))
    {
        logger_printf(1, THIS_MODULE_PREF "no display associated");
        return -ENOENT;
    };

    return 0;
};

static int d3d9_release(void** pctx, void* obj, void* config, vzTVSpec* tv)
{
    int i;
    d3d9_runtime_context_t *ctx;

    if(!pctx)
        return -EINVAL;

    ctx = (d3d9_runtime_context_t *)(*pctx);
    *pctx = NULL;

    SAFE_RELEASE(ctx->d3d);

    free(ctx);

    return 0;
};

static int d3d9_setup(void* pctx, HANDLE* sync_event, unsigned long** sync_cnt)
{
    int i, c;
    d3d9_runtime_context_t *ctx;

    if(!pctx)
        return -EINVAL;

    ctx = (d3d9_runtime_context_t *)pctx;

    for(i = 0, c = 0; i < MAX_DISPLAYS; i++)
        if(ctx->displays.list[i].f_confirmed)
            c++;

    if(c && sync_event)
    {
        logger_printf(0, THIS_MODULE_PREF "output enabled");

        ctx->sync.dst = *sync_event;
        *sync_event = NULL;

        ctx->sync.cnt = *sync_cnt;
        *sync_cnt = NULL;
    };

    return 0;
};

static unsigned long WINAPI d3d9_display_thread(void* obj);
static unsigned long WINAPI d3d9_thread_syncer(void* obj);

static int d3d9_run(void* pctx)
{
    int i, c;
    HRESULT hr;
    HINSTANCE hinst = GetModuleHandle(NULL);
    d3d9_runtime_context_t *ctx;

    if(!pctx)
        return -EINVAL;

    ctx = (d3d9_runtime_context_t *)pctx;

    /* reset exit flag */
    ctx->f_exit = 0;

    /* create a focus window */
    ctx->displays.hwnd = create_window(ctx, hinst, NULL);

    for(c = 0, i = 0; i < MAX_DISPLAYS; i++)
    {
        D3DPRESENT_PARAMETERS param;

        /* check if display active */
        if(!ctx->displays.list[i].f_confirmed)
            continue;

        /* clear param struct */
        memset(&param, 0, sizeof(D3DPRESENT_PARAMETERS));

        /* setup params */
        param.MultiSampleType = D3DMULTISAMPLE_NONE;
        param.MultiSampleQuality = 0;
        param.BackBufferCount = 1;
        param.BackBufferWidth = ctx->displays.list[i].mode.Width;
        param.BackBufferHeight = ctx->displays.list[i].mode.Height;
        param.BackBufferFormat = D3DFMT_X8R8G8B8;
        param.FullScreen_RefreshRateInHz = ctx->displays.list[i].mode.RefreshRate;
        param.SwapEffect = D3DSWAPEFFECT_FLIP;
        param.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
        param.hDeviceWindow = ctx->displays.hwnd;
        ctx->displays.list[i].param = param;

        logger_printf(0, THIS_MODULE_PREF "display[%d]: adapter=%d, mode=%dx%d@%d", i,
            ctx->displays.list[i].adapter_idx,
            param.BackBufferWidth,
            param.BackBufferHeight,
            param.FullScreen_RefreshRateInHz);

        /* create device */
        hr = ctx->d3d->CreateDevice
        (
            ctx->displays.list[i].adapter_idx, D3DDEVTYPE_HAL,
            GetConsoleWindow(), D3DCREATE_HARDWARE_VERTEXPROCESSING,
            &ctx->displays.list[i].param, &ctx->displays.list[i].dev
        );
        if(D3D_OK != hr)
            logger_printf(1, THIS_MODULE_PREF "failed to CreateDevice: %s", D3DERR(hr));
        else
        {
            c++;
            hr = ctx->displays.list[i].dev->GetSwapChain(0, &ctx->displays.list[i].swap);
            if(D3D_OK != hr)
                logger_printf(1, THIS_MODULE_PREF "failed to GetSwapChain: %s", D3DERR(hr));
        };
    };

    /* run syncer thread */
    if(!ctx->vsync)
    {
        ctx->sync.th = CreateThread
        (
            NULL,
            1024,
            d3d9_thread_syncer,
            ctx,
            0,
            NULL
        );

        /* set thread priority */
        SetThreadPriority(ctx->sync.th , THREAD_PRIORITY_HIGHEST);
    };

    if(c)
    {
        /* run syncer thread */
        ctx->th = CreateThread
        (
            NULL,
            1024,
            d3d9_display_thread,
            ctx,
            0,
            NULL
        );
    }
    else
    {
        if(ctx->displays.hwnd)
        {
            DestroyWindow(ctx->displays.hwnd);
            ctx->displays.hwnd = NULL;
        };
    };

    return 0;
};

static unsigned long WINAPI d3d9_thread_syncer(void* obj)
{
    d3d9_runtime_context_t* ctx = (d3d9_runtime_context_t*)obj;

    while(ctx->f_exit != 2)
    {
        hr_sleep(&ctx->timer_data, 1000 * ctx->tv->TV_FRAME_PS_DEN / ctx->tv->TV_FRAME_PS_NOM);
        PulseEvent(ctx->sync.src);
    };
    PulseEvent(ctx->sync.src);

    return 0;
};

static int d3d9_stop(void* pctx)
{
    int i;
    d3d9_runtime_context_t *ctx;

    if(!pctx)
        return -EINVAL;

    ctx = (d3d9_runtime_context_t *)pctx;

    /* reset exit flag */
    ctx->f_exit = 1;

    /* wait for threads finished */
    if(ctx->th)
    {
        logger_printf(0, THIS_MODULE_PREF "waiting for thread to finish...");
        WaitForSingleObject(ctx->th, INFINITE);
        CloseHandle(ctx->th);
        ctx->th = NULL;
        logger_printf(0, THIS_MODULE_PREF "ok");
    };

    for(i = 0; i < MAX_DISPLAYS; i++)
    {
        SAFE_RELEASE(ctx->displays.list[i].swap);
        SAFE_RELEASE(ctx->displays.list[i].dev);
    };

    if(ctx->displays.hwnd)
    {
        DestroyWindow(ctx->displays.hwnd);
        ctx->displays.hwnd = NULL;
    };

    /* syncer notify to exit */
    if(ctx->sync.th)
    {
        ctx->f_exit = 2;
        WaitForSingleObject(ctx->sync.th, INFINITE);
        CloseHandle(ctx->sync.th);
    };

    return 0;
};

extern "C" __declspec(dllexport) vzOutputModule_t vzOutputModule =
{
    d3d9_init,
    d3d9_release,
    d3d9_setup,
    d3d9_run,
    d3d9_stop
};

static unsigned long WINAPI d3d9_display_swap(void* obj);
static unsigned long WINAPI d3d9_display_draw(void* obj);
static unsigned long WINAPI d3d9_display_load(void* obj);

static unsigned long WINAPI d3d9_display_thread(void* obj)
{
    int i, h, b, j, a;
    HRESULT hr;
    d3d9_runtime_context_t *ctx = (d3d9_runtime_context_t *)obj;

    vzImage img_origin;
    vzImage *img_alpha = NULL;

    logger_printf(0, THIS_MODULE_PREF "d3d9_display_thread started...");

    while(!ctx->f_exit)
    {
        if(ctx->sync.th)
            /* wait for internal sync */
            WaitForSingleObject(ctx->sync.src, INFINITE);

        /* request frame */
        vzOutputOutGet(ctx->output_context, &img_origin);

        /* create surfaces */
        for(i = 0; i < MAX_DISPLAYS; i++)
        {
            if(!ctx->displays.list[i].dev)
                continue;

            /* create GPU surface */
            if(!ctx->displays.list[i].gpu)
            {
                hr = ctx->displays.list[i].dev->CreateOffscreenPlainSurface(
                    img_origin.width, img_origin.height, D3DFMT_X8R8G8B8,
                    D3DPOOL_DEFAULT, &ctx->displays.list[i].gpu, NULL);
                if(D3D_OK != hr)
                {
                    logger_printf(1, THIS_MODULE_PREF "CreateOffscreenPlainSurface(D3DPOOL_DEFAULT) failed: %s",
                        D3DERR(hr));
                    return -1;
                }
                else
                    logger_printf(0, THIS_MODULE_PREF "offscreen surface D3DPOOL_DEFAULT for display % created", i);
            };

            /* create CPU surface */
            if(!ctx->displays.list[i].cpu)
            {
                hr = ctx->displays.list[i].dev->CreateOffscreenPlainSurface(
                    img_origin.width, img_origin.height, D3DFMT_X8R8G8B8,
                    D3DPOOL_SYSTEMMEM, &ctx->displays.list[i].cpu, NULL);
                if(D3D_OK != hr)
                {
                    logger_printf(1, THIS_MODULE_PREF "CreateOffscreenPlainSurface(D3DPOOL_SYSTEMMEM) failed: %s",
                        D3DERR(hr));
                    return -1;
                }
                else
                    logger_printf(0, THIS_MODULE_PREF "offscreen surface D3DPOOL_SYSTEMMEM for display % created", i);
            };
        };

        /* load surfaces*/
        for(a = 0, i = 0; i < MAX_DISPLAYS; i++)
        {
            if(!ctx->displays.list[i].dev)
                continue;

            if(ctx->displays.list[i].f_alpha)
            {
                if(!img_alpha)
                    vzImageCreate(&img_alpha, img_origin.width, img_origin.height);

                if(!a)
                {
                    vzImageConv_BGRA_to_AAAA(&img_origin, img_alpha);
                    a = 1;
                };

                ctx->displays.list[i].img = img_alpha;
            }
            else
                ctx->displays.list[i].img = &img_origin;

            ctx->displays.list[i].th = CreateThread(NULL, 1024, d3d9_display_load,
                &ctx->displays.list[i], 0, NULL);
        };

        /* wait drawer finish */
        for(i = 0; i < MAX_DISPLAYS; i++)
        {
            if(!ctx->displays.list[i].th)
                continue;

            WaitForSingleObject(ctx->displays.list[i].th, INFINITE);
            CloseHandle(ctx->displays.list[i].th);
        };

        /* release frame back */
        vzOutputOutRel(ctx->output_context, &img_origin);

        /* notify main */
        *ctx->sync.cnt = *ctx->sync.cnt + 1;
        PulseEvent(ctx->sync.dst);

        /* as minimal supported D3DPRESENT_INTERVAL_ONE
           we should draw n times specified by vsync */
        if(ctx->vsync)
            b = ctx->vsync;
        else
            b = 1;

        for(j = 0; j < b; j++)
        {

            /* draw surfaces*/
            for(i = 0; i < MAX_DISPLAYS; i++)
            {
                if(!ctx->displays.list[i].dev)
                    continue;

                ctx->displays.list[i].th = CreateThread(NULL, 1024, d3d9_display_draw,
                    &ctx->displays.list[i], 0, NULL);
            };

            /* wait drawer finish */
            for(i = 0; i < MAX_DISPLAYS; i++)
            {
                if(!ctx->displays.list[i].th)
                    continue;

                WaitForSingleObject(ctx->displays.list[i].th, INFINITE);
                CloseHandle(ctx->displays.list[i].th);
            };

            /* swap surfaces*/
            for(i = 0; i < MAX_DISPLAYS; i++)
            {
                if(!ctx->displays.list[i].dev)
                    continue;

                ctx->displays.list[i].th = CreateThread(NULL, 1024, d3d9_display_swap,
                    &ctx->displays.list[i], 0, NULL);
            };

            /* wait swapper finish */
            for(i = 0; i < MAX_DISPLAYS; i++)
            {
                if(!ctx->displays.list[i].th)
                    continue;

                WaitForSingleObject(ctx->displays.list[i].th, INFINITE);
                CloseHandle(ctx->displays.list[i].th);
            };

        };
    };

    if(img_alpha)
        vzImageRelease(&img_alpha);

    return 0;
};

static unsigned long WINAPI d3d9_display_load(void* obj)
{
    int h;
    HRESULT hr;
    D3DLOCKED_RECT rect;
    d3d9_display_context_t *display = (d3d9_display_context_t*)obj;
    d3d9_runtime_context_t *ctx = (d3d9_runtime_context_t *)display->ctx;

    /* copy FRAME to CPU surface */
    hr = display->cpu->LockRect(&rect, NULL, 0);
    if(D3D_OK != hr)
    {
        logger_printf(1, THIS_MODULE_PREF "pImageBuffer_cpu->LockRect failed: %s", D3DERR(hr));
        return -1;
    };
    int dst_pitch = rect.Pitch, src_pitch = display->img->line_size, cpy_pitch;
    unsigned char
        *dst_data = (unsigned char *)rect.pBits,
        *src_data = (unsigned char *)display->img->surface + src_pitch * (display->img->height - 1);
    if(dst_pitch > src_pitch)
        cpy_pitch = src_pitch;
    else
        cpy_pitch = dst_pitch;
    for(h = 0; h < display->img->height; h++)
    {
        memcpy(dst_data, src_data, cpy_pitch);
        dst_data += dst_pitch;
        src_data -= src_pitch;
    };
    hr = display->cpu->UnlockRect();

    /* copy CPU surface to GPU surface */
    hr = display->dev->UpdateSurface(display->cpu, NULL, display->gpu, NULL);

    return 0;
};

static unsigned long WINAPI d3d9_display_draw(void* obj)
{
    int i;
    HRESULT hr;
    LPDIRECT3DSURFACE9 pBackBuffer = NULL;
    d3d9_display_context_t *display = (d3d9_display_context_t*)obj;
    d3d9_runtime_context_t *ctx = (d3d9_runtime_context_t *)display->ctx;

    /* request backbuffer */
    hr = display->swap->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);

    /* clear backbuffer */
    hr = display->dev->ColorFill(pBackBuffer, NULL, D3DCOLOR_RGBA(0, 0, 0, 0));

    /* find target to draw */
    for(i = 0; i < display->mapping_count; i++)
        /* copy/stretch */
        hr = display->dev->StretchRect
        (
            display->gpu, &display->mapping_data[i].src,
            pBackBuffer, &display->mapping_data[i].dst,
            D3DTEXF_LINEAR
        );

    SAFE_RELEASE(pBackBuffer);

    return 0;
};

static unsigned long WINAPI d3d9_display_swap(void* obj)
{
    HRESULT hr;
    d3d9_display_context_t *display = (d3d9_display_context_t*)obj;
    d3d9_runtime_context_t *ctx = (d3d9_runtime_context_t *)display->ctx;

#ifdef DEBUG_TIMINGS
    logger_printf(1, THIS_MODULE_PREF "d3d9_display_swap...");
#endif /* DEBUG_TIMINGS */

    /* flip */
    for(;;)
    {
#ifdef DEBUG_TIMINGS
        logger_printf(1, THIS_MODULE_PREF "presenting...");
#endif /* DEBUG_TIMINGS */

        hr = display->swap->Present(NULL, NULL, NULL, NULL, D3DPRESENT_DONOTWAIT);

#ifdef DEBUG_TIMINGS
        logger_printf(1, THIS_MODULE_PREF "Present result: %s", D3DERR(hr));
#endif /* DEBUG_TIMINGS */

        if (D3D_OK == hr || D3DERR_WASSTILLDRAWING != hr)
            break;

        Sleep(1);
    };

    if(D3D_OK != hr)
        logger_printf(1, THIS_MODULE_PREF "Present failed: %s", D3DERR(hr));

    return 0;
};
