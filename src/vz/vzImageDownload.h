#ifndef vzImageDownload_h
#define vzImageDownload_h

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

static int curl_downloads_cnt;
static HANDLE curl_downloads_lock;

static size_t vzImageDownload_cb_write( void *ptr, size_t size, size_t nmemb, void *stream)
{
#if 0
    if(stream)
        fwrite(ptr, size, nmemb, FILE*)stream);

    return size * nmemb;
#else
    return fwrite(ptr, size, nmemb, (FILE *)stream);
#endif
};

static int vzImageDownload(char* filename_url, char* filename_local)
{
    int r, proto = 0;
    FILE* f;
    CURLcode res;
    CURL *curl_handle;
    long http_responce = 0;
#ifdef _DEBUG
    char errorBuffer[CURL_ERROR_SIZE];
#endif /* _DEBUG */

    /* check if protocol given is http or ftp */
    if(!proto && !_strnicmp(filename_url, "http://", 7)) proto = 1;
    if(!proto && !_strnicmp(filename_url, "ftp://", 6)) proto = 2;
    if(!proto)
        return 0;

    /* inc counter */
    WaitForSingleObject(curl_downloads_lock, INFINITE);
    r = curl_downloads_cnt++;
    ReleaseMutex(curl_downloads_lock);

    /* build name */
    _snprintf(filename_local, MAX_PATH10, "%s/vzImage-%.5d-%.5d",
        getenv("TEMP"), GetCurrentProcessId(), r);

    /* open local file */
    f = fopen(filename_local, "wb");
    if(!f)
        return -200;

    /* init the curl session */ 
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, filename_url);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL , 1);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, vzImageDownload_cb_write);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, f);
    curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1); 
#ifdef _DEBUG
    curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, errorBuffer);
#endif /* _DEBUG */

    res = curl_easy_perform(curl_handle);

    fclose(f);

    if(!res)
    {
        curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &http_responce);

        if
        (
            (1 == proto && 200 == http_responce) ||
            (2 == proto && 226 == http_responce)
        )
        {
            char* ext = NULL;
            char* content_type = NULL;

            /* get content-type */
            curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &content_type);

            /* get extenstion - part of image/XXX content-type header */
            if(content_type && !_strnicmp("image/", content_type, 6))
                ext = content_type + 6;

            /* check if content type not supported */
            if(!ext)
            {
                int i, k, l;
                static char* exts[] = { ".tga", ".png", ".jpg", ".jpeg", ".bmp", ".dib", NULL };

                /* try to find extension */
                l = (int)strlen(filename_url);

                for(i = 0; exts[i] && !ext; i++)
                {
                    k = (int)strlen(exts[i]);

                    if((l > k) && !_stricmp(filename_url + l - k, exts[i]))
                        ext = exts[i] + 1;
                };
            };

            /* append ext */
            if(ext)
            {
                char buf[MAX_PATH10];

                /* backup localname */
                strncpy(buf, filename_local, MAX_PATH10);

                /* append ext to local name */
                strcat(filename_local, ".");
                strcat(filename_local, ext);

                /* rename file */
                MoveFile(buf, filename_local);
            };

            r = 1;
        }
        else
            r = -201;
    }
    else
#ifdef _DEBUG
    {
        fprintf(stderr, "curl_easy_perform: %s\n", errorBuffer);
#endif /* _DEBUG */
        r = -202;
#ifdef _DEBUG
    };
#endif /* _DEBUG */

    /* always cleanup */ 
    curl_easy_cleanup(curl_handle);

    return r;
};

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            curl_global_init(CURL_GLOBAL_ALL);
            curl_downloads_lock = CreateMutex(NULL, FALSE, NULL);
            curl_downloads_cnt = 0;
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            CloseHandle(curl_downloads_lock);
            break;
    }
    return TRUE;
}

#endif /* vzImageDownload_h */
