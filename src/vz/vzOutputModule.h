#ifndef vzOutputModule_h
#define vzOutputModule_h

#include <windows.h>

typedef struct vzOutputModule_desc
{
    /* initialize module after loading */
    int (*init)(void* obj, void* config, vzTVSpec* tv);

    /* release module handler */
    int (*release)(void* obj, void* config, vzTVSpec* tv);

    /* setup input/outputs handler */
    int (*setup)(HANDLE* sync_event, unsigned long** sync_cnt);

    /* run output/capture threads */
    int (*run)();

    /* stop output/capture threads */
    int (*stop)();
} vzOutputModule_t;

#endif /* vzOutputModule_h */
