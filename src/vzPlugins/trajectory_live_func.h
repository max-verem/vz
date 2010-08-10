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

static struct trajectory_live_func* trajectory_live_func_lookup(char* name);

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

static struct trajectory_live_func* trajectory_live_func_lookup(char* name, int* l)
{
    for(int j = 0, s = 1; trajectory_live_funcs[j].name && s; j++)
    {
        /* func name length */
        *l = strlen(trajectory_live_funcs[j].name);

        /* check for FUNCNAME */
        if(memcmp(trajectory_live_funcs[j].name, name, *l)) continue;

        /* inc usefull length */
        if('(' != name[*l]) continue;

        return &trajectory_live_funcs[j];
    };

    return NULL;
};

#endif /* TRAJECTORY_LIVE_FUNC_H */
