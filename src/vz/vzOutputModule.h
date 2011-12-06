#ifndef vzOutputModule_h
#define vzOutputModule_h

#include <windows.h>

typedef struct vzOutputModule_desc
{
    /* initialize module after loading */
    int (*init)(void** pctx, void* obj, void* config, vzTVSpec* tv);

    /* release module handler */
    int (*release)(void** pctx, void* obj, void* config, vzTVSpec* tv);

    /* setup input/outputs handler */
    int (*setup)(void* pctx, HANDLE* sync_event, unsigned long** sync_cnt);

    /* run output/capture threads */
    int (*run)(void* pctx);

    /* stop output/capture threads */
    int (*stop)(void* pctx);
} vzOutputModule_t;

#endif /* vzOutputModule_h */
