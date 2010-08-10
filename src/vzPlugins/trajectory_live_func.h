#ifndef TRAJECTORY_LIVE_FUNC_H
#define TRAJECTORY_LIVE_FUNC_H

struct trajectory_live_func
{
    char* name;
    int (*init)(void** context, char* params, float* value);
    int (*destroy)(void** context);
    int (*calc)(void* context, float* value);
    int (*dur)(void* context);
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
        tlf_set_dur,
    },

    {
        "LINETO",
        tlf_lineto_init,        /* init */
        tlf_lineto_destroy,     /* destroy */
        tlf_lineto_calc,        /* calc */
        tlf_lineto_dur,
    },

    {
        "PARABOLATO",
        tlf_parabolato_init,    /* init */
        tlf_parabolato_destroy, /* destroy */
        tlf_parabolato_calc,    /* calc */
        tlf_parabolato_dur,
    },

    {
        NULL,
        NULL,                   /* init */
        NULL,                   /* destroy */
        NULL,                   /* calc */
        NULL,                   /* dur */
    },
};

#endif /* TRAJECTORY_LIVE_FUNC_H */
