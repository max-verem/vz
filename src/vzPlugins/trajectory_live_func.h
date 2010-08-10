#ifndef TRAJECTORY_LIVE_FUNC_H
#define TRAJECTORY_LIVE_FUNC_H

struct trajectory_live_func
{
    char* name;
    int (*init)(void** context, char* params, float* value);
    int (*destroy)(void** context);
    int (*calc)(void* context, float* value);
};

#include "trajectory_live_func_set.h"
#include "trajectory_live_func_lineto.h"
#include "trajectory_live_func_parabolato.h"

static struct trajectory_live_func trajectory_live_funcs[] = 
{
    {
        "SET",
        tlf_set_init,           /* init */
        tlf_set_destroy,        /* destroy */
        tlf_set_calc,           /* calc */
    },

    {
        "LINETO",
        tlf_lineto_init,        /* init */
        tlf_lineto_destroy,     /* destroy */
        tlf_lineto_calc,        /* calc */
    },

    {
        "PARABOLATO",
        tlf_parabolato_init,    /* init */
        tlf_parabolato_destroy, /* destroy */
        tlf_parabolato_calc,    /* calc */
    },

    {
        NULL,
        NULL,                   /* init */
        NULL,                   /* destroy */
        NULL,                   /* calc */
    },
};

#endif /* TRAJECTORY_LIVE_FUNC_H */
