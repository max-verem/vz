#ifndef TRAJECTORY_LIVE_FUNC_SET_H
#define TRAJECTORY_LIVE_FUNC_SET_H

static int tlf_set_init(void** context, char* params, float* value)
{
    *value = (float)atof(params);
    return 0;
};

static int tlf_set_destroy(void** context)
{
    return 0;
};

static int tlf_set_calc(void* context, float* value)
{
    return 0;
};

#endif /* TRAJECTORY_LIVE_FUNC_SET_H */
