#ifndef vzOutputContext_h
#define vzOutputContext_h

#define MAX_MODULES 4
#define MAX_INPUTS  16

#include "vzOutputModule.h"
#include "vzOutputBuffers.h"

typedef struct vzOutputContext_desc
{
    /** number of loaded modules */
    int count;
    /** handles to modules */
    HMODULE libs[MAX_MODULES];
    /** method of modules */
    vzOutputModule_t* modules[MAX_MODULES];

    void* config;
    vzTVSpec* tv;

    /* exit flag */
    int f_exit;

    /** syncer */
    struct
    {
        HANDLE th;
        HANDLE ev;
        unsigned long* cnt;
    } syncer;

    /* output buffers */
    vzOutputBuffers_t output;

    /** inputs */
    int inputs_count;
    struct
    {
        HANDLE lock;
        void* front;
        void* back;
    } inputs_data[MAX_INPUTS];

}vzOutputContext_t;

#endif /* vzOutputContext_h */
