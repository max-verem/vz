#ifndef TRAJECTORY_LIVE_FUNC_INIT_H
#define TRAJECTORY_LIVE_FUNC_INIT_H

static int tlf_init_init(void** context, char* params, float* value)
{
    *value = (float)atof(params);
    return 0;
};

static int tlf_init_destroy(void** context)
{
    return 0;
};

static int tlf_init_calc(void* context, float* value)
{
    return 0;
};

static int tlf_init_dur(void* context)
{
    return 0;
};

#endif /* TRAJECTORY_LIVE_FUNC_INIT_H */
